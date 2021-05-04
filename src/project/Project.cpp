#include "ogm/project/Project.hpp"

#include "macro.hpp"

#include "ogm/project/resource/ResourceScript.hpp"
#include "ogm/project/resource/ResourceSprite.hpp"
#include "ogm/project/resource/ResourceObject.hpp"
#include "ogm/project/resource/ResourceSound.hpp"
#include "ogm/project/resource/ResourceBackground.hpp"
#include "ogm/project/resource/ResourceRoom.hpp"
#include "ogm/project/resource/ResourceShader.hpp"
#include "ogm/project/resource/ResourceFont.hpp"
#include "ogm/project/arf/arf_parse.hpp"

#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"
#include "ogm/asset/Config.hpp"
#include "ogm/ast/parse.h"
#include "XMLError.hpp"

#include <nlohmann/json.hpp>
#include <pugixml.hpp>

#include <iostream>
#include <fstream>
#include <chrono>
#include <set>
#include <atomic>

using json = nlohmann::json;

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

ResourceList* Project::asset_tree(ResourceType type)
{
    return m_tree.subdirectory(RESOURCE_TREE_NAMES[type]);
}

namespace
{
    template<typename ResourceType>
    std::function<std::unique_ptr<Resource>()>
    construct_resource(const std::string& path, const std::string& name)
    {
        return [path, name]()
        {
            return std::make_unique<ResourceType>
                (path.c_str(), name.c_str());
        };
    }
}

void Project::add_resource_from_path(ResourceType type, const std::string& path, ResourceList* list, std::string name)
{
    // we can't handle these yet.
    if (type == PATH || type == TIMELINE)
    {
        return;
    }
    
    assert(type != CONSTANT && type != NONE);
    if (!list) list = asset_tree(type);
    
    if (name == "")
    // determine resource id from path
    {
        name = remove_extension(path_leaf(path));
    }
    
    resource_id_t id = name;
    
    ResourceLeaf* resource_node = list->leaf(name, id);
    (void)resource_node;
    assert(resource_node);
    
    // create resource constructor
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
            1100, 
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


static void checkModelName(const json& j, std::string expected)
{
    std::string field = j.at("modelName").get<std::string>();
    if (!ends_with(field, expected)) throw ProjectError(
        1003,
        "Expected modelName compatible with \"OGM{}\"; model name is \"{}\".",
        expected, field
    );
}

void Project::process_json()
{
    std::string path = path_join(m_root, m_project_file);
    std::fstream ifs(path);
    
    if (!ifs.good()) throw ProjectError(1005, "Error opening file \"{}\"", path);
    
    json j;
    ifs >> j;
    
    checkModelName(j, "Project");
    
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
    }
}

void Project::process_xml()
{
    std::string file_name = path_join(m_root, m_project_file);
    
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(file_name.c_str());

    std::cout << "reading project file " << file_name << std::endl;

    check_xml_result<ProjectError>(1061, result, file_name.c_str(), "Error parsing .project.gmx file: " + file_name);

    pugi::xml_node assets = doc.child("assets");

    for (int r = 0; r < NONE; r++)
    {
        int prev_rte_size = m_resources.size();
        ResourceType r_type = (ResourceType)r;
        read_resource_tree_xml(asset_tree(r_type), assets, r_type);

        std::cout << "Added "<< m_resources.size() - prev_rte_size << " " << RESOURCE_TREE_NAMES[r_type] <<std::endl;
    }

    // check for extensions
    pugi::xml_node extensions = assets.child("NewExtensions");

    m_extension_init_script_source = "#define ogm_extension_init\n\n";
    for (pugi::xml_node extension : extensions)
    {
        std::string extension_path = extension.text().get() + std::string(".extension.gmx");
        process_extension(extension_path.c_str());
    }

    // add extensions init script
    add_script("extension^init", m_extension_init_script_source);

    m_processed = true;
}

