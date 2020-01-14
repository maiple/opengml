#include "catch/catch.hpp"
#include "ogm/bytecode/bytecode.hpp"
#include "ogm/interpreter/SparseContiguousMap.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/StandardLibrary.hpp"

#include <map>
#include <iostream>

using namespace ogm::interpreter;

TEST_CASE( "execute_bytecode sets global", "[sparse contiguous map]" )
{
    const char* t = "global.rv = 15;";
    ogm_ast* ast = ogm_ast_parse(t);

    Bytecode b;
    bytecode_generate(b, {ast}, standardLibrary);

    bytecode_dis(b, std::cout);

    Instance i;
    staticExecutor.pushSelf(&i);
    execute_bytecode(b);

    REQUIRE(staticExecutor.m_frame.get_global_variable(0).castCoerce<int>() == 15);
    staticExecutor.popSelf();
    ogm_ast_free(ast);
    staticExecutor.m_frame.reset_hard();
}
