#pragma once

#include "ogm/interpreter/Variable.hpp"
#include "ogm/interpreter/Instance.hpp"
#include "Frame.hpp"

#include "ogm/bytecode/Namespace.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/interpreter/serialize.hpp"

#include <climits>
#include <cstddef>

#ifdef __GNUC__
#pragma GCC push_options
#pragma GCC optimize ("Ofast")
#endif

#define VARSTACK_SIZE ((1 << 24) / sizeof(ogm::interpreter::Variable))
#define STACKFRAME_COUNT (1 << 18) / sizeof(bytecode::BytecodeStream)

namespace ogm { namespace interpreter
{
    class Debugger;

    template<bool write>
    void __serialize(
        typename state_stream<write>::state_stream_t& s, Bytecode& bs
    );

    template<bool write>
    void __serialize(
        typename state_stream<write>::state_stream_t& s, BytecodeStream& bs
    )
    {
        __serialize<write>(s, bs.m_bytecode);
        _serialize<write>(s, bs.m_pos);
    }

    class Executor
    {
    public:
        // points to the most recently-added variable (NOT +1.)
        size_t m_varStackIndex = 0;
        ogm::interpreter::Variable m_varStack[VARSTACK_SIZE];

        // where this stack frame begins in the variable stack
        size_t m_locals_start = 0;

        // program counter (bytecode and current line).
        BytecodeStream m_pc;

        // stack of return addresses.
        std::vector<BytecodeStream> m_return_addresses;

        // condition flag (branches should be taken.)
        bool m_statusCond = false;

        // copy-on-write flag (for arrays)
        // conceptually, the 'x' flag is !m_statusCOW.
        bool m_statusCOW = true;

        Instance* m_other = nullptr;
        Instance* m_self = nullptr;

        // the stack of currently-in-use "with" iterators.
        std::vector<WithIterator> m_with_iterators;

        // library of functions
        // required to compile additional code and to disassemble existing code.
        const ogm::bytecode::Library* m_library = nullptr;

        // global data beyond the immediate/stack context.
        // (may eventually become a pointer.
        // currently it is a non-pointer member only for (premature)
        // optimization reasons.)
        Frame m_frame;

        // if this is set, execute in debug mode.
        Debugger* m_debugger = nullptr;

    public:
        inline ogm::interpreter::Variable
        pop()
        {
            return std::move(m_varStack[m_varStackIndex--]);
        }

        inline void
        push(ogm::interpreter::Variable&& v)
        {
            m_varStack[++m_varStackIndex] = std::move(v);
        }

        inline ogm::interpreter::Variable&
        popRef()
        {
            ogm_assert(m_varStackIndex > 0);
            return m_varStack[m_varStackIndex--];
        }

        inline ogm::interpreter::Variable&
        popRef(size_t count)
        {
            ogm_assert(m_varStackIndex >= count);
            m_varStackIndex -= count;
            return m_varStack[m_varStackIndex];
        }

        inline ogm::interpreter::Variable&
        pushRef()
        {
            return m_varStack[++m_varStackIndex];
        }

        inline ogm::interpreter::Variable&
        peekRef(size_t offset = 0)
        {
            return m_varStack[m_varStackIndex - offset];
        }

        inline void pushSelf(Instance* self)
        {
            pushRef() = (void*)m_other;
            m_other = m_self;
            m_self = self;
        }

        inline Instance* popSelf()
        {
            Instance* self = m_self;
            m_self = m_other;
            // no need to clean up the pointer.
            m_other = static_cast<Instance*>(popRef().get<void*>());
            return self;
        }

        // as above, but pushes twice
        inline void pushSelfDouble(Instance* self)
        {
            pushSelf(self);
            pushSelf(self);
        }

        // pops self twice. Only to be used to invert pushSelfDouble.
        inline Instance* popSelfDouble()
        {
            Instance* a = popSelf();
            Instance* b = popSelf();
            ogm_assert(a == b);
            return a;
        }

        // gets a reference to the given local
        inline Variable& local(size_t id)
        {
            return m_varStack[m_locals_start + id];
        }

        // gets a reference to a variable stored before the locals on the stack
        // 0 is the first variable before the locals, etc.
        inline Variable& prelocal(size_t id)
        {
            ogm_assert(m_locals_start > id + 1);
            return m_varStack[m_locals_start - id - 1];
        }

        // retrieves stack pointer
        inline size_t get_sp() const
        {
            return m_varStackIndex;
        }

        std::string print_stack()
        {
            std::stringstream ss;
            for (size_t i = 0; i <= m_varStackIndex; i++)
            {
                if (i > 0)
                {
                    ss << " ";
                }
                ss << m_varStack[i];
            }
            return ss.str();
        }

