#include "ogm/project/resource/ResourceRoom.hpp"

#include "ogm/common/util.hpp"
#include "ogm/common/types.hpp"
#include "project/XMLError.hpp"

#include <pugixml.hpp>

namespace ogm::project
{
    
namespace
{
    // 0 or -1
    int32_t bool_to_save_int(bool b)
    {
        return b
            ? -1
            : 0;
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

    std::string quote(std::string s)
    {
        return "\"" + s + "\"";
    }

    std::string quote(double r)
    {
        return quote(to_string(r));
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
}
    
void ResourceRoom::load_file_xml()
{
    std::string _path = native_path(m_path);
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(_path.c_str(), pugi::parse_default | pugi::parse_escapes | pugi::parse_comments);

    check_xml_result<ResourceError>(ErrorCode::F::filexml, result, _path.c_str(), "Error parsing room " + _path, this);

    pugi::xml_node comment = doc.first_child();

    if (!comment.name() || strcmp(comment.name(), "room") != 0)
    {
        m_comment = comment.value();
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

        asset::AssetRoom::ViewDefinition& def = m_data.m_views.emplace_back();
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

bool ResourceRoom::save_file_xml(std::ofstream& of)
{
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
        of << indent << "<width>" << to_string(m_data.m_dimensions.x) << "</width>" << endl;
        of << indent << "<height>" << to_string(m_data.m_dimensions.y) << "</height>" << endl;
        of << indent << "<vsnap>" << to_string(m_snap.x) << "</vsnap>" << endl;
        of << indent << "<hsnap>" << to_string(m_snap.y) << "</hsnap>" << endl;
        of << indent << "<isometric>" << bool_to_save_int(m_isometric) << "</isometric>" << endl;
        of << indent << "<speed>" << to_string(m_data.m_speed) << "</speed>" << endl;
        of << indent << "<persistent>" << bool_to_save_int(m_data.m_persistent) << "</persistent>" << endl;
        of << indent << "<colour>" << std::to_string(m_data.m_colour) << "</colour>" << endl;
        of << indent << "<showcolour>" << std::to_string(bool_to_save_int(m_data.m_show_colour)) << "</showcolour>" << endl;
        std::string source = m_cc_room.m_source;
        xml_sanitize(source);
        of << indent << "<code>" << source << "</code>" << endl;
        of << indent << "<enableViews>" << std::to_string(bool_to_save_int(m_data.m_enable_views)) << "</enableViews>" << endl;
        // TODO:
        of << indent << "<clearViewBackground>-1</clearViewBackground>" << endl;
        of << indent << "<clearDisplayBuffer>-1</clearDisplayBuffer>" << endl;
        of << indent << "<makerSettings>" << endl;
        {
            of << indent2 << "<isSet>0</isSet>" << endl;
            of << indent2 << "<w>0</w>" << endl;
            of << indent2 << "<h>0</h>" << endl;
            of << indent2 << "<showGrid>0</showGrid>" << endl;
            of << indent2 << "<showObjects>0</showObjects>" << endl;
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
            of << indent2 << "<background";
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
            of << " objName=" + quote(std::string("&lt;undefined&gt;"));
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
                xml_sanitize(code, true);
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
}