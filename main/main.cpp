#include <fstream>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>

#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"
#include "ogm/ast/ast_util.h"
#include "ogm/bytecode/bytecode.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/Debugger.hpp"
#include "ogm/interpreter/StandardLibrary.hpp"
#include "ogm/interpreter/Instance.hpp"
#include "ogm/asset/AssetTable.hpp"
#include "ogm/project/Project.hpp"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#define VERSION "OpenGML v0.6 (alpha)"

using namespace std;

int main (int argn, char** argv) {

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
  const char* filename = "in.gml";
  bool show_ast = false, dis = false, dis_raw = false, execute = false, strip = false, lines = false, debug = false, version=false;
  for (int i=1;i<argn;i++) {
    if (strncmp(argv[i],"--",2) == 0) {
      char* arg = (argv[i]+2);
      if (strcmp(arg,"ast") == 0 || strcmp(arg, "tree") == 0) {
        show_ast = true;
      }
      else if (strcmp(arg,"dis") == 0) {
        dis = true;
      }
      else if (strcmp(arg,"raw") == 0) {
        dis_raw = true;
      }
      else if (strcmp(arg,"exec") == 0 || strcmp(arg,"execute") == 0) {
        execute = true;
      }
      else if (strcmp(arg,"strip") == 0) {
        strip = true;
      }
      else if (strcmp(arg,"source-inline") == 0) {
        lines = true;
      }
      else if (strcmp(arg,"debug") == 0) {
        debug = true;
        execute = true;
      }
      else if (strcmp(arg,"version") == 0) {
        version = true;
      }
    } else {
      filename_index = i;
      if (i == 1)
      {
          execute = true;
      }
      break;
    }
  }

  if (version)
  {
      std::cout << VERSION << std::endl;
      exit(0);
  }

  if (filename_index == -1)
  {
      std::cout << "Basic usage: " << argv[0] << " [--execute] [--dis] [--ast] [--debug] file [parameters...]" << std::endl;
      exit(0);
  }
  else
  {
      filename = argv[filename_index];
  }

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
      ogm::project::Project project(filename);

      bool process_project = false;
      bool process_gml = false;

      bool compile = dis || execute;

      if (ends_with(filename, ".gml"))
      {
          process_gml = true;
      }
      else if (ends_with(filename, ".project.gmx"))
      {
          process_project = true;
      }

      using namespace ogm::bytecode;
      ReflectionAccumulator reflection;
      ogm::interpreter::staticExecutor.m_frame.m_reflection = &reflection;
      ogm::asset::AssetTable& assetTable = ogm::interpreter::staticExecutor.m_frame.m_assets;
      ogm::bytecode::BytecodeTable& bytecode = ogm::interpreter::staticExecutor.m_frame.m_bytecode;
      bytecode.reserve(4096);

      if (!process_project && !process_gml)
      {
          std::cout << "Cannot handle file type for " << filename << ".\n";
          std::cout << "Please supply a .gml or .project.gmx file.\n";
          inFile.close();
          exit(1);
      }
      else if (process_project)
      {
          inFile.close();
          project.process();
          if (compile)
          {
              ogm::bytecode::ProjectAccumulator acc{ogm::interpreter::staticExecutor.m_frame.m_reflection, &ogm::interpreter::staticExecutor.m_frame.m_assets, &ogm::interpreter::staticExecutor.m_frame.m_bytecode, &ogm::interpreter::staticExecutor.m_frame.m_config};
              project.compile(acc, ogm::interpreter::standardLibrary);
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


          ogm_ast_t* ast_test_create = ogm_ast_parse("m=4;");
          ogm_ast_t* ast_test_destroy = ogm_ast_parse("m=2");

          if (show_ast)
          {
              ogm_ast_tree_print(ast);
          }

          ogm::bytecode::ProjectAccumulator acc{ogm::interpreter::staticExecutor.m_frame.m_reflection, &ogm::interpreter::staticExecutor.m_frame.m_assets, &ogm::interpreter::staticExecutor.m_frame.m_bytecode, &ogm::interpreter::staticExecutor.m_frame.m_config};
          Bytecode b;
          ogm::bytecode::bytecode_generate(b, {ast, 0, 0, filename, fileContents.c_str()}, ogm::interpreter::standardLibrary, &acc);
          bytecode.add_bytecode(0, std::move(b));
      }
      else
      {
          assert(false);
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
                  if (bytecode_section.m_debug_symbols && bytecode_section.m_debug_symbols->m_name)
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
              std::cout<<"\nExecuting..."<<std::endl;
              ogm::interpreter::Instance anonymous;
              ogm::interpreter::staticExecutor.m_self = &anonymous;
              auto& parameters = ogm::interpreter::staticExecutor.m_frame.m_data.m_clargs;
              for (size_t i = filename_index + 1; i < argn; ++i)
              {
                  parameters.push_back(argv[i]);
              }
              ogm::interpreter::Debugger debugger;
              if (debug)
              {
                  ogm::interpreter::staticExecutor.debugger_attach(&debugger);
                  debugger.break_execution();
              }

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
          }
      }
  }

  return 0;
}