// adds contents of xml matching resource type to the given list.
void Project::read_resource_tree_xml(ResourceList* list, pugi::xml_node& xml, ResourceType t)
{
    for (pugi::xml_node node: xml.children()) {
        // subtree, e.g. <objects>
        if (node.name() == std::string(RESOURCE_TREE_NAMES[t])) {
            ResourceList* sublist = list->subdirectory(
                node.attribute("name").value()
            );
            
            // recurse into subtree
            read_resource_tree_xml(sublist, node, t);
        }

        // actual resource, e.g. <object>
        if (node.name() == std::string(RESOURCE_TYPE_NAMES[t])) {
            if (t == CONSTANT)
            // constants appear in the xml with different syntax from other resources.
            {
                std::string value = node.text().get();

                // determine resource name:
                std::string name = node.attribute("name").value();
                
                // read constant directly from xml.
                if (m_ignored_assets.find(name) != m_ignored_assets.end())
                {
                    // ignore this asset.
                    continue;
                }
                
                m_resources.emplace(name,
                    LazyResource(
                        CONSTANT,
                        [value, name]()
                        {
                            return std::make_unique<ResourceConstant>(
                                name, value
                            );
                        }
                    )
                );
                m_tree.subdirectory("constants")->leaf(name, name);
            }
            else
            {
                // name of resource (e.g. "objFoo")
                std::string value = node.text().get();
                
                // project xml contains these suffices for no apparent reason.
                value = remove_suffix(value, ".gml");
                value = remove_suffix(value, ".shader");
                
                if (m_ignored_assets.find(value) != m_ignored_assets.end())
                {
                    // ignore this asset.
                    continue;
                }
                
                std::string path;
                bool case_lookup = false;
                path = case_insensitive_native_path(this->m_root, value + RESOURCE_EXTENSION[t], &case_lookup);
                
                if (!path_exists(path)) throw ProjectError(1004, "Resource path not found: \"{}\"", path);
                
                if (case_lookup)
                {
                    std::cout << "WARNING: path \"" <<
                        this->m_root << value << RESOURCE_EXTENSION[t] <<
                        "\" required case-insensitive lookup\n  Became \"" <<
                        path <<
                        "\"\n  This is time-consuming and should be corrected.\n";
                }
                
                add_resource_from_path(t, path.c_str(), list, path_leaf(value));
            }
        }
    }
}

