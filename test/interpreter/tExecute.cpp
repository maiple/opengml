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
    
    // we create a ProjectAccumulator so we can access its namespace later.
    ReflectionAccumulator ra;
    BytecodeTable bt;
    AssetTable at;
    ProjectAccumulator pa{ standardLibrary, &ra, &at, &bt };
    ogm::interpreter::standardLibrary->reflection_add_instance_variables(ra);
    
    bytecode_index_t index = bytecode_generate({ast}, pa);

    Bytecode b = bt.get_bytecode(index);

    bytecode_dis(b, std::cout);

    Instance i;
    staticExecutor.pushSelf(&i);
    execute_bytecode(b);
    
    INFO(ra.m_namespace_instance.id_count())

    REQUIRE(ra.m_namespace_instance.has_id("rv"));

    REQUIRE(staticExecutor.m_frame.get_global_variable(
        ra.m_namespace_instance.find_id("rv")
    ).castCoerce<int>() == 15);
    staticExecutor.popSelf();
    ogm_ast_free(ast);
    staticExecutor.m_frame.reset_hard();
}
