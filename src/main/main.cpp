#include <fstream>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>

#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"
#include "ogm/bytecode/bytecode.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/Debugger.hpp"
#include "ogm/interpreter/StandardLibrary.hpp"
#include "ogm/interpreter/Instance.hpp"
#include "ogm/interpreter/Garbage.hpp"
#include "ogm/asset/AssetTable.hpp"
#include "ogm/project/Project.hpp"

#include "unzip.hpp"

#ifdef IMGUI
#include "ogm/gui/editor.hpp"
#endif

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#if __has_include("src/common/license.inc")
#include "src/common/license.inc"
#define OGM_LICENSE_AVAILABLE
#endif

// for popup
#include "interpreter/library/library.h"

#include <boost/program_options.hpp>

#ifndef VERSION
    #define VERSION OpenGML (development build)
#endif

#define _STR(A) #A
#define STR(A) _STR(A)
#define VERSION_STR STR(VERSION)

using namespace std;
using namespace ogm;
namespace po = boost::program_options;

// determine how to handle a file given only its extension.
struct filetype_info_t
{
    bool project = false; // is this a project file?
    bool script = false;  // is this a lone script?
    bool unzip = false;   // do we need to unzip?
};
filetype_info_t analyze_extension (const std::string& filename)
{
    if (ends_with(filename, ".gml"))
    {
        return {false, true, false};
    }
    else if (ends_with(filename, ".project.gmx") || ends_with(filename, ".project.arf") || ends_with(filename, ".project.ogm") || ends_with(filename, ".yyp"))
    {
        return {true, false, false};
    }
    else if (ends_with(filename, ".gmz") || ends_with(filename, ".7z") || ends_with(filename, "zip") || ends_with(filename, ".yyz"))
    {
        return {true, false, true};
    }
    return {};
}

// allows boost to parse -D string pairs as x=y
namespace std
{
    static std::istream& operator>>(istream& in, std::pair<std::string, std::string>& v)
    {
        std::stringstream ss;
        while (!in.eof() && in.peek() != '=' && !isspace(in.peek()))
        {
            char c;
            in >> c;
            ss << c;
        }
        v.first = ss.str();
        if (in.peek() == '=')
        {
            in.ignore(1);
            in >> v.second;
        }
        else
        {
            // defualt define is 'true' if none provided.
            v.second = "true";
        }

        return in;
    }
}

// parser for -D arguments
static pair<string, string> parse_defines(const string& s)
{
    if (s.find("-D") == 0 && s.length() > 2)
    {
        return { "-D", s.substr(2) };
    } else {
        return {};
    }
}

// parses command line args to find filename, and returns its index in args.
size_t get_filename_index(const po::options_description& desc, const po::positional_options_description& p, int argc, char const* const* argv)
{
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
          options(desc).extra_parser(parse_defines).positional(p)
          .allow_unregistered().run(), vm);
    po::notify(vm);

    if (vm.count("input-file"))
    {
        std::string filename = vm["input-file"].as<std::string>();
        for (size_t i = 0; i < argc; ++i)
        {
            if (filename.compare(argv[i]) == 0)
            {
                return i;
            }
        }
    }

    return std::numeric_limits<size_t>::max();
}