void Project::process_extension(const char* extension_path)
{
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(case_insensitive_native_path(m_root, extension_path).c_str());

    std::cout<<"reading extension file " << m_root << extension_path <<std::endl;

    if (!result)
    {
        std::cout << "ERROR: could not read file " << extension_path << std::endl;
        return;
    }

    std::string extension_base = remove_suffix(path_leaf(extension_path), ".extension.gmx");

    pugi::xml_node extension = doc.child("extension");
    pugi::xml_node files = extension.child("files");
    for (pugi::xml_node file : files.children("file"))
    {
        std::string filename = file.child("filename").text().get();
        std::string filename_base = remove_extension(filename);
        std::string dir = "extensions" + std::string(1, PATH_SEPARATOR) + extension_base + std::string(1, PATH_SEPARATOR);
        std::string path = case_insensitive_native_path(m_root, dir + filename);

        std::string rettype_name[2] = {"ty_real", "ty_string"};

        struct FunctionDef
        {
            std::string m_name; // name as called from ogm
            std::string m_external_name; // name as appears in extension library
            std::vector<bool> m_args; // true if argument is a string, false: real.
            bool m_ret_type; // true if returns a string. False: real.
        };

        std::vector<FunctionDef> functions;

        // add functions
        for (pugi::xml_node function : file.child("functions").children("function"))
        {
            functions.emplace_back();

            functions.back().m_name = function.child("name").text().get();
            functions.back().m_external_name = function.child("externalName").text().get();
            //size_t argc = functions.back().m_name = std::atoi(function.child("argCount").text().get());
            functions.back().m_ret_type = function.child("returnType").text().get() == std::string("1");

            for (pugi::xml_node arg : function.child("args").children("arg"))
            {
                functions.back().m_args.push_back(arg.text().get() == std::string("1"));
            }
        }

        // contains script resources
        if (ends_with(path, ".gml"))
        {
            // add to resource tree as a script
            std::string value = remove_suffix(path, ".gml");
            std::string name = "ogm-extension^" + path_leaf(value);
            LazyResource rte(
                ResourceType::SCRIPT, 
                construct_resource<ResourceScript>((value + ".gml"), name)
            );

            m_resources.emplace(name, std::move(rte));
            m_tree.subdirectory(".hidden")->leaf(name, name);
        }
        else if (ends_with(path, ".dll") || ends_with(path, ".so") || ends_with(path, ".dylib"))
        {
            std::set<std::string> proxies;
            proxies.insert(filename);

            for (pugi::xml_node proxy : file.child("ProxyFiles").children("ProxyFile"))
            {
                // TODO: use target mask
                // for now just check the extension.
                proxies.insert(proxy.child("Name").text().get());
            }

            std::stringstream ss_init_code;
            std::string bc_name = "ogm_init_extension_" + extension_base + "_" + filename_base;
            ss_init_code << "#define " << bc_name << "\n\n";
            ss_init_code << "var _libpath = \"" + filename + "\";\n\n";
            for (const std::string& file : proxies)
            {
                std::string path = case_insensitive_native_path(m_root, dir + file);

                if (ends_with(path, ".dll"))
                {
                    ss_init_code << "if (os_type == os_windows)\n{\n";
                    ss_init_code << "    _libpath = \"" + path + "\";\n}\n";
                }
                else if (ends_with(path, ".so"))
                {
                    ss_init_code << "if (os_type == os_linux)\n{\n";
                    ss_init_code << "    _libpath = \"" + path + "\";\n}\n";
                }
                else if (ends_with(path, ".dylib"))
                {
                    ss_init_code << "if (os_type == os_macosx)\n{\n";
                    ss_init_code << "    _libpath = \"" + path + "\";\n}\n";
                }
            }

            std::stringstream fbodies;
            for (const FunctionDef& fn : functions)
            {
                // fully-qualified name
                std::string fname_fq = "global.ogm_extension_external_" + extension_base + "_" + filename_base + "_" + fn.m_name;

                // semi-qualified name
                std::string fname_sq = fn.m_name;

                // external define
                ss_init_code << fname_fq << " = external_define(_libpath, \"" << fn.m_external_name << "\"";

                // TODO: is cdecl the correct assumption? Does the extension file state which should be used?
                ss_init_code << ", dll_cdecl, " << rettype_name[fn.m_ret_type] << ", " << fn.m_args.size();
                for (bool arg : fn.m_args)
                {
                    ss_init_code << ", " << rettype_name[arg];
                }
                ss_init_code << ");\n";

                // function body code
                fbodies << "#define " << fname_sq << "\n\n";
                fbodies << "return external_call(" << fname_fq;
                for (size_t i = 0; i < fn.m_args.size(); ++i)
                {
                    fbodies << ", argument" << i;
                }
                fbodies << ");\n\n";
            }

            ss_init_code << "\n";
            ss_init_code << fbodies.str();

            // add to script
            // so as not to throw errors from missing definitions, we only skip
            // if the extension is ignored, we only skip running its initialization code.
            if (m_ignored_assets.find(extension_base) == m_ignored_assets.end())
            {
                m_extension_init_script_source += bc_name + "();\n";
            }

            std::string name = "extension^" + bc_name;
            add_script(name, ss_init_code.str());
        }

        // add constants
        for (pugi::xml_node constant : file.child("constants").children("constant"))
        {
            std::string name = constant.child("name").text().get();
            std::string value = constant.child("value").text().get();

            add_constant(name, value);
        }
    }
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

void ResourceConstant::precompile(bytecode::ProjectAccumulator& acc)
{
    set_macro(m_name.c_str(), m_value.c_str(), *acc.m_reflection);
}
}
