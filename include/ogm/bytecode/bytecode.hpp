/// generates bytecode from AST

#ifndef OGMC_BYTECODE_HPP
#define OGMC_BYTECODE_HPP

#include "BytecodeTable.hpp"
#include "Library.hpp"

#include "ogm/ast/parse.h"
#include "ogm/bytecode/Namespace.hpp"
#include "ogm/asset/AssetTable.hpp"
#include "ogm/asset/Config.hpp"
#include "ogm/common/parallel.hpp"

#include <vector>
#include <set>
#include <string>

namespace ogm { namespace bytecode {

class Library;

namespace opcode
{
enum opcode_t
{
    // dsc: loads false.
    // imm:
    // pop:
    // psh: bool
    ldi_false,

    // dsc: loads false.
    // imm:
    // pop:
    // psh: bool
    ldi_true,

    // dsc: loads undefined.
    // imm:
    // pop:
    // psh: undef
    ldi_undef,

    // dsc: loads a float from immediate
    // imm: float32_t
    // pop:
    // psh: ogm::real
    ldi_f32,

    // dsc: loads a double from immediate
    // imm: float64_t
    // pop:
    // psh: ogm::real
    ldi_f64,

    // dsc: loads an int from immediate
    // imm: int32_t
    // pop:
    // psh: ogm::int
    ldi_s32,

    // dsc: loads an int from immediate
    // imm: uint64_t
    // pop:
    // psh: ogm::int64
    ldi_u64,

    // dsc: loads a string from immediate
    // imm: string
    // pop:
    // psh: string
    ldi_string,

    // dsc: increments number
    // imm:
    // pop: num
    // psh: num
    inc,

    // dsc: decrements number
    // imm:
    // pop: num
    // psh: num
    dec,

    // dsc: increments local variable
    // imm: id32
    // pop:
    // psh:
    incl,

    // dsc: decrements local variable
    // imm: id32
    // pop:
    // psh:
    decl,

    // dsc: adds 2 numbers together
    // imm:
    // pop: any any
    // psh: any
    add2,

    // dsc: subtracts 2nd from first
    // imm:
    // pop: num num
    // psh: num
    sub2,

    // dsc: multiplies 2 numbers
    // imm:
    // pop: num num
    // psh: num
    mult2,

    // dsc: divides 2 numbers (floating point)
    // imm:
    // pop: num num
    // psh: num
    fdiv2,

    // dsc: divides 2 numbers (integer)
    // imm:
    // pop: num num
    // psh: num
    idiv2,

    // dsc: computes modulo
    // imm:
    // pop: num num
    // psh: num
    mod2,

    // dsc: leftshift
    // imm:
    // pop: num num
    // psh: num
    lsh2,

    // dsc: rightshift
    // imm:
    // pop: num num
    // psh: num
    rsh2,

    // dsc: >
    // imm:
    // pop: num num
    // psh: bool
    gt,

    // dsc: <
    // imm:
    // pop: num num
    // psh: bool
    lt,

    // dsc: >=
    // imm:
    // pop: num num
    // psh: bool
    gte,

    // dsc: <=
    // imm:
    // pop: num num
    // psh: bool
    lte,

    // dsc: ==
    // imm:
    // pop: any any
    // psh: bool
    eq,

    // dsc: !=
    // imm:
    // pop: any any
    // psh: bool
    neq,

    // dsc: &&
    // imm:
    // pop: bool bool
    // psh: bool
    bland,

    // dsc: ||
    // imm:
    // pop: bool bool
    // psh: bool
    blor,

    // dsc: ^^
    // imm:
    // pop: bool bool
    // psh: bool
    blxor,

    // dsc: &
    // imm:
    // pop: num num
    // psh: num
    band,

    // dsc: |
    // imm:
    // pop: num num
    // psh: num
    bor,

    // dsc: ^
    // imm:
    // pop: num num
    // psh: num
    bxor,

    // dsc: ~
    // imm:
    // pop: num
    // psh: num
    bnot,

    // dsc: computes condition
    // imm:
    // pop: any
    // psh:
    // ofg: C
    cond,

    // dsc: computes condition (negative)
    // imm:
    // pop: any
    // psh:
    // ifg:
    // ofg: C
    ncond,

    // dsc: pushes condition flag as bool
    // imm:
    // pop:
    // psh: bool
    // ifg: C
    // ofg:
    pcond,

    // dsc: denotes that store commands should be no-copy
    // imm:
    // pop:
    // psh:
    // ofg: X
    sfx,

