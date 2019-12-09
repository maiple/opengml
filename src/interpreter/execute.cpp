#ifdef OPTIMIZE_CORE
#ifdef __GNUC__
#pragma GCC optimize ("O3")
#endif
#endif

#include "ogm/bytecode/bytecode.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/Debugger.hpp"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/bytecode/stream_macro.hpp"
#include "ogm/common/error.hpp"

#include <cstddef>
#include <map>

namespace ogm { namespace interpreter
{

typedef int32_t bytecode_address_t;

using namespace ogm;
using namespace ogm::bytecode;

// static address for quicker access.
Executor staticExecutor;

void Executor::debugger_attach(Debugger* d)
{
    ogm_assert(d != nullptr);
    if (m_debugger)
    {
        debugger_detach();
    }
    m_debugger = d;
    d->on_attach();
}

void Executor::debugger_detach()
{
    if (m_debugger)
    {
        m_debugger->on_detach();
    }
    m_debugger = nullptr;
}

bool execute_bytecode_safe(bytecode_index_t bytecode_index)
{
    if (bytecode_index != ogm::bytecode::k_no_bytecode)
    {
        return execute_bytecode(bytecode_index);
    }
    return false;
}

bool call_bytecode(Bytecode bytecode, Variable* retv, uint8_t argc, const Variable* argv)
{
    size_t pre_varStackIndex = staticExecutor.m_varStackIndex;

    // push arguments onto stack
    for (size_t i = 0; i < argc; ++i)
    {
        staticExecutor.pushRef().copy(argv[i]);
    }
    staticExecutor.pushRef() = argc;

    // run bytecode
    bool suspended = execute_bytecode(bytecode, true);
    if (suspended) return true;

    // argc, argv popped by callee

    // copy out return values.
    for (int32_t i = bytecode.m_retc - 1; i >= 0; i--)
    {
        retv[i] = std::move(staticExecutor.popRef());
    }
    ogm_assert(pre_varStackIndex == staticExecutor.m_varStackIndex);
    return false;
}

namespace
{
    std::string trace_line_for_pc(const bytecode::BytecodeStream& bs, int32_t offset)
    {
        std::stringstream ss;
        const bytecode::DebugSymbols* symbols = bs.m_bytecode.m_debug_symbols.get();
        if (symbols && symbols->m_name)
        {
            ss << symbols->m_name;
        }
        else if (symbols)
        {
            ss << "<anonymous>";
        }
        else
        {
            ss << "?";
        }
        ss << ": ";
        std::string line = "<source not found>";
        if (symbols)
        {
            bytecode::DebugSymbolSourceMap::Range range;
            size_t pos = bs.m_pos;
            if (pos < -offset)
            {
                pos = 0;
            }
            else
            {
                pos += offset;
            }
            if (pos == 0 && !symbols->m_source_map.get_location_at(pos, range))
            // work around for start-of-function "all" call being off-line.
            {
                pos += 5;
            }

            if (symbols->m_source_map.get_location_at(pos, range))
            {
                ss << "line " << range.m_source_start.m_line;
                if (symbols->m_source)
                {
                    line = nth_line(symbols->m_source, range.m_source_start.m_line);
                }
                ss << ", ";
            }
        }
        ss << "pc " << bs.m_pos;
        ss << "   " << line;
        return ss.str();
    }
}

std::string stack_trace(const std::vector<bytecode::BytecodeStream>& bs)
{
    if (bs.empty())
    {
        return "<no trace data>";
    }
    else
    {
        std::stringstream ss;
        ss << "  at " << trace_line_for_pc(bs.back(), -1);
        for (auto iter = bs.rbegin() + 1; iter != bs.rend(); ++iter)
        {
            ss << "\n  by " << trace_line_for_pc(*iter, -1);
        }
        return ss.str();
    }
}

namespace
{
void pop_row_col(uint32_t& row, uint32_t& col)
{
    Variable& v_col = staticExecutor.popRef();
    Variable& v_row = staticExecutor.popRef();
    int32_t irow, icol;
    irow = v_row.castCoerce<int32_t>();
    icol = v_col.castCoerce<int32_t>();

    if (irow < 0 || icol < 0)
    {
        throw MiscError("Invalid array index " + std::to_string(row) + "," + std::to_string(col));
    }

    v_col.cleanup();
    v_row.cleanup();

    row = irow;
    col = icol;
}

#define TRACE(s) if (debug && staticExecutor.m_debugger->m_config.m_trace_addendums) {std::stringstream sss; sss << s; staticExecutor.m_debugger->trace_addendum(sss.str());}
#define TRACE_STACK(n) if (debug && staticExecutor.m_debugger->m_config.m_trace) \
{                                           \
    std::stringstream ss;                   \
    for (size_t i = 0; i < n; ++i)          \
    {                                       \
        if (i == 0) ss << "(";              \
        else ss << ", ";                    \
        ss << staticExecutor.peekRef(n-i-1);\
        if (i == n - 1) ss << ")";          \
    }                                       \
    TRACE(ss.str())                         \
}

template<bool debug>
bool execute_bytecode_loop()
{
    #ifndef NDEBUG
    size_t pre_varStackIndex = staticExecutor.m_varStackIndex;
    #endif
    using namespace opcode;
    BytecodeStream& in = staticExecutor.m_pc;
    while (true)
    {
        if (debug)
        {
            staticExecutor.m_debugger->tick(in);
        }

        #ifndef NDEBUG
        size_t op_pre_varStackIndex = staticExecutor.m_varStackIndex;
        #endif

        try
        {
            opcode_t op;
            read_op(in, op);
            switch (op)
            {
            case ldi_false:
                {
                    staticExecutor.pushRef() = false;
                }
                break;
            case ldi_true:
                {
                    staticExecutor.pushRef() = true;
                }
                break;
            case ldi_undef:
                {
                    Variable v;
                    staticExecutor.pushRef() = std::move(v);
                }
                break;
            case ldi_f32:
                {
                    float immf;
                    read(in, immf);
                    staticExecutor.pushRef() = immf;
                }
                break;
            case ldi_f64:
                {
                    double immf;
                    read(in, immf);
                    staticExecutor.pushRef() = immf;
                }
                break;
            case ldi_s32:
                {
                    int32_t immi;
                    read(in, immi);
                    staticExecutor.pushRef() = immi;
                }
                break;
            case ldi_u64:
                {
                    uint64_t immu;
                    read(in, immu);
                    //staticExecutor.pushRef() = immu;
                    staticExecutor.pushRef() = immu;
                }
                break;
            case ldi_string:
                {
                    std::string imms;
                    char c;
                    while (true)
                    {
                        read(in, c);
                        if (c == 0)
                        {
                            break;
                        }
                        else
                        {
                            imms.push_back(c);
                        }
                    }
                    staticExecutor.pushRef() = imms;
                }
                break;
            case ldi_arr:
                {
                    staticExecutor.pushRef() = 0;
                    staticExecutor.peekRef().array_ensure(true);
                }
                break;
            case inc:
                {
                    staticExecutor.peekRef()+=1;
                    TRACE(staticExecutor.peekRef());
                }
                break;
            case dec:
                {
                    staticExecutor.peekRef()-=1;
                    TRACE(staticExecutor.peekRef());
                }
                break;
            case incl:
                {
                    uint32_t id;
                    read(in, id);
                    staticExecutor.local(id) += 1;
                    TRACE(staticExecutor.local(id));

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex);
                }
                break;
            case decl:
                {
                    uint32_t id;
                    read(in, id);
                    staticExecutor.local(id) -= 1;
                    TRACE(staticExecutor.local(id));

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex);
                }
                break;
            case seti:
                {
                    ogm::interpreter::Variable& value = staticExecutor.popRef();
                    ogm::interpreter::Variable& j = staticExecutor.popRef();
                    ogm::interpreter::Variable& i = staticExecutor.popRef();
                    ogm::interpreter::Variable& arr = staticExecutor.peekRef();
                    arr.array_get(i.castCoerce<int32_t>(), j.castCoerce<int32_t>(), staticExecutor.m_statusCOW)
                        = std::move(value);
                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 3);
                }
                break;
            case add2:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.peekRef();
                    v1 += v2;
                    v2.cleanup();
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case sub2:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.peekRef();
                    v1 -= v2;
                    v2.cleanup();
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case mult2:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.peekRef();
                    v1 *= v2;
                    v2.cleanup();
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case fdiv2:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.peekRef();
                    v1 /= v2;
                    v2.cleanup();
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case idiv2:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.peekRef();
                    v1.idiv(v2);
                    v2.cleanup();
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case mod2:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.peekRef();
                    v1 %= v2;
                    v2.cleanup();
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case lsh2:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.peekRef();
                    v1 <<= v2;
                    v2.cleanup();
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case rsh2:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.peekRef();
                    v1 >>= v2;
                    v2.cleanup();
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case lt:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.popRef();
                    staticExecutor.m_statusCond = v1 < v2;
                    v1.cleanup();
                    v2.cleanup();
                    TRACE(staticExecutor.m_statusCond);

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 2);
                }
                break;
            case lte:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.popRef();
                    staticExecutor.m_statusCond = v1 <= v2;
                    v1.cleanup();
                    v2.cleanup();
                    TRACE(staticExecutor.m_statusCond);

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 2);
                }
                break;
            case gt:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.popRef();
                    staticExecutor.m_statusCond = v1 > v2;
                    v1.cleanup();
                    v2.cleanup();
                    TRACE(staticExecutor.m_statusCond);

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 2);
                }
                break;
            case gte:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.popRef();
                    staticExecutor.m_statusCond = v1 >= v2;
                    v1.cleanup();
                    v2.cleanup();
                    TRACE(staticExecutor.m_statusCond);

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 2);
                }
                break;
            case eq:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.popRef();
                    staticExecutor.m_statusCond = v1 == v2;
                    v1.cleanup();
                    v2.cleanup();
                    TRACE(staticExecutor.m_statusCond);

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 2);
                }
                break;
            case neq:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.popRef();
                    staticExecutor.m_statusCond = v1 != v2;
                    v1.cleanup();
                    v2.cleanup();
                    TRACE(staticExecutor.m_statusCond);

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 2);
                }
                break;
            case bland:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.popRef();
                    bool b = v1.cond() && v2.cond();
                    v1.cleanup();
                    v2.cleanup();
                    staticExecutor.pushRef() = b;
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case blor:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.popRef();
                    bool b = v1.cond() || v2.cond();
                    v1.cleanup();
                    v2.cleanup();
                    staticExecutor.pushRef() = b;
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case blxor:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.popRef();
                    v1.cleanup();
                    v2.cleanup();
                    staticExecutor.pushRef() = v1.cond() != v2.cond();
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case band:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.peekRef();
                    v1 &= v2;
                    v2.cleanup();
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case bor:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.peekRef();
                    v1 |= v2;
                    v2.cleanup();
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case bxor:
                {
                    ogm::interpreter::Variable& v2 = staticExecutor.popRef();
                    ogm::interpreter::Variable& v1 = staticExecutor.peekRef();
                    v1 ^= v2;
                    v2.cleanup();
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case bnot:
                {
                    ogm::interpreter::Variable& v1 = staticExecutor.peekRef();
                    v1.invert();
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex);
                }
                break;
            case cond:
                {
                    ogm::interpreter::Variable& v = staticExecutor.popRef();
                    staticExecutor.m_statusCond = v.cond();
                    v.cleanup();
                    TRACE(staticExecutor.m_statusCond);

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case ncond:
                {
                    ogm::interpreter::Variable& v = staticExecutor.popRef();
                    staticExecutor.m_statusCond = !v.cond();
                    v.cleanup();
                    TRACE(staticExecutor.m_statusCond);

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case pcond:
                {
                    staticExecutor.pushRef() = staticExecutor.m_statusCond;
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex + 1);
                }
                break;
            case sfx:
                {
                    staticExecutor.m_statusCOW = false;
                }
                break;
            case ufx:
                {
                    staticExecutor.m_statusCOW = true;
                }
                break;
            case all:
                // allocate locals
                {
                    // push previous locals-start onto the stack
                    staticExecutor.pushRef() = static_cast<uint64_t>(staticExecutor.m_locals_start);
                    uint32_t count;
                    read(in, count);

                    // remember this stack pointer.
                    staticExecutor.m_locals_start = staticExecutor.get_sp() + 1;

                    // put undefined variables onto stack.
                    Variable undef;
                    for (size_t i = 0; i < count; i++)
                    {
                        staticExecutor.pushRef().copy(undef);
                    }

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex + 1 + count);
                }
                break;
            case stl:
                {
                    uint32_t id;
                    read(in, id);

                    // cleanup previous value
                    staticExecutor.local(id).cleanup();

                    // new value
                    staticExecutor.local(id) = std::move(staticExecutor.popRef());
                }
                break;
            case ldl:
                {
                    uint32_t id;
                    read(in, id);
                    staticExecutor.pushRef().copy(staticExecutor.local(id));
                    TRACE(staticExecutor.peekRef());
                }
                break;
            case sts:
                {
                    variable_id_t id;
                    read(in, id);
                    staticExecutor.m_self->getVariable(id).cleanup();
                    staticExecutor.m_self->storeVariable(id, std::move(staticExecutor.popRef()));

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case lds:
                {
                    variable_id_t id;
                    read(in, id);
                    staticExecutor.pushRef().copy(staticExecutor.m_self->findVariable(id));
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex + 1);
                }
                break;
            case sto:
                {
                    variable_id_t variable_id;
                    read(in, variable_id);
                    Variable& v = staticExecutor.popRef();
                    ex_instance_id_t owner_id;
                    {
                        Variable& v_owner_id = staticExecutor.popRef();
                        owner_id = v_owner_id.castCoerce<ex_instance_id_t>();
                        v_owner_id.cleanup();
                    }
                    Instance* instance = staticExecutor.m_frame.get_ex_instance_from_ex_id(owner_id);
                    switch (reinterpret_cast<uintptr_t>(instance))
                    {
                    case 0:
                        v.cleanup();
                        break;
                    case k_uint_self:
                        staticExecutor.m_self->getVariable(variable_id).cleanup();
                        staticExecutor.m_self->storeVariable(variable_id, std::move(v));
                        break;
                    case k_uint_other:
                        staticExecutor.m_other->getVariable(variable_id).cleanup();
                        staticExecutor.m_other->storeVariable(variable_id, std::move(v));
                        break;
                    case k_uint_multi:
                        {
                            WithIterator iter;
                            for (staticExecutor.m_frame.get_multi_instance_iterator(owner_id, iter); !iter.complete(); ++iter)
                            {
                                (*iter)->getVariable(variable_id).cleanup();
                                (*iter)->storeVariable(variable_id, std::move(v));
                            }
                        }
                        break;
                    default:
                        instance->getVariable(variable_id).cleanup();
                        instance->storeVariable(variable_id, std::move(v));
                        break;
                    }

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 2);
                }
                break;
            case ldo:
                {
                    variable_id_t id;
                    read(in, id);
                    Instance* instance;
                    ex_instance_id_t owner_id;
                    {
                        Variable& v_owner_id = staticExecutor.popRef();
                        owner_id = v_owner_id.castCoerce<ex_instance_id_t>();
                        v_owner_id.cleanup();
                        instance = staticExecutor.m_frame.get_ex_instance_from_ex_id(owner_id);
                    }
                    switch (reinterpret_cast<uintptr_t>(instance))
                    {
                    case 0:
                        throw MiscError("Attempted to load variable from non-existent instance.");
                        break;
                    case k_uint_self:
                        staticExecutor.pushRef().copy(staticExecutor.m_self->findVariable(id));
                        break;
                    case k_uint_other:
                        staticExecutor.pushRef().copy(staticExecutor.m_other->findVariable(id));
                        break;
                    case k_uint_multi:
                        {
                            WithIterator iter;
                            staticExecutor.m_frame.get_multi_instance_iterator(owner_id, iter);
                            if (iter.complete())
                            {
                                throw MiscError("No instances to read value from.");
                            }
                            else
                            {
                                staticExecutor.pushRef().copy((*iter)->findVariable(id));
                            }
                        }
                        break;
                    default:
                        staticExecutor.pushRef().copy(instance->findVariable(id));
                        break;
                    }

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex);
                }
                break;
            case stg:
                {
                    variable_id_t id;
                    read(in, id);
                    staticExecutor.m_frame.get_global_variable(id).cleanup();
                    staticExecutor.m_frame.store_global_variable(id, std::move(staticExecutor.popRef()));
                }
                break;
            case ldg:
                {
                    variable_id_t id;
                    read(in, id);
                    staticExecutor.pushRef().copy(staticExecutor.m_frame.get_global_variable(id));
                    TRACE(staticExecutor.peekRef());
                }
                break;
            case stt:
                {
                    variable_id_t id;
                    read(in, id);
                    Variable& v = staticExecutor.popRef();
                    staticExecutor.m_self->set_value(id, v);
                    v.cleanup();
                }
                break;
            case ldt:
                {
                    variable_id_t id;
                    read(in, id);
                    Variable& v = staticExecutor.pushRef();
                    staticExecutor.m_self->get_value(id, v);
                    TRACE(staticExecutor.peekRef());
                }
                break;
            case stp:
                {
                    variable_id_t id;
                    read(in, id);
                    Variable& v = staticExecutor.popRef();
                    ex_instance_id_t owner_id;
                    {
                        Variable& v_owner_id = staticExecutor.popRef();
                        owner_id = v_owner_id.castCoerce<ex_instance_id_t>();
                        v_owner_id.cleanup();
                    }
                    Instance* instance = staticExecutor.m_frame.get_ex_instance_from_ex_id(owner_id);
                    switch (reinterpret_cast<uintptr_t>(instance))
                    {
                    case 0:
                        v.cleanup();
                        break;
                    case k_uint_self:
                        staticExecutor.m_self->set_value(id, v);
                        v.cleanup();
                        break;
                    case k_uint_other:
                        staticExecutor.m_other->set_value(id, v);
                        break;
                    case k_uint_multi:
                        {
                            WithIterator iter;
                            for (staticExecutor.m_frame.get_multi_instance_iterator(owner_id, iter); !iter.complete(); ++iter)
                            {
                                (*iter)->set_value(id, v);
                            }
                            break;
                        }
                    default:
                        instance->set_value(id, v);
                        v.cleanup();
                        break;
                    }

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 2);
                }
                break;
            case ldp:
                {
                    variable_id_t id;
                    read(in, id);
                    Instance* instance;
                    ex_instance_id_t owner_id;
                    {
                        Variable& v_owner_id = staticExecutor.popRef();
                        owner_id = v_owner_id.castCoerce<ex_instance_id_t>();
                        v_owner_id.cleanup();
                        instance = staticExecutor.m_frame.get_ex_instance_from_ex_id(owner_id);
                    }
                    Variable& v = staticExecutor.pushRef();
                    switch (reinterpret_cast<uintptr_t>(instance))
                    {
                    case 0:
                        throw MiscError("Attempted to load variable from non-existent instance.");
                        break;
                    case k_uint_self:
                        staticExecutor.m_self->get_value(id, v);
                        break;
                    case k_uint_other:
                        staticExecutor.m_other->get_value(id, v);
                        break;
                    case k_uint_multi:
                        {
                            WithIterator iter;
                            staticExecutor.m_frame.get_multi_instance_iterator(owner_id, iter);
                            if (iter.complete())
                            {
                                throw MiscError("No instances to read value from.");
                            }
                            else
                            {
                                (*iter)->get_value(id, v);
                            }
                        }
                        break;
                    default:
                        instance->get_value(id, v);
                        break;
                    }
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex);
                }
                break;
            case stla:
                {
                    uint32_t id;
                    read(in, id);

                    Variable& v = staticExecutor.popRef();

                    uint32_t row, col;
                    pop_row_col(row, col);

                    // set array value
                    staticExecutor.local(id)
                        .array_get(row, col, staticExecutor.m_statusCOW)
                        = std::move(v);

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 3);
                }
                break;
            case ldla:
                {
                    uint32_t id;
                    read(in, id);

                    uint32_t row, col;
                    pop_row_col(row, col);

                    // get array value
                    const Variable& v = staticExecutor.local(id).array_at(row, col);
                    staticExecutor.pushRef().copy(v);
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case stsa:
                {
                    variable_id_t id;
                    read(in, id);

                    Variable& v = staticExecutor.popRef();

                    uint32_t row, col;
                    pop_row_col(row, col);

                    staticExecutor.m_self->getVariable(id)
                        .array_get(row, col, staticExecutor.m_statusCOW)
                        = std::move(v);

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 3);
                }
                break;
            case ldsa:
                {
                    variable_id_t id;
                    read(in, id);

                    uint32_t row, col;
                    pop_row_col(row, col);

                    // get array value
                    const Variable& v = staticExecutor.m_self->findVariable(id).array_at(row, col);
                    staticExecutor.pushRef().copy(v);
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case stoa:
                {
                    variable_id_t variable_id;
                    read(in, variable_id);
                    Variable& v = staticExecutor.popRef();
                    ex_instance_id_t owner_id;
                    {
                        Variable& v_owner_id = staticExecutor.popRef();
                        owner_id = v_owner_id.castCoerce<ex_instance_id_t>();
                        v_owner_id.cleanup();
                    }
                    Instance* instance = staticExecutor.m_frame.get_ex_instance_from_ex_id(owner_id);

                    uint32_t row, col;
                    pop_row_col(row, col);

                    switch (reinterpret_cast<uintptr_t>(instance))
                    {
                    case 0:
                        v.cleanup();
                        break;
                    case k_uint_self:
                        staticExecutor.m_self->getVariable(variable_id)
                            .array_get(row, col, staticExecutor.m_statusCOW)
                            = std::move(v);
                        break;
                    case k_uint_other:
                        staticExecutor.m_other->getVariable(variable_id)
                            .array_get(row, col, staticExecutor.m_statusCOW)
                            = std::move(v);
                        break;
                    case k_uint_multi:
                        {
                            WithIterator iter;
                            for (staticExecutor.m_frame.get_multi_instance_iterator(owner_id, iter); !iter.complete(); ++iter)
                            {
                                (*iter)->getVariable(variable_id)
                                    .array_get(row, col, staticExecutor.m_statusCOW)
                                    .copy(v);
                            }
                            v.cleanup();
                            break;
                        }
                    default:
                        instance->getVariable(variable_id)
                            .array_get(row, col, staticExecutor.m_statusCOW)
                            = std::move(v);
                        break;
                    }

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 4);
                }
                break;
            case ldoa:
                {
                    variable_id_t id;
                    read(in, id);
                    Instance* instance;
                    {
                        Variable& v_owner_id = staticExecutor.popRef();
                        ex_instance_id_t owner_id = v_owner_id.castCoerce<ex_instance_id_t>();
                        v_owner_id.cleanup();
                        instance = staticExecutor.m_frame.get_ex_instance_from_ex_id(owner_id);
                    }

                    uint32_t row, col;
                    pop_row_col(row, col);

                    switch (reinterpret_cast<uintptr_t>(instance))
                    {
                    case 0:
                        throw MiscError("Attempted to load variable from non-existent instance.");
                        break;
                    case k_uint_self:
                        staticExecutor.pushRef().copy(staticExecutor.m_self->findVariable(id).array_at(row, col));
                        break;
                    case k_uint_other:
                        staticExecutor.pushRef().copy(staticExecutor.m_other->findVariable(id).array_at(row, col));
                        break;
                    case k_uint_multi:
                        {
                            WithIterator iter;
                            staticExecutor.m_frame.get_multi_instance_iterator(id, iter);
                            if (iter.complete())
                            {
                                throw MiscError("No instances to read value from.");
                            }
                            else
                            {
                                staticExecutor.pushRef().copy((*iter)->findVariable(id).array_at(row, col));
                            }
                        }
                        break;
                    default:
                        staticExecutor.pushRef().copy(instance->findVariable(id).array_at(row, col));
                        break;
                    }
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 2);
                }
                break;
            case stga:
                {
                    variable_id_t id;
                    read(in, id);

                    Variable& v = staticExecutor.popRef();

                    uint32_t row, col;
                    pop_row_col(row, col);

                    staticExecutor.m_frame.get_global_variable(id)
                        .array_get(row, col, staticExecutor.m_statusCOW) = std::move(v);

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 3);
                }
                break;
            case ldga:
                {
                    variable_id_t id;
                    read(in, id);

                    uint32_t row, col;
                    pop_row_col(row, col);

                    staticExecutor.pushRef().copy(staticExecutor.m_frame.get_global_variable(id).array_at(row, col));
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);
                }
                break;
            case stpa:
                {
                    variable_id_t variable_id;
                    read(in, variable_id);
                    Variable& v = staticExecutor.popRef();
                    ex_instance_id_t owner_id;
                    {
                        Variable& v_owner_id = staticExecutor.popRef();
                        owner_id = v_owner_id.castCoerce<ex_instance_id_t>();
                        v_owner_id.cleanup();
                    }
                    Instance* instance = staticExecutor.m_frame.get_ex_instance_from_ex_id(owner_id);

                    uint32_t row, col;
                    pop_row_col(row, col);

                    switch (reinterpret_cast<uintptr_t>(instance))
                    {
                    case 0:
                        v.cleanup();
                        break;
                    case k_uint_self:
                        staticExecutor.m_self->set_value_array(variable_id, row, col, v);
                        v.cleanup();
                        break;
                    case k_uint_other:
                        staticExecutor.m_other->set_value_array(variable_id, row, col, v);
                        v.cleanup();
                        break;
                    case k_uint_multi:
                        {
                            WithIterator iter;
                            for (staticExecutor.m_frame.get_multi_instance_iterator(owner_id, iter); !iter.complete(); ++iter)
                            {
                                (*iter)->set_value_array(variable_id, row, col, v);
                            }
                            v.cleanup();
                            break;
                        }
                    default:
                        instance->set_value_array(variable_id, row, col, v);
                        v.cleanup();
                        break;
                    }

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 4);
                }
                break;
            case ldpa:
                {
                    variable_id_t id;
                    read(in, id);
                    Instance* instance;
                    {
                        Variable& v_owner_id = staticExecutor.popRef();
                        ex_instance_id_t owner_id = v_owner_id.castCoerce<ex_instance_id_t>();
                        v_owner_id.cleanup();
                        instance = staticExecutor.m_frame.get_ex_instance_from_ex_id(owner_id);
                    }

                    uint32_t row, col;
                    pop_row_col(row, col);

                    switch (reinterpret_cast<uintptr_t>(instance))
                    {
                    case 0:
                        throw MiscError("Attempted to load built-in variable from non-existent instance.");
                        break;
                    case k_uint_self:
                        staticExecutor.m_self->get_value_array(id, row, col, staticExecutor.pushRef());
                        break;
                    case k_uint_other:
                        staticExecutor.m_other->get_value_array(id, row, col, staticExecutor.pushRef());
                        break;
                    case k_uint_multi:
                        {
                            WithIterator iter;
                            staticExecutor.m_frame.get_multi_instance_iterator(id, iter);
                            if (iter.complete())
                            {
                                throw MiscError("No instances to read built-in value from.");
                            }
                            else
                            {
                                (*iter)->get_value_array(id, row, col, staticExecutor.pushRef());
                            }
                        }
                        break;
                    default:
                        instance->get_value_array(id, row, col, staticExecutor.pushRef());
                        break;
                    }
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 2);
                }
                break;
            case pop:
                {
                    TRACE(staticExecutor.peekRef() << " popped.");
                    staticExecutor.popRef().cleanup();
                    TRACE(staticExecutor.peekRef() << " revealed.");
                }
                break;
            case dup:
                {
                    ogm::interpreter::Variable& v = (staticExecutor.peekRef());
                    staticExecutor.pushRef().copy(v);
                    TRACE(staticExecutor.peekRef());
                }
                break;
            case dup2:
                {
                    ogm::interpreter::Variable& v0 = (staticExecutor.peekRef());
                    ogm::interpreter::Variable& v1 = (staticExecutor.peekRef(1));
                    staticExecutor.pushRef().copy(v1);
                    TRACE(staticExecutor.peekRef());
                    staticExecutor.pushRef().copy(v0);
                    TRACE(staticExecutor.peekRef());
                }
                break;
            case dup3:
                {
                    ogm::interpreter::Variable& v0 = (staticExecutor.peekRef());
                    ogm::interpreter::Variable& v1 = (staticExecutor.peekRef(1));
                    ogm::interpreter::Variable& v2 = (staticExecutor.peekRef(2));
                    staticExecutor.pushRef().copy(v2);
                    TRACE(staticExecutor.peekRef());
                    staticExecutor.pushRef().copy(v1);
                    TRACE(staticExecutor.peekRef());
                    staticExecutor.pushRef().copy(v0);
                    TRACE(staticExecutor.peekRef());
                }
                break;
            case dupn:
                {
                    uint8_t n;
                    read(in, n);

                    for (size_t i = 0; i < n; ++i)
                    {
                        staticExecutor.pushRef().copy(staticExecutor.peekRef(n - 1));
                        TRACE(staticExecutor.peekRef());
                    }
                }
                break;
            case dupi:
                {
                    uint8_t n;
                    read(in, n);

                    ogm::interpreter::Variable& v = (staticExecutor.peekRef(n));
                    staticExecutor.pushRef().copy(v);
                    TRACE(staticExecutor.peekRef());
                }
                break;
            case deli:
                {
                    uint8_t n;
                    read(in, n);

                    staticExecutor.peekRef(n).cleanup();
                    for (size_t i = 0; i < n; ++i)
                    {
                        staticExecutor.peekRef(n - i) = std::move(
                            staticExecutor.peekRef(n - i - 1)
                        );
                    }
                    staticExecutor.popRef();
                }
                break;
            case swap:
                {
                    ogm::interpreter::Variable v0 = std::move(staticExecutor.popRef());
                    ogm::interpreter::Variable v1 = std::move(staticExecutor.popRef());
                    staticExecutor.pushRef() = std::move(v0);
                    staticExecutor.pushRef() = std::move(v1);
                }
                break;
            case nat:
                // for efficiency reasons, this is hardcoded (and not part of the StandardLibrary class.)
                {
                    void* vfptr;
                    int8_t argc;
                    read(in, vfptr);
                    read(in, argc);
                    switch (argc)
                    {
                        case 0:
                            {
                                auto* fn = (void (*) (Variable&)) vfptr;

                                Variable v;
                                fn(v);
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 1:
                            {
                                TRACE_STACK(1);
                                Variable v;
                                Variable& arg0 = staticExecutor.peekRef(0);
                                auto* fn = (void (*) (Variable&, const Variable&)) vfptr;
                                fn(v, arg0);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 2:
                            {
                                TRACE_STACK(2);
                                Variable v;
                                Variable& arg1 = staticExecutor.peekRef(0);
                                Variable& arg0 = staticExecutor.peekRef(1);
                                auto* fn = (void (*) (Variable&, const Variable&, const Variable&)) vfptr;
                                fn(v, arg0, arg1);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 3:
                            {
                                TRACE_STACK(3);
                                Variable v;
                                Variable& arg2 = staticExecutor.peekRef(0);
                                Variable& arg1 = staticExecutor.peekRef(1);
                                Variable& arg0 = staticExecutor.peekRef(2);
                                auto* fn = (void (*) (Variable&, const Variable&, const Variable&, const Variable&)) vfptr;
                                fn(v, arg0, arg1, arg2);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 4:
                            {
                                TRACE_STACK(4);
                                Variable v;
                                Variable& arg3 = staticExecutor.peekRef(0);
                                Variable& arg2 = staticExecutor.peekRef(1);
                                Variable& arg1 = staticExecutor.peekRef(2);
                                Variable& arg0 = staticExecutor.peekRef(3);
                                auto* fn = (void (*) (Variable&, const Variable&, const Variable&, const Variable&, const Variable&)) vfptr;
                                fn(v, arg0, arg1, arg2, arg3);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 5:
                            {
                                TRACE_STACK(5);
                                Variable v;
                                Variable& arg4 = staticExecutor.peekRef(0);
                                Variable& arg3 = staticExecutor.peekRef(1);
                                Variable& arg2 = staticExecutor.peekRef(2);
                                Variable& arg1 = staticExecutor.peekRef(3);
                                Variable& arg0 = staticExecutor.peekRef(4);
                                auto* fn = (void (*) (Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&)) vfptr;
                                fn(v, arg0, arg1, arg2, arg3, arg4);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 6:
                            {
                                TRACE_STACK(6);
                                Variable v;
                                Variable& arg5 = staticExecutor.peekRef(0);
                                Variable& arg4 = staticExecutor.peekRef(1);
                                Variable& arg3 = staticExecutor.peekRef(2);
                                Variable& arg2 = staticExecutor.peekRef(3);
                                Variable& arg1 = staticExecutor.peekRef(4);
                                Variable& arg0 = staticExecutor.peekRef(5);
                                auto* fn = (void (*) (Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&)) vfptr;
                                fn(v, arg0, arg1, arg2, arg3, arg4, arg5);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 7:
                            {
                                TRACE_STACK(7);
                                Variable v;
                                Variable& arg6 = staticExecutor.peekRef(0);
                                Variable& arg5 = staticExecutor.peekRef(1);
                                Variable& arg4 = staticExecutor.peekRef(2);
                                Variable& arg3 = staticExecutor.peekRef(3);
                                Variable& arg2 = staticExecutor.peekRef(4);
                                Variable& arg1 = staticExecutor.peekRef(5);
                                Variable& arg0 = staticExecutor.peekRef(6);
                                auto* fn = (void (*) (Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&)) vfptr;
                                fn(v, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 8:
                            {
                                TRACE_STACK(8);
                                Variable v;
                                Variable& arg7 = staticExecutor.peekRef(0);
                                Variable& arg6 = staticExecutor.peekRef(1);
                                Variable& arg5 = staticExecutor.peekRef(2);
                                Variable& arg4 = staticExecutor.peekRef(3);
                                Variable& arg3 = staticExecutor.peekRef(4);
                                Variable& arg2 = staticExecutor.peekRef(5);
                                Variable& arg1 = staticExecutor.peekRef(6);
                                Variable& arg0 = staticExecutor.peekRef(7);
                                auto* fn = (void (*) (Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&)) vfptr;
                                fn(v, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 9:
                            {
                                TRACE_STACK(9);
                                Variable v;
                                Variable& arg8 = staticExecutor.peekRef(0);
                                Variable& arg7 = staticExecutor.peekRef(1);
                                Variable& arg6 = staticExecutor.peekRef(2);
                                Variable& arg5 = staticExecutor.peekRef(3);
                                Variable& arg4 = staticExecutor.peekRef(4);
                                Variable& arg3 = staticExecutor.peekRef(5);
                                Variable& arg2 = staticExecutor.peekRef(6);
                                Variable& arg1 = staticExecutor.peekRef(7);
                                Variable& arg0 = staticExecutor.peekRef(8);
                                auto* fn = (void (*) (Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&)) vfptr;
                                fn(v, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 10:
                            {
                                TRACE_STACK(10);
                                Variable v;
                                Variable& arg9 = staticExecutor.peekRef(0);
                                Variable& arg8 = staticExecutor.peekRef(1);
                                Variable& arg7 = staticExecutor.peekRef(2);
                                Variable& arg6 = staticExecutor.peekRef(3);
                                Variable& arg5 = staticExecutor.peekRef(4);
                                Variable& arg4 = staticExecutor.peekRef(5);
                                Variable& arg3 = staticExecutor.peekRef(6);
                                Variable& arg2 = staticExecutor.peekRef(7);
                                Variable& arg1 = staticExecutor.peekRef(8);
                                Variable& arg0 = staticExecutor.peekRef(9);
                                auto* fn = (void (*) (Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&)) vfptr;
                                fn(v, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 11:
                            {
                                TRACE_STACK(11);
                                Variable v;
                                Variable& arg10 = staticExecutor.peekRef(0);
                                Variable& arg9 = staticExecutor.peekRef(1);
                                Variable& arg8 = staticExecutor.peekRef(2);
                                Variable& arg7 = staticExecutor.peekRef(3);
                                Variable& arg6 = staticExecutor.peekRef(4);
                                Variable& arg5 = staticExecutor.peekRef(5);
                                Variable& arg4 = staticExecutor.peekRef(6);
                                Variable& arg3 = staticExecutor.peekRef(7);
                                Variable& arg2 = staticExecutor.peekRef(8);
                                Variable& arg1 = staticExecutor.peekRef(9);
                                Variable& arg0 = staticExecutor.peekRef(10);
                                auto* fn = (void (*) (Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&)) vfptr;
                                fn(v, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 12:
                            {
                                TRACE_STACK(12);
                                Variable v;
                                Variable& arg11 = staticExecutor.peekRef(0);
                                Variable& arg10 = staticExecutor.peekRef(1);
                                Variable& arg9 = staticExecutor.peekRef(2);
                                Variable& arg8 = staticExecutor.peekRef(3);
                                Variable& arg7 = staticExecutor.peekRef(4);
                                Variable& arg6 = staticExecutor.peekRef(5);
                                Variable& arg5 = staticExecutor.peekRef(6);
                                Variable& arg4 = staticExecutor.peekRef(7);
                                Variable& arg3 = staticExecutor.peekRef(8);
                                Variable& arg2 = staticExecutor.peekRef(9);
                                Variable& arg1 = staticExecutor.peekRef(10);
                                Variable& arg0 = staticExecutor.peekRef(11);
                                auto* fn = (void (*) (Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&)) vfptr;
                                fn(v, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 13:
                            {
                                TRACE_STACK(13);
                                Variable v;
                                Variable& arg12 = staticExecutor.peekRef(0);
                                Variable& arg11 = staticExecutor.peekRef(1);
                                Variable& arg10 = staticExecutor.peekRef(2);
                                Variable& arg9 = staticExecutor.peekRef(3);
                                Variable& arg8 = staticExecutor.peekRef(4);
                                Variable& arg7 = staticExecutor.peekRef(5);
                                Variable& arg6 = staticExecutor.peekRef(6);
                                Variable& arg5 = staticExecutor.peekRef(7);
                                Variable& arg4 = staticExecutor.peekRef(8);
                                Variable& arg3 = staticExecutor.peekRef(9);
                                Variable& arg2 = staticExecutor.peekRef(10);
                                Variable& arg1 = staticExecutor.peekRef(11);
                                Variable& arg0 = staticExecutor.peekRef(12);
                                auto* fn = (void (*) (Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&)) vfptr;
                                fn(v, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 14:
                            {
                                TRACE_STACK(14);
                                Variable v;
                                Variable& arg13 = staticExecutor.peekRef(0);
                                Variable& arg12 = staticExecutor.peekRef(1);
                                Variable& arg11 = staticExecutor.peekRef(2);
                                Variable& arg10 = staticExecutor.peekRef(3);
                                Variable& arg9 = staticExecutor.peekRef(4);
                                Variable& arg8 = staticExecutor.peekRef(5);
                                Variable& arg7 = staticExecutor.peekRef(6);
                                Variable& arg6 = staticExecutor.peekRef(7);
                                Variable& arg5 = staticExecutor.peekRef(8);
                                Variable& arg4 = staticExecutor.peekRef(9);
                                Variable& arg3 = staticExecutor.peekRef(10);
                                Variable& arg2 = staticExecutor.peekRef(11);
                                Variable& arg1 = staticExecutor.peekRef(12);
                                Variable& arg0 = staticExecutor.peekRef(13);
                                auto* fn = (void (*) (Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&)) vfptr;
                                fn(v, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 15:
                            {
                                TRACE_STACK(15);
                                Variable v;
                                Variable& arg14 = staticExecutor.peekRef(0);
                                Variable& arg13 = staticExecutor.peekRef(1);
                                Variable& arg12 = staticExecutor.peekRef(2);
                                Variable& arg11 = staticExecutor.peekRef(3);
                                Variable& arg10 = staticExecutor.peekRef(4);
                                Variable& arg9 = staticExecutor.peekRef(5);
                                Variable& arg8 = staticExecutor.peekRef(6);
                                Variable& arg7 = staticExecutor.peekRef(7);
                                Variable& arg6 = staticExecutor.peekRef(8);
                                Variable& arg5 = staticExecutor.peekRef(9);
                                Variable& arg4 = staticExecutor.peekRef(10);
                                Variable& arg3 = staticExecutor.peekRef(11);
                                Variable& arg2 = staticExecutor.peekRef(12);
                                Variable& arg1 = staticExecutor.peekRef(13);
                                Variable& arg0 = staticExecutor.peekRef(14);
                                auto* fn = (void (*) (Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&)) vfptr;
                                fn(v, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case 16:
                            {
                                TRACE_STACK(16);
                                Variable v;
                                Variable& arg15 = staticExecutor.peekRef(0);
                                Variable& arg14 = staticExecutor.peekRef(1);
                                Variable& arg13 = staticExecutor.peekRef(2);
                                Variable& arg12 = staticExecutor.peekRef(3);
                                Variable& arg11 = staticExecutor.peekRef(4);
                                Variable& arg10 = staticExecutor.peekRef(5);
                                Variable& arg9 = staticExecutor.peekRef(6);
                                Variable& arg8 = staticExecutor.peekRef(7);
                                Variable& arg7 = staticExecutor.peekRef(8);
                                Variable& arg6 = staticExecutor.peekRef(9);
                                Variable& arg5 = staticExecutor.peekRef(10);
                                Variable& arg4 = staticExecutor.peekRef(11);
                                Variable& arg3 = staticExecutor.peekRef(12);
                                Variable& arg2 = staticExecutor.peekRef(13);
                                Variable& arg1 = staticExecutor.peekRef(14);
                                Variable& arg0 = staticExecutor.peekRef(15);
                                auto* fn = (void (*) (Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&, const Variable&)) vfptr;
                                fn(v, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15);
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.popRef().cleanup();
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        case -2: // 1 argument, void.
                            {
                                TRACE_STACK(1);
                                Variable& arg0 = staticExecutor.peekRef();
                                auto* fn = (void (*) (const Variable&)) vfptr;
                                fn(arg0);
                                staticExecutor.popRef().cleanup();
                            }
                            break;
                        case -1: // variadic
                            {
                                Variable v;
                                uint8_t argc_supplied = staticExecutor.popRef().get<int32_t>();
                                TRACE_STACK(argc_supplied);
                                Variable* argv = nullptr;
                                if (argc_supplied > 0)
                                {
                                    argv = &(staticExecutor.peekRef(argc_supplied - 1));
                                }
                                auto* fn = (void (*) (Variable&, unsigned char, const Variable*)) vfptr;
                                fn(v, argc_supplied, argv);
                                for (size_t i = 0; i < argc_supplied; i++)
                                {
                                    staticExecutor.popRef().cleanup();
                                }
                                staticExecutor.pushRef() = std::move(v);
                            }
                            break;
                        default:
                            throw LanguageFeatureNotImplementedError("too many arguments for native function call.");
                    }
                }
                break;
            case wti:
                {
                    // pop what to iterate over.
                    Variable& v = staticExecutor.popRef();
                    ex_instance_id_t id = v.castCoerce<ex_instance_id_t>();
                    v.cleanup();

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 1);

                    // push context.
                    size_t iterator_index = staticExecutor.m_with_iterators.size();
                    staticExecutor.m_with_iterators.emplace_back();
                    WithIterator& withIterator = staticExecutor.m_with_iterators.back();
                    staticExecutor.m_frame.get_instance_iterator(id, withIterator, staticExecutor.m_self, staticExecutor.m_other);
                    staticExecutor.pushSelf(nullptr);
                    staticExecutor.pushRef() = static_cast<uint64_t>(iterator_index);
                    TRACE(staticExecutor.peekRef());

                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex + 1);
                }
                break;
            case wty:
                {
                    size_t iterator_index;
					iterator_index = staticExecutor.peekRef().get<uint64_t>();
                    WithIterator& withIterator = staticExecutor.m_with_iterators.at(iterator_index);
                    if (withIterator.complete())
                    {
                        // no need to clean up -- known statically to be size_t
                        staticExecutor.pop();
                        staticExecutor.popSelf();
                        staticExecutor.m_with_iterators.pop_back();
                        staticExecutor.m_statusCond = true;
                        TRACE("(Complete.)");
                        ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 2);
                    }
                    else
                    {
                        // set self
                        staticExecutor.m_self = *withIterator;
                        staticExecutor.m_statusCond = false;
                        ++withIterator;
                        TRACE(staticExecutor.peekRef());
                        ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex);
                    }
                }
                break;
            case wtd:
                {
                    staticExecutor.pop();
                    staticExecutor.popSelf();
                    staticExecutor.m_with_iterators.pop_back();
                    ogm_assert(staticExecutor.m_varStackIndex == op_pre_varStackIndex - 2);
                }
                break;
            case jmp:
                {
                    bytecode_address_t address;
                    read(in, address);
                    in.seekg(address);
                }
                break;
            case bcond:
                {
                    bytecode_address_t address;
                    read(in, address);
                    if (staticExecutor.m_statusCond)
                    {
                        in.seekg(address);
                        TRACE("Branch taken.");
                    }
                    else
                    {
                        TRACE("Ignored.");
                    }
                }
                break;
            case call: // call another bytecode section
                {
                    bytecode_index_t index;
                    uint8_t argc;
                    read(in, index);
                    read(in, argc);
                    TRACE_STACK(argc);
                    Bytecode bytecode = staticExecutor.m_frame.m_bytecode.get_bytecode(index);
                    ogm_assert(bytecode.m_argc == static_cast<decltype(bytecode.m_argc)>(-1) || bytecode.m_argc == argc);

                    // push number of arguments onto stack
                    staticExecutor.pushRef() = argc;

                    // TODO: change this to not be recursive, but rather to use the
                    // interpreted stack instead.
                    bool suspended = execute_bytecode(index, true);

                    // FIXME: how to propagate a suspension upward preserving the stack?
                    ogm_assert(!suspended);

                    // argc, argv popped by callee.
                    ogm_assert(op_pre_varStackIndex + bytecode.m_retc - argc == staticExecutor.m_varStackIndex);
                }
                break;
            case ret:
                {
                    // save the most recent n variables on the stack
                    uint8_t count;
                    read(in, count);
                    staticExecutor.m_varStackIndex -= count;
                    // start of return values
                    size_t retsrc = staticExecutor.m_varStackIndex + 1;

                    // pop local variables off of stack.
                    while (staticExecutor.get_sp() >= staticExecutor.m_locals_start)
                    {
                        staticExecutor.popRef().cleanup();
                    }

                    // reset stack pointer to that of caller.
                    staticExecutor.m_locals_start = staticExecutor.popRef().get<uint64_t>();

                    ogm_assert(staticExecutor.m_varStackIndex == pre_varStackIndex);

                    // pop argc, argv
                    uint8_t argc = staticExecutor.popRef().castExact<size_t>();
                    ogm_assert(argc < 256); // assertion can be removed if this turns out to be a problem
                    for (size_t i = 0; i < argc; ++i)
                    {
                        staticExecutor.popRef().cleanup();
                    }

                    // copy the return values to the head of the stack.
                    for (size_t i = 0; i < count; i++)
                    {
                        staticExecutor.pushRef() = std::move(staticExecutor.m_varStack[retsrc + i]);
                    }

                    // pop the return address from the RA stack.
                    staticExecutor.m_pc = staticExecutor.m_return_addresses.back();
                    staticExecutor.m_return_addresses.pop_back();

                    // return
                    return false;
                }
                break;
            case sus:
                {
                    // suspend execution.
                    // since pc is set, can be returned to later.
                    // FIXME: this generally only works from the top-level call, because
                    // 'return' brings us up to the above bytecode execute loop.
                    return true;
                }
            case eof:
                {
                    throw MiscError("Execution reached end of unit");
                }
                break;
            case nop:
                break;
            default:
                throw MiscError(std::string("Attempted to execute bytecode command which is not implemented: ") + std::string(get_opcode_string(op)));
            }
        }
        catch (const ExceptionTrace& e)
        {
            throw e;
        }
        catch (const std::exception& e)
        {
            if (debug && staticExecutor.m_debugger && staticExecutor.m_debugger->execution_is_inline())
            {
                // let the inline execution caller handle this.
                throw e;
            }

            // stack trace is all return addresses + current pc:
            std::vector<BytecodeStream> ra_stack = staticExecutor.m_return_addresses;
            ra_stack.erase(ra_stack.begin(), ra_stack.begin() + 1); // remove meaningless first value in ra stack.
            ra_stack.push_back(staticExecutor.m_pc);

            // wrap exception.
            ExceptionTrace trace{
                e,
                std::move(ra_stack)
            };
            if (debug)
            {
                std::cout << trace.what() << std::endl << std::endl;
                if (staticExecutor.m_debugger)
                {
                    std::cout << "Entering debug mode.\n(Program will terminate at the next cycle.)\n";
                    staticExecutor.m_debugger->break_execution();

                    // allow poking around.
                    staticExecutor.m_debugger->tick(in);
                }
            }

            // does not throw a std::exception, so not caught by
            // this catch higher up.
            throw trace;
        }
    }
}

// executes bytecode, either in debug mode or not.
// sets program counter to the provided bytecode.
template<bool debug>
bool execute_bytecode_helper(bytecode::Bytecode bytecode)
{
    ogm_assert(staticExecutor.m_self != nullptr);
    ogm_assert(!!staticExecutor.m_debugger == debug);
    using namespace opcode;

    // alias the program counter for ease of use in this function.

    // push program counter onto the return address stack.
    staticExecutor.m_return_addresses.push_back(staticExecutor.m_pc);
    // set the program counter to the input bytecode.
    staticExecutor.m_pc = { bytecode };
    return execute_bytecode_loop<debug>();
}
}

bool execute_bytecode(bytecode::Bytecode bytecode, bool args)
{
    if (!args)
    {
        // bytecode execution expects the number of arguments to be on the stack
        // so if no args are provided then we must at least pass "zero."

        staticExecutor.pushRef() = 0;
    }

    if (staticExecutor.m_debugger)
    {
        return execute_bytecode_helper<true>(bytecode);
    }
    else
    {
        return execute_bytecode_helper<false>(bytecode);
    }
}

bool execute_resume()
{
    if (staticExecutor.m_debugger)
    {
        return execute_bytecode_loop<true>();
    }
    else
    {
        return execute_bytecode_loop<false>();
    }
}

bool execute_bytecode(bytecode_index_t bytecode_index, bool args)
{
    return execute_bytecode(staticExecutor.m_frame.m_bytecode.get_bytecode(bytecode_index), args);
}

}}
