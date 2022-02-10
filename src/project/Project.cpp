#include "ogm/project/Project.hpp"

#include "ogm/project/resource/ResourceScript.hpp"
#include "ogm/project/resource/ResourceSprite.hpp"
#include "ogm/project/resource/ResourceObject.hpp"
#include "ogm/project/resource/ResourceSound.hpp"
#include "ogm/project/resource/ResourceBackground.hpp"
#include "ogm/project/resource/ResourceRoom.hpp"
#include "ogm/project/resource/ResourceShader.hpp"
#include "ogm/project/resource/ResourceFont.hpp"
#include "ProjectResources.hpp"

#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"
#include "ogm/asset/Config.hpp"
#include "ogm/ast/parse.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <set>
#include <atomic>

namespace ogm::project {

// source code for some default code
// set in Builtin.hpp
extern const char* k_default_entrypoint;
extern const char* k_default_step_builtin;
extern const char* k_default_draw_normal;

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
    "audiogroup",
    "constant"
};

const char* RESOURCE_TYPE_NAMES_ALT[] =
{
    "~",
    "~",
    "tileset",
    "~",
    "~",
    "~",
    "~",
    "~",
    "~",
    "~",
    "~",
    "~"
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
    "audiogroups",
    "constants"
};

const char* RESOURCE_EXTENSION[] =
{
    ".sprite.gmx",
    ".sound.gmx", //sounds
    ".background.gmx",
    ".path.gmx", //paths
    ".gml", //scripts
    ".shader", //shaders
    ".font.gmx", //fonts
    ".timeline.gmx", //timelines
    ".object.gmx",
    ".room.gmx",
    "", // audiogroups do not have file associatsions
    "" // constants do not have file associations
};

const char* RESOURCE_EXTENSION_OGM[] =
{
    ".sprite.ogm",
    ".sound.ogm", //sounds
    ".background.ogm",
    ".path.ogm", //paths
    ".ogm", //scripts
    ".shader.ogm", //shaders
    ".font.ogm", //fonts
    ".timeline.ogm", //timelines
    ".object.ogm",
    ".room.ogm",
    "", // audiogroups do not have file associatsions
    "" // constants do not have file associations
};

const char* RESOURCE_EXTENSION_ARF[] =
{
    ".sprite.arf",
    ".sound.arf", //sounds
    ".background.arf",
    ".path.arf", //paths
    ".ogm", //scripts
    ".shader.arf", //shaders
    ".font.arf", //fonts
    ".timeline.arf", //timelines
    ".object.arf",
    ".room.arf",
    "", // audiogroups do not have file associatsions
    "" // constants do not have file associations
};

Resource* LazyResource::get()
{
    if (!m_ptr)
    {
        m_ptr = m_construct_resource();
    }
    assert(m_ptr);
    return m_ptr.get();
}

Project::Project(const char* path)
    : m_root(path_directory(path))
    , m_project_file(path_leaf(path))
{ }

void Project::populate_tree_default()
{
    for (int r = 0; r < NONE; r++)
    {
        ResourceType r_type = static_cast<ResourceType>(r);
        std::string name = RESOURCE_TREE_NAMES[r_type];
        if (name != "")
        {
            m_tree.subdirectory(name);
        }
    }
}

void Project::process()
{
    if (m_processed) return;
    
    // add top-level folders to resource tree.
    // (e.g. sprites/, objects/, etc.)
    populate_tree_default();
    
    if (ends_with(m_project_file, ".gmx"))
    {
        process_xml();
    }
    else if (ends_with(m_project_file, ".arf") || (ends_with(m_project_file, ".ogm")))
    {
        process_arf();
    }
    else if (ends_with(m_project_file, ".yyp"))
    {
        process_json();
    }
    else
    {
        throw ProjectError(1000, "Unrecognized extension for project file \"{}\"", m_project_file);
    }
}

bool Project::infer_resource_type_from_extension(const std::string& extension, ResourceType& out_type)
{
    if (extension == "")
    {
        return false;
    }
    if (extension == ".yy")
    {
        out_type = NONE;
        return true;
    }
    else for (size_t i = 0; i < static_cast<size_t>(NONE); ++i)
    {
        if (extension == RESOURCE_EXTENSION[i])
        {
            out_type = static_cast<ResourceType>(i);
            return true;
        }
        else if (extension == RESOURCE_EXTENSION_OGM[i])
        {
            out_type = static_cast<ResourceType>(i);
            return true;
        }
        else if (extension == RESOURCE_EXTENSION_ARF[i])
        {
            out_type = static_cast<ResourceType>(i);
            return true;
        }
    }
    return false;
}

ResourceList& Project::asset_tree(ResourceType type)
{
    return *m_tree.subdirectory(RESOURCE_TREE_NAMES[type]);
}

