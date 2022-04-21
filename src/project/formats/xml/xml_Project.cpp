#include "ogm/project/Project.hpp"
#include "project/ProjectResources.hpp"
#include "ogm/project/resource/ResourceScript.hpp"

#include "project/XMLError.hpp"

#include <pugixml.hpp>

namespace ogm::project{

extern const char* RESOURCE_EXTENSION[];

void Project::process_xml()
{
    std::string file_name = path_join(m_root, m_project_file);
    
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(file_name.c_str());

    std::cout << "reading project file " << file_name << std::endl;

    check_xml_result<ProjectError>(ErrorCode::F::filexml, result, file_name.c_str(), "Error parsing .project.gmx file: " + file_name);

    pugi::xml_node assets = doc.child("assets");

    for (int r = 0; r < NONE; r++)
    {
        int prev_rte_size = m_resources.size();
        ResourceType r_type = (ResourceType)r;
        read_resource_tree_xml(&asset_tree(r_type), assets, r_type);

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
                
                if (!path_exists(path)) throw ProjectError(ErrorCode::F::file, "Resource path not found: \"{}\"", path);
                
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
}