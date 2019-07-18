#include "ogm/project/Project.hpp"

#include "macro.hpp"

#include "ogm/project/resource/ResourceScript.hpp"
#include "ogm/project/resource/ResourceSprite.hpp"
#include "ogm/project/resource/ResourceObject.hpp"
#include "ogm/project/resource/ResourceSound.hpp"
#include "ogm/project/resource/ResourceBackground.hpp"
#include "ogm/project/resource/ResourceRoom.hpp"

#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"
#include "ogm/asset/Config.hpp"
#include "ogm/ast/parse.h"

#include <pugixml.hpp>
#include <iostream>
#include <fstream>

#include <chrono>

namespace ogm { namespace project {

const char* RESOURCE_TYPE_NAMES[] =
{
    "sprite",
    "sound",
    "background",
    "path",
    "script",
    "shader",
    "font",
    "timeline",
    "object",
    "room",
    "constant"
};

const char* RESOURCE_TREE_NAMES[] =
{
    "sprites",
    "sounds",
    "backgrounds",
    "paths",
    "scripts",
    "shaders",
    "fonts",
    "timelines",
    "objects",
    "rooms",
    "constants"
};

ResourceTableEntry::ResourceTableEntry(ResourceType r, const char* path, const char* name): m_type(r), m_path(path), m_name(name), m_ptr(nullptr)
{ }

ResourceTableEntry::ResourceTableEntry(ResourceType r, Resource* ptr): m_type(r), m_ptr(ptr), m_name("")
{ }

ResourceTableEntry::ResourceTableEntry(const ResourceTableEntry& r): m_type(r.m_type), m_path(r.m_path), m_name(r.m_name), m_ptr(r.m_ptr)
{ }

Resource* ResourceTableEntry::get()
{
    if (m_ptr)
    {
        return m_ptr;
    }

    // if resource not realized, construct it:
    switch (m_type) {
        case SCRIPT:
            m_ptr = new ResourceScript(m_path.c_str(), m_name.c_str());
            break;
        case OBJECT:
            m_ptr = new ResourceObject(m_path.c_str(), m_name.c_str());
            break;
        case SPRITE:
            m_ptr = new ResourceSprite(m_path.c_str(), m_name.c_str());
            break;
        case ROOM:
            m_ptr = new ResourceRoom(m_path.c_str(), m_name.c_str());
            break;
        case BACKGROUND:
            m_ptr = new ResourceBackground(m_path.c_str(), m_name.c_str());
            break;
        case SOUND:
            m_ptr = new ResourceSound(m_path.c_str(), m_name.c_str());
            break;
        default:
            m_ptr = nullptr;
            std::cout << "Can't construct resource " << m_path << " (not supported)\n";
            break;
    }
    return m_ptr;
}

Project::Project(const char* path): m_root(path_directory(path)), m_project_file(path_leaf(path))
{ }

void Project::process()
{
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file((m_root + m_project_file).c_str());

    std::cout<<"reading project file " << m_root<<std::endl;
    std::cout<<"Load result: "<<result.description()<<std::endl;

    new (&m_resourceTree) ResourceTree();

    pugi::xml_node assets = doc.child("assets");

    for (int r = 0; r < NONE; r++) {
        int prev_rte_size = m_resourceTable.size();
        ResourceType r_type = (ResourceType)r;
        read_resource_tree(m_resourceTree, &assets, r_type);
        bool add = false;
        if (m_resourceTree.list.empty()){
            add = true;
        }
        else if (m_resourceTree.list.back().m_type != r_type) {
            add = true;
        }
        if (add)
            m_resourceTree.list.push_back(ResourceTree());

        std::cout<<"Added "<<m_resourceTable.size() - prev_rte_size<<" "<<RESOURCE_TREE_NAMES[r_type]<<std::endl;
    }
    m_processed = true;
}

const char* RESOURCE_EXTENSION[] =
{
    ".sprite.gmx",
    ".sound.gmx", //sounds
    ".background.gmx",
    "", //paths
    ".gml", //scripts
    "", //shaders
    "", //fonts
    "", //timelines
    ".object.gmx",
    ".room.gmx",
    "" // constants do not have file associations
};

void Project::read_resource_tree(ResourceTree& m_root, void* xml_v, ResourceType t)
{
    pugi::xml_document& xml = *(pugi::xml_document*)xml_v;

    for (pugi::xml_node node: xml.children()) {
        // subtree
        if (node.name() == std::string(RESOURCE_TREE_NAMES[t])) {
            m_root.list.push_back(ResourceTree());
            m_root.list.back().m_type = t;
            ResourceTree& rt = m_root.list.back();
            read_resource_tree(rt, &node, t);
        }

        // actual resource
        if (node.name() == std::string(RESOURCE_TYPE_NAMES[t])) {
            //! add m_resourceTable entry
            m_root.list.push_back(ResourceTree());
            m_root.list.back().m_type = t;
            std::string value = remove_suffix(node.text().get(), ".gml");
            ResourceTableEntry rte(t, case_insensitive_native_path(this->m_root, value + RESOURCE_EXTENSION[t]).c_str(), path_leaf(value).c_str());
            std::string name;
            if (t == CONSTANT) {
                // constants are defined directly in the .project.gmx file; no additional cost
                // to realizing them immediately.
                ResourceConstant* res = new ResourceConstant();
                res->m_value = value;
                new(&rte) ResourceTableEntry(t, res);

                // determine resource name:
                name = node.attribute("name").value();
                res->m_name = name;
            } else {
                // determine resource name:
                name = path_leaf(value);
            }
            // insert resource table entry
            m_resourceTable.insert(std::make_pair(name, rte));
            m_root.list.back().rtkey = name;
            m_root.list.back().is_leaf = true;
        }
    }
}

#ifdef PARALLEL_COMPILE
namespace
{
    ThreadPool g_tp(std::thread::hardware_concurrency());
    std::vector<std::future<void>> g_jobs;

