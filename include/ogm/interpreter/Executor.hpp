#pragma once

#include "ogm/interpreter/Variable.hpp"
#include "ogm/interpreter/Instance.hpp"
#include "Frame.hpp"

#include "ogm/bytecode/Namespace.hpp"

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

    class Executor
    {
    public:
        // points to the most recently-added variable (NOT +1.)
        size_t m_varStackIndex = 0;
        ogm::interpreter::Variable m_varStack[VARSTACK_SIZE];

        // program counter (bytecode and current line).
        BytecodeStream m_pc;

        // stack of return addresses.
        std::vector<BytecodeStream> m_return_addresses;

        // condition flag
        bool m_statusCond = false;

        // copy-on-write flag (for arrays)
        // conceptually, the 'x' flag is !m_statusCOW.
        bool m_statusCOW = true;

        // where this stack entry begins in the variable stack
        size_t m_locals_start = 0;

        Instance* m_other = nullptr;
        Instance* m_self = nullptr;

        // the stack of currently-in-use "with" iterators.
        std::vector<WithIterator> m_with_iterators;

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
            return m_varStack[m_varStackIndex--];
        }

        inline ogm::interpreter::Variable&
        popRef(size_t count)
        {
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

        // gets a reference to the given local
        inline Variable& local(size_t id)
        {
            return m_varStack[m_locals_start + id];
        }

        // gets a reference to a variable stored before the locals on the stack
        // 0 is the first variable before the locals, etc.
        inline Variable& prelocal(size_t id)
        {
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

        void debugger_attach(Debugger* d);
        void debugger_detach();
    };

    // executor used for executing all bytecode.
    extern Executor staticExecutor;
}}

#ifdef __GNUC__
#pragma GCC pop_options
#endif