    // dsc: denotes that store commands should copy-on-write (default)
    // imm:
    // pop:
    // psh:
    // ofg: X
    ufx,

    // dsc: allocates local variables
    // imm: uint32
    // pop:
    // psh:
    all,

    // dsc: stores local variable
    // imm: id32
    // pop: num
    // psh:
    stl,

    // dsc: retrieves local variable
    // imm: id32
    // pop:
    // psh: num
    ldl,

    // dsc: store instance variable self
    // imm: id32
    // pop: any
    // psh:
    sts,

    // dsc: store instance variable self
    // imm: id32
    // pop:
    // psh: any
    lds,

    // dsc: store instance variable other
    // imm: id32
    // pop: id any
    // psh:
    sto,

    // dsc: store instance variable self
    // imm: id32
    // pop: id
    // psh: any
    ldo,

    // dsc: store global variable
    // imm: id32
    // pop: any
    // psh:
    stg,

    // dsc: load global variable
    // imm: id32
    // pop:
    // psh: any
    ldg,

    // dsc: store built-in instance variable
    // imm: id32
    // pop: any
    // psh:
    stt,

    // dsc: load built-in instance variable
    // imm: id32
    // pop:
    // psh: any
    ldt,

    // dsc: store built-in instance variable
    // imm: id32
    // pop: id any
    // psh:
    stp,

    // dsc: load built-in instance variable
    // imm: id32
    // pop: id
    // psh: any
    ldp,

    // dsc: stores local variable array
    // imm: id8
    // pop: num num num
    // psh:
    stla,

    // dsc: retrieves local variable array
    // imm: id8
    // pop: num num
    // psh: num
    ldla,

    // dsc: store instance variable self array
    // imm: id32
    // pop: num num any
    // psh:
    stsa,

    // dsc: store instance variable self array
    // imm: id32
    // pop: num num
    // psh: any
    ldsa,

    // dsc: store instance variable other array
    // imm: id32
    // pop: num num id any
    // psh:
    stoa,

    // dsc: store instance variable self array
    // imm: id32
    // pop: num num id
    // psh: any
    ldoa,

    // dsc: store global variable array
    // imm: id32
    // pop: num num any
    // psh:
    stga,

    // dsc: load global variable array
    // imm: id32
    // pop: num num
    // psh: any
    ldga,

    // dsc: store built-in instance variable array
    // imm: id32
    // pop: num num id any
    // psh:
    stpa,

    // dsc: load built-in instance variable array
    // imm: id32
    // pop: num num id
    // psh: any
    ldpa,

    // dsc: pops value (ignore it)
    // imm:
    // pop: any
    // psh:
    pop,

    // dsc: duplicates value
    // imm:
    // pop: any
    // psh: any any
    dup,

    // dsc: duplicates 2 values from the stack.
    // imm:
    // pop: any any
    // psh: any any any any
    dup2,

    // dsc: duplicates 3 values from the stack.
    // imm:
    // pop: any any any
    // psh: any any any any any any
    dup3,

    // dsc: duplicates n values from the stack.
    // imm: u8
    // pop: any*
    // psh: any*
    dupn,

    // dsc: call native function
    // imm: fptr(32/64) byte (number of arguments)
    // pop: any*
    // psh: any
    nat,

    // dsc: obtain 'with' iterator. Pushes current id on stack.
    // imm:
    // pop: id
    // psh: id32 iter32
    wti,

    // dsc: 'with' yield. Sets condition flag to true when complete and restores id from stack
    //      If not complete, pushes the iterator id onto the stack and sets the current id.
    // imm:
    // pop: id? iter32
    // psh: iter32?
    // ifg:
    // ofg: C
    wty,

    // dsc: 'with' drop. Frees with iterator prematurely and restores id from stack
    // imm:
    // pop: id iter32
    // psh:
    // ifg:
    // ofg: C
    wtd,

    // jumps to given bytecode pointer
    jmp,

    // conditionally jumps to given bytecode pointer
    bcond,

    // dsc: calls the bytecode at the given bytecode section index
    // imm: bytecode_section_index_t argc
    // pop: any*
    // psh: any
    call,

    // dsc: returns from subroutine
    // imm: number of return values
    // pop: (up to last all)
    // psh:
    ret,

    // dsc: does nothing
    // imm:
    // pos:
    // psh:
    nop,

