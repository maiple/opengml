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
#include "ogm/asset/AssetTable.hpp"
#include "ogm/project/Project.hpp"

#include "unzip.hpp"

#ifdef IMGUI
#include "ogm/gui/editor.hpp"
#endif

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

// for popup
#include "interpreter/library/library.h"

#ifndef NDEBUG
    #define VERSION_ADDENDUM "* dev"
#else
    #define VERSION_ADDENDUM ""
#endif

#define VERSION "OpenGML v0.7.8 (alpha)" VERSION_ADDENDUM

using namespace std;
using namespace ogm;

int umain (int argn, char** argv)
{
    #if defined(EMSCRIPTEN)
    // substitute in dummy argments
    if (argn <= 1)
    {
        argn = 2;
        const char** _argv = new const char*[2];
        _argv[0] = argv[0];
        _argv[1] = "./demo/projects/example/example.project.gmx";
        argv = const_cast<char**>(_argv);
    }
    #endif

// FIXME: indentation

  int32_t filename_index = -1;
  std::string filename = "in.gml";
  std::vector<std::pair<std::string, std::string>> defines;
  std::vector<std::string> debug_args;
  bool
    default_execute = true,
    show_ast = false,
    single_thread_compile = false,
    dis = false,
    dis_raw = false,
    execute = false,
    strip = false,
    lines = false,
    debug = false,
    allow_trace = false,
    rundebug = false,
    version=!is_terminal(),
    compile=false,
    verbose=false,
    gui=false,
    popup=(argn <= 1) && !is_terminal(),
    sound=true,
    unzip_project=false,
    cache=false;
  for (int i=1;i<argn;i++) {
    char* arg = argv[i];
    size_t dashc = 0;
    while (strncmp(arg, "-", 1) == 0)
    {
        arg++;
        dashc++;
    }
    if (dashc == 1)
    {
        if (starts_with(arg, "D"))
        {
            // define constant
            const char* pos = strchr(arg, '=');
            if (pos == arg || pos == arg + 1)
            {
                std::cout << "Definition malformed: " << arg << std::endl;
                std::exit(2);
            }
            else if (pos == 0)
            {
                defines.emplace_back(arg + 1, "true");
            }
            else
            {
                defines.emplace_back(
                    std::pair<std::string, std::string>{
                        {arg + 1, static_cast<uint16_t>(pos - arg - 1)},
                        pos + 1
                    }
                );
            }
        }
        continue;
    }
    if (dashc == 2)
    {
      if (strcmp(arg,"ast") == 0 || strcmp(arg, "tree") == 0) {
          show_ast = true;
          default_execute = false;
      }
      else if (strcmp(arg,"dis") == 0) {
          dis = true;
          default_execute = false;
      }
      else if (strcmp(arg,"raw") == 0) {
          dis_raw = true;
          default_execute = false;
      }
      else if (strcmp(arg,"exec") == 0 || strcmp(arg,"execute") == 0) {
          execute = true;
          default_execute = false;
      }
      else if (strcmp(arg,"strip") == 0) {
          strip = true;
      }
      else if (strcmp(arg,"source-inline") == 0) {
          lines = true;
          default_execute = false;
      }
      else if (strcmp(arg,"debug") == 0) {
          debug = true;
          execute = true;
          default_execute = false;
      }
      else if (strcmp(arg,"rdebug") == 0) {
          debug = true;
          execute = true;
          rundebug = true;
          default_execute = false;
      }
      else if (strcmp(arg, "trace-enabled") == 0)
      {
          allow_trace = true;
          continue;
      }
      else if (strcmp(arg, "single-thread") == 0)
      {
          single_thread_compile = true;
          continue;
      }
      else if (strcmp(arg,"version") == 0) {
          version = true;
          default_execute = false;
      }
      else if (strcmp(arg,"compile") == 0) {
          compile = true;
          default_execute = false;
      }
      else if (strcmp(arg,"verbose") == 0) {
          verbose = true;
          continue;
      }
      else if (strcmp(arg,"gui") == 0) {
          gui = true;
          default_execute = false;
      }
      else if (strcmp(arg,"mute") == 0) {
          sound = false;
          continue;
      }
      else if (strcmp(arg,"mute") == 0) {
          sound = false;
          continue;
      }
      else if (strcmp(arg,"cache") == 0) {
          cache = true;
          continue;
      }
      else if (starts_with(arg, "ex="))
      {
          debug_args.push_back(arg + 3);
          continue;
      }
      else if (strcmp(arg, "popup") == 0)
      {
          popup=true;
          default_execute = false;
      }
    }
    if (dashc == 0)
    {
        // project file name
        filename_index = i;
        if (default_execute)
        {
            execute = true;
        }
        break;
    }
  }

  if (version)
  {
      std::cout << VERSION << std::endl;
      if (!popup)
      {
          exit(0);
      }
  }

  if (popup)
  {
      if (!is_terminal())
      {
          std::cout << "Please run from a console for more options." << std::endl;
          sleep(500);
      }
      std::cout << "Opening popup window..." << std::endl;
      ogm::interpreter::Variable filter = "project file|*.project.gmx;*.project.ogm;*.yyp";
      ogm::interpreter::Variable fname = "";
      ogm::interpreter::Variable selected;

      // select file from dialogue.
      ogm::interpreter::fn::get_open_filename(selected, filter, fname);

      // set options
      filename = selected.castCoerce<std::string>();
      filename_index = argn;
      compile = true;
      execute = true;

      if (filename == "")
      {
          std::cout << "No file was selected." << std::endl;
          sleep(2000);
      }
      else
      {
          std::cout << "Selected file " << filename << std::endl;
      }

      // cleanup
      filter.cleanup();
      fname.cleanup();
      selected.cleanup();
  }
  else
  {
      if (filename_index == -1)
      {
          std::cout << "Basic usage: " << argv[0] << " [--execute] [--dis] [--ast] [--gui] [--debug] [--rdebug] [--compile] [--single-thread] [--verbose] [--cache] file [parameters...]" << std::endl;
          exit(0);
      }
      else
      {
          filename = argv[filename_index];
      }
  }

  if (gui && compile)
  {
      std::cout << "build and gui mode are mutually exclusive." << std::endl;
      exit(1);
  }

  ogm::interpreter::staticExecutor.m_frame.m_config.m_cache = cache;
  ogm::interpreter::staticExecutor.m_frame.m_config.m_parallel_compile = !single_thread_compile;

  #ifdef EMSCRIPTEN
  if (!can_read_file(filename))
  {
      std::cout << "Cannot open file with fopen(): " << filename << std::endl;
      exit(1);
  }
  #endif

  ifstream inFile;

  inFile.open(filename);
  if (!inFile)
  {
      std::cout << "Could not open file " << filename << std::endl;
      exit(1);
  }
  else
  {
      bool process_project = false;
      bool process_gml = false;

      compile |= dis || execute;

      if (ends_with(filename, ".gml"))
      {
          process_gml = true;
          if (gui)
          {
              std::cout << "gui mode can only be used on projects." << std::endl;
              exit(1);
          }
      }
      else if (ends_with(filename, ".project.gmx") || ends_with(filename, ".project.arf") || ends_with(filename, ".project.ogm") || ends_with(filename, ".yyp"))
      {
          process_project = true;
      }
      else if (ends_with(filename, ".gmz") || ends_with(filename, ".7z") || ends_with(filename, "zip") || ends_with(filename, ".yyz"))
      {
          process_project = true;
          unzip_project = true;
      }

      if (unzip_project)
      {
          #ifdef LIBZIP_ENABLED
          std::string extract = create_temp_directory();
          if (!ogm::unzip(filename, extract))
          {
              std::cout << "Error extracting project." << std::endl;
              exit(1);
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
                  exit(1);
              }

              std::cout << "Project file is " << filename << std::endl;
          }
          #else
          std::cout << "zlib support not enabled; cannot extract project." << std::endl;
          std::cout << "(You can try extracting it yourself, however.)" << std::endl;
          exit(1);
          #endif
      }

      ogm::project::Project project(filename.c_str());

      if (verbose)
      {
          project.m_verbose = true;
      }

      using namespace ogm::bytecode;
      ReflectionAccumulator reflection;
      ogm::interpreter::standardLibrary->reflection_add_instance_variables(reflection);
      ogm::interpreter::staticExecutor.m_frame.m_reflection = &reflection;
      ogm::bytecode::BytecodeTable& bytecode = ogm::interpreter::staticExecutor.m_frame.m_bytecode;

      // reserve bytecode slots. (It can expand if needed.)
      bytecode.reserve(4096);

      ogm::interpreter::staticExecutor.m_frame.m_data.m_sound_enabled = sound;

      if (!process_project && !process_gml)
      {
          std::cout << "Cannot handle file type for " << filename << ".\n";
          std::cout << "Please supply a .gml or .project.gmx file.\n";
          inFile.close();
          exit(1);
      }

      if (process_project)
      {
          inFile.close();

          // ignore assets with name matching a define.
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

          if (compile)
          {
              if (verbose) std::cout << "Compiling..." << std::endl;
              ogm::bytecode::ProjectAccumulator acc{
                  ogm::interpreter::standardLibrary,
                  ogm::interpreter::staticExecutor.m_frame.m_reflection,
                  &ogm::interpreter::staticExecutor.m_frame.m_assets,
                  &ogm::interpreter::staticExecutor.m_frame.m_bytecode,
                  &ogm::interpreter::staticExecutor.m_frame.m_config
              };
              if (!project.build(acc))
              {
                  return 7;
              }
              ogm::interpreter::staticExecutor.m_frame.m_fs.set_included_directory(acc.m_included_directory);
              if (verbose) std::cout << "Build complete." << std::endl;
          }
      }
      else if (process_gml)
      {
          std::string fileContents = read_file_contents(inFile);

          ogm_ast_t* ast = ogm_ast_parse(fileContents.c_str());
          if (!ast)
          {
              std::cout << "An error occurred while parsing the code.";
              return true;
          }

          if (show_ast)
          {
              ogm_ast_tree_print(ast);
          }

          if (verbose) std::cout << "Compiling..." << std::endl;

          ogm::bytecode::ProjectAccumulator acc{ogm::interpreter::standardLibrary, ogm::interpreter::staticExecutor.m_frame.m_reflection, &ogm::interpreter::staticExecutor.m_frame.m_assets, &ogm::interpreter::staticExecutor.m_frame.m_bytecode, &ogm::interpreter::staticExecutor.m_frame.m_config};
          DecoratedAST dast{ast, filename.c_str(), fileContents.c_str()};
          ogm::bytecode::bytecode_preprocess(dast, reflection);

          // set command-line definitions
          for (auto& [name, value] : defines)
          {
              try
              {
                  ogm_ast_t* defn_ast = ogm_ast_parse_expression(value.c_str());
              }
              catch (const std::exception& e)
              {
                  std::cout << "Error in definition \"" << name << "\": " << e.what();
                  exit(2);
              }
          }

          bytecode_index_t index = ogm::bytecode::bytecode_generate(dast, acc, nullptr, acc.next_bytecode_index());
          ogm_assert(index == 0);

          if (verbose) std::cout << "Compile complete." << std::endl;
      }
      else
      {
          ogm_assert(false);
      }

      if (compile)
      {
          if (strip)
          {
              bytecode.strip();
          }

          if (dis)
          {
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
                  ogm::bytecode::bytecode_dis(bytecode_section, cout, ogm::interpreter::standardLibrary, dis_raw ? nullptr : &reflection, lines);
              }
          }

          if (execute)
          {
              if (verbose) std::cout << "Executing..." << std::endl;
              
              // TODO: get rid of "anonymous" instance, replace with global instance.
              ogm::interpreter::Instance anonymous;
              anonymous.m_data.m_frame_owner = &ogm::interpreter::staticExecutor.m_frame;
              ogm::interpreter::staticExecutor.m_library = ogm::interpreter::standardLibrary;
              ogm::interpreter::staticExecutor.m_self = &anonymous;
              auto& parameters = ogm::interpreter::staticExecutor.m_frame.m_data.m_clargs;
              for (size_t i = filename_index; i < argn; ++i)
              {
                  parameters.push_back(argv[i]);
              }
              ogm::interpreter::Debugger debugger;
              debugger.m_config.m_trace_permitted = allow_trace;
              if (debug)
              {
                  ogm::interpreter::staticExecutor.debugger_attach(&debugger);

                  if (!rundebug)
                  {
                      debugger.break_execution();
                  }

                  for (std::string& debug_arg : debug_args)
                  {
                      debugger.queue_command(std::move(debug_arg));
                  }
                  debug_args.clear();
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

                  if (verbose) std::cout << "Exection completed normally." << std::endl;
              }
              catch (const ogm::interpreter::ExceptionTrace& e)
              {
                  std::cout << "\nThe program aborted due to a fatal error.\n";
                  std::cout << e.what() << std::endl;
              }
          }
      }
      else if (gui)
      {
          #ifdef IMGUI
          ogm::gui::run(&project);
          #else
          std::cout << "OpenGML was not built with GUI support." << std::endl;
          exit(1);
          #endif
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
        umain(argc, argv);
    }
    catch (std::exception& e)
    {
        std::string exception_name = pretty_typeid(typeid(e).name());
        std::cout << std::string("An unhandled exception `") + pretty_typeid(exception_name) + "` was thrown:\n" + e.what() + "\n";
    }
    
    // cleanup.
    restore_terminal_colours();
}
