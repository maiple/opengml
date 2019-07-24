#pragma once

#include "ogm/bytecode/BytecodeTable.hpp"

#include <vector>
#include <string>
#include <fstream>

namespace ogm { namespace interpreter
{

class Debugger
{
private:
    // if false, will continue until break.
    bool m_paused = true;

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

public:
    struct {
        bool m_show_asm = false;
        bool m_trace = false;
        std::string m_trace_path = "";
        uint32_t m_trace_stack_count = 0;
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

    void trace_addendum(std::string message);

    void on_attach();

    void on_detach();

    // pauses debugger on next bytecode executed.
    void break_execution();

private:
    void tick_trace(ogm::bytecode::BytecodeStream& in);

    void parse_command(DebuggerCommand& out_command, std::string input);

    // extracts the given line from the given string
    std::string source_line(const char* source, size_t line);

    // produces a string like "fn: line 15, pc 23" for a given bytecode stream.
    std::string location_for_bytecode_stream(const ogm::bytecode::BytecodeStream&, bool show_asm, bool show_source=false);

    void cmd_help(std::string command);

    void cmd_print(std::string expression);

    void cmd_info(std::string expression);

    void cmd_set(const std::vector<std::string>& arguments);

    void cmd_info_instance();

    void cmd_info_locals();

    void cmd_info_stack();
};

}}
