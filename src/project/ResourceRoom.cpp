#include "ogm/project/resource/ResourceRoom.hpp"

#include "ogm/common/util.hpp"
#include "ogm/common/types.hpp"
#include "project/XMLError.hpp"

#include "ogm/ast/parse.h"

#include <string>

namespace ogm::project
{
    
    typedef ogm::asset::layer::Layer Layer;
    typedef ogm::asset::layer::LayerElement LayerElement;

ResourceRoom::ResourceRoom(const char* path, const char* name)
    : Resource(name)
    , m_path(path)
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
    else if (ends_with(m_path, ".yy"))
    {
        load_file_json();
    }
    else
    {
        throw ResourceError(1024, this, "Unrecognized file extension for room file \"{}\"", m_path);
    }
}

bool ResourceRoom::save_file()
{
    std::ofstream of;
    bool rv;
    of.open(m_path);
    if (!of.good()) return false;

    if (ends_with(m_path, ".gmx"))
    {
        rv = save_file_xml(of);
    }
    else if (ends_with(m_path, ".ogm") || ends_with(m_path, ".arf"))
    {
        rv = save_file_arf(of);
    }
    else
    {
        // TODO -- save_file_json
        rv = false;
    }

    of.close();
    return rv;
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
        m_cc_room.m_ast = std::unique_ptr<ogm_ast_t, ogm_ast_deleter_t>{
            ogm_ast_parse(
                m_cc_room.m_source.c_str(), ogm_ast_parse_flag_no_decorations | acc.m_config->m_parse_flags
            )
        };
        m_cc_room.m_name = "cc for room " + m_name;
    }
    else
    {
        m_cc_room.m_bytecode_index = bytecode::k_no_bytecode;
    }

    m_asset_room->m_cc = m_cc_room.m_bytecode_index;
    
    // backgrounds and tiles (unless gmv2 layer model)
    if (!m_layers_enabled)
    {
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
                throw ResourceError(1022, this, "Cannot find sprite asset with name \"{}\"", _def.m_background_name);
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
    }
    
    // layers
    #ifdef OGM_LAYERS
    // map of layer index in room to proper layer id.
    std::vector<layer_id_t> layer_index_to_id;
    if (m_layers_enabled)
    {
        for (const LayerDefinition& layer : m_layers)
        {
            if (layer.m_type == LayerDefinition::lt_folder)
            {
                // this is just a folder. Ignore.
                layer_index_to_id.emplace_back(k_no_layer);
                continue;
            }
            else
            {
                layer_id_t layer_id;
                {
                    // single-threaded access, so no lock needed.
                    layer_id = acc.m_config->m_next_layer_id++;
                }
                layer_index_to_id.emplace_back(layer_id);
                Layer& def = m_asset_room->m_layers[layer_id] = {};
                
                // transfer layer properties to room asset's layer definition
                def.m_name = layer.m_name;
                def.m_name = layer.m_depth;
                def.m_depth = layer.m_depth;
                def.m_velocity = layer.m_velocity;
                def.m_position = layer.m_position;
                
                // background layers are given a (primary) element for the background
                if (layer.m_type == layer.lt_background)
                {
                    asset_index_t sprite_id;
                    {
                        acc.m_assets->get_asset(layer.m_asset_name.c_str(), sprite_id);
                    }
                    LayerElement& elt = add_layer_element(
                        *m_asset_room,
                        *acc.m_config,
                        asset::layer::LayerElement::et_background,
                        layer_id,
                        def.m_primary_element
                    );
                    elt.background.set_sprite(sprite_id);
                    elt.background.m_blend = layer.m_colour;
                    elt.background.m_htiled = layer.m_htile;
                    elt.background.m_vtiled = layer.m_vtile;
                }
            }
        }
    }
    #endif

    // instances
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
            asset::AssetRoom::InstanceDefinition& def = m_asset_room->m_instances.emplace_back();

            // TODO: what to do with m_name?

            def.m_id = id;
            def.m_object_index = object_index;
            def.m_position = _def.m_position;
            def.m_scale = _def.m_scale;
            def.m_angle = _def.m_angle;
            def.m_blend = _def.m_colour;
            
            #ifdef OGM_LAYERS
            if (m_layers_enabled)
            {
                layer_id_t layer_id = layer_index_to_id.at(_def.m_layer_index);
                ogm_assert(layer_id != k_no_layer);
                
                add_layer_element(
                    *m_asset_room,
                    *acc.m_config,
                    LayerElement::et_instance,
                    layer_id,
                    def.m_layer_elt_id
                ).instance.m_id = id;
            }
            #endif

            // creation code
            if (_def.m_code != "")
            {
                m_cc_instance.emplace_back();
                CreationCode& cc = m_cc_instance.back();
                cc.m_source = _def.m_code;
                cc.m_ast =  std::unique_ptr<ogm_ast_t, ogm_ast_deleter_t>{
                    ogm_ast_parse(
                        _def.m_code.c_str(), ogm_ast_parse_flag_no_decorations | acc.m_config->m_parse_flags
                    )
                };
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
            throw ResourceError(1022, this, "Cannot find object asset with name \"{}\"", _def.m_object_name);
        }
    }

    // add backgrounds and views to get up to 8 in each
    while (m_asset_room->m_views.size() < 8)
    {
        m_asset_room->m_views.emplace_back();
    }
    
    if (!m_layers_enabled)
    {
        while (m_asset_room->m_bg_layers.size() < 8)
        {
            m_asset_room->m_bg_layers.emplace_back();
        }
    }
}

void ResourceRoom::compile(bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(COMPILED)) return;
    // compile room cc
    if (m_cc_room.m_bytecode_index != bytecode::k_no_bytecode)
    {
        ogm::bytecode::bytecode_generate(
            {m_cc_room.m_ast.get(), (m_cc_room.m_name).c_str(), m_cc_room.m_source.c_str()},
            acc,
            nullptr,
            m_cc_room.m_bytecode_index
        );
    }

    // compile instance cc
    for (auto& cc : m_cc_instance)
    {
        ogm::bytecode::bytecode_generate(
            {cc.m_ast.get(), (cc.m_name).c_str(), cc.m_source.c_str()},
            acc,
            nullptr,
            cc.m_bytecode_index
        );
    }
}
}