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

#define VERSION "OpenGML v0.6 (alpha)"

using namespace std;

int main (int argn, char** argv) {
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
      if (i == 1 && argn == 2)
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
      ogmi::staticExecutor.m_frame.m_reflection = &reflection;
      ogm::asset::AssetTable& assetTable = ogmi::staticExecutor.m_frame.m_assets;
      ogm::bytecode::BytecodeTable& bytecode = ogmi::staticExecutor.m_frame.m_bytecode;
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
              ogm::bytecode::ProjectAccumulator acc{ogmi::staticExecutor.m_frame.m_reflection, &ogmi::staticExecutor.m_frame.m_assets, &ogmi::staticExecutor.m_frame.m_bytecode, &ogmi::staticExecutor.m_frame.m_config};
              project.compile(acc, ogmi::standardLibrary);
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

          ogm::bytecode::ProjectAccumulator acc{ogmi::staticExecutor.m_frame.m_reflection, &ogmi::staticExecutor.m_frame.m_assets, &ogmi::staticExecutor.m_frame.m_bytecode, &ogmi::staticExecutor.m_frame.m_config};
          Bytecode b;
          ogm::bytecode::bytecode_generate(b, {ast, 0, 0, filename, fileContents.c_str()}, ogmi::standardLibrary, &acc);
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
                  ogm::bytecode::bytecode_dis(bytecode_section, cout, ogmi::standardLibrary, dis_raw ? nullptr : &reflection, lines);
              }
          }

          if (execute)
          {
              std::cout<<"\nExecuting..."<<std::endl;
              ogmi::Instance anonymous;
              ogmi::staticExecutor.m_self = &anonymous;
              auto& parameters = ogmi::staticExecutor.m_frame.m_data.m_clargs;
              for (size_t i = filename_index + 1; i < argn; ++i)
              {
                  parameters.push_back(argv[i]);
              }
              ogmi::Debugger debugger;
              if (debug)
              {
                  ogmi::staticExecutor.debugger_attach(&debugger);
              }
              ogmi::execute_bytecode();
          }
      }
  }

  return 0;
}
