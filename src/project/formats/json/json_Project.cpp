#include "ogm/project/Project.hpp"
#include "project/ProjectResources.hpp"

#include "json_parse.hpp"

namespace ogm::project {

void Project::process_json()
{
    std::string path = path_join(m_root, m_project_file);
    std::fstream ifs(path);
    
    if (!ifs.good()) throw ProjectError(1005, "Error opening file \"{}\"", path);
    
    json j;
    ifs >> j;
    
    checkModelName(j, "Project");
    
    // read resources
    for (const json& resource : j.at("resources"))
    {
        const json& resource_value = resource.at("Value");
        std::string path = native_path(resource_value.at("resourcePath").get<std::string>());
        std::string type = string_lowercase(resource_value.at("resourceType").get<std::string>());
        ResourceType rtype = NONE;
        // skip unknown resources
        for (size_t r = 0; r < static_cast<size_t>(NONE); ++r)
        {
            if (ends_with(type, RESOURCE_TYPE_NAMES[r]))
            {
                rtype = static_cast<ResourceType>(r);
            }
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