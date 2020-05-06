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

#include <pugixml.hpp>
#include <iostream>
#include <fstream>

#include <chrono>
#include <set>

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

ResourceTableEntry::ResourceTableEntry(ResourceType r, const char* path, const char* name): m_type(r), m_path(path), m_name(name), m_ptr(nullptr)
{ }

ResourceTableEntry::ResourceTableEntry(ResourceType r, Resource* ptr): m_type(r), m_ptr(ptr), m_name("")
{ }

Resource* ResourceTableEntry::get(std::vector<std::string>* init_files)
{
    if (m_ptr)
    {
        return m_ptr.get();
    }

    // if resource not realized, construct it:
    switch (m_type) {
        case SCRIPT:
            {
                if (starts_with(m_path, "::transient::"))
                {
                    // initialize from init file vector instead of from a file on the disk.
                    size_t index = std::atoi(m_path.substr(13).c_str());
                    m_ptr = std::make_unique<ResourceScript>
                        (false, init_files->at(index).c_str(), m_name.c_str());
                }
                else
                {
                    m_ptr = std::make_unique<ResourceScript>
                        (m_path.c_str(), m_name.c_str());
                }
            }
            break;
        case OBJECT:
            m_ptr = std::make_unique<ResourceObject>(m_path.c_str(), m_name.c_str());
            break;
        case SPRITE:
            m_ptr = std::make_unique<ResourceSprite>(m_path.c_str(), m_name.c_str());
            break;
        case ROOM:
            m_ptr = std::make_unique<ResourceRoom>(m_path.c_str(), m_name.c_str());
            break;
        case BACKGROUND:
            m_ptr = std::make_unique<ResourceBackground>(m_path.c_str(), m_name.c_str());
            break;
        case SOUND:
            m_ptr = std::make_unique<ResourceSound>(m_path.c_str(), m_name.c_str());
            break;
        case SHADER:
            m_ptr = std::make_unique<ResourceShader>(m_path.c_str(), m_name.c_str());
            break;
        case FONT:
            m_ptr = std::make_unique<ResourceFont>(m_path.c_str(), m_name.c_str());
            break;
        default:
            m_ptr = nullptr;
            std::cout << "Can't construct resource " << m_path << " (not supported)\n";
            break;
    }
    return m_ptr.get();
}

Project::Project(const char* path): m_root(path_directory(path)), m_project_file(path_leaf(path))
{ }

