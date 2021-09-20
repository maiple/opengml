#include "ogm/interpreter/Layers.hpp"
#include "ogm/interpreter/Executor.hpp"

#ifdef OGM_LAYERS
namespace ogm::interpreter
{
    LayerElement& Layers::add_element(LayerElement::Type type, layer_id_t layer_id, layer_elt_id_t& out_elt_id);
    {
        return ogm::asset::layer::add_layer_element(*this, staticExecutor.m_frame.m_config, type, layer_id);
    }
    
    LayerElement& Layers::add_element(LayerElement::Type type, layer_id_t layer_id)
    {
        layer_elt_id_t dummy;
        return ogm::asset::layer::add_layer_element(*this, staticExecutor.m_frame.m_config, type, layer_id, dummy);
    }
    
    Layer& Layers::layer_create(real_t depth, std::string name, layer_id_t& layer_id_out)
    {
        return layer_create_at(depth, name, layer_id_out = staticExecutor.m_frame.m_config.m_next_layer_id++);
    }
    
    Layer& Layers::layer_create(real_t depth, std::string name)
    {
        layer_id_t dummy;
        return layer_create(depth, name, dummy);
    }
}
#endif