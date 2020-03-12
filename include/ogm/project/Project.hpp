#pragma once

#include "resource/Resource.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/bytecode/bytecode.hpp"

#include <cstring>
#include <string>
#include <vector>
#include <set>

namespace pugi
{
    struct xml_node;
}

namespace ogm { namespace asset {
    class Config;
}}

namespace ogm { namespace project {

extern const char* k_default_entrypoint;
extern const char* k_default_step_builtin;
extern const char* k_default_draw_normal;

class ResourceConstant : public Resource {
public:
    std::string m_name;
    std::string m_value;
    void precompile(bytecode::ProjectAccumulator& accumulator);
};

struct ResourceTree {
    bool is_leaf;
    
    bool is_hidden = false;

    ResourceType m_type;

    std::string m_name;

    // for trees --
    std::vector<ResourceTree> list;

    // for leaves --
    //! key for resource's entry in resource table
    std::string rtkey;
};

//! Manages a GMX project on the disk
class Project {
public:
    Project(const char* path_to_project_file);

    // reads the project file and stores the resource tree.
    void process();

    // compiles the project into a asset and bytecode chunks
    // invokes process() if needed.
    void compile(bytecode::ProjectAccumulator& accumulator, const bytecode::Library* library = &bytecode::defaultLibrary);

    bool m_verbose=false;

    std::string get_project_file_path()
    {
        return m_root + m_project_file;
    }

public:
    ResourceTree  m_resourceTree;
    ResourceTable m_resourceTable;

private:
    bool m_processed;
    std::string m_root; // path to directory containing project file
    std::string m_project_file; // path to project file relative to root
    std::vector<std::string> m_transient_files; // temporary script files
    std::string m_extension_init_script_source; // source for extension init (generated script)
    std::set<std::string> m_ignored_assets; // don't load these assets.
    // reads an arf project file
    void process_arf();

    // reads an xml project file
    void process_xml();

    // parses the given DOM tree for the given type of resources
    void read_resource_tree(ResourceTree& out, pugi::xml_node& xml, ResourceType type);

    // adds the resources from the given extension to the project.
    void process_extension(const char* path_to_extension);

public:
    void add_constant(const std::string& name, const std::string& value);
    
    void ignore_asset(const std::string& name);

private:
    void add_script(const std::string& name, const std::string& source);

    template<typename ResourceType>
    void load_file_asset(ResourceTree& tree);

    template<typename ResourceType>
    void parse_asset(const bytecode::ProjectAccumulator&, ResourceTree& tree);

    template<typename ResourceType>
    void assign_ids(bytecode::ProjectAccumulator&, ResourceTree& tree);

    template<typename ResourceType>
    void precompile_asset(bytecode::ProjectAccumulator&, ResourceTree& tree);

    template<typename ResourceType>
    void compile_asset(bytecode::ProjectAccumulator&, ResourceTree& tree, const bytecode::Library* library);
};

}}
