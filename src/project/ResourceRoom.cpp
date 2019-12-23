#include "ogm/project/resource/ResourceRoom.hpp"

#include "ogm/common/util.hpp"
#include "ogm/common/types.hpp"
#include "ogm/project/arf/arf_parse.hpp"

#include "ogm/ast/parse.h"

#include <pugixml.hpp>
#include <string>

namespace ogm { namespace project {

ResourceRoom::ResourceRoom(const char* path, const char* name): m_path(path), m_name(name)
{ }

void ResourceRoom::load_file()
{
    if (mark_progress(FILE_LOADED)) return;
    if (ends_with(m_path, ".gmx"))
    {
        load_file_xml();
    }
    else if (ends_with(m_path, ".ogm") || ends_with(m_path, ".arf"))
    {
        load_file_arf();
    }
    else
    {
        throw MiscError("Unrecognized file extension for room file " + m_path);
    }
}

bool ResourceRoom::save_file()
{
    std::ofstream of;
    of.open(m_path);
    if (!of.good()) return false;

    if (ends_with(m_path, ".gmx"))
    {
        return save_file_xml(of);
    }
    else if (ends_with(m_path, ".ogm") || ends_with(m_path, ".arf"))
    {
        return save_file_arf(of);
    }
    else
    {
        return false;
    }
}

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
}

bool ResourceRoom::save_file_arf(std::ofstream& of)
{
    const std::string endl = "\r\n";

    // TODO: implement this method.
    of << "# room" << endl;
    of << endl;

    return false;
}

// 0 or -1
int32_t bool_to_save_int(bool b)
{
    return b
        ? -1
        : 0;
}

std::string quote(std::string s)
{
    return "\"" + s + "\"";
}

std::string quote(real_t r)
{
    return quote(std::to_string(r));
}

std::string quote(int32_t r)
{
    return quote(std::to_string(r));
}

std::string quote(uint32_t r)
{
    return quote(std::to_string(r));
}

std::string quote(int64_t r)
{
    return quote(std::to_string(r));
}

std::string quote(bool b)
{
    return "\"" + std::to_string(bool_to_save_int(b)) + "\"";
}

bool ResourceRoom::save_file_xml(std::ofstream& of)
{
    // endline character
    const std::string endl = "\r\n";

    // indent
    const std::string indent = "  ";
    const std::string indent2 = indent + indent;

    // comment
    if (m_comment != "")
    {
        of << "<!--" << m_comment << "-->" << endl;
    }

    of << "<room>" << endl;
    {
        of << indent << "<caption>" << m_data.m_caption << "</caption>" << endl;
        of << indent << "<width>" << m_data.m_dimensions.x << "</width>" << endl;
        of << indent << "<height>" << m_data.m_dimensions.y << "</height>" << endl;
        of << indent << "<vsnap>" << m_snap.x << "</vsnap>" << endl;
        of << indent << "<hsnap>" << m_snap.y << "</hsnap>" << endl;
        of << indent << "<isometric>" << bool_to_save_int(m_isometric) << "</isometric>" << endl;
        of << indent << "<colour>" << m_data.m_colour << "</colour>" << endl;
        of << indent << "<showcolour>" << bool_to_save_int(m_data.m_show_colour) << "</showcolour>" << endl;
        std::string source = m_cc_room.m_source;
        xml_sanitize(source);
        of << indent << "<code>" << source << "</code>" << endl;
        of << indent << "<enableViews>" << bool_to_save_int(m_data.m_enable_views) << "</enableViews>" << endl;
        of << indent << "<makerSettings>" << endl;
        {
            of << indent2 << "<isSet>0</isSet>" << endl;
            of << indent2 << "<w>0</w>" << endl;
            of << indent2 << "<h>0</h>" << endl;
            of << indent2 << "<showGrid>0</showGrid>" << endl;
            of << indent2 << "<showTiles>0</showTiles>" << endl;
            of << indent2 << "<showBackgrounds>0</showBackgrounds>" << endl;
            of << indent2 << "<showForegrounds>0</showForegrounds>" << endl;
            of << indent2 << "<showViews>0</showViews>" << endl;
            of << indent2 << "<deleteUnderlyingObj>0</deleteUnderlyingObj>" << endl;
            of << indent2 << "<deleteUnderlyingTiles>0</deleteUnderlyingTiles>" << endl;
            of << indent2 << "<page>0</page>" << endl;
            of << indent2 << "<xoffset>0</xoffset>" << endl;
            of << indent2 << "<yoffset>0</yoffset>" << endl;
        }
        of << indent << "</makerSettings>" << endl;

        // backgrounds
        of << indent << "<backgrounds>" << endl;
        for (const BackgroundLayerDefinition& layer : m_backgrounds)
        {
            of << indent2 << "<view";
            of << " visible=" + quote(layer.m_visible);
            of << " foreground=" + quote(layer.m_foreground);
            of << " name=" + quote(layer.m_background_name);
            of << " x=" + quote(layer.m_position.x);
            of << " y=" + quote(layer.m_position.y);
            of << " htiled=" + quote(layer.m_tiled_x);
            of << " vtiled=" + quote(layer.m_tiled_y);
            of << " hspeed=" + quote(layer.m_velocity.x);
            of << " vspeed=" + quote(layer.m_velocity.y);
            of << " stretch=" + quote(layer.m_stretch);
            of << "/>" << endl;
        }
        of << indent << "</backgrounds>" << endl;

        // views
        of << indent << "<views>" << endl;
        for (const asset::AssetRoom::ViewDefinition& view : m_data.m_views)
        {
            of << indent2 << "<view";
            of << " visible=" + quote(view.m_visible);
            of << " objName=" + quote("&lt;undefined&gt;");
            of << " xview=" + quote(view.m_position.x);
            of << " yview=" + quote(view.m_position.y);
            of << " wview=" + quote(view.m_dimension.x);
            of << " hview=" + quote(view.m_dimension.y);
            of << " xport=" + quote(view.m_viewport.left());
            of << " yport=" + quote(view.m_viewport.top());
            of << " wport=" + quote(view.m_viewport.width());
            of << " hport=" + quote(view.m_viewport.height());
            of << " hborder=" + quote(view.m_border.x);
            of << " vborder=" + quote(view.m_border.y);
            of << " hspeed=" + quote(view.m_velocity.x);
            of << " vspeed=" + quote(view.m_velocity.y);
            of << "/>" << endl;
        }
        of << indent << "</views>" << endl;

        // instances
        if (m_instances.empty())
        {
            of << indent << "<instances/>" << endl;
        }
        else
        {
            of << indent << "<instances>" << endl;
            for (const InstanceDefinition& instance : m_instances)
            {
                of << indent2 << "<instance";
                of << " objName=" + quote(instance.m_object_name);
                of << " x=" + quote(instance.m_position.x);
                of << " y=" + quote(instance.m_position.y);
                of << " name=" + quote(instance.m_name);
                of << " locked=" + quote(instance.m_locked);
                std::string code = instance.m_code;
                of << " code=" + quote(code);
                of << " scaleX=" + quote(instance.m_scale.x);
                of << " scaleY=" + quote(instance.m_scale.y);
                // TODO: alpha is in here somewhere.
                of << " colour=" + quote(instance.m_colour);
                of << " rotation=" + quote(instance.m_angle);
                of << "/>" << endl;
            }
            of << indent << "</instances>" << endl;
        }

        // TODO: tiles
        // instances
        if (m_tiles.empty())
        {
            of << indent << "<tiles/>" << endl;
        }
        else
        {
            of << indent << "<tiles>" << endl;
            for (const TileDefinition& tile : m_tiles)
            {
                of << indent2 << "<tile";
                of << " bgName=" + quote(tile.m_background_name);
                of << " x=" + quote(tile.m_position.x);
                of << " y=" + quote(tile.m_position.y);
                of << " w=" + quote(tile.m_dimensions.x);
                of << " h=" + quote(tile.m_dimensions.y);
                of << " xo=" + quote(tile.m_bg_position.x);
                of << " yo=" + quote(tile.m_bg_position.x);
                of << " id=" + quote(tile.m_id);
                of << " depth=" + quote(tile.m_depth);
                of << " locked=" + quote(tile.m_locked);
                // TODO: verify this is how alpha is encoded
                uint32_t alpha = 0xff * tile.m_alpha;
                of << " colour=" + quote(tile.m_blend | (alpha << 24));
                of << " scaleX=" + quote(tile.m_scale.x);
                of << " scaleY=" + quote(tile.m_scale.y);
                of << "/>" << endl;
            }
            of << indent << "</tiles>" << endl;
        }
    }


    // FIXME: do physics world properly
    of << indent << "<PhysicsWorld>0</PhysicsWorld>" << endl;
    of << indent << "<PhysicsWorldTop>0</PhysicsWorldTop>" << endl;
    of << indent << "<PhysicsWorldLeft>0</PhysicsWorldLeft>" << endl;
    of << indent << "<PhysicsWorldRight>1024</PhysicsWorldRight>" << endl;
    of << indent << "<PhysicsWorldBottom>768</PhysicsWorldBottom>" << endl;
    of << indent << "<PhysicsWorldGravityX>0</PhysicsWorldGravityX>" << endl;
    of << indent << "<PhysicsWorldGravityY>10</PhysicsWorldGravityY>" << endl;
    of << indent << "<PhysicsWorldPixToMeters>0.100000001490116</PhysicsWorldPixToMeters>" << endl;

    of << "</room>" << endl;

    return true;
}

void ResourceRoom::load_file_arf()
{
    // TODO -- case-insensitive native path
    std::string _path = native_path(m_path);
    std::string _dir = path_directory(_path);
    std::string file_contents = read_file_contents(_path.c_str());

    ARFSection room_section;

    try
    {
        arf_parse(arf_room_schema, file_contents.c_str(), room_section);
    }
    catch (std::exception& e)
    {
        throw MiscError(
            "Error parsing room file \"" + _path + "\": " + e.what()
        );
    }

    std::vector<std::string_view> arr;
    std::string arrs;

    // dimensions
    arrs = room_section.get_value("dimensions", room_section.get_value("size", "[640, 480]"));
    arf_parse_array(arrs.c_str(), arr);
    if (arr.size() != 2) throw MiscError("field \"dimensions\" should be a 2-tuple.");
    m_data.m_dimensions.x = svtod(arr[0]);
    m_data.m_dimensions.y = svtod(arr[1]);
    arr.clear();

    m_comment = room_section.get_value("comment", "");

    m_data.m_caption = room_section.get_value("caption", "");

    m_data.m_speed = std::stod(room_section.get_value("speed", "60"));

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
                    throw MiscError("encountered cc \"" + section->m_details.at(0) + "\", expected room cc.");
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
                throw MiscError("expected background name as detail.");
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
            if (arr.size() != 2) throw MiscError("field \"offset\" or \"position\" should be a 2-tuple.");
            def.m_position.x = svtod(arr[0]);
            def.m_position.y = svtod(arr[1]);
            arr.clear();

            // velocity
            arrs = section->get_value(
                "velocity", section->get_value("speed", "[0, 0]")
            );
            arf_parse_array(arrs.c_str(), arr);
            if (arr.size() != 2) throw MiscError("field \"velocity\" or \"speed\" should be a 2-tuple.");
            def.m_velocity.x = svtod(arr[0]);
            def.m_velocity.y = svtod(arr[1]);
            arr.clear();

            // tiled
            arrs = section->get_value(
                "tiled", "[1, 1]"
            );
            arf_parse_array(arrs.c_str(), arr);
            if (arr.size() != 2) throw MiscError("field \"tiled\" should be a 2-tuple.");
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
            if (arr.size() != 2) throw MiscError("field \"offset\" or \"position\" should be a 2-tuple.");
            view.m_position.x = svtod(arr[0]);
            view.m_position.y = svtod(arr[1]);
            arr.clear();

            // dimensions
            arrs = section->get_value(
                "dimensions", section->get_value("size", "[-1, -1]")
            );
            arf_parse_array(arrs.c_str(), arr);
            if (arr.size() != 2) throw MiscError("field \"dimensions\" or \"size\" should be a 2-tuple.");
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
                throw MiscError("instance requires object detail.");
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
                    if (arr.size() != 2) throw MiscError("position detail should be a 2-tuple.");
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
                if (arr.size() != 2) throw MiscError("field \"scale\" should be a 2-tuple.");
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
                            throw MiscError("encountered cc \"" + cc->m_details.at(0) + "\"; expected cc instance.");
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
            throw MiscError("unrecognized \"views: " + views + "\"");
        }
    }
}