    eof
};

extern const char* opcode_string[];

// retrieves opcode string, or "?" for unknown opcodes.
const char* get_opcode_string(opcode_t);
}

static_assert((int)opcode::eof <= 0xff, "Opcode must be 1 byte.");

// TODO: extract these struct definitions into another header.
struct EnumTable;
class ReflectionAccumulator
{
public:
    Namespace m_namespace_instance;
    Namespace m_namespace_global;

    // globals declared as globalvar.
    std::set<std::string> m_bare_globals;

    std::map<std::string, ogm_ast_t*> m_ast_macros;

    // pointer set in constructor and guaranteed to exist.
    EnumTable* const m_enums;

public:
    // member functions
    ReflectionAccumulator();
    ~ReflectionAccumulator();

    inline bool has_bare_global(const std::string& s) const
    {
        READ_LOCK(m_mutex_bare_globals)
        return m_bare_globals.find(s) != m_bare_globals.end();
    }

    // checks if macro exists without attaining write lock.
    // (by compilation time, macros should be fully set.)
    inline bool has_macro_NOMUTEX(const std::string& s) const
    {
        return m_ast_macros.find(s) != m_ast_macros.end();
    }

    #ifdef PARALLEL_COMPILE
    // TODO: make these private.
    mutable std::mutex m_mutex_bare_globals;
    mutable std::mutex m_mutex_macros;
    mutable std::mutex m_mutex_enums;
    #endif
};

struct DisassembledBytecodeInstruction
{
    DisassembledBytecodeInstruction()
        : m_op(opcode::eof)
        , m_address(0)
        , m_immediate()
    { }

    DisassembledBytecodeInstruction(opcode::opcode_t op, size_t address, std::string immediate, ogm_ast_line_column start = ogm_ast_line_column())
        : m_op(op)
        , m_address(address)
        , m_immediate(immediate)
    { }
    opcode::opcode_t m_op;
    size_t m_address;
    std::string m_immediate;
};

// describes a block of code that can be compiled into bytecode
// an AST coupled with number of arguments, return values, and optional
//  source information (for the debug symbols).
struct DecoratedAST
{
    ogm_ast_t* m_ast;
    uint8_t m_retc;
    uint8_t m_argc;
    const char* m_name;
    const char* m_source;

    DecoratedAST(ogm_ast_t* ast, uint8_t retc=0, uint8_t argc=0, const char* name = nullptr, const char* source = nullptr)
        : m_ast(ast)
        , m_retc(retc)
        , m_argc(argc)
        , m_name(name)
        , m_source(source)
    { }
};

// extra information required to compile bytecode within a larger project.
struct ProjectAccumulator
{
    // accumulates variable indices, globalvar settings, and enums
    ReflectionAccumulator* m_reflection = nullptr;

    // assets
    asset::AssetTable* m_assets = nullptr;

    // list of bytecode sections
    bytecode::BytecodeTable* m_bytecode = nullptr;

    //// the following are not used by bytecode_generate but are used in project/ ////

    // runtime and project configuration
    asset::Config* m_config = nullptr;

    bytecode_index_t next_bytecode_index()
    {
        return m_next_bytecode_index++;
    }

private:
    // next bytecode index
    bytecode_index_t m_next_bytecode_index = 0;

public:
    ProjectAccumulator(ReflectionAccumulator* reflection = nullptr, asset::AssetTable* assets = nullptr, bytecode::BytecodeTable* bytecode = nullptr, asset::Config* config=nullptr)
        : m_reflection(reflection)
        , m_assets(assets)
        , m_bytecode(bytecode)
        , m_config(config)
    { }
};

// scans te ast to determine the number of arguments and return vales associated with the function.
void bytecode_preprocess(const ogm_ast_t& ast, uint8_t& out_retc, uint8_t& out_argc, ReflectionAccumulator& in_out_reflection_accumulator);

// compiles bytecode from the given abstract syntax tree.
// if the ast is an ogm_ast_st_imp_body_list, then there must be at most one body in that list.

void bytecode_generate(Bytecode& out_bytecode, const DecoratedAST& in, const Library* library = &defaultLibrary, ProjectAccumulator* accumulator = nullptr);

// disassembles bytecode to vector of instructions
void bytecode_dis(bytecode::BytecodeStream inBytecode, std::vector<struct DisassembledBytecodeInstruction>& outInstructions, const Library* library = &defaultLibrary);

// disassembles bytecode to string
void bytecode_dis(bytecode::BytecodeStream inBytecode, std::ostream& outDis, const Library* library = &defaultLibrary, const ReflectionAccumulator* accumulator = nullptr, bool show_source_inline = false, size_t end_pos = std::numeric_limits<size_t>::max());

}
}
#endif