void Project::add_resource_from_path(ResourceType type, const std::string& path, ResourceList* list, std::string name)
{
    // we can't handle these yet.
    if (type == PATH || type == TIMELINE)
    {
        return;
    }
    
    assert(type != CONSTANT && type != NONE && type != AUDIOGROUP);
    if (!list) list = &asset_tree(type);
    
    if (name == "")
    // determine resource id from path
    {
        name = remove_extension(path_leaf(path));
    }
    
    resource_id_t id = name;
    
    ResourceLeaf* resource_node = list->leaf(name, id);
    (void)resource_node;
    assert(resource_node);
    
    // resource constructor
    std::function<std::unique_ptr<Resource>()> fn;
    
    switch (type)
    {
    case SCRIPT:
        fn = construct_resource<ResourceScript>(path, name);
        break;
    case OBJECT:
        fn = construct_resource<ResourceObject>(path, name);
        break;
    case SPRITE:
        fn = construct_resource<ResourceSprite>(path, name);
        break;
    case ROOM:
        fn = construct_resource<ResourceRoom>(path, name);
        break;
    case BACKGROUND:
        fn = construct_resource<ResourceBackground>(path, name);
        break;
    case SOUND:
        fn = construct_resource<ResourceSound>(path, name);
        break;
    case SHADER:
        fn = construct_resource<ResourceShader>(path, name);
        break;
    case FONT:
        fn = construct_resource<ResourceFont>(path, name);
        break;
    case TIMELINE:
    case PATH:
        throw ProjectError(1006, "Resource type not yet supported: \"{}\"", RESOURCE_TYPE_NAMES[type]);
    default:
        throw ProjectError(1001, "Cannot add resource from path \"{}\"", path);
    }
    
    LazyResource rte(type,
        std::move(fn)
    );
    m_resources.emplace(name, std::move(rte));
}

void Project::scan_resource_directory(const std::string& resource_path, ResourceType default_type)
{
    // scan resource directory
    std::vector<std::string> paths;
    list_paths_recursive(resource_path + "/", paths);

    for (const std::string& path : paths)
    {
        ResourceType type;
        if (
            infer_resource_type_from_extension(
                get_extension(path), type
            )
        )
        // path to resource file
        {
            // default, or skip if can't infer.
            type = (type == NONE) ? default_type : type;
            assert(type != CONSTANT);
            if (type == NONE) continue;
            
            add_resource_from_path(type, path);
        }
    }
}

void Project::add_script(const resource_id_t& name, const std::string& source)
{
    LazyResource rte{
        SCRIPT,
        [source, name]()
        {
            // produces a script directly from source code.
            return std::make_unique<ResourceScript>
                (false, source.c_str(), name.c_str());
        }
    };
    
    m_resources.emplace(name, std::move(rte));
    m_tree.subdirectory(".hidden")->leaf(name, name);
}

void Project::ignore_asset(const std::string& name)
{
    m_ignored_assets.insert(name);
}

void Project::add_constant(const std::string& name, const std::string& value)
{
    // create "constant" resource.
    LazyResource rte(ResourceType::CONSTANT,
        [name, value]()
        {
            return std::make_unique<ResourceConstant>(
                name, value
            );
        }
    );

    // insert/replace resource table entry in table.
    m_resources.erase(name);
    m_resources.emplace(name, std::move(rte));
    m_tree.subdirectory(".hidden")->leaf(name, name);
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
            ogm_assert(job.valid());
            job.get();
        }
        g_jobs.clear();
    }
}
#else
namespace
{
    inline void join()
    { }
}
#endif