void ResourceRoom::load_file_xml()
{
    std::string _path = native_path(m_path);
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(_path.c_str(), pugi::parse_default | pugi::parse_escapes | pugi::parse_comments);

    // TODO: check result
    (void)result;

    pugi::xml_node comment = doc.first_child();

    if (!comment.name() || strcmp(comment.name(), "room") != 0)
    {
        m_comment = doc.first_child().text().get();
    }

    pugi::xml_node node_room = doc.child("room");

    // FIXME: error checking
    std::string s_w = node_room.child("width").text().get();
    std::string s_h = node_room.child("height").text().get();
    std::string s_speed = node_room.child("speed").text().get();
    std::string s_enable_views = node_room.child("enableViews").text().get();
    std::string s_colour = node_room.child("colour").text().get();
    std::string s_show_colour = node_room.child("showcolour").text().get();
    std::string s_caption = node_room.child("caption").text().get();
    m_data.m_dimensions.x = std::stod(s_w);
    m_data.m_dimensions.y = std::stod(s_h);
    m_data.m_speed = std::stod(s_speed);
    m_data.m_show_colour = s_show_colour != "0";
    m_data.m_colour = stoi(s_colour);
    m_data.m_caption = s_caption;
    // TODO: remaining room properties

    // room cc
    m_cc_room.m_source = node_room.child("code").text().get();
    trim(m_cc_room.m_source);

    // backgrounds
    pugi::xml_node node_bgs = node_room.child("backgrounds");
    for (pugi::xml_node node_bg: node_bgs.children("background"))
    {
        std::string bgname_s = node_bg.attribute("name").value();
        std::string x_s = node_bg.attribute("x").value();
        std::string y_s = node_bg.attribute("y").value();
        std::string vx_s = node_bg.attribute("hspeed").value();
        std::string vy_s = node_bg.attribute("vspeed").value();
        bool visible = node_bg.attribute("visible").value() != std::string("0");
        bool foreground = node_bg.attribute("foreground").value() != std::string("0");
        bool htiled = node_bg.attribute("htiled").value() != std::string("0");
        bool vtiled = node_bg.attribute("vtiled").value() != std::string("0");
        bool stretch = node_bg.attribute("stretch").value() != std::string("0");

        if (bgname_s != "" && bgname_s != "<undefined>")
        {
            m_backgrounds.emplace_back();
            BackgroundLayerDefinition& def = m_backgrounds.back();
            def.m_background_name = bgname_s;
            def.m_position = { std::stod(x_s), std::stod(y_s) };
            def.m_velocity = { std::stod(vx_s), std::stod(vy_s) };
            def.m_tiled_x = htiled;
            def.m_tiled_y = vtiled;
            def.m_visible = visible;
            def.m_foreground = foreground;
            def.m_stretch = stretch;

            // TODO: bg speed, stretch.
        }
    }

    // tiles
    pugi::xml_node node_tiles = node_room.child("tiles");

    for (pugi::xml_node node_tile: node_tiles.children("tile"))
    {
        // OPTIMIZE -- change strings out for char* or string_view
        std::string bgname_s = node_tile.attribute("bgName").value();
        std::string id_s = node_tile.attribute("id").value();
        trim(bgname_s);
        std::string x_s = node_tile.attribute("x").value();
        std::string y_s = node_tile.attribute("y").value();
        std::string w_s = node_tile.attribute("w").value();
        std::string h_s = node_tile.attribute("h").value();
        std::string xo_s = node_tile.attribute("xo").value();
        std::string yo_s = node_tile.attribute("yo").value();
        std::string name_s = node_tile.attribute("name").value();
        std::string scale_x_s = node_tile.attribute("scaleX").value();
        std::string scale_y_s = node_tile.attribute("scaleY").value();
        std::string colour_s = node_tile.attribute("colour").value();
        std::string depth_s = node_tile.attribute("depth").value();

        if (bgname_s != "" && bgname_s != "<undefined>")
        {
            real_t depth = std::stof(depth_s);

            TileDefinition& def = m_tiles.emplace_back();

            // define the tile
            def.m_background_name = bgname_s;
            def.m_id = std::stoi(id_s);
            def.m_depth = std::stod(depth_s);
            def.m_position = { std::stof(x_s), std::stof(y_s) };
            def.m_bg_position = { std::stof(xo_s), std::stof(yo_s) };
            def.m_scale = { std::stof(scale_x_s), std::stof(scale_y_s) };
            def.m_dimensions = { std::stof(w_s), std::stof(h_s) };
            def.m_blend = std::stoll(colour_s);
        }
    }

    // instances
    pugi::xml_node node_instances = node_room.child("instances");
    for (pugi::xml_node node_instance: node_instances.children("instance"))
    {
        // OPTIMIZE -- change strings out for char*
        std::string objname_s = node_instance.attribute("objName").value();
        trim(objname_s);
        std::string x_s = node_instance.attribute("x").value();
        std::string y_s = node_instance.attribute("y").value();
        std::string name_s = node_instance.attribute("name").value();
        std::string code_s = node_instance.attribute("code").value();
        trim(code_s);
        std::string scale_x_s = node_instance.attribute("scaleX").value();
        std::string scale_y_s = node_instance.attribute("scaleY").value();
        std::string rotation_s = node_instance.attribute("rotation").value();
        std::string colour_s = node_instance.attribute("colour").value();

        if (objname_s != "" && objname_s != "<undefined>")
        {
            // instance parameters
            InstanceDefinition& def = m_instances.emplace_back();
            def.m_name = name_s;
            def.m_object_name = objname_s;
            def.m_position = { std::stof(x_s), std::stof(y_s) };
            def.m_scale = { std::stof(scale_x_s), std::stof(scale_y_s) };
            def.m_code = code_s;
            def.m_angle = std::stod(rotation_s);
            // TODO: alpha seems to be in here also.
            def.m_colour = std::stoll(colour_s);
        }
    }

    // views
    pugi::xml_node node_views = node_room.child("views");
    m_data.m_enable_views = s_enable_views != "0";
    for (pugi::xml_node node_view: node_views.children("view"))
    {
        std::string visible = node_view.attribute("visible").value();
        std::string xview = node_view.attribute("xview").value();
        std::string yview = node_view.attribute("yview").value();
        std::string wview = node_view.attribute("wview").value();
        std::string hview = node_view.attribute("hview").value();
        std::string xport = node_view.attribute("xport").value();
        std::string yport = node_view.attribute("yport").value();
        std::string wport = node_view.attribute("wport").value();
        std::string hport = node_view.attribute("hport").value();
        std::string hborder = node_view.attribute("hborder").value();
        std::string vborder = node_view.attribute("vborder").value();
        std::string hspeed = node_view.attribute("hspeed").value();
        std::string vspeed = node_view.attribute("vspeed").value();

        m_data.m_views.emplace_back();
        asset::AssetRoom::ViewDefinition& def = m_data.m_views.back();
        def.m_visible = visible != "0";
        def.m_position = { std::stod(xview), std::stod(yview) };
        def.m_dimension = { std::stod(wview), std::stod(hview) };
        def.m_viewport = {
            std::stod(xview), std::stod(yview),
            std::stod(wview), std::stod(hview)
        };
        def.m_border = {
            std::stod(hborder), std::stod(vborder)
        };
        def.m_velocity = {
            std::stod(hspeed), std::stod(vspeed)
        };
    }
}

