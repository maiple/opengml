#include "ogm/project/resource/ResourceRoom.hpp"

#include "ogm/common/util.hpp"
#include "ogm/common/types.hpp"

#include "ogm/ast/parse.h"

#include <pugixml.hpp>
#include <string>

namespace ogm { namespace project {

ResourceRoom::ResourceRoom(const char* path, const char* name): m_path(path), m_name(name)
{ }

void ResourceRoom::precompile(bytecode::ProjectAccumulator& acc)
{
    std::string _path = native_path(m_path);
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(_path.c_str(), pugi::parse_default | pugi::parse_escapes | pugi::parse_comments);

    // TODO: check result
    (void)result;

    pugi::xml_node node_room = doc.child("room");

    // FIXME: error checking
    std::string s_w = node_room.child("width").text().get();
    std::string s_h = node_room.child("height").text().get();
    std::string s_speed = node_room.child("speed").text().get();
    std::string s_enable_views = node_room.child("enableViews").text().get();
    std::string s_colour = node_room.child("colour").text().get();
    std::string s_show_colour = node_room.child("showcolour").text().get();
    m_dimension.x = std::stod(s_w);
    m_dimension.y = std::stod(s_h);
    m_speed = std::stod(s_speed);

    asset_index_t asset_index;
    m_asset_room = acc.m_assets->add_asset<asset::AssetRoom>(m_name.c_str(), &asset_index);

    m_asset_room->m_dimensions = m_dimension;
    m_asset_room->m_speed = m_speed;
    m_asset_room->m_show_colour = s_show_colour != "0";
    m_asset_room->m_colour = stoi(s_colour);
    // TODO: remaining room properties

    // room cc
    m_cc_room.m_source = node_room.child("code").text().get();
    trim(m_cc_room.m_source);
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

        if (bgname_s != "" && bgname_s != "<undefined>")
        {
            asset_index_t background_asset;
            const asset::AssetBackground* bg_asset = dynamic_cast<asset::AssetBackground*>(acc.m_assets->get_asset(bgname_s.c_str(), background_asset));
            if (bg_asset)
            {
                m_asset_room->m_bg_layers.emplace_back();
                asset::AssetRoom::BackgroundLayerDefinition& def = m_asset_room->m_bg_layers.back();
                def.m_background_index = background_asset;
                def.m_position = { std::stod(x_s), std::stod(y_s) };
                def.m_velocity = { std::stod(vx_s), std::stod(vy_s) };
                def.m_tiled_x = htiled;
                def.m_tiled_y = vtiled;
                def.m_visible = visible;
                def.m_foreground = foreground;

                // TODO: bg speed, stretch.
            }
        }
    }

    // cache
    std::map<std::string, std::pair<asset_index_t, const asset::AssetBackground*>> background_cache;

    // tiles
    pugi::xml_node node_tiles = node_room.child("tiles");
    typedef decltype(asset::AssetRoom::TileLayerDefinition::m_contents) TileVector;
    std::map<real_t, TileVector> layers;

    for (pugi::xml_node node_tile: node_tiles.children("tile"))
    {
        // OPTIMIZE -- change strings out for char*
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
            asset_index_t background_asset;
            const asset::AssetBackground* object_asset;

            // lookup tile
            auto cache_iter = background_cache.find(bgname_s);
            if (cache_iter == background_cache.end())
            {
                object_asset = dynamic_cast<asset::AssetBackground*>(acc.m_assets->get_asset(bgname_s.c_str(), background_asset));
                background_cache[bgname_s] = { background_asset, object_asset };
            }
            else
            {
                background_asset = cache_iter->second.first;
                object_asset = cache_iter->second.second;
            }

            if (object_asset)
            {
                // TODO: use actual ID tag (requires a lot of gruntwork to separate out the instance IDs, though.)
                instance_id_t id;
                {
                    // single-threaded access, so no lock needed.
                    id = acc.m_config->m_next_instance_id++;
                }

                real_t depth = std::stof(depth_s);

                // get the appropriate layer for this tile
                auto& layer = layers[depth];

                // add a new tile definition to the layer
                layer.emplace_back();
                asset::AssetRoom::TileDefinition& def = layer.back();

                // define the tile
                def.m_background_index = background_asset;
                def.m_id = id;
                def.m_position = { std::stof(x_s), std::stof(y_s) };
                def.m_bg_position = { std::stof(xo_s), std::stof(yo_s) };
                def.m_scale = { std::stof(scale_x_s), std::stof(scale_y_s) };
                def.m_dimension = { std::stof(w_s), std::stof(h_s) };
                def.m_blend = std::stoll(colour_s);
            }
            else
            {
                MiscError("Room references background resource '" + bgname_s + ", which does not exist.");
            }
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
            asset_index_t object_index;
            const asset::AssetObject* object_asset = dynamic_cast<asset::AssetObject*>(acc.m_assets->get_asset(objname_s.c_str(), object_index));
            if (object_asset)
            {
                instance_id_t id;
                {
                    // single-threaded access, so no lock needed.
                    id = acc.m_config->m_next_instance_id++;
                }
                m_asset_room->m_instances.emplace_back();
                asset::AssetRoom::InstanceDefinition& def = m_asset_room->m_instances.back();

                // instance parameters
                def.m_id = id;
                def.m_object_index = object_index;
                def.m_position = { std::stof(x_s), std::stof(y_s) };
                def.m_scale = { std::stof(scale_x_s), std::stof(scale_y_s) };

                // creation code
                if (code_s != "")
                {
                    m_cc_instance.emplace_back();
                    CreationCode& cc = m_cc_instance.back();
                    cc.m_source = code_s;
                    cc.m_ast = ogm_ast_parse(
                        code_s.c_str(), ogm_ast_parse_flag_no_decorations
                    );
                    cc.m_name = "cc for instance " + name_s + " (id " + std::to_string(id) + ")";
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
                MiscError("Room references object '" + objname_s + ", which does not exist.");
            }
        }
    }


    pugi::xml_node node_views = node_room.child("views");
    m_asset_room->m_enable_views = s_enable_views != "0";
    for (pugi::xml_node node_view: node_views.children("view"))
    {
        std::string visible = node_view.attribute("visible").value();
        std::string xview = node_view.attribute("xview").value();
        std::string yview = node_view.attribute("yview").value();
        std::string wview = node_view.attribute("wview").value();
        std::string hview = node_view.attribute("hview").value();

        m_asset_room->m_views.emplace_back();
        asset::AssetRoom::ViewDefinition& def = m_asset_room->m_views.back();
        def.m_visible = visible != "0";
        def.m_position = { std::stod(xview), std::stod(yview) };
        def.m_dimension = { std::stod(wview), std::stod(hview) };
    }
}

void ResourceRoom::compile(bytecode::ProjectAccumulator& acc, const bytecode::Library* library)
{
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