    void join()
    {
        for (auto& job : g_jobs)
        {
            assert(job.valid());
            job.get();
        }
        g_jobs.clear();
    }
}
#else
namespace
{
    void join()
    { }
}
#endif

void Project::compile(bytecode::ProjectAccumulator& accumulator, const bytecode::Library* library)
{
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    // skip 0, which is the special entrypoint.
    accumulator.next_bytecode_index();

    std::cout << "Constants.\n";
    precompile_asset<ResourceConstant>(accumulator, m_resourceTree.list[CONSTANT]);

    // load files
    std::cout << "Scripts (load).\n";
    load_file_asset<ResourceScript>(m_resourceTree.list[SCRIPT]);
    load_file_asset<ResourceObject>(m_resourceTree.list[OBJECT]);
    std::cout << "Rooms (load).\n";
    load_file_asset<ResourceRoom>(m_resourceTree.list[ROOM]);

    join(); // paranoia

    // parsing
    std::cout << "Scripts (parse).\n";
    parse_asset<ResourceScript>(m_resourceTree.list[SCRIPT]);
    join();
    std::cout << "Objects (parse).\n";
    parse_asset<ResourceObject>(m_resourceTree.list[OBJECT]);
    join();
    std::cout << "Rooms (parse).\n";
    parse_asset<ResourceRoom>(m_resourceTree.list[ROOM]);
    join();

    std::cout << "Built-in events.\n";
    if (accumulator.m_config)
    {
        {
            bytecode::Bytecode default_step;
            ogm_ast* default_step_ast = ogm_ast_parse(k_default_step_builtin);
            bytecode::bytecode_generate(
                default_step,
                {default_step_ast, 0, 0, "default step_builtin", k_default_step_builtin},
                library, &accumulator
            );
            bytecode_index_t bi = accumulator.next_bytecode_index();
            accumulator.m_bytecode->add_bytecode(bi, std::move(default_step));
            accumulator.m_config->m_default_events[static_cast<size_t>(asset::StaticEvent::STEP_BUILTIN)] = bi;
            ogm_ast_free(default_step_ast);
        }

        {
            bytecode::Bytecode default_draw;
            ogm_ast* default_draw_ast = ogm_ast_parse(k_default_draw_normal);
            bytecode::bytecode_generate(
                default_draw,
                {default_draw_ast, 0, 0, "default draw", k_default_draw_normal},
                library, &accumulator
            );
            bytecode_index_t bi = accumulator.next_bytecode_index();
            accumulator.m_bytecode->add_bytecode(bi, std::move(default_draw));
            accumulator.m_config->m_default_events[static_cast<size_t>(asset::StaticEvent::DRAW)] = bi;
            ogm_ast_free(default_draw_ast);
        }
    }

    // build assets and take note of code which has global effects like enums, globalvar, etc.
    std::cout << "Sprites.\n";
    precompile_asset<ResourceSprite>(accumulator, m_resourceTree.list[SPRITE]);
    std::cout << "Backgrounds.\n";
    precompile_asset<ResourceBackground>(accumulator, m_resourceTree.list[BACKGROUND]);
    std::cout << "Sounds.\n";
    precompile_asset<ResourceSound>(accumulator, m_resourceTree.list[SOUND]);
    std::cout << "Scripts (accumulate).\n";
    precompile_asset<ResourceScript>(accumulator, m_resourceTree.list[SCRIPT]);
    std::cout << "Objects (accumulate).\n";
    precompile_asset<ResourceObject>(accumulator, m_resourceTree.list[OBJECT]);
    std::cout << "Rooms (accumulate).\n";
    precompile_asset<ResourceRoom>(accumulator, m_resourceTree.list[ROOM]);

    if (!accumulator.m_bytecode->has_bytecode(0))
    // default entrypoint
    {
        ogm_ast_t* entrypoint_ast;
        // TODO: put in an actual game loop here, preferably written in its own file.
        entrypoint_ast = ogm_ast_parse(k_default_entrypoint);
        bytecode::Bytecode b;
        bytecode::bytecode_generate(
            b,
            {entrypoint_ast, 0, 0, "Default Entrypoint", k_default_entrypoint},
            library, &accumulator
        );
        accumulator.m_bytecode->add_bytecode(0, std::move(b));
        ogm_ast_free(entrypoint_ast);
    }

    join(); // paranoia

    // compile code
    std::cout << "Scripts (compile).\n";
    compile_asset<ResourceScript>(accumulator, m_resourceTree.list[SCRIPT], library);
    join();
    std::cout << "Objects (compile).\n";
    compile_asset<ResourceObject>(accumulator, m_resourceTree.list[OBJECT], library);
    join();
    std::cout << "Rooms (compile).\n";
    compile_asset<ResourceRoom>(accumulator, m_resourceTree.list[ROOM], library);
    join();
    std::cout << "Build complete.\n";
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();
    std::cout << "Compilation took " << duration << " ms" << std::endl;
}

template<typename AssetType>
void Project::load_file_asset(ResourceTree& tree)
{
    if (!tree.is_leaf)
    {
        for (auto iter : tree.list)
        {
            load_file_asset<AssetType>(iter);
        }
    }
    else
    {
        m_resourceTable[tree.rtkey].get()->load_file();
    }
}

template<typename AssetType>
void Project::parse_asset(ResourceTree& tree)
{
    if (!tree.is_leaf)
    {
        for (auto iter : tree.list)
        {
            parse_asset<AssetType>(iter);
        }
    }
    else
    {
        #ifdef PARALLEL_COMPILE
        // parse_asset for subtree asynchronously.
        g_jobs.push_back(
            g_tp.enqueue(
                [](Resource* a)
                {
                    a->parse();
                },
                m_resourceTable[tree.rtkey].get()
            )
        );
        #else
        m_resourceTable[tree.rtkey].get()->parse();
        #endif
    }
}

template<typename AssetType>
void Project::precompile_asset(bytecode::ProjectAccumulator& accumulator, ResourceTree& tree)
{
    // single-threaded due to the global accumulation, including
    // - assignment of asset indices
    // - macros and globalvar declarations

    if (!tree.is_leaf)
    {
        for (auto iter : tree.list)
        {
            precompile_asset<AssetType>(accumulator, iter);
        }
    }
    else
    {
        dynamic_cast<AssetType*>(m_resourceTable[tree.rtkey].get())->precompile(accumulator);
    }
}

template<typename ResourceType>
void Project::compile_asset(bytecode::ProjectAccumulator& accumulator, ResourceTree& tree, const bytecode::Library* library)
{
    if (!tree.is_leaf) {
        for (auto& iter : tree.list) {
            compile_asset<ResourceType>(accumulator, iter, library);
        }
    } else {
        #ifdef PARALLEL_COMPILE
        // compile for subtree asynchronously.
        g_jobs.push_back(
            std::async(std::launch::async,
                [](ResourceType* a, bytecode::ProjectAccumulator& accumulator, const bytecode::Library* library)
                {
                    a->compile(accumulator, library);
                },
                dynamic_cast<ResourceType*>(m_resourceTable[tree.rtkey].get()),
                std::ref(accumulator),
                library
            )
        );
        #else
        dynamic_cast<ResourceType*>(m_resourceTable[tree.rtkey].get())->compile(accumulator, library);
        #endif
    }
}

void ResourceConstant::precompile(bytecode::ProjectAccumulator& acc)
{
    set_macro(m_name.c_str(), m_value.c_str(), *acc.m_reflection);
}
}}