void Project::process()
{
    if (ends_with(m_project_file, ".gmx"))
    {
        process_xml();
    }
    else if (ends_with(m_project_file, ".arf") || (ends_with(m_project_file, ".ogm")))
    {
        process_arf();
    }
    else
    {
        throw MiscError("Unrecognized extension for project file " + m_project_file);
    }
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
        throw MiscError(
            "Failed to parse project file " + m_project_file + ":\n" + e.what()
        );
    }

    // resources
    {
        // scan subdirectories for files.
        for (int r = 0; r < NONE; r++)
        {
            ResourceTree& tree = m_resourceTree.list.emplace_back();
            ResourceType type = static_cast<ResourceType>(r);
            if (r == CONSTANT)
            {
                // skip constants
                continue;
            }
            tree.m_name = RESOURCE_TREE_NAMES[r];
            std::string resource_directory_path = m_root + RESOURCE_TYPE_NAMES[r];
            if (!path_exists(resource_directory_path))
            {
                resource_directory_path += "s";
            }
            if (!path_exists(resource_directory_path))
            {
                continue;
            }

            // scan resource directory
            std::vector<std::string> paths;
            list_paths_recursive(resource_directory_path + "/", paths);

            for (const std::string& path : paths)
            {
                if (
                    ends_with(path, RESOURCE_EXTENSION[r])
                    || ends_with(path, RESOURCE_EXTENSION_OGM[r])
                    || ends_with(path, RESOURCE_EXTENSION_ARF[r])
                )
                // path to resource file
                {
                    ResourceTree& asset_tree = tree.list.emplace_back();
                    asset_tree.m_type = type;

                    // remove suffix from path.
                    std::string name = path_leaf(path);
                    name = remove_suffix(name, RESOURCE_EXTENSION[r]);
                    name = remove_suffix(name, RESOURCE_EXTENSION_OGM[r]);
                    name = remove_suffix(name, RESOURCE_EXTENSION_ARF[r]);

                    ResourceTableEntry rte(type, path.c_str(), name.c_str());
                    m_resourceTable.insert(std::make_pair(name, std::move(rte)));

                    asset_tree.rtkey = name;
                    asset_tree.is_leaf = true;
                }
            }
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

void Project::add_script(const std::string& name, const std::string& source)
{
    ResourceTableEntry rte(ResourceType::SCRIPT, ("::transient::" + std::to_string(m_transient_files.size())).c_str(), name.c_str());
    m_resourceTable.insert(std::make_pair(name, std::move(rte)));
    m_resourceTree.list[SCRIPT].list.push_back(ResourceTree());
    m_resourceTree.list[SCRIPT].list.back().m_type = ResourceType::SCRIPT;
    m_resourceTree.list[SCRIPT].list.back().rtkey = name;
    m_resourceTree.list[SCRIPT].list.back().is_leaf = true;
    m_resourceTree.list[SCRIPT].list.back().is_hidden = true;
    m_transient_files.push_back(source);
}

void Project::ignore_asset(const std::string& name)
{
    m_ignored_assets.insert(name);
}

void Project::process_xml()
{
    pugi::xml_document doc;
    std::string file_name = (m_root + m_project_file);
    pugi::xml_parse_result result = doc.load_file(file_name.c_str());

    std::cout << "reading project file " << m_root << m_project_file << std::endl;

    check_xml_result(result, file_name.c_str(), "Error parsing .project.gmx file: " + m_project_file);

    new (&m_resourceTree) ResourceTree();

    pugi::xml_node assets = doc.child("assets");

    for (int r = 0; r < NONE; r++)
    {
        int prev_rte_size = m_resourceTable.size();
        ResourceType r_type = (ResourceType)r;
        read_resource_tree(m_resourceTree, assets, r_type);
        bool add = false;

        // add blank resource tree if read_resource_tree did not add one.
        if (m_resourceTree.list.empty())
        {
            add = true;
        }
        else if (m_resourceTree.list.back().m_type != r_type)
        {
            add = true;
        }
        if (add)
        {
            m_resourceTree.list.push_back(ResourceTree());
            m_resourceTree.list.back().is_leaf = false;
            m_resourceTree.list.back().m_type = r_type;
            m_resourceTree.list.back().m_name = "";
        }

        std::cout << "Added "<<m_resourceTable.size() - prev_rte_size<<" "<<RESOURCE_TREE_NAMES[r_type]<<std::endl;
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

void Project::read_resource_tree(ResourceTree& root, pugi::xml_node& xml, ResourceType t)
{
    for (pugi::xml_node node: xml.children()) {
        // subtree, e.g. <objects>
        if (node.name() == std::string(RESOURCE_TREE_NAMES[t])) {
            root.list.push_back(ResourceTree());
            root.list.back().m_type = t;
            root.list.back().is_leaf = false;
            root.list.back().m_name = node.attribute("name").value();
            ResourceTree& rt = root.list.back();
            read_resource_tree(rt, node, t);
        }

        // actual resource, e.g. <object>
        if (node.name() == std::string(RESOURCE_TYPE_NAMES[t])) {
            //! add m_resourceTable entry
            root.list.push_back(ResourceTree());
            root.list.back().m_type = t;
            std::string value = remove_suffix(node.text().get(), ".gml");
            value = remove_suffix(value, ".shader");
            bool case_lookup = false;
            std::string path;
            // FIXME constant special casing is awful.
            if (t != CONSTANT)
            {
                // find path to resource
                path = case_insensitive_native_path(this->m_root, value + RESOURCE_EXTENSION[t], &case_lookup);
                if (!path_exists(path)) throw MiscError("Resource path not found: " + path);
                if (case_lookup) std::cout << "WARNING: path \""<< this->m_root << value << RESOURCE_EXTENSION[t] << "\" required case-insensitive lookup\n  Became \"" << path << "\"\n  This is time-consuming and should be corrected.\n";
            }
            ResourceTableEntry rte(t, path.c_str(), path_leaf(value).c_str());
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
                // FIXME why is this needed?

                // determine resource name:
                name = path_leaf(value);
            }

            if (m_ignored_assets.find(name) != m_ignored_assets.end())
            {
                // ignore this asset.
                root.list.pop_back();
                continue;
            }

            // insert resource table entry
            m_resourceTable.insert(std::make_pair(name, std::move(rte)));
            root.list.back().rtkey = name;
            root.list.back().is_leaf = true;
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

            for (pugi::xml_node arg : file.child("args").children("arg"))
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
            ResourceTableEntry rte(ResourceType::SCRIPT, (value + ".gml").c_str(), name.c_str());

            // insert resource table entry
            m_resourceTable.insert(std::make_pair(name, std::move(rte)));
            m_resourceTree.list[SCRIPT].list.push_back(ResourceTree());
            m_resourceTree.list[SCRIPT].list.back().m_type = ResourceType::SCRIPT;
            m_resourceTree.list[SCRIPT].list.back().rtkey = name;
            m_resourceTree.list[SCRIPT].list.back().is_leaf = true;
            m_resourceTree.list[SCRIPT].list.back().is_hidden = true;
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
                    ss_init_code << ", " << rettype_name[fn.m_ret_type];
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

            // add as a transient (internal) source
            std::string name = "extension^" + bc_name;
            ResourceTableEntry rte(ResourceType::SCRIPT, ("::transient::" + std::to_string(m_transient_files.size())).c_str(), name.c_str());
            m_resourceTable.insert(std::make_pair(name, std::move(rte)));
            m_resourceTree.list[SCRIPT].list.push_back(ResourceTree());
            m_resourceTree.list[SCRIPT].list.back().m_type = ResourceType::SCRIPT;
            m_resourceTree.list[SCRIPT].list.back().rtkey = name;
            m_resourceTree.list[SCRIPT].list.back().is_leaf = true;
            m_resourceTree.list[SCRIPT].list.back().is_hidden = true;
            m_transient_files.push_back(ss_init_code.str());
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
    ResourceConstant* res = new ResourceConstant();
    res->m_value = value;
    ResourceTableEntry rte(ResourceType::CONSTANT, res);
    res->m_name = name;

    // insert/replace resource table entry in tree.
    auto prev_iter = m_resourceTable.find(name);
    if (prev_iter != m_resourceTable.end())
    {
        delete prev_iter->second.get();
        m_resourceTable.erase(prev_iter);
    }
    m_resourceTable.insert(std::make_pair(name, std::move(rte)));
    m_resourceTree.list[CONSTANT].list.push_back(ResourceTree());
    m_resourceTree.list[CONSTANT].list.back().m_type = ResourceType::CONSTANT;
    m_resourceTree.list[CONSTANT].list.back().rtkey = name;
    m_resourceTree.list[CONSTANT].list.back().is_leaf = true;
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
    void join()
    { }
}
#endif

void Project::compile(bytecode::ProjectAccumulator& accumulator)
{
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    // skip 0, which is the special entrypoint.
    accumulator.next_bytecode_index();

    accumulator.m_project_base_directory = m_root;
    accumulator.m_included_directory = m_root + "datafiles" + PATH_SEPARATOR;

    std::cout << "Constants."<< std::endl;
    precompile_asset<ResourceConstant>(accumulator, m_resourceTree.list[CONSTANT]);

    // load files
    std::cout << "Sprites (load)." << std::endl;
    load_file_asset<ResourceSprite>(m_resourceTree.list[SPRITE]);
    std::cout << "Backgrounds (load)." << std::endl;
    load_file_asset<ResourceBackground>(m_resourceTree.list[BACKGROUND]);
    std::cout << "Scripts (load)." << std::endl;
    load_file_asset<ResourceScript>(m_resourceTree.list[SCRIPT]);
    std::cout << "Objects (load)." << std::endl;
    load_file_asset<ResourceObject>(m_resourceTree.list[OBJECT]);
    std::cout << "Rooms (load)." << std::endl;
    load_file_asset<ResourceRoom>(m_resourceTree.list[ROOM]);
    std::cout << "Shaders (load)." << std::endl;
    load_file_asset<ResourceShader>(m_resourceTree.list[SHADER]);
    std::cout << "Fonts (load)." << std::endl;
    load_file_asset<ResourceFont>(m_resourceTree.list[FONT]);
    std::cout << "Sounds (load)." << std::endl;
    load_file_asset<ResourceSound>(m_resourceTree.list[SOUND]);

    join(); // paranoia

    // parsing
    std::cout << "Fonts (parse)."<< std::endl;
    parse_asset<ResourceFont>(accumulator, m_resourceTree.list[FONT]);
    std::cout << "Scripts (parse)."<< std::endl;
    parse_asset<ResourceScript>(accumulator, m_resourceTree.list[SCRIPT]);
    join();
    std::cout << "Objects (parse)."<< std::endl;
    parse_asset<ResourceObject>(accumulator, m_resourceTree.list[OBJECT]);
    join();
    std::cout << "Rooms (parse)."<< std::endl;
    parse_asset<ResourceRoom>(accumulator, m_resourceTree.list[ROOM]);
    join();

    std::cout << "Built-in events."<< std::endl;
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
    std::cout << "Assigning asset IDs." << std::endl;
    assign_ids<ResourceObject>(accumulator, m_resourceTree.list[OBJECT]);
    // TODO: assign other asset IDs here.

    // build assets and take note of code which has global effects like enums, globalvar, etc.
    std::cout << "Sprites."<< std::endl;
    precompile_asset<ResourceSprite>(accumulator, m_resourceTree.list[SPRITE]);
    std::cout << "Backgrounds."<< std::endl;
    precompile_asset<ResourceBackground>(accumulator, m_resourceTree.list[BACKGROUND]);
    std::cout << "Fonts (accumulate)."<< std::endl;
    precompile_asset<ResourceFont>(accumulator, m_resourceTree.list[FONT]);
    std::cout << "Sounds."<< std::endl;
    precompile_asset<ResourceSound>(accumulator, m_resourceTree.list[SOUND]);
    std::cout << "Scripts (accumulate)."<< std::endl;
    precompile_asset<ResourceScript>(accumulator, m_resourceTree.list[SCRIPT]);
    std::cout << "Objects (accumulate)."<< std::endl;
    precompile_asset<ResourceObject>(accumulator, m_resourceTree.list[OBJECT]);
    std::cout << "Rooms (accumulate)."<< std::endl;
    precompile_asset<ResourceRoom>(accumulator, m_resourceTree.list[ROOM]);
    std::cout << "Shaders (accumulate)."<< std::endl;
    precompile_asset<ResourceShader>(accumulator, m_resourceTree.list[SHADER]);
    std::cout << "Sounds (accumulate)."<< std::endl;
    precompile_asset<ResourceSound>(accumulator, m_resourceTree.list[SOUND]);

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
            0
        );
        ogm_assert(index == 0);
        ogm_ast_free(entrypoint_ast);
    }

    join(); // paranoia

    // compile code
    std::cout << "Sprites (load data).\n";
    compile_asset<ResourceSprite>(accumulator, m_resourceTree.list[SPRITE]);
    std::cout << "Scripts (compile).\n";
    compile_asset<ResourceScript>(accumulator, m_resourceTree.list[SCRIPT]);
    join();
    std::cout << "Objects (compile).\n";
    compile_asset<ResourceObject>(accumulator, m_resourceTree.list[OBJECT]);
    join();
    std::cout << "Rooms (compile).\n";
    compile_asset<ResourceRoom>(accumulator, m_resourceTree.list[ROOM]);
    join();
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();
    std::cout << "Build complete (" << duration << " ms)" << std::endl;
}

template<typename ResourceType>
void Project::load_file_asset(ResourceTree& tree)
{
    if (!tree.is_leaf)
    {
        for (auto iter : tree.list)
        {
            load_file_asset<ResourceType>(iter);
        }
    }
    else
    {
        auto* asset = m_resourceTable[tree.rtkey].get(&m_transient_files);
        if (m_verbose)
        {
            std::cout << "loading files for " << asset->get_name() << "\n";
        }
        asset->load_file();
    }
}

template<typename ResourceType>
void Project::parse_asset(const bytecode::ProjectAccumulator& acc, ResourceTree& tree)
{
    if (!tree.is_leaf)
    {
        for (auto iter : tree.list)
        {
            parse_asset<ResourceType>(acc, iter);
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
                    a->parse(acc);
                    if (m_verbose)
                    {
                        std::cout << "parsing " << a->get_name() << "\n";
                    }
                },
                m_resourceTable[tree.rtkey].get(&m_transient_files)
            )
        );
        #else
        Resource* a = m_resourceTable[tree.rtkey].get(&m_transient_files);
        if (m_verbose)
        {
            std::cout << "parsing " << a->get_name() << "\n";
        }
        a->parse(acc);
        #endif
    }
}

template<typename ResourceType>
void Project::assign_ids(bytecode::ProjectAccumulator& accumulator, ResourceTree& tree)
{
    // single-threaded due to the global accumulation, including
    // - assignment of asset indices
    // - macros and globalvar declarations

    if (!tree.is_leaf)
    {
        for (auto iter : tree.list)
        {
            assign_ids<ResourceType>(accumulator, iter);
        }
    }
    else
    {
        ResourceType* rt = dynamic_cast<ResourceType*>(m_resourceTable[tree.rtkey].get(&m_transient_files));
        if (m_verbose)
        {
            std::cout << "assiging id for " << rt->get_name() << "\n";
        }
        rt->assign_id(accumulator);
    }
}

template<typename ResourceType>
void Project::precompile_asset(bytecode::ProjectAccumulator& accumulator, ResourceTree& tree)
{
    // single-threaded due to the global accumulation, including
    // - assignment of asset indices
    // - macros and globalvar declarations

    if (!tree.is_leaf)
    {
        for (auto iter : tree.list)
        {
            precompile_asset<ResourceType>(accumulator, iter);
        }
    }
    else
    {
        ResourceType* r = dynamic_cast<ResourceType*>(m_resourceTable[tree.rtkey].get(&m_transient_files));
        if (m_verbose)
        {
            std::cout << "precompiling " << r->get_name() << "\n";
        }
        r->precompile(accumulator);
    }
}

template<typename ResourceType>
void Project::compile_asset(bytecode::ProjectAccumulator& accumulator, ResourceTree& tree)
{
    if (!tree.is_leaf) {
        for (auto& iter : tree.list) {
            compile_asset<ResourceType>(accumulator, iter);
        }
    } else {
        #ifdef PARALLEL_COMPILE
        // compile for subtree asynchronously.
        g_jobs.push_back(
            std::async(std::launch::async,
                [](ResourceType* a, bytecode::ProjectAccumulator& accumulator)
                {
                    if (m_verbose)
                    {
                        std::cout << "compiling " << a->get_name() << "\n";
                    }
                    a->compile(accumulator);
                },
                dynamic_cast<ResourceType*>(m_resourceTable[tree.rtkey].get()),
                std::ref(accumulator)
            )
        );
        #else
        ResourceType* r = dynamic_cast<ResourceType*>(m_resourceTable[tree.rtkey].get(&m_transient_files));

        if (m_verbose)
        {
            std::cout << "compiling " << r->get_name() << "\n";
        }
        r->compile(accumulator);
        #endif
    }
}

void ResourceConstant::precompile(bytecode::ProjectAccumulator& acc)
{
    set_macro(m_name.c_str(), m_value.c_str(), *acc.m_reflection);
}
}}
