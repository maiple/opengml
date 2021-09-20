#pragma once

#include "ogm/asset/LayerCommon.hpp"

#include "ogm/common/types.hpp"
#include "ogm/geometry/Vector.hpp"

#include <set>
#include <string>
#include <map>

#ifdef OGM_LAYERS

namespace ogm::interpreter
{
    typedef ogm::asset::layer::Layer Layer;
    typedef ogm::asset::layer::LayerElement LayerElement;
    
    class Layers
    {
    public:
        // if nonnegative, this is a layer id; if negative, this is a (negative) instance id.
        typedef id_t layer_id_or_managed_t;
        
        // ssot for (active) layers
        // (room asset layers are stored in the room asset.)
        std::map<layer_id_t, Layer> m_layers;
        
        // (active) layer ids by name
        std::map<std::string, layer_id_t> m_layer_ids_by_name;
        
        // (active) layer ids (and managed instances) by depth
        std::map<real_t, std::set<layer_id_or_managed_t>> m_layer_ids_by_depth;

        // ssot for (active) elements
        // (room asset elements are stored in the room asset)
        std::map<layer_elt_id_t, LayerElement> m_layer_elements;
        
        void clear()
        {
            m_layer_elements.clear();
            m_layer_ids_by_name.clear();
            m_layer_ids_by_depth.clear();
            m_layer_elements.clear();
        }
        
        bool element_exists(layer_elt_id_t elt_id) const
        {
            return m_layer_elements.find(elt_id) != m_layer_elements.end();
        }
        
        bool layer_exists(layer_id_t layer_id) const
        {
            return m_layers.find(layer_id) != m_layers.end();
        }
        
        // creates a layer with existing id
        Layer& layer_create_at(layer_id_t id, real_t depth, const std::string& name)
        {
            assert(!layer_exists(id));
            Layer& layer = m_layers[id] = {};
            layer.m_depth = depth;
            layer.m_name = (name == "")
                ? generate_layer_name(id)
                : name;
            m_layer_ids_by_depth[depth].emplace(id);
            m_layer_ids_by_name[layer.m_name] = id;
        }
        
        Layer& layer_create(real_t depth, const std::string& name, layer_id_t& layer_id_out);
        Layer& layer_create(real_t depth, const std::string& name);
        
        LayerElement& add_element(LayerElement::Type type, layer_id_t layer_id);
        LayerElement& add_element(LayerElement::Type type, layer_id_t layer_id, layer_elt_id_t& out_elt_id);
        
        // adds a layer element with an existing id
        LayerElement& add_element_at(layer_elt_id_t elt_id, LayerElement::Type type, layer_id_t layer_id)
        {
            return ogm::asset::layer::add_layer_element_at(*this, elt_id, type, layer_id);
        }
        
        void move_element(layer_elt_id_t elt_id, layer_id_t layer_id)
        {
            assert(element_exists(elt_id));
            assert(layer_exists(layer_id));
            
            LayerElement& elt_prev = m_layer_elements.at(elt_id);
            assert(layer_exists(elt_prev.m_layer));
            
            // do not create a new element
            m_layers.at(elt_prev.m_layer).m_element_ids.erase(elt_id)
            elt_prev.m_layer = layer_id;
            m_layers.at(layer_id).m_element_ids.emplace(elt_id);
        }
        
        // adds a "managed instance" (one that has no layer and no element)
        void add_managed(real_t depth, id_t id)
        {
            m_layer_ids_by_depth[depth].emplace(-id);
        }
        
        void remove_managed(real_t depth, id_t id)
        {
            auto& depth_set = m_layer_ids_by_depth[depth]
            depth_set.erase(-id);
            if (depth_set.empty())
            {
                m_layer_ids_by_depth.erase(dept);
            }
        }
    
    private:
        std::string generate_layer_name(layer_id_t id)
        {
            return "_layer_" + hex_string(id);
        }
    };
}

#endif