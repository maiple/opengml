
#include "ogm/project/resource/ResourceSprite.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"
#include "ogm/project/arf/arf_parse.hpp"

namespace ogm::project {

namespace
{
    ARFSchema arf_sprite_schema
    {
        "sprite",
        ARFSchema::DICT,
        {
            "images",
            ARFSchema::LIST
        }
    };
}

void ResourceSprite::load_file_arf()
{
    std::string _path = native_path(m_path);
    std::string _dir = path_directory(_path);
    std::string file_contents = read_file_contents(_path.c_str());

    ARFSection sprite_section;
    
    arf_parse(arf_sprite_schema, file_contents.c_str(), sprite_section);
    
    // this transformation will be applied to the image.
    typedef asset::AssetSprite::SubImage Image;
    std::vector<std::function<Image(Image&)>> transforms;
    std::function<Image(Image&)> transform = [&transforms](Image& in) -> Image
    {
        Image c = in;
        for (auto transform : transforms)
        {
            c = transform(c);
        }
        return c;
    };
    
    // scale
    std::vector<double> scale;
    std::vector<std::string_view> scale_sv;
    std::string scale_s;
    
    scale_s = sprite_section.get_value(
        "scale",
        "[1, 1]"
    );
    
    {
        arf_parse_array(scale_s.c_str(), scale_sv);
        
        for (const std::string_view& sv : scale_sv)
        {
            scale.push_back(svtod_or_percent(std::string(sv)));
        }
        
        // simple scale function.
        if (scale[0] != 1 || scale[1] != 1)
        {
            transforms.emplace_back(
                [s0=scale[0], s1=scale[1]](Image& img) -> Image
                {
                    return img.scaled(s0, s1);
                }
            );
        }
    }
    
    if (scale.size() != 2)
    {
        throw ResourceError(1046, this, "scale must be 2-tuple.");
    }
    
    // xbr
    real_t xbr_amount = std::stoi(sprite_section.get_value(
        "xbr",
        "1"
    ));
    
    if (xbr_amount != 1)
    {
        if (xbr_amount != 2 && xbr_amount != 3 && xbr_amount != 4)
        {
            throw ResourceError(1047, this, "xbr must be 1, 2, 3, or 4.");
        }
        
        bool xbr_alpha = sprite_section.get_value(
            "xbr_scale_alpha",
            "0"
        ) != "0";
        
        bool xbr_blend = sprite_section.get_value(
            "xbr_blend",
            "0"
        ) != "0";
        
        transforms.push_back([xbr_amount, xbr_alpha, xbr_blend](Image& img) -> Image {
            return img.xbr(xbr_amount, xbr_blend, xbr_alpha);
        });
        
        // up scale
        scale[0] *= xbr_amount;
        scale[1] *= xbr_amount;
    }
    
    // sprite images
    {
        // sheet range
        std::vector<int32_t> sheet_range;
        std::vector<std::string_view> sheet_range_sv;
        std::string sheet_range_s;
        
        // negative number = unbounded (i.e. 0 or max).
        sheet_range_s = sprite_section.get_value(
            "sheet_range",
            "[-1, -1]"
        );
        
        arf_parse_array(sheet_range_s.c_str(), sheet_range_sv);
        
        for (const std::string_view& sv : sheet_range_sv)
        {
            sheet_range.push_back(std::stoi(std::string(sv)));
        }
        
        if (sheet_range.size() != 2)
        {
            throw ResourceError(1048, this, "Sheet_range must be 2-tuple. (Hint: negative numbers can be used for unbounded range.)");
        }

        // sheet dimension
        std::vector<int32_t> sheet;
        std::vector<std::string_view> sheet_sv;
        std::string sheet_s;

        sheet_s = sprite_section.get_value(
            "sheet",
            "[1, 1]"
        );
        
        arf_parse_array(sheet_s.c_str(), sheet_sv);
        
        for (const std::string_view& sv : sheet_sv)
        {
            sheet.push_back(std::stoi(std::string(sv)));
        }
        
        if (sheet.size() != 2)
        {
            throw ResourceError(1049, this, "Sheet size must be 2-tuple.");
        }
        
        if (sheet[0] < 1 || sheet[1] < 1)
        {
            throw ResourceError(1050, this, "Sheet dimensions must be positive.");
        }
        
        // clamp negative sheet range
        if (sheet_range[0] < 0) sheet_range[0] = 0;
        if (sheet_range[1] < 0) sheet_range[1] = sheet[0] * sheet[1];

        // subimages
        for (ARFSection* subimages : sprite_section.m_sections)
        {
            for (const std::string& subimage : subimages->m_list)
            {
                Image image{ std::move(_dir + subimage) };
                
                if (sheet[0] > 1 || sheet[1] > 1)
                {
                    // split the image
                    image.realize_data();
                    
                    geometry::Vector<int32_t> subimage_dimensions{
                        image.m_dimensions.x / sheet[0],
                        image.m_dimensions.y / sheet[1],
                    };
                    
                    size_t index = 0;
                    
                    for (int j = 0; j < sheet[1]; ++j)
                    {
                        for (int i = 0; i < sheet[0]; ++i)
                        {
                            if (index >= sheet_range[0] && index < sheet_range[1])
                            {
                                geometry::AABB<int32_t> subimage_region {
                                    subimage_dimensions.x * i,
                                    subimage_dimensions.y * j,
                                    subimage_dimensions.x * (i + 1),
                                    subimage_dimensions.y * (j + 1)
                                };
                                
                                Image image_cropped = image.cropped(subimage_region);
                                
                                if (transforms.size())
                                {
                                    m_subimages.emplace_back(transform(image_cropped));
                                }
                                else
                                {
                                    m_subimages.emplace_back(std::move(image_cropped));
                                }
                            }
                            
                            ++index;
                        }
                    }
                }
                else
                {
                    // just add the image
                    
                    if (transforms.size())
                    {
                        m_subimages.emplace_back(transform(image));
                    }
                    else
                    {
                        m_subimages.emplace_back(std::move(image));
                    }
                }
            }
        }

        if (m_subimages.empty())
        {
            throw ResourceError(1051, this, "needs at least one subimage.");
        }
    }

    std::vector<std::string_view> arr;
    std::string arrs;

    // dimensions
    arrs = sprite_section.get_value(
        "dimensions",
        sprite_section.get_value("size", "[-1, -1]")
    );
    arf_parse_array(arrs.c_str(), arr);
    if (arr.size() != 2) ResourceError(1052, this, "field \"dimensions\" or \"size\" should be a 2-tuple.");
    m_dimensions.x = svtoi(arr[0]) * std::abs(scale[0]);
    m_dimensions.y = svtoi(arr[1]) * std::abs(scale[1]);
    arr.clear();

    if (m_dimensions.x < 0 || m_dimensions.y < 0)
    // load sprite and read its dimensions
    {
        asset::Image& sb = m_subimages.front();

        sb.realize_data();

        m_dimensions = sb.m_dimensions;
    }
    else
    {
        // let subimages know the expected image dimensions (for error-checking later.)
        for (asset::Image& image : m_subimages)
        {
            image.m_dimensions = m_dimensions;
        }
    }

    // origin
    arrs = sprite_section.get_value("origin", "[0, 0]");
    if (sprite_section.has_value("offset")) // alias
    {
        arrs = sprite_section.get_value("offset", "[0, 0]");
    }
    arf_parse_array(arrs.c_str(), arr);
    if (arr.size() != 2) throw ResourceError(1053, this, "field \"origin\" should be a 2-tuple.");
    m_offset.x = scale[0] ? svtod_or_percent(arr[0], m_dimensions.x / scale[0]) * scale[0] : 0;
    m_offset.y = scale[1] ? svtod_or_percent(arr[1], m_dimensions.y / scale[1]) * scale[1] : 0;
    arr.clear();

    // bbox
    if (sprite_section.has_value("bbox"))
    {
        arrs = sprite_section.get_value("bbox", "[0, 0], [1, 1]");
        std::vector<std::string> subarrs;
        split(subarrs, arrs, "x");
        if (subarrs.size() != 2) goto bbox_error;
        for (std::string& s : subarrs)
        {
            trim(s);
        }

        // parse sub-arrays

        // x bounds
        arf_parse_array(std::string{ subarrs.at(0) }.c_str(), arr);
        if (arr.size() != 2) goto bbox_error;
        m_aabb.m_start.x = svtod(arr[0]) * scale[0];
        m_aabb.m_end.x = svtod(arr[1]) * scale[0];
        arr.clear();

        // y bounds
        arf_parse_array(std::string{ subarrs.at(1) }.c_str(), arr);
        if (arr.size() != 2) goto bbox_error;
        m_aabb.m_start.y = svtod(arr[0]) * scale[1];
        m_aabb.m_end.y = svtod(arr[1]) * scale[1];
        arr.clear();

        if (false)
        {
        bbox_error:
            throw ResourceError(1054, this, "field \"bbox\" should be 2-tuple x 2-tuple.");
        }
    }
    else
    {
        m_aabb = { {0, 0}, m_dimensions };
    }
    
    // rotsprite and rotate
    real_t rotsprite_amount = std::stod(sprite_section.get_value(
        "rotsprite",
        "0"
    ));
    
    real_t rotate_amount = std::stod(sprite_section.get_value(
        "rotate",
        "0"
    ));
    
    if ((rotate_amount != 0 || rotsprite_amount != 0) && !m_subimages.empty())
    {
        std::function<geometry::Vector<real_t>(Image&, geometry::Vector<real_t>)> transform;
        if (rotsprite_amount != 0)
        {
            transform = [rotsprite_amount, rotate_amount]
                (Image& img, geometry::Vector<real_t> offset)
                -> geometry::Vector<real_t>
            {
                offset = offset * 8;
                img = img
                    .xbr(2, false, false)
                    .xbr(2, false, false)
                    .xbr(2, false, false)
                    .rotated(rotsprite_amount, offset)
                    .scaled(1/8.0, 1/8.0);
                offset = offset / 8;
                img = img.rotated(rotate_amount, offset);
                return offset;
            };
        }
        else
        {
            transform = [rotate_amount]
                (Image& img, geometry::Vector<real_t> offset)
                -> geometry::Vector<real_t>
            {
                img.realize_data();
                img = img
                    .rotated(rotate_amount, offset);
                return offset;
            };
        }
        
        // center bounds
        m_aabb.m_start -= m_offset;
        m_aabb.m_end -= m_offset;
        
        geometry::Vector<real_t> prev_offset = m_offset;
        for (auto& image : m_subimages)
        {
            m_offset = transform(image, prev_offset);
        }
        
        // rotate bounds
        m_aabb = m_aabb.rotated(rotsprite_amount);
        
        // uncenter bounds
        m_aabb.m_start += m_offset;
        m_aabb.m_end += m_offset;
        
        // update dimensions
        m_dimensions = m_subimages.front().m_dimensions;
    }

    m_speed = std::stod(sprite_section.get_value(
        "speed",
        "1"
    ));

    m_separate_collision_masks = svtoi(sprite_section.get_value("sepmasks", "0"));
    m_alpha_tolerance = svtoi(
        sprite_section.get_value("tolerance", "0")
    );

    std::string colkind = sprite_section.get_value(
        "colkind", sprite_section.get_value("collision", "1")
    );
    if (is_digits(colkind))
    {
        m_colkind = std::stoi(colkind);
    }
    else
    {
        if (colkind == "precise" || colkind == "raster")
        {
            m_colkind = 0;
        }
        else if (colkind == "separate")
        {
            m_colkind = 0;
            m_separate_collision_masks = true;
        }
        else if (colkind == "rect" || colkind == "aabb" || colkind == "rectangle" || colkind == "bbox")
        {
            m_colkind = 1;
        }
        else if (colkind == "ellipse" || colkind == "circle")
        {
            m_colkind = 2;
        }
        else if (colkind == "diamond")
        {
            m_colkind = 3;
        }
        else
        {
            throw ResourceError(1055, this, "Unrecognized collision type \'{}\"", colkind);
        }
    }
}

}