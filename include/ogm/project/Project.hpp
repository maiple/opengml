#pragma once

#include "resource/Resource.hpp"
#include "ResourceTree.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/bytecode/bytecode.hpp"
#include "ogm/common/util.hpp"
#include "ogm/common/util_sys.hpp"
#include "ogm/common/parallel.hpp"

#include <cstring>
#include <string>
#include <vector>
#include <set>

#ifdef PARALLEL_COMPILE
#include <mutex>
#endif

namespace pugi
{
    struct xml_node;
}

namespace ogm { namespace asset {
    class Config;
}}

namespace ogm { namespace project {
    
class LazyResource {
    // produces a resource
    std::function<std::unique_ptr<Resource>()> m_construct_resource;

    // pointer to resource (if realized)
    std::unique_ptr<Resource> m_ptr;
    
public:
    ResourceType m_type;
    
    LazyResource(LazyResource&&)=default;
    LazyResource(ResourceType rt, std::function<std::unique_ptr<Resource>()>&& construct)
        : m_construct_resource(construct)
        , m_type(rt)
    {};
    
    // loads the resource
    Resource* get();
    
    inline Resource* operator*()
    {
        return get();
    }
};

class ResourceConstant : public Resource {
public:
    std::string m_value;
    void precompile(bytecode::ProjectAccumulator& accumulator);
    const char* get_type_name() override { return "constant"; };
    
    ResourceConstant(const std::string& name, const std::string& value)
        : Resource(name)
        , m_value(value)
    { }
};

//! Manages a GMX project on the disk
class Project {
public:
    Project(const char* path_to_project_file);

    // reads the project file and stores the resource tree.
    void process();

    // builds the project, including all resources,
    // placing all assets into the accumulator's asset table.
    // invokes process() if needed.
    // returns false if error.
    bool build(bytecode::ProjectAccumulator& accumulator);

    bool m_verbose=false;

    // scans a directory for resources and automatically adds them to the project.
    // if a resource is encountered with an ambiguous name, the type supplied is used.
    // if the type NONE is given, ambiguous resources will be skipped.
    void scan_resource_directory(const std::string& path, ResourceType default_type=NONE);
    
    void add_resource_from_path(ResourceType, const std::string& path, ResourceList* list=nullptr, std::string name="");

    // type-specific resource tree.
    // (this is a top-level folder.)
    ResourceList* asset_tree(ResourceType);

    std::string get_project_file_path() const
    {
        return path_join(m_root, m_project_file);
    }

public:
    // introduces a resource when dereferenced for the first time.
    // (lazy-loading.)
    
    // resources in the project.
    std::map<std::string, LazyResource> m_resources;
    
    // tree of resource names. Entries can be looked up in m_resources.
    ResourceList m_tree;

private:
    bool m_processed = false;
    bool m_error = false;
    std::string m_root; // path to directory containing project file
    std::string m_project_file; // path to project file relative to root
    std::string m_extension_init_script_source; // source for extension init (generated script)
    std::set<std::string> m_ignored_assets; // don't load these assets.
    
    bool infer_resource_type_from_extension(const std::string& extension, ResourceType& out_type);
    
    // adds top-level folders to resource tree
    void populate_tree_default();
    
    // reads an arf project file
    void process_arf();
    
    // reads a json project file
    void process_json();

    // reads an xml project file
    void process_xml();

    // parses the given DOM tree for the given type of resources
    void read_resource_tree_xml(ResourceList* list, pugi::xml_node& xml, ResourceType type);

    // adds the resources from the given extension to the project.
    void process_extension(const char* path_to_extension);

public:
    void add_constant(const std::string& name, const std::string& value);

    void ignore_asset(const std::string& name);

private:
    // adds a script directly from source
    void add_script(const resource_id_t& name, const std::string& source);

    template<bool parallel=false>
    void for_resource(ResourceTree* tree, const std::function<void(Resource*)>&, const char* description=nullptr);

    void for_resource_parallel(ResourceTree* tree, const std::function<void(Resource*)>& f, const char* description)
    {
        for_resource<true>(tree, f, description);
    }

private:
    #ifdef PARALLEL_COMPILE
    std::mutex m_progress_mutex;
    #endif
    unsigned int m_build_progress;
    unsigned int m_build_max;
    bool m_parallel_compile;
};

}}
