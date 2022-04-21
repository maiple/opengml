#include "ogm/project/resource/ResourceRoom.hpp"

#include "ogm/common/util.hpp"
#include "ogm/common/types.hpp"
#include "ogm/project/arf/arf_parse.hpp"

#include <string>

namespace ogm::project
{
    
namespace
{
    ARFSchema arf_room_schema
    (
        "room",
        ARFSchema::DICT,
        {{
            "background",
            ARFSchema::DICT
        },
        {
            "view",
            ARFSchema::DICT
        },
        {
            "cc",
            ARFSchema::TEXT
        },
        {
            "tile",
            ARFSchema::DICT
        },
        {
            "instance",
            ARFSchema::DICT,
            {
                "cc",
                ARFSchema::TEXT
            }
        }}
    );
    
    uint32_t svtocolour(const std::string_view& sv)
    {
        std::string colour{ sv };
        if (is_digits(colour))
        {
            return std::stoll(colour);
        }
        else
        {
            if (starts_with(colour, "0x"))
            {
                colour = remove_prefix(colour, "0x");
                return std::stoll(colour, nullptr, 16);
            }
            else
            {
                // todo: lookup in colour chart
                throw MiscError("unrecognized colour \"" + colour + "\"");
            }
        }
    }

    const char* endl = "\n";

    std::string to_string(double d)
    {
        if (d == std::floor(d))
        {
            return std::to_string(static_cast<int64_t>(d));
        }
        return std::to_string(d);
    }

    std::string to_string(geometry::Vector<real_t> v)
    {
        return "[" + to_string(v.x) + ", " + to_string(v.y) + "]";
    }

