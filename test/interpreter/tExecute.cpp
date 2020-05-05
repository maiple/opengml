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
    
    // we create a ProjectAccumulator so we can access its namespace later.
    ProjectAccumulator pa;
    ReflectionAccumulator ra;
    ogm::interpreter::standardLibrary->reflection_add_instance_variables(ra);
    pa.m_reflection = &ra;
    
    bytecode_generate(b, {ast}, standardLibrary, &pa);

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