int _main(int argc, char const* const* argv)
{
    #if defined(EMSCRIPTEN)
    // substitute in dummy argments if none provided
    char const* const[] emscripten_dummy_argv {
        argv[0],
        "./demo/projects/example/example.project.gmx"
    };
    if (argc <= 1)
    {
        argc = 2;
        argv = emscripten_dummy_argv;
    }
    #endif

    #define DIS_EX " Disables execution by default."

    // non-positional arguments
    po::options_description desc(VERSION_STR " command-line options");
    desc.add_options()
        ("help", "Produce help message." DIS_EX)
        ("version", "Show version information." DIS_EX)
        ("show-license", "Shows license information." DIS_EX)
        ("popup", "Show UI popup to select project file." DIS_EX)

        ("ast,tree", "Display AST (for debugging)." DIS_EX)
        ("dis,disassemble", "Displays bytecode disassembly." DIS_EX)
        ("dis-raw,disassemble-raw", "Raw disassembly." DIS_EX)
        ("dis-source-inline", "Displays source inline in bytecode disassembly.")

        ("input-file", po::value<std::string>(), "the gml script or project file")
        ("exec,execute", "Executes provided code after building (enabled by default).")
        ("debug", "Enables debug mode. Implies --exec.")
        ("rdebug", "Same as --debug, but without pause at start.")
        ("ex", po::value<std::vector<std::string>>()->composing(), "run debug command.")
        ("verbose", "Show additional information during build.")
        ("D", po::value<std::vector<std::pair<std::string, std::string>>>()->composing(), "define symbols")

        ("cache", "cache build")
        ("single-thread,single-threaded", "Single threaded build process.")
        ("strip", "Disables debug symbols and reflection.")
        ("garbage-disabled", "Disable garbage collector. May cause memory leak.")
        ("trace-enabled", "Allow tracing code execution.")
        ("mute", "disable sound")
    ;

    // positional options
    po::positional_options_description p;
    p.add("input-file", -1);

    // parse clargs
    po::variables_map vm;
    size_t filename_index = get_filename_index(desc, p, argc, argv);
    po::store(po::command_line_parser(std::min<size_t>(filename_index + 1, argc), argv).
          options(desc).extra_parser(parse_defines).positional(p).run(), vm);
    po::notify(vm);

    // show popup by default if running not from terminal and without args
    // (also show if specifically requested via the --popup arg)
    const bool show_popup =
        ((argc <= 1) && !is_terminal())
        || vm.count("popup");
    
    // debug the provided code?
    const bool do_debug = vm.count("debug") || vm.count("rdebug");

    // run the code provided?
    // (disable by default if we are just disassembling it.)
    // (enable if we are executing or debugging [including if selecting code by popup])
    const bool do_execute = 
        (vm.count("exec") || do_debug || show_popup)
        || !(vm.count("ast") || vm.count("dis") || vm.count("dis-raw"));

    // compile the code provided?
    const bool do_compile = do_execute || vm.count("dis") || vm.count("dis-raw");

    // user definitions
    const auto defines = vm.count("D") ? vm["D"].as<std::vector<std::pair<std::string, std::string>>>() : std::vector<std::pair<std::string, std::string>>();

    std::string filename = vm.count("input-file") ? vm["input-file"].as<std::string>() : "";

    if (vm.count("show-license"))
    {
        #ifdef OGM_LICENSE_AVAILABLE
            // (from auto-generated license.inc)
            std::cout << _ogm_license_ << std::endl;
            return 0;
        #else
            std::cout << "OpenGML was not compiled with license information available. If this is a public release, then this is an error and should be reported immediately.\n";
            return 1;
        #endif
    }

    if (vm.count("version"))
    {
        std::cout << VERSION_STR << std::endl;
        return 0;
    }

    if (show_popup)
    {
        if (!is_terminal())
        {
            std::cout << "Please run from a console for more options." << std::endl;
            sleep(500);
        }

        // FIXME -- is using the interpreter's get_open_filename function really the best way to do this..?
        std::cout << "Opening popup window..." << std::endl;
        ogm::interpreter::Variable filter = "project file|*.project.gmx;*.project.ogm;*.yyp";
        ogm::interpreter::Variable fname = "";
        ogm::interpreter::Variable selected;

        // select file from dialogue.
        ogm::interpreter::fn::get_open_filename(selected, filter, fname);

        // set options
        filename = selected.castCoerce<std::string>();

        // cleanup
        filter.cleanup();
        fname.cleanup();
        selected.cleanup();

        if (filename == "")
        {
            std::cout << "No file was selected." << std::endl;
            sleep(2000);
            return 0;
        }
        else
        {
            std::cout << "Selected file " << filename << std::endl;
        }
    }
    else
    {
        if (!vm.count("input-file"))
        {
            std::cout << desc;
            return 1;
        }
        else
        {
            filename = vm["input-file"].as<std::string>();
        }
    }

    ogm::interpreter::staticExecutor.m_frame.m_config.m_cache = vm.count("cache");
    ogm::interpreter::staticExecutor.m_frame.m_config.m_parallel_compile = !vm.count("single-thread");

    #ifdef EMSCRIPTEN
    // (to help debug emscripten builds)
    if (!can_read_file(filename))
    {
        std::cout << "Cannot open file with fopen(): " << filename << std::endl;
        return 1;
    }
    #endif

    // open input file.
    // determine how to handle file type given extension.
    const auto filetype = analyze_extension(filename);

    if (!filetype.project && !filetype.script)
    {
        std::cout << "unrecognized extension on path \"" << filename << "\"" << std::endl;
        return 1;
    }

    if (filetype.unzip)
    {
        #ifdef LIBZIP_ENABLED
        std::string extract = create_temp_directory();
        if (!ogm::unzip(filename, extract))
        {
            std::cout << "Error extracting project." << std::endl;
            return (1);
        }

        std::cout << "Extracted archive successfully." << std::endl;

        // change filename to enclosed .project.gmx
        std::string project_name = path_leaf(remove_extension(filename));

        filename = extract + project_name + ".gmx";
        if (!path_exists(filename))
        {
            // identify project file in archive.
            std::vector<std::string> matches;
            list_paths(extract, matches);
            bool found = false;
            for (const std::string& match : matches)
            {
                if (ends_with(match, ".project.gmx"))
                {
                    filename = match;
                    found = true;
                }
            }
            if (!found)
            {
                std::cout << "Unable to find .project.gmx in archive.\n";
                return (1);
            }

            std::cout << "Project file is " << filename << std::endl;
        }
        #else
        std::cout << "zlib support not enabled; cannot extract project." << std::endl;
        std::cout << "(You can try extracting it manually, however; then try opengml again.)" << std::endl;
        return 1;
        #endif
    }

    using namespace ogm::bytecode;
    ReflectionAccumulator reflection;
    ogm::interpreter::standardLibrary->reflection_add_instance_variables(reflection);
    ogm::interpreter::staticExecutor.m_frame.m_reflection = &reflection;
    ogm::bytecode::BytecodeTable& bytecode = ogm::interpreter::staticExecutor.m_frame.m_bytecode;

    // reserve bytecode slots. (It can expand if needed.)
    bytecode.reserve(4096);

    ogm::interpreter::staticExecutor.m_frame.m_data.m_sound_enabled = !vm.count("mute");

    if (filetype.project)
    {
        // parse/build/run just an entire project.
        ogm::project::Project project(filename.c_str());
        project.m_verbose = vm.count("verbose");

        // Ignore assets with name matching a define.
        // (this is a cheap hack to allow skipping certain assets that aren't
        // opengml compatible by manually defining them out on the command line.)
        for (auto& [name, value] : defines)
        {
            project.ignore_asset(name);
        }

        project.process();

        // set command-line definitions
        for (auto& [name, value] : defines)
        {
            project.add_constant(name, value);
        }

        if (do_compile)
        {
            if (vm.count("verbose")) std::cout << "Compiling..." << std::endl;
            ogm::interpreter::staticExecutor.m_library = ogm::interpreter::standardLibrary;
            ogm::bytecode::ProjectAccumulator acc = ogm::interpreter::staticExecutor.create_project_accumulator();
            if (!project.build(acc))
            {
                return 7;
            }
            ogm::interpreter::staticExecutor.m_frame.m_fs.set_included_directory(acc.m_included_directory);
            if (vm.count("verbose")) std::cout << "Build complete." << std::endl;
        }
    }
    else if (filetype.script)
    {
        // parse/build/run just a single script.
        std::string fileContents = read_file_contents(filename);

        ogm_ast_t* ast = ogm_ast_parse(fileContents.c_str());
        if (!ast)
        {
            // TODO: give more information about the error.
            std::cout << "An error occurred while parsing the code.";
            return 1;
        }

        if (vm.count("ast"))
        {
            ogm_ast_tree_print(ast);
        }

        if (vm.count("verbose")) std::cout << "Compiling..." << std::endl;

        ogm::interpreter::staticExecutor.m_library = ogm::interpreter::standardLibrary;
        ogm::bytecode::ProjectAccumulator acc = ogm::interpreter::staticExecutor.create_project_accumulator();
        DecoratedAST dast{ast, filename.c_str(), fileContents.c_str()};
        ogm::bytecode::bytecode_preprocess(dast, reflection);

        // set command-line definitions (-Dabc=xyz)
        for (auto& [name, value] : defines)
        {
            try
            {
                ogm_ast_t* defn_ast = ogm_ast_parse(value.c_str(), ogm_ast_parse_flag_expression);
            }
            catch (const std::exception& e)
            {
                std::cout << "Error in definition \"" << name << "\": " << e.what();
                return (2);
            }
        }

        bytecode_index_t index = ogm::bytecode::bytecode_generate(dast, acc, nullptr, acc.next_bytecode_index());
        ogm_assert(index == 0);

        if (vm.count("verbose")) std::cout << "Compile complete." << std::endl;
    }
    else
    {
        ogm_assert(false);
    }

    if (do_compile)
    {
        if (vm.count("strip"))
        {
            bytecode.strip();
        }

        if (vm.count("dis") || vm.count("dis-raw"))
        {
            // disassemble all bytecode sections.
            for (size_t i = 0; i < bytecode.count(); ++i)
            {
                const Bytecode& bytecode_section = bytecode.get_bytecode(i);

                std::cout << "\n";
                if (bytecode_section.m_debug_symbols && bytecode_section.m_debug_symbols->m_name.length())
                {
                    std::cout << bytecode_section.m_debug_symbols->m_name;
                }
                else
                {
                    if (i == 0)
                    {
                        std::cout << "Entry Point:";
                    }
                    else
                    {
                        std::cout << "<anonymous section " << i << ">";
                    }
                }
                std::cout << " ";
                if (bytecode_section.m_argc == static_cast<uint8_t>(-1))
                {
                    std::cout << "*";
                }
                else
                {
                    std::cout << (int32_t)bytecode_section.m_argc;
                }
                std::cout << " -> " << (int32_t)bytecode_section.m_retc << std::endl;
                ogm::bytecode::bytecode_dis(bytecode_section, cout, ogm::interpreter::standardLibrary, vm.count("dis-raw") ? nullptr : &reflection, vm.count("dis-source-inline"));
            }
        }

        if (do_execute)
        {
            if (vm.count("verbose")) std::cout << "Executing..." << std::endl;
            
            // set gc enabled
            #ifdef OGM_GARBAGE_COLLECTOR
            ogm::interpreter::g_gc.set_enabled(!vm.count("garbage-disabled"));
            #endif
            
            // TODO: get rid of "anonymous" instance, replace with global instance.
            ogm::interpreter::Instance anonymous;
            anonymous.m_data.m_frame_owner = &ogm::interpreter::staticExecutor.m_frame;
            ogm::interpreter::staticExecutor.m_self = &anonymous;
            auto& parameters = ogm::interpreter::staticExecutor.m_frame.m_data.m_clargs;
            for (size_t i = filename_index; i < argc; ++i)
            {
                parameters.push_back(argv[i]);
            }
            ogm::interpreter::Debugger debugger;
            debugger.m_config.m_trace_permitted = vm.count("trace-enabled");
            if (do_debug)
            {
                ogm::interpreter::staticExecutor.debugger_attach(&debugger);

                if (!vm.count("rdebug"))
                {
                    debugger.break_execution();
                }

                if (vm.count("ex"))
                {
                    for (const std::string& debug_arg : vm["ex"].as<std::vector<std::string>>())
                    {
                        debugger.queue_command(debug_arg);
                    }
                }
            }

            ogm_assert(
                ogm::interpreter::staticExecutor.m_frame.m_bytecode.has_bytecode(0)
            );

            try
            {
                #ifndef EMSCRIPTEN
                // execute first bytecode
                if (ogm::interpreter::execute_bytecode(0))
                {
                    // if execution suspended (does not generally happen),
                    // repeat until properly terminates.
                    while (ogm::interpreter::execute_resume());
                }
                #else
                // emscripten requires execution to suspend every frame.
                if (ogm::interpreter::execute_bytecode(0))
                {
                    emscripten_set_main_loop([]()
                    {
                        static bool first_loop = true;
                        if (first_loop)
                        // already performed one frame; skip.
                        {
                            first_loop = false;
                        }

                        // resume execution for one frame.
                        if (!ogm::interpreter::execute_resume())
                        {
                            // properly halted; quit.
                            emscripten_cancel_main_loop();
                        }
                    }, 0, true);
                }
                #endif

                if (vm.count("verbose")) std::cout << "Exection completed normally." << std::endl;
            }
            catch (const ogm::interpreter::ExceptionTrace& e)
            {
                std::cout << "\nThe program aborted due to a fatal error.\n";
                std::cout << e.what() << std::endl;
            }
        }
    }

  return 0;
}

int main (int argc, char** argv)
{
    if (is_terminal())
    {
        enable_terminal_colours();
    }
    
    try
    {
        _main(argc, argv);
    }
    catch (std::exception& e)
    {
        std::string exception_name = pretty_typeid(typeid(e).name());
        std::cout << std::string("An unhandled exception `") + pretty_typeid(exception_name) + "` was thrown:\n" + e.what() + "\n";
    }
    
    // cleanup.
    restore_terminal_colours();
}

#ifdef OGM_MINIZIP_NOOPT
extern "C"
{
    extern volatile uint8_t Write_Zip64EndOfCentralDirectoryRecord;
    static volatile uint8_t _w=Write_Zip64EndOfCentralDirectoryRecord;
}
#endif