#pragma once

#include "ogm/common/types.hpp"
#include "ogm/geometry/Vector.hpp"

namespace ogm::interpreter
{
    #ifdef OGM_LAYERS
    struct LayerElement;
    struct LayerTileMap;
    
    // a gmv2-style layer
    struct Layer
    {
        // keys that can look up this layer.
        layer_id_t m_id;
        real_t m_depth = 0;
        std::string m_name;
        
        // layer properties
        Vector<coord_t> m_position{ 0, 0 };
        Vector<coord_t> m_velocity{ 0, 0 };
        bool m_visible = true;
        
        std::map<layer_elt_id_t, LayerElement>;
    };
    
    // something which can be on the layer
    // keyed by id
    struct LayerElement
    {
        enum class ElementType
        {
            background,
            instance,
            sprite,
            tilemap,
            particlesystem,
            tile // gmv2 import only
        } m_type;
        
        // what thing this refers to
        id_t m_ref;
    };
    
    // keyed by id by Frame
    struct LayerTileMap
    {
        
    };
    
    // keyed by id by Frame
    struct LayerBackground
    {
        
    };
    
    // keyed by id by Frame
    struct LayerSprite
    {
        
    };
    
    struct Layers
    {
        
    };
    #endif
}