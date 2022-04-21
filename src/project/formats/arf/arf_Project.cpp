#include "ogm/project/Project.hpp"
#include "project/ProjectResources.hpp"

#include "ogm/project/arf/arf_parse.hpp"

namespace ogm::project
{

namespace
{
    ARFSchema arf_project_schema
    {
        "project",
        ARFSchema::DICT,
        {
            "constants",
            ARFSchema::DICT
        }
    };
}

extern const char* RESOURCE_EXTENSION_ARF[];

void Project::process_arf()
{
    ARFSection project;
    try
    {
        std::string contents = read_file_contents(m_root + m_project_file);
        arf_parse(arf_project_schema, contents.c_str(), project);
    }
    catch (std::exception& e)
    {
        throw ProjectError(
            ErrorCode::F::arfproj, 
            "Failed to parse project file {}\n:{}", m_project_file, e.what()
        );
    }

    // resources
    {
        // scan subdirectories for files.
        for (int r = 0; r < NONE; r++)
        {
            ResourceType type = static_cast<ResourceType>(r);
            if (r == CONSTANT)
            {
                // skip constants
                continue;
            }
            
            // determine directory on disk where these resources are stored.
            std::string resource_directory_path = m_root + RESOURCE_TYPE_NAMES[r];
            
            if (!path_exists(resource_directory_path))
            {
                resource_directory_path += "s";
            }
            if (!path_exists(resource_directory_path))
            {
                continue;
            }
            
            scan_resource_directory(resource_directory_path, type);
        }
    }

    // constants
    for (const ARFSection* section : project.m_sections)
    {
        if (section->m_name == "constants")
        {
            for (auto pair : section->m_dict)
            {
                add_constant(pair.first, pair.second);
            }
        }
    }

    // check for extensions
    m_extension_init_script_source = "#define ogm_extension_init\n\n";
    {
        std::vector<std::string> paths;
        list_paths(m_root + "extensions", paths);
        for (const std::string& path : paths)
        {
            if (ends_with(path, ".extension.gmx"))
            {
                process_extension(path.c_str());
            }
        }
    }

    add_script("extension^init", m_extension_init_script_source);

    m_processed = true;
}

}