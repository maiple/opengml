#pragma once

#include "ogm/geometry/Vector.hpp"
#include "ogm/common/types.hpp"

#include <cassert>
#include <string>
#include <set>
#include <map>

#ifdef OGM_LAYERS
// There are two (or three) notions of 'layer' in gmsv2:
//
//   1. layers at runtime
//   2. layers as defined in a room asset
//  (3. layers as defined in a room resource)
//
// These classes serve as a bridge between 1 and 2, storing the data common to both.
namespace ogm::asset::layer
{
    // something which can be on the layer
    // keyed by id
    #ifdef COMPRESS_OGM_LAYER_ELTS
    #define COMPRESS_OGM_LAYER_ELTS_BF(x) : x
    #pragma pack(push,1)
    #else
    #define COMPRESS_OGM_LAYER_ELTS_BF(x)
    #endif
    struct LayerElement
    {
        enum Type : unsigned char
        {
            et_background,
            et_instance,
            et_sprite,
            et_tilemap,
            et_particlesystem,
            et_tile,
            et_sequence,
        } m_type COMPRESS_OGM_LAYER_ELTS_BF(8);
        
        layer_id_t m_layer COMPRESS_OGM_LAYER_ELTS_BF(24);
        
        union
        {
            // Instance
            struct
            {
                // the instance this refers to.
                // can be looked up in ogm::interpreter::Frame
                instance_id_t m_id;
            } instance;
            
            // Background
            struct
            {
                // asset index of sprite
                // if no sprite, then this is a pure colour.
                #ifdef COMPRESS_OGM_LAYER_ELTS
                    uint32_t m_blend : 32;
                private:
                    asset_index_t m_sprite : 30;
                public:
                    bool m_vtiled : 1;
                    bool m_htiled : 1;
                    
                    void set_sprite(asset_index_t s)
                    {
                        m_sprite = s & 0x3fffffff;
                    }
                    asset_index_t get_sprite()
                    {
                        return (m_sprite == 0x3fffffff)
                            ? k_no_asset
                            : m_sprite;
                    }
                #else
                private:
                    asset_index_t m_sprite;
                public:
                    uint32_t m_blend;
                    bool m_vtiled;
                    bool m_htiled;
                    
                    void set_sprite(asset_index_t s)
                    {
                        m_sprite = s;
                    }
                    asset_index_t get_sprite()
                    {
                        return m_sprite;
                    }
                #endif
            } background;
        };
    };
    #ifdef COMPRESS_OGM_LAYER_ELTS
    #pragma pack(pop)
    static_assert(
        sizeof(sizeof(LayerElement::instance) <= 8),
        "layer union data structs must be 8 bytes"
    );
    static_assert(
        sizeof(sizeof(LayerElement::background) <= 8),
        "layer union data structs must be 8 bytes"
    );
    static_assert(
        sizeof(sizeof(LayerElement) <= 12),
        "layer elements should be at most 12 bytes"
    );
    #endif

    // keyed by id
    struct Layer
    {
        // keys
        real_t m_depth = 0; // ACCURACY -- should this be an int?
        std::string m_name;
        
        // layer properties
        geometry::Vector<coord_t> m_position{ 0, 0 };
        geometry::Vector<coord_t> m_velocity{ 0, 0 };
        bool m_visible = true;
        
        // some layers have a special element which is returned
        // e.g. from 'layer_background_get_id'
        id_t m_primary_element = -1;
        
        // all elements on this layer.
        std::set<layer_elt_id_t> m_element_ids;
    };
    
    // adds layer element to given layer with existing element id
    template<typename LayersContainer>
    inline LayerElement& add_layer_element_at(LayersContainer& container, layer_elt_id_t elt_id, LayerElement::Type type, layer_id_t layer_id)
    {
        // container must actually have the given layer.
        assert(container.m_layers.find(layer_id) != container.m_layers.end());
        Layer& layer{ container.m_layers.at(layer_id) };
        
        // container musn't have existing element
        assert(container.m_layer_elements.find(elt_id) == container.m_layer_elements.end());
        
        // layer musn't have existing element
        assert(layer.m_element_ids.find(elt_id) == layer.m_element_ids.end());
        
        // add element to layer
        layer.m_element_ids.emplace(elt_id);
        LayerElement& elt = container.m_layer_elements[elt_id];
        elt.m_layer = layer_id;
        elt.m_type = type;
        return elt;
    }
    
    // adds layer element to a given layer
    template<typename LayersContainer, typename LayerElementIDStore>
    inline LayerElement& add_layer_element(LayersContainer& container, LayerElementIDStore& store, LayerElement::Type type, layer_id_t layer_id, layer_elt_id_t& out_elt_id)
    {
        return add_layer_element_at(container, out_elt_id = store.m_next_layer_elt_id++, type, layer_id);
    }
    
    template<typename LayersContainer, typename LayerElementIDStore>
    inline LayerElement& add_layer_element(LayersContainer& container, LayerElementIDStore& store, LayerElement::Type type, layer_id_t layer_id)
    {
        layer_elt_id_t dummy;
        return add_layer_element(container, store, type, layer_id, dummy);
    }
}
#endif