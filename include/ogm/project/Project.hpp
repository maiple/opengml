#pragma once

#include "resource/Resource.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/bytecode/bytecode.hpp"

#include <cstring>
#include <string>
#include <vector>

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

    ResourceType m_type;

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

private:
    bool m_processed;
    ResourceTree  m_resourceTree;
    ResourceTable m_resourceTable;
    std::string m_root;
    std::string m_project_file;

    //! parses the given DOM tree for the given type of resources
    void read_resource_tree(ResourceTree& out, void* xml, ResourceType type);

    template<typename AssetType>
    void load_file_asset(ResourceTree& tree);

    template<typename AssetType>
    void parse_asset(ResourceTree& tree);

    template<typename AssetType>
    void precompile_asset(bytecode::ProjectAccumulator&, ResourceTree& tree);

    template<typename AssetType>
    void compile_asset(bytecode::ProjectAccumulator&, ResourceTree& tree, const bytecode::Library* library);
};

}}
