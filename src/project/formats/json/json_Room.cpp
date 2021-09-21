#include "ogm/project/resource/ResourceRoom.hpp"

#include "ogm/common/util.hpp"
#include "ogm/common/types.hpp"

#include <nlohmann/json.hpp>

#include "json_parse.hpp"

using nlohmann::json;

namespace ogm::project
{
    
namespace
{
std::string readCreationCode(const json& j, const std::string from_directory)
{
    if (j.find("creationCodeFile") != j.end())
    {
        std::string ccfile = j.at("creationCodeFile").get<std::string>();
        if (ccfile != "")
        {
            // load creation code from external file
            std::string cc_path = path_join(from_directory, ccfile);
            try
            {
                return read_file_contents(cc_path);
            }
            catch (...)
            {
                std::cout << "WARNING: unable to read creation code file '"
                    << cc_path << std::endl;
            }
        }
    }
    return "";
}
}
    
void ResourceRoom::load_file_json()
{
    std::fstream ifs(m_path);
    
    if (!ifs.good()) throw ResourceError(1030, this, "Error loading file \"{}\"", m_path);
    
    json j;
    ifs >> j;
    
    checkModelName(j, "Room");
    
    #ifdef OGM_LAYERS
    // TODO: this should be a global configuration instead of decided per-room.
    m_layers_enabled = true;
    #endif
    
    m_v2_id = j.at("id");
    
    // settings --------
    
    const json& settings = j.at("roomSettings");
    checkModelName(settings, "RoomSettings");
    
    m_data.m_dimensions.x = settings.at("Width").get<real_t>();
    m_data.m_dimensions.y = settings.at("Height").get<real_t>();
    m_persistent = settings.at("persistent").get<bool>();
    
    // room creation code
    m_cc_room.m_source = readCreationCode(j, path_directory(m_path));
    
    // layers
    for (const json& layer : j.at("layers"))
    {
        load_file_json_layer(&layer);
    }
    
    // views -----------
    const json& view_settings = j.at("viewSettings");
    checkModelName(view_settings, "RoomViewSettings");
    
    m_data.m_enable_views = view_settings.at("enableViews").get<bool>();
    for (const json& v : j.at("views"))
    {
        asset::AssetRoom::ViewDefinition& view = m_data.m_views.emplace_back();
        checkModelName(v, "RView");
        view.m_visible = v.at("visible").get<bool>();
        view.m_position = {
            v.at("xview").get<coord_t>(),
            v.at("yview").get<coord_t>()
        };
        view.m_dimension = {
            v.at("wview").get<coord_t>(),
            v.at("hview").get<coord_t>()
        };
        view.m_velocity = {
            v.at("hspeed").get<coord_t>(),
            v.at("vspeed").get<coord_t>()
        };
        view.m_border = {
            v.at("hborder").get<coord_t>(),
            v.at("vborder").get<coord_t>()
        };
        view.m_viewport = {
            v.at("xport").get<coord_t>(),
            v.at("yport").get<coord_t>(),
            v.at("wport").get<coord_t>(),
            v.at("hport").get<coord_t>(),
        };
    }
}

// loads a 'layer' from a json object representing the layer
void ResourceRoom::load_file_json_layer(const void* vjl)
{
    const json& jl = *(const json*)vjl;
    #ifdef OGM_LAYERS
    assert(m_layers_enabled);
    
    size_t layer_index = m_layers.size();
    LayerDefinition& layer = m_layers.emplace_back();
    
    // IMPROVEMENT: there is more than one type of layer, they have
    // different model names, but conveniently all end in "Layer",
    // so this works.
    checkModelName(jl, "Layer");
    
    // check layer type
    std::string modelName = jl.at("modelName").get<std::string>();
    if (ends_with(modelName, "RLayer"))
    {
        layer.m_type = layer.lt_folder;
    }
    else if(ends_with(modelName, "RInstanceLayer"))
    {
        layer.m_type = layer.lt_instance;
    }
    else if (ends_with(modelName, "RBackgroundLayer"))
    {
        layer.m_type = layer.lt_background;
    }
    
    // misc
    layer.m_name = jl.at("name").get<std::string>();
    layer.m_depth = jl.at("depth").get<real_t>();
    layer.m_visible = getDefault<bool>(jl, "visible", true);
    if (hasKey(jl, "colour"))
    {
        if (!jl.at("colour").is_number())
        {
            layer.m_colour = getDefault(jl.at("colour"), "Value", 0xffffffff);
        }
        else
        {
            layer.m_colour = jl.at("colour").get<uint32_t>();
        }
    }
    layer.m_vtile = getDefault<bool>(jl, "vtiled", false);
    layer.m_htile = getDefault<bool>(jl, "htiled", false);
    if (hasKey(jl, "x") && hasKey(jl, "y"))
    {
        layer.m_position = {
            jl.at("x").get<coord_t>(),
            jl.at("y").get<coord_t>()
        };
    }
    if (hasKey(jl, "hspeed") && hasKey(jl, "vspeed"))
    {
        layer.m_velocity = {
            jl.at("hspeed").get<coord_t>(),
            jl.at("vspeed").get<coord_t>()
        };
    }
    
    // instances
    if (hasKey(jl, "instances"))
    {
        for (const json& i : jl.at("instances"))
        {
            checkModelName(i, "RInstance");
            InstanceDefinition& instance = m_instances.emplace_back();
            instance.m_layer_index = layer_index;
            instance.m_name = i.at("name").get<std::string>();
            instance.m_object_name = i.at("objId").get<std::string>();
            instance.m_colour = i.at("colour").at("Value").get<uint32_t>();
            instance.m_code = readCreationCode(i, path_directory(m_path));
            instance.m_position = {
                i.at("x").get<coord_t>(),
                i.at("y").get<coord_t>()
            };
            instance.m_angle = i.at("rotation").get<real_t>();
            instance.m_scale = {
                i.at("scaleX").get<coord_t>(),
                i.at("scaleY").get<coord_t>()
            };
        }
    }
    
    // recurse
    if (hasKey(jl, "layers"))
    {
        for (const json& l : jl.at("layers"))
        {
            load_file_json_layer(&l);
        }
    }
    
    #else
    throw MiscError("cannot parse room layer in room \"{}\" -- no support for layers. ", m_name);
    #endif
}

}