bool Project::build(bytecode::ProjectAccumulator& accumulator)
{
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    // skip 0, which is the special entrypoint.
    bytecode_index_t entrypoint_bytecode_index = accumulator.next_bytecode_index();

    m_build_progress = 0;
    m_build_max = 0;
    m_parallel_compile = accumulator.m_config->m_parallel_compile;
    
    // FIXME: bit of a hack to put this here
    if (ends_with(m_project_file, ".yyp"))
    {
        // mark v2 parsing to get e.g. @-strings and 0x-style hex
        accumulator.m_config->m_parse_flags |= 2;
    }

    auto print_category = [](std::string s) {
        std::cout << ansi_colour("1;35") << s << ansi_colour("0");
    };

    // determine how much work there is to do.
    for_resource(&m_tree, [this](Resource* r)
        {
            // number of steps per resource.
            m_build_max += 5;
        }
    );
    
    // reset error flag.
    m_error = false;

    accumulator.m_project_base_directory = m_root;
    accumulator.m_included_directory = m_root + "datafiles" + PATH_SEPARATOR;
    
    #define CHECK_ERROR_SET()           \
    if (m_error)                        \
    {                                   \
        std::cout << "Build failed.\n"; \
        return false;                   \
    };

    // load files
    print_category("Loading files...\n");
    for_resource(
        &m_tree,
        [](Resource* r)
        {
            r->load_file();
        },
        "loading files for"
    );
    join(); // paranoia
    CHECK_ERROR_SET();

    // parsing
    for_resource(
        &m_tree,
        [&accumulator](Resource* r)
        {
            r->parse(accumulator);
        },
        "parsing"
    );
    join();
    CHECK_ERROR_SET();

    // built-in events
    print_category("Built-in events...\n");
    if (accumulator.m_config)
    {
        {
            ogm_ast* default_step_ast = ogm_ast_parse(k_default_step_builtin);
            bytecode_index_t bi =bytecode::bytecode_generate(
                {default_step_ast, "default step_builtin", k_default_step_builtin},
                accumulator
            );
            accumulator.m_config->m_default_events[static_cast<size_t>(asset::StaticEvent::STEP_BUILTIN)] = bi;
            ogm_ast_free(default_step_ast);
        }

        {
            ogm_ast* default_draw_ast = ogm_ast_parse(k_default_draw_normal);
            bytecode_index_t bi = bytecode::bytecode_generate(
                {default_draw_ast, "default draw", k_default_draw_normal},
                accumulator
            );
            accumulator.m_config->m_default_events[static_cast<size_t>(asset::StaticEvent::DRAW)] = bi;
            ogm_ast_free(default_draw_ast);
        }
    }

    // assign asset IDs
    print_category("Assigning asset IDs...\n");
    for_resource(
        &m_tree,
        [&accumulator](Resource* r)
        {
            r->assign_id(accumulator);
        },
        "assigning IDs for"
    );
    join(); // paranoia
    CHECK_ERROR_SET();

    // build assets and take note of code which has global effects like enums, globalvar, etc.
    print_category("Accumulating...\n");
    for_resource(
        &m_tree,
        [&accumulator](Resource* r)
        {
            r->precompile(accumulator);
        },
        "precompiling"
    );
    join(); // paranoia
    CHECK_ERROR_SET();

    if (!accumulator.m_bytecode->has_bytecode(0))
    // default entrypoint
    {
        ogm_ast_t* entrypoint_ast;
        // TODO: put in an actual game loop here, preferably written in its own file.
        entrypoint_ast = ogm_ast_parse(k_default_entrypoint);
        bytecode_index_t index = bytecode::bytecode_generate(
            {entrypoint_ast, "default_entrypoint", k_default_entrypoint},
            accumulator,
            nullptr,
            entrypoint_bytecode_index
        );
        ogm_assert(index == 0);
        ogm_ast_free(entrypoint_ast);
    }

    // compile code
    print_category("Compiling...\n");
    for_resource_parallel(
        &m_tree,
        [&accumulator](Resource* r)
        {
            r->compile(accumulator);
        },
        "compiling"
    );
    join(); // paranoia
    CHECK_ERROR_SET();
    
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();
    print_category("Build complete");
    std::cout << " (" << duration << " ms)" << std::endl;
    return true;
}

template<bool parallel>
void Project::for_resource(ResourceTree* tree, const std::function<void(Resource*)>& f, const char* description)
{
    if (m_error) return;
    if (!tree->is_leaf())
    // tree node
    {
        for (size_t i = 0; i < tree->get_child_count(); ++i)
        {
            for_resource<parallel>(tree->get_child(i), f, description);
        }
    }
    else
    // leaf node
    {
        Resource* r = m_resources.at(tree->get_resource_id()).get();
        auto lambda = [this, &f, r, description]()
        {
            try
            {
                // TODO: don't lock if parallel disabled.
                if (description)
                {
                    WRITE_LOCK(m_progress_mutex)
                    m_build_progress++;
                    real_t progress = std::min<real_t>(1, m_build_progress / std::max<real_t>(m_build_max - 1, 1));
                    fmt::print(
                        "[{8:>3}%]{6} {0} {3}{1}{7} resource {5}{2}{4}\n",
                        description,
                        r->get_type_name(),
                        r->get_name(),
                        ansi_colour("1"),
                        ansi_colour("0"),
                        ansi_colour("1;36"),
                        ansi_colour("32"),
                        ansi_colour("0;32"),
                        static_cast<int>(progress * 100) // progress
                    );
                }
            }
            catch(...) {}
            try
            {
                f(r);
            }
            catch(ogm::Error& e)
            {
                if (description) std::cout << "\nAn error occurred " << description << " the project.\n";
                e.detail<resource_type>(r->get_type_name())
                 .detail<resource_name>(r->get_name());
                std::cout << e.what();
                m_error = true;
            }
            catch(std::exception& e)
            {
                if (description) std::cout << "Exception while " << description << " " << r->get_type_name() << " resource " << r->get_name() << ":\n";
                std::cout << e.what() << std::endl;
                m_error = true;
            }
            catch(...)
            {   
                if (description) std::cout << "Unknown exception while " << description << " " << r->get_type_name() << " resource " << r->get_name() << ".\n";
                m_error = true;
            }
        };

        #ifdef PARALLEL_COMPILE
        if (parallel && m_parallel_compile)
        {
            // compile for subtree asynchronously.
            g_jobs.push_back(
                g_tp.enqueue(
                    lambda
                )
            );
        }
        else
        #endif
        {
            lambda();
        }
    }
}

}