        template<bool write>
        void serialize(typename state_stream<write>::state_stream_t& s)
        {
            ogm_assert(s.good());
            if (!write)
                ogm_assert(!s.eof());

            // we don't actually need to serialize the stack
            #if 0

            // it's best to serialize this when the stack is shallow.
            _serialize<write>(s, m_varStackIndex);
            for (size_t i = 0; i <= m_varStackIndex; ++i)
            {
                _serialize<write>(s, m_varStack[i]);
            }
            _serialize<write>(s, m_locals_start);

            // TODO: more efficiently pack bits?
            _serialize<write>(s, m_statusCond);
            _serialize<write>(s, m_statusCOW);

            auto bytecode_to_index = [this](const BytecodeStream& b, std::pair<size_t, bytecode_index_t>& out)
            {
                bytecode_index_t bi;
                if (!this->m_frame.m_bytecode.get_bytecode_index(b.m_bytecode, bi))
                {
                    if (b.m_bytecode.m_data == nullptr)
                    {
                        bi = k_no_bytecode;
                    }
                    else
                    {
                        throw MiscError("Failed to serialize bytecode.");
                    }
                }
                out = {
                    b.m_pos,
                    bi
                };
            };

            auto index_to_bytecode = [this](const std::pair<size_t, bytecode_index_t>& p, BytecodeStream& out)
            {
                Bytecode b;
                if (p.second == k_no_bytecode)
                {
                    b.m_data = nullptr;
                    b.m_length = 0;
                    b.m_retc = 0;
                    b.m_argc = 0;
                    b.m_debug_symbols = nullptr;
                }
                else
                {
                    b = this->m_frame.m_bytecode.get_bytecode(p.second);
                }
                out = { b, p.first };
            };
            #endif

            // We don't serialize the program counter because we want to be able
            // to serialize/deserialize from different points in the code.
            #if 0
            __serialize<write>(s, m_pc);
            _serialize_vector_map<write, std::pair<size_t, bytecode_index_t>>(s, m_return_addresses, bytecode_to_index, index_to_bytecode);
            #endif

            // TODO: haven't implemented with-iterator serialization yet because
            // serialization is intended to be called from a shallow stack anyway.
            ogm_assert(m_with_iterators.size() == 0);

            // not serializing m_library.

            m_frame.serialize<write>(s);

            ogm_assert(s.good());

            // *m_debugger does not get to be serialized.
        }

        void debugger_attach(Debugger* d);
        void debugger_detach();

        std::string stack_trace() const;
        
        // note that this does not cleanup stack variables.
        void reset()
        {
            m_varStackIndex = 0;
            m_locals_start = 0;
            m_pc = {};
            m_return_addresses.clear();
            m_statusCond = false;
            m_statusCOW = true;
            m_other = nullptr;
            m_self = nullptr;
            m_with_iterators.clear();
            m_library = nullptr;
            m_frame.reset_hard();
        }
    };

    // executor used for executing all bytecode.
    extern Executor staticExecutor;

    std::string stack_trace(const std::vector<bytecode::BytecodeStream>&);

    // wraps an exception
    struct ExceptionTrace : public std::exception
    {
        // could be nullptr
        std::string m_what;
        // lower indices are deeper.
        // stores the bytecode address *just after* the exception-throwing one
        // stores return addresses, which are *just after* the call.
        std::vector<bytecode::BytecodeStream> m_pc_stack;

        ExceptionTrace(
            const std::exception& e,
            std::vector<bytecode::BytecodeStream> && stack
        )
            : m_pc_stack(stack)
        {
            std::string exception_name = typeid(e).name();
            m_what = std::string("An exception `") + pretty_typeid(exception_name) + "` was thrown:\n" + e.what() + "\n";
            m_what += stack_trace(stack);
        }

        const char* what() const noexcept override
        {
            return m_what.c_str();
        }
    };

    // (un)swizzles by looking up bytecode in staticExecutor.m_frame->m_bytecode.
    template<bool write>
    void __serialize(
        typename state_stream<write>::state_stream_t& s, Bytecode& bc
    )
    {
        bytecode_index_t index;
        BytecodeTable& bt = staticExecutor.m_frame.m_bytecode;
        if (write)
        {
            // look up bytecode_index
            if (!bt.get_bytecode_index(bc, index))
            {
                ogm_assert(false); // failed to find bytecode index.
            }
            _serialize<write>(s, index);
        }
        else
        {
            _serialize<write>(s, index);
            bc = bt.get_bytecode(index);
        }
    }
}}

#ifdef __GNUC__
#pragma GCC pop_options
#endif
