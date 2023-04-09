#include "ogm/project/Project.hpp"
#include "project/ProjectResources.hpp"

#include "json_parse.hpp"

namespace ogm::project {

void Project::process_json()
{
    std::string path = path_join(m_root, m_project_file);
    std::fstream ifs(path);
    
    if (!ifs.good()) throw ProjectError(ErrorCode::F::file, "Error opening file \"{}\"", path);
    
    json j;
    ifs >> j;
    
    checkModelName(j, "Project");
    
    // read resources
    for (const json& resource : j.at("resources"))
    {
        const json& resource_value = resource.at("Value");
        std::string type = string_lowercase(resource_value.at("resourceType").get<std::string>());
        if (ends_with(type, "folder"))
        {
            // skip folders
            continue;
        }
        
        std::string path = native_path(resource_value.at("resourcePath").get<std::string>());
        ResourceType rtype = NONE;
        
        for (size_t r = 0; r < static_cast<size_t>(NONE); ++r)
        {
            if (ends_with(type, RESOURCE_TYPE_NAMES[r]) || ends_with(type, RESOURCE_TYPE_NAMES_ALT[r]))
            {
                rtype = static_cast<ResourceType>(r);
                break;
            }
        }
        
        // scripts replace .yy path with .gml
        if (rtype == ResourceType::SCRIPT)
        {
            path = remove_extension(path) + ".gml";
        }

        if (rtype != NONE)
        {
            add_resource_from_path(rtype, path_join(m_root, path));
        }
        else
        {
            std::cout << "WARNING: unknown resource type, " << resource_value.at("resourceType").get<std::string>() << std::endl;
        }
    }
}

}