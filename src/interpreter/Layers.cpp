#include "ogm/interpreter/Layers.hpp"
#include "ogm/interpreter/Executor.hpp"

#ifdef OGM_LAYERS
namespace ogm::interpreter
{
    LayerElement& Layers::add_element(LayerElement::Type type, layer_id_t layer_id, layer_elt_id_t& out_elt_id)
    {
        return ogm::asset::layer::add_layer_element(*this, staticExecutor.m_frame.m_config, type, layer_id);
    }
    
    LayerElement& Layers::add_element(LayerElement::Type type, layer_id_t layer_id)
    {
        layer_elt_id_t dummy;
        return ogm::asset::layer::add_layer_element(*this, staticExecutor.m_frame.m_config, type, layer_id, dummy);
    }
    
    Layer& Layers::layer_create(real_t depth, const char* name, layer_id_t& layer_id_out)
    {
        return layer_create_at(layer_id_out = staticExecutor.m_frame.m_config.m_next_layer_id++, depth, name);
    }
    
    Layer& Layers::layer_create(real_t depth, const char* name)
    {
        layer_id_t dummy;
        return layer_create(depth, name, dummy);
    }
    
    void Layers::layer_remove(layer_id_t layer_id)
    {
        auto iter = m_layers.find(layer_id);
        if (iter != m_layers.end())
        {
            // layer must be empty to remove it.
            const Layer& layer = iter->second;
            ogm_assert(layer.m_element_ids.empty());
            
            // remove from depth list
            m_layer_ids_by_depth.at(layer.m_depth).erase(layer_id);
            if (m_layer_ids_by_depth.at(layer.m_depth).empty())
            {
                m_layer_ids_by_depth.erase(layer.m_depth);
            }
            
            // remove frame name list
            auto& name_ids = m_layer_ids_by_name.at(layer.m_name);
            name_ids.erase(std::remove(name_ids.begin(), name_ids.end(), layer_id), name_ids.end());
            if (name_ids.empty())
            {
                m_layer_ids_by_name.erase(layer.m_name);
            }
            
            // remove layer
            m_layers.erase(iter);
        }
    }
    
    void Layers::element_remove(layer_elt_id_t elt_id)
    {
        auto iter = m_layer_elements.find(elt_id);
        if (iter != m_layer_elements.end())
        {
            const layer_id_t layer_id = iter->second.m_layer;
            Layer& layer = m_layers.at(layer_id);
            layer.m_element_ids.erase(elt_id);
            m_layer_elements.erase(iter);
        }
    }
}
#endif