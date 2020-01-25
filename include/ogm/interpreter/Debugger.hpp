#pragma once

#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/interpreter/Variable.hpp"

#include <vector>
#include <string>
#include <fstream>

// forward declarations
namespace ogm { namespace bytecode {
    struct DisassembledBytecodeInstruction;
}

}

namespace ogm { namespace interpreter
{

struct SuspendPoint
{
    enum {
        DELETED = 0,
        BREAK,
        WATCH
    } m_type;

    bool m_active = true; // enabled/disabled

    bool m_temporary = false;

    struct BreakPoint
    {
        ogm::bytecode::BytecodeStream m_pc;
    };

    struct WatchPoint
    {
        std::string m_description;
        bool m_is_location;

        // expression watchpoints
        bool m_scoped;
        ogm::bytecode::Bytecode m_expression_scope;
        ogm::bytecode::Bytecode m_expression;
        ogm::bytecode::BytecodeStream m_last_evaluation;
        Variable m_current_value;
        int64_t m_eval_ticks;
        int64_t m_eval_interval;

        ~WatchPoint()
        {
            m_current_value.cleanup();
        }
    };

    union
    {
        BreakPoint* m_breakpoint;
        WatchPoint* m_watchpoint;
    };

    bool m_conditional = false;
    std::string m_condition_str;
    ogm::bytecode::Bytecode m_condition_bc;

    void set_deleted()
    {
        switch(m_type)
        {
        case BREAK:
            delete m_breakpoint;
            m_breakpoint = nullptr;
            break;
        case WATCH:
            delete m_watchpoint;
            m_watchpoint = nullptr;
            break;
        default:
            break;
        }
        m_type = DELETED;
    }
};

class Debugger
{
private:
    // if false, will continue until break.
    bool m_paused = false;

    // show the startup spiel
    bool m_spiel = true;

    // stores previous command so that blank lines can re-run previous command
    std::string m_prev_input;

    // intention with regards to linestepping
    enum
    {
        LS_NONE, // don't stop unless a breakpoint is encountered.
        LS_IN,   // continue until the next line is reached in any frame.
        LS_OVER, // continue to next line in same frame or above.
        LS_OUT,  // continue until the stack depth decreases.
        LS_FINISH,  // continue until the stack depth decreases OR frame changes at the same depth.

        // instruction-level versions of the above.
        LS_I_IN,
        LS_I_OVER,
        LS_I_OUT,
        LS_I_FINISH,
    } m_ls_state = LS_NONE;

    std::vector<SuspendPoint> m_suspend_points;

    // previous program counter info for the purposes of linestepping (commands s, n, f).
    size_t m_ls_prev_address;

    // previous stack frame index for the purposes of linestepping (commands s, n, f).
    size_t m_ls_prev_frame;

    // the bytecode section pointer for the purposses of linestepping (commands s, n, f)
    const void* m_ls_prev_section_data;

    // previous bytecode data for the purposes of tracing
    size_t m_trace_prev_frame;
    const void* m_trace_prev_section_data;
    std::ofstream* m_trace_ofstream = nullptr;

    // displayed after the next trace statement
    std::string m_trace_addendum;

    // delete self on detach
    bool m_destroy_on_detach;

    // inline execution currently in progress
    bool m_executing_inline = false;

    struct StackFrame
    {
        // index in staticExecutor.m_varStackIndex
        size_t m_locals_start;

        // index in staticExecutor.m_return_addresses
        size_t m_return_address_index;

        StackFrame(size_t locals_start, size_t return_address_index)
            : m_locals_start(locals_start)
            , m_return_address_index(return_address_index)
        { }
    };

    // stack frames cache
    // most recent first
    std::vector<StackFrame> m_frames;
    bool m_stack_broken = false;
    size_t m_current_frame = 0;

public:
    struct {
        bool m_show_asm = false;
        bool m_trace = false;
        std::string m_trace_path = "";
        uint32_t m_trace_stack_count = 0;
        bool m_trace_only_section_names = false;
        bool m_trace_addendums = false;
    } m_config;

public:
    struct DebuggerCommand
    {
    public:
        std::string m_command;
        std::vector<std::string> m_arguments;
        std::string m_arguments_str;
    };

    Debugger(bool destroy_on_detach=false)
        : m_destroy_on_detach(destroy_on_detach)
    { }

    // executes once per update
    void tick(ogm::bytecode::BytecodeStream&);

    void tick_suspendpoints(ogm::bytecode::BytecodeStream&);

    void trace_addendum(const std::string& message);

    void on_attach();

    void on_detach();

    // pauses debugger on next bytecode executed.
    void break_execution();

    bool execution_is_inline()
    {
        return m_executing_inline;
    }

private:
    void tick_trace(ogm::bytecode::BytecodeStream& in);

    void parse_command(DebuggerCommand& out_command, std::string input);

    // extracts the given line from the given string
    std::string source_line(const char* source, size_t line);

    // produces a string like "fn: line 15, pc 23" for a given bytecode stream.
    std::string location_for_bytecode_stream(const ogm::bytecode::BytecodeStream& bs, bool show_asm, bool show_source=false);

    std::string list_source(ogm::bytecode::Bytecode& bc, size_t range_min, size_t range_max);

    std::string list_instructions(ogm::bytecode::Bytecode& bc, size_t range_min, size_t range_max, std::vector<bytecode::DisassembledBytecodeInstruction>* precalc_dis = nullptr);

    // executes the given string immediately
    void string_execute_inline(std::string code);

    // compile code for running inline the given host bytecode (respecting the same variable names)
    bytecode::Bytecode compile_inline(std::string code, const bytecode::Bytecode& host, uint8_t retc=0, bool unscoped=false);

    void execute_inline(const bytecode::Bytecode&, interpreter::Variable* outv=nullptr);

    void cmd_help(std::string command);

    void cmd_print(std::string expression);

    void cmd_info(std::string expression);

    bool cmd_break(const std::vector<std::string>& arguments, bool temporary);

    bool cmd_watch(const std::vector<std::string>& arguments, bool temporary=false);

    void cmd_enable(const std::vector<std::string>& arguments, int32_t mode=2); // mode: 0, delete. 1: disable. 2: enable

    void cmd_set(const std::vector<std::string>& arguments);

    void cmd_info_instance();

    void cmd_info_locals();

    void cmd_info_stack();

    void cmd_info_suspendpoints();

    // list information about a particular datastructure instance
    void cmd_info_ds(const std::vector<std::string>& args);

    // list information about a buffer
    void cmd_info_buffer(const std::vector<std::string>& args);

    void cmd_list(const std::vector<std::string>* arguments, bool instruction);

    void cmd_frame_shift(int32_t to);

    void set_tracing_addendums();

    void cache_stack_frames();

    ogm::bytecode::BytecodeStream& get_frame_pc(size_t frame);
    ogm::bytecode::BytecodeStream& get_frame_pc();

public: // implementation use only.
    char** autocomplete(const char* text, int start, int end);
};

}}
