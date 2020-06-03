#include "catch/catch.hpp"
#include "ogm/bytecode/bytecode.hpp"
#include "ogm/interpreter/debug_log.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/StandardLibrary.hpp"
#include "interpreter/library/library.h"

#include <string>
#include <vector>

using namespace ogm::interpreter;
using namespace ogm::bytecode;

namespace
{
    void test_ast_ops(const ogm_ast_t* ast)
    {
        /*ogm_ast_t* copy = ogm_ast_copy(ast);
        REQUIRE(ogm_ast_tree_equal(copy, ast));
        REQUIRE(ogm_ast_tree_equal(ast, copy));*/

        std::stringstream ss{ };
        ogm_ast_write(ast, ss);
        ss.seekg(0);
        ogm_ast_t* deserialized = ogm_ast_load(ss);
        REQUIRE(ogm_ast_tree_equal(ast, deserialized));
        REQUIRE(ogm_ast_tree_equal(deserialized, ast));

        //ogm_ast_free(copy);
        ogm_ast_free(deserialized);
    }

    // runs a script by its path
    // checks its ogm_expected value.
    void script_unit_test(const std::string& path)
    {
        staticExecutor.reset();

        // parse file
        std::string fileContents = read_file_contents(path);
        ogm_ast_t* ast = ogm_ast_parse(fileContents.c_str());
        std::string info = ("In " + path);
        INFO(info.c_str());
        REQUIRE(ast);

        test_ast_ops(ast);

        // compile
        ogm::interpreter::staticExecutor.m_frame.reset_hard();
        ReflectionAccumulator reflection;
        ogm::interpreter::standardLibrary->reflection_add_instance_variables(reflection);

        ogm::interpreter::staticExecutor.m_frame.m_reflection = &reflection;
        ogm::bytecode::ProjectAccumulator acc{ogm::interpreter::standardLibrary, ogm::interpreter::staticExecutor.m_frame.m_reflection, &ogm::interpreter::staticExecutor.m_frame.m_assets, &ogm::interpreter::staticExecutor.m_frame.m_bytecode, &ogm::interpreter::staticExecutor.m_frame.m_config};
        DecoratedAST dast{ast, path.c_str(), fileContents.c_str()};
        ogm::bytecode::bytecode_preprocess(dast, reflection);
        bytecode_index_t init_index = ogm::bytecode::bytecode_generate(dast, acc, nullptr, acc.next_bytecode_index());

        // runtime parameters
        Instance* anonymous = new Instance();
        staticExecutor.m_library = ogm::interpreter::standardLibrary;
        staticExecutor.m_self = anonymous;
        auto& parameters = ogm::interpreter::staticExecutor.m_frame.m_data.m_clargs = {path};
        clear_debug_log();
        set_collect_debug_info(true);
        Variable dummy;
        fn::ogm_garbage_collector_process(dummy);

        // execute bytecode
        ogm::interpreter::execute_bytecode(init_index);

        // check expected value
        std::string log = get_debug_log();
        std::string expected = get_debug_expected();
        trim(log);
        trim(expected);
        if (expected != "")
        {
            REQUIRE(log == expected);
        }

        // clean up.
        set_collect_debug_info(false);
        clear_debug_log();
        delete anonymous;
        staticExecutor.reset();
        fn::ogm_garbage_collector_process(dummy);

        // check that garbage count is 0.
        fn::ogm_garbage_collector_count(dummy);
        REQUIRE(dummy.castCoerce<int32_t>() == 0);
    }
}

TEST_CASE( "Demo scripts run", "[Demo]" )
{
    std::vector<std::string> paths;
    REQUIRE(path_exists("demo/scripts"));
    list_paths("demo/scripts/", paths);
    bool found_any_tests = false;
    for (const std::string& path : paths)
    {
        if (ends_with(path, ".gml"))
        {
            found_any_tests = true;
            script_unit_test(path);
        }
    }

    REQUIRE(found_any_tests);
}