    // horizontal rule for string
    std::string hrule_string(std::string s, size_t line_width=80)
    {
        while (s.length() < line_width) s += "-";
        return s;
    }
}

bool ResourceRoom::save_file_arf(std::ofstream& of)
{
    // TODO: implement this method.
    of << "# room" << endl << endl;
    of << endl << endl;
    if (m_comment != "")
    {
        of << "comment: " + m_comment << endl;
    }
    if (m_data.m_caption != "")
    {
        of << "caption: " + m_data.m_caption << endl;
    }
    of << "dimensions: " << to_string(m_data.m_dimensions) << endl;
    of << "speed: " << to_string(m_data.m_speed) << endl;
    of << "colour: 0x" << std::hex << m_data.m_colour << endl;
    if (!m_data.m_show_colour)
    {
        of << "show_color: false" << endl;
    }
    if (m_data.m_persistent)
    {
        of << "persistent: true" << endl;
    }
    if (m_snap.x != 16 || m_snap.y != 16)
    {
        of << "snap: " << to_string(m_snap) << endl;
    }
    if (m_isometric)
    {
        of << "isometric: true" << endl;
    }
    // TODO: only write enable views if differs from expectation.
    if (m_data.m_enable_views)
    {
        of << "enable_views: true" << endl;
    }
    else
    {
        of << "enable_views: false" << endl;
    }

    // backgrounds
    for (const BackgroundLayerDefinition& def : m_backgrounds)
    {
        // ignore backgrounds without names.
        if (def.m_background_name != "")
        {
            of << hrule_string("# background " + def.m_background_name + " ") << endl;
            of << endl;
            if (def.m_position.x != 0 || def.m_position.y != 0)
            {
                of << "position: " << to_string(def.m_position) << endl;
            }
            if (def.m_velocity.x != 0 || def.m_velocity.y != 0)
            {
                of << "speed: " << to_string(def.m_velocity) << endl;
            }
            if (def.m_tiled_x || def.m_tiled_y)
            {
                of << "tiled:"
                  << to_string(geometry::Vector<int32_t>{ def.m_tiled_x, def.m_tiled_y }) << endl;
            }
            if (def.m_foreground)
            {
                of << "foreground: true" << endl;
            }
            if (def.m_stretch)
            {
                of << "stretch: true" << endl;
            }
            of << endl;
        }
    }

    // views
    for (const asset::AssetRoom::ViewDefinition& view : m_data.m_views)
    {
        // skip views that are not set up
        if (view.m_visible || !view.m_position.is_zero() || !view.m_velocity.is_zero())
        {
            of << hrule_string("# view ") << endl;
            if (!view.m_visible)
            {
                of << "visible: false" << endl;
            }
            of << "offset: " << to_string(view.m_position) << endl;
            of << "dimensions: " << to_string(view.m_dimension) << endl;
            if (!view.m_viewport.m_start.is_zero())
            {
                of << "port_offset: " << to_string(view.m_viewport.m_start) << endl;
            }
            if (view.m_viewport.diagonal() != view.m_dimension)
            {
                of << "port_dimensions: " << to_string(view.m_viewport.diagonal()) << endl;
            }
            if (!view.m_border.is_zero())
            {
                of << "border: " << to_string(view.m_border) << endl;
            }
            of << endl;
        }
    }

    // room cc

    if (m_cc_room.m_source.length() > 0)
    {
        of << hrule_string("# cc room ") << endl;
        of << m_cc_room.m_source << endl;
    }

    // instances
    for (const InstanceDefinition& instance : m_instances)
    {
        bool is_simple = true;
        if (instance.m_name != "" || instance.m_scale.x != 1 || instance.m_scale.y != 1)
        {
            is_simple = false;
        }
        if (instance.m_angle != 0 || (instance.m_colour != 0xffffffff && instance.m_colour != 0xffffff) || instance.m_locked || instance.m_code != "")
        {
            is_simple = false;
        }

        if (is_simple)
        {
            of << "# instance " << instance.m_object_name << " " << to_string(instance.m_position) << endl;
        }
        else
        {
            of << hrule_string("# instance " + instance.m_object_name + " " + to_string(instance.m_position) + " ") << endl;
            of << endl;
            bool endl_needed = false;
            if (instance.m_scale.x != 1 || instance.m_scale.y != 1)
            {
                of << "scale: " << to_string(instance.m_scale) << endl;
                endl_needed = true;
            }
            if (instance.m_angle != 0)
            {
                of << "angle: " << to_string(instance.m_scale) << endl;
                endl_needed = true;
            }
            if (instance.m_colour != 0xffffffff)
            {
                of << "colour: 0x" << std::hex << instance.m_colour << endl;
                endl_needed = true;
            }
            if (endl_needed) of << endl;

            if (instance.m_code.length() > 0)
            {
                of << hrule_string("# cc instance ") << endl;
                of << instance.m_code << endl;
            }
        }
    }

    // TODO: tiles

    return false;
}

void ResourceRoom::load_file_arf()
{
    // TODO -- case-insensitive native path
    std::string _path = native_path(m_path);
    std::string _dir = path_directory(_path);
    std::string file_contents = read_file_contents(_path.c_str());

    ARFSection room_section;

    arf_parse(arf_room_schema, file_contents.c_str(), room_section);

    std::vector<std::string_view> arr;
    std::string arrs;

    // dimensions
    arrs = room_section.get_value("dimensions", room_section.get_value("size", "[640, 480]"));
    arf_parse_array(arrs.c_str(), arr);
    if (arr.size() != 2) throw ResourceError(ErrorCode::F::arfdim, this, "field \"dimensions\" should be a 2-tuple.");
    m_data.m_dimensions.x = svtod(arr[0]);
    m_data.m_dimensions.y = svtod(arr[1]);
    arr.clear();

    m_comment = room_section.get_value("comment", "");

    m_data.m_caption = room_section.get_value("caption", "");

    m_data.m_speed = std::stod(room_section.get_value("speed", "60"));
    m_data.m_persistent = room_section.get_value("persistent", "0") != "0";

    m_data.m_show_colour = true;
    std::string colour = room_section.get_value(
        "colour",
        room_section.get_value("color", "0x808080")
    );

    m_data.m_colour = svtocolour(colour);

    if (room_section.has_value("show_colour") || room_section.has_value("show_color"))
    {
        m_data.m_show_colour = room_section.get_value(
            "show_color",
            room_section.get_value("show_color", "1")
        ) != "0";
    }

    // set this later.
    std::string views = "disabled";

    // sections
    for (ARFSection* section : room_section.m_sections)
    {
        if (section->m_name == "cc")
        {
            // room creation code
            if (section->m_details.size() > 0)
            {
                if (section->m_details.at(0) != "room")
                {
                    throw ResourceError(ErrorCode::F::arfcc, this, "encountered cc \"{}\", expected room cc.", section->m_details.at(0));
                }
            }

            m_cc_room.m_source = section->m_text;
            trim(m_cc_room.m_source);
        }
        else if (section->m_name == "background")
        {
            // background
            if (section->m_details.size() != 1)
            {
                throw ResourceError(ErrorCode::F::arfbgdet, this, "expected background name as detail.");
            }
            std::string bgname_s = section->m_details.at(0);
            m_backgrounds.emplace_back();
            BackgroundLayerDefinition& def = m_backgrounds.back();
            def.m_background_name = bgname_s;

            // offset
            arrs = section->get_value(
                "offset", section->get_value("position", "[0, 0]")
            );
            arf_parse_array(arrs.c_str(), arr);
            if (arr.size() != 2) throw ResourceError(ErrorCode::F::arfoffset, this, "field \"offset\" or \"position\" should be a 2-tuple.");
            def.m_position.x = svtod(arr[0]);
            def.m_position.y = svtod(arr[1]);
            arr.clear();

            // velocity
            arrs = section->get_value(
                "velocity", section->get_value("speed", "[0, 0]")
            );
            arf_parse_array(arrs.c_str(), arr);
            if (arr.size() != 2) throw ResourceError(ErrorCode::F::arfspd, this, "field \"velocity\" or \"speed\" should be a 2-tuple.");
            def.m_velocity.x = svtod(arr[0]);
            def.m_velocity.y = svtod(arr[1]);
            arr.clear();

            // tiled
            arrs = section->get_value(
                "tiled", "[1, 1]"
            );
            arf_parse_array(arrs.c_str(), arr);
            if (arr.size() != 2) throw ResourceError(ErrorCode::F::arftiled, this, "field \"tiled\" should be a 2-tuple.");
            def.m_tiled_x = arr[0] != "0";
            def.m_tiled_y = arr[1] != "0";
            arr.clear();

            // flags
            def.m_visible = section->get_value(
                "visible", "1"
            ) != "0";
            def.m_foreground = section->get_value(
                "foreground", "0"
            ) != "0";

            // default to enabled.
            views = "enabled";
        }
        else if (section->m_name == "view")
        {
            auto& view = m_data.m_views.emplace_back();

            // offset
            arrs = section->get_value(
                "offset", section->get_value("position", "[0, 0]")
            );
            arf_parse_array(arrs.c_str(), arr);
            if (arr.size() != 2) throw ResourceError(ErrorCode::F::arfoffset, this, "field \"offset\" or \"position\" should be a 2-tuple.");
            view.m_position.x = svtod(arr[0]);
            view.m_position.y = svtod(arr[1]);
            arr.clear();

            // dimensions
            arrs = section->get_value(
                "dimensions", section->get_value("size", "[-1, -1]")
            );
            arf_parse_array(arrs.c_str(), arr);
            if (arr.size() != 2) throw ResourceError(ErrorCode::F::arfdim, this, "field \"dimensions\" or \"size\" should be a 2-tuple.");
            view.m_dimension.x = svtod(arr[0]);
            view.m_dimension.y = svtod(arr[1]);
            if (view.m_dimension.x < 0)
            {
                view.m_dimension.x = m_data.m_dimensions.x;
            }
            if (view.m_dimension.y < 0)
            {
                view.m_dimension.y = m_data.m_dimensions.y;
            }
            arr.clear();

            // flags
            view.m_visible = section->get_value(
                "visible", "1"
            ) != "0";
        }
        else if (section->m_name == "instance")
        {
            if (section->m_details.empty())
            {
                throw ResourceError(ErrorCode::F::arfobjdet, this, "instance requires object detail.");
            }
            else
            {
                InstanceDefinition& def = m_instances.emplace_back();
                def.m_object_name = section->m_details.at(0);

                // position
                if (section->m_details.size() >= 2)
                {
                    arrs = section->m_details.at(1);
                    arf_parse_array(arrs.c_str(), arr);
                    if (arr.size() != 2) ResourceError(ErrorCode::F::arfposdet, this, "position detail should be a 2-tuple.");
                    def.m_position.x = svtod(arr[0]);
                    def.m_position.y = svtod(arr[1]);
                    arr.clear();
                }
                else
                {
                    def.m_position = {1, 1};
                }

                // scale
                arrs = section->get_value(
                    "scale", "[1, 1]"
                );
                arf_parse_array(arrs.c_str(), arr);
                if (arr.size() != 2) ResourceError(ErrorCode::F::arfscale, this, "field \"scale\" should be a 2-tuple.");
                def.m_scale.x = svtod(arr[0]);
                def.m_scale.y = svtod(arr[1]);
                arr.clear();

                // angle
                def.m_angle = svtod(section->get_value("angle", "0"));

                // colour
                def.m_colour = svtocolour(
                    section->get_value("color",
                        section->get_value("colour",
                            section->get_value("blend" ,"0xffffff")
                        )
                    )
                );

                for (ARFSection* cc : section->m_sections)
                {
                    if (cc->m_name == "cc")
                    {
                        if (cc->m_details.empty() || cc->m_details.at(0) == "instance")
                        {
                            def.m_code = cc->m_text;
                        }
                        else
                        {
                            throw ResourceError(ErrorCode::F::arfcc, this, "encountered cc \"\"; expected cc instance.", cc->m_details.at(0));
                        }
                    }
                }
            }
        }
    }

    // views enabled?
    views = room_section.get_value("views", views);
    if (is_digits(views))
    {
        m_data.m_enable_views = views != "0";
    }
    else
    {
        if (views == "enabled")
        {
            m_data.m_enable_views = true;
        }
        else if (views == "disabled")
        {
            m_data.m_enable_views = false;
        }
        else
        {
            throw ResourceError(ErrorCode::F::arfunkview, this, "unrecognized \"views: \"", views);
        }
    }
}
    
}