void ResourceRoom::precompile(bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(PRECOMPILED)) return;
    asset_index_t asset_index;
    m_asset_room = acc.m_assets->add_asset<asset::AssetRoom>(m_name.c_str(), &asset_index);
    *m_asset_room = m_data;

    if (m_cc_room.m_source != "")
    {
        m_cc_room.m_bytecode_index = acc.next_bytecode_index();
        m_cc_room.m_ast = ogm_ast_parse(
            m_cc_room.m_source.c_str(), ogm_ast_parse_flag_no_decorations
        );
        m_cc_room.m_name = "cc for room " + m_name;
    }
    else
    {
        m_cc_room.m_bytecode_index = bytecode::k_no_bytecode;
    }

    m_asset_room->m_cc = m_cc_room.m_bytecode_index;

    // backgrounds
    for (const BackgroundLayerDefinition& _def: m_backgrounds)
    {
        asset_index_t background_asset;
        const asset::AssetBackground* bg_asset = dynamic_cast<asset::AssetBackground*>(acc.m_assets->get_asset(_def.m_background_name.c_str(), background_asset));
        if (bg_asset)
        {
            auto& def = m_asset_room->m_bg_layers.emplace_back();
            def.m_background_index = background_asset;
            def.m_position = _def.m_position;
            def.m_velocity = _def.m_velocity;
            def.m_tiled_x = _def.m_tiled_x;
            def.m_tiled_y = _def.m_tiled_y;
            def.m_visible = _def.m_visible;
            def.m_foreground = _def.m_foreground;
        }
    }

    // tiles

    // cache
    std::map<std::string, std::pair<asset_index_t, const asset::AssetBackground*>> background_cache;
    typedef decltype(asset::AssetRoom::TileLayerDefinition::m_contents) TileVector;
    std::map<real_t, TileVector> layers;

    for (const TileDefinition& _def: m_tiles)
    {
        // FIXME: rename background_asset -> background_index
        asset_index_t background_asset;
        // FIXME: rename object_asset -> background_asset
        const asset::AssetBackground* object_asset;

        // lookup tile
        auto cache_iter = background_cache.find(_def.m_background_name);
        if (cache_iter == background_cache.end())
        {
            object_asset = dynamic_cast<asset::AssetBackground*>(acc.m_assets->get_asset(_def.m_background_name.c_str(), background_asset));
            background_cache[_def.m_background_name] = { background_asset, object_asset };
        }
        else
        {
            background_asset = cache_iter->second.first;
            object_asset = cache_iter->second.second;
        }

        if (object_asset)
        {
            // get the appropriate layer for this tile
            auto& layer = layers[_def.m_depth];

            // add a new tile definition to the layer
            layer.emplace_back();
            asset::AssetRoom::TileDefinition& def = layer.back();

            // TODO: use actual ID tag (requires a lot of gruntwork to separate out the instance IDs, though.)
            instance_id_t id;
            {
                // single-threaded access, so no lock needed.
                id = acc.m_config->m_next_instance_id++;
            }

            // define the tile
            def.m_background_index = background_asset;
            def.m_id = id;
            def.m_position = _def.m_position;
            def.m_bg_position = _def.m_bg_position;
            def.m_scale = _def.m_scale;
            def.m_dimension = _def.m_dimensions;
            def.m_blend = _def.m_blend;
        }
        else
        {
            MiscError("Room references background resource '" + _def.m_background_name + ", which does not exist.");
        }
    }

    // add the tile layers defined above to the room asset.
    for (auto& pair : layers)
    {
        m_asset_room->m_tile_layers.emplace_back();
        asset::AssetRoom::TileLayerDefinition& layer = m_asset_room->m_tile_layers.back();
        layer.m_depth = std::get<0>(pair);
        layer.m_contents.swap(std::get<1>(pair));
    }

    // objects
    for (const InstanceDefinition& _def : m_instances)
    {
        asset_index_t object_index;
        const asset::AssetObject* object_asset = dynamic_cast<asset::AssetObject*>(acc.m_assets->get_asset(_def.m_object_name.c_str(), object_index));
        if (object_asset)
        {
            instance_id_t id;
            {
                // single-threaded access, so no lock needed.
                id = acc.m_config->m_next_instance_id++;
            }
            m_asset_room->m_instances.emplace_back();
            asset::AssetRoom::InstanceDefinition& def = m_asset_room->m_instances.back();

            // TODO: what to do with m_name?

            def.m_id = id;
            def.m_object_index = object_index;
            def.m_position = _def.m_position;
            def.m_scale = _def.m_scale;
            def.m_angle = _def.m_angle;
            def.m_blend = _def.m_colour;

            // creation code
            if (_def.m_code != "")
            {
                m_cc_instance.emplace_back();
                CreationCode& cc = m_cc_instance.back();
                cc.m_source = _def.m_code;
                cc.m_ast = ogm_ast_parse(
                    _def.m_code.c_str(), ogm_ast_parse_flag_no_decorations
                );
                cc.m_name = "cc for instance " + _def.m_name + " (id " + std::to_string(id) + ")";
                cc.m_bytecode_index = acc.next_bytecode_index();
                def.m_cc = cc.m_bytecode_index;
            }
            else
            {
                def.m_cc = bytecode::k_no_bytecode;
            }
        }
        else
        {
            MiscError("Room references object '" + _def.m_object_name + ", which does not exist.");
        }
    }

    // add backgrounds and views to get up to 8 in each
    while (m_asset_room->m_bg_layers.size() < 8)
    {
        m_asset_room->m_bg_layers.emplace_back();
    }

    while (m_asset_room->m_views.size() < 8)
    {
        m_asset_room->m_views.emplace_back();
    }
}

void ResourceRoom::compile(bytecode::ProjectAccumulator& acc, const bytecode::Library* library)
{
    if (mark_progress(COMPILED)) return;
    // compile room cc
    if (m_cc_room.m_bytecode_index != bytecode::k_no_bytecode)
    {
        bytecode::Bytecode b;
        ogm::bytecode::bytecode_generate(
            b,
            {m_cc_room.m_ast, 0, 0, (m_cc_room.m_name).c_str(), m_cc_room.m_source.c_str()},
            library, &acc);
        acc.m_bytecode->add_bytecode(m_cc_room.m_bytecode_index, std::move(b));
    }

    // compile instance cc
    for (auto& cc : m_cc_instance)
    {
        bytecode::Bytecode b;
        ogm::bytecode::bytecode_generate(
            b,
            {cc.m_ast, 0, 0, (cc.m_name).c_str(), cc.m_source.c_str()},
            library,
            &acc);
        acc.m_bytecode->add_bytecode(cc.m_bytecode_index, std::move(b));
    }
}
}}
