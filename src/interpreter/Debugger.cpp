#include "ogm/interpreter/Debugger.hpp"

#include "ogm/interpreter/StandardLibrary.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "./input.hpp"

#include "ogm/ast/parse.h"
#include "ogm/bytecode/bytecode.hpp"

#include <iostream>
#include <iomanip>
#include <csignal>

namespace ogm { namespace interpreter
{
using namespace ogm;


namespace
{
// expands condensed command such as "p" to full command "print"
std::string expand_command(std::string command)
{
#define EXPAND(s, full) if (command == s) return full;
    EXPAND("p", "print");
    EXPAND("i", "info");
    EXPAND("bt", "backtrace");
    EXPAND("n", "next");
    EXPAND("ni", "next-instruction");
    EXPAND("s", "step");
    EXPAND("si", "step-instruction");
    EXPAND("c", "continue");
    EXPAND("r", "restart");
    EXPAND("run", "restart");
    EXPAND("f", "finish");
    EXPAND("fin", "finish");
    EXPAND("fi", "finish-instruction");
    EXPAND("outi", "out-instruction");
    EXPAND("b", "break");
    EXPAND("tb", "tbreak");
    EXPAND("w", "watch");
    EXPAND("v", "view");
    EXPAND("en", "enable");
    EXPAND("dis", "disable");
    EXPAND("x", "execute");
    EXPAND(".", "source");
    EXPAND("l", "list");
    EXPAND("look", "list");
    EXPAND("bc", "bytecode");
    EXPAND("d", "delete");
    EXPAND("disp", "display");
    EXPAND("udisp", "udisplay");
    EXPAND("h", "help");
    EXPAND("'h'", "help");
    EXPAND("\"h\"", "help");
    return command;
}

void show_spiel()
{
    std::cout <<
    "OpenGML Debugger\n\n"
    "Type \"help\" for help.\n";
}

// signal handling (ctrl+C)
Debugger* g_signal_debugger = nullptr;

void signal_handler(int s)
{
    if (s == SIGINT && g_signal_debugger)
    {
        g_signal_debugger->break_execution();
    }
}
}

void Debugger::on_attach()
{
    g_signal_debugger = this;
    std::signal(SIGINT, signal_handler);
}

void Debugger::on_detach()
{
    assert(g_signal_debugger == this);
    g_signal_debugger = nullptr;
    std::signal(SIGINT, SIG_DFL);
}

// retrives source code line
std::string Debugger::source_line(const char* source, size_t _line)
{
    std::string acc = "";
    size_t line = 0;
    while (*source)
    {
        if (*source == '\n')
        {
            ++line;
        }
        else if (line == _line)
        {
            acc += *source;
        }

        ++source;
    }

    return acc;
}

std::string Debugger::location_for_bytecode_stream(const ogm::interpreter::bytecode::BytecodeStream& bytecode, bool show_asm, bool show_source)
{
    std::stringstream ss;
    const bytecode::DebugSymbols* symbols = bytecode.m_bytecode.m_debug_symbols.get();
    if (symbols && symbols->m_name)
    {
        ss << symbols->m_name;
    } else {
        ss << "<anonymous>";
    }
    ss<<": ";
    bool show_pc = true;
    std::string line = "<source not found>";
    if (symbols)
    {
        bytecode::DebugSymbolSourceMap::Range range;
        if (symbols->m_source_map.get_location_at(staticExecutor.m_pc.m_pos, range))
        {
            ss << "line " << range.m_source_start.m_line;
            show_pc = bytecode.m_pos != range.m_address_start;
            if (symbols->m_source)
            {
                line = source_line(symbols->m_source, range.m_source_start.m_line);
            }
            if (show_pc)
            {
                ss << ", ";
            }
        }
        else
        {
            show_pc = true;
        }
    }
    if (show_pc)
    {
        ss << "pc " << bytecode.m_pos;
    }

    if (show_asm)
    {
        std::stringstream ss;
        ogm::bytecode::bytecode_dis(staticExecutor.m_pc, ss, standardLibrary, staticExecutor.m_frame.m_reflection, false, staticExecutor.m_pc.m_pos + 1);
        line = ss.str();
        trim(line);
    }

    if (show_source)
    {
        ss << "   " << line;
    }

    return ss.str();
}

void Debugger::parse_command(DebuggerCommand& out_command, std::string input)
{
    std::vector<std::string> strings;
    split(strings, input);
    if (strings.empty())
    {
        out_command.m_command = "";
    }
    else
    {
        out_command.m_command = expand_command(strings[0]);
        if (strings.size() > 1)
        {
            out_command.m_arguments_str = input.substr(input.find(strings[1]));
        }
        else
        {
            out_command.m_arguments_str = "";
        }
        for (size_t i = 1; i < strings.size(); ++i)
        {
            out_command.m_arguments.push_back(strings[i]);
        }
    }
}

void Debugger::trace_addendum(std::string message)
{
    if (m_config.m_trace)
    {
        m_trace_addendum += "; " + message;
    }
}


void Debugger::break_execution()
{
    m_paused = true;
    m_ls_state = LS_NONE;
}

void Debugger::tick(bytecode::BytecodeStream& in)
{
    if (m_spiel)
    {
        show_spiel();
        m_spiel = false;
    }

    const size_t ls_address = staticExecutor.m_pc.m_pos;
    const size_t ls_frame = staticExecutor.m_return_addresses.size();

    tick_trace(in);

    const bytecode::DebugSymbols* symbols = staticExecutor.m_pc.m_bytecode.m_debug_symbols.get();

    // pause if linestepping/finishing (s, n, f)
    if (!m_paused)
    {
        // We are on an "exact line" if the bytecode address
        // lies exactly on the start of a statement.
        // These are the lines that s, n, f, etc. will end  on.
        bool exact_line = false;
        if (symbols)
        {
            std::vector<DebugSymbolSourceMap::Range> ranges;
            symbols->m_source_map.get_locations_at(ls_address, ranges);

            for (const auto& range : ranges)
            {
                if (range.m_statement && range.m_address_start == ls_address)
                {
                    exact_line = true;
                    break;
                }
            }
        }

        switch (m_ls_state)
        {
            case LS_NONE:
                break;
            case LS_IN:
                if (exact_line)
                {
                    m_paused = true;
                }
                break;
            case LS_OVER:
                if (exact_line && ls_frame <= m_ls_prev_frame)
                {
                    m_paused = true;
                }
                break;
            case LS_OUT:
                if (exact_line && ls_frame < m_ls_prev_frame)
                {
                    m_paused = true;
                }
                break;
            case LS_FINISH:
                if (exact_line &&
                    (ls_frame < m_ls_prev_frame ||
                        (ls_frame == m_ls_prev_frame && m_ls_prev_section_data != staticExecutor.m_pc.m_bytecode.m_data.get())))
                {
                    m_paused = true;
                }
                break;
            case LS_I_IN:
                m_paused = true;
                break;
            case LS_I_OVER:
                if (ls_frame <= m_ls_prev_frame)
                {
                    m_paused = true;
                }
                break;
            case LS_I_OUT:
                if (ls_frame < m_ls_prev_frame)
                {
                    m_paused = true;
                }
                break;
            case LS_I_FINISH:
                if (ls_frame < m_ls_prev_frame ||
                    (ls_frame == m_ls_prev_frame && m_ls_prev_section_data != staticExecutor.m_pc.m_bytecode.m_data.get()))
                {
                    m_paused = true;
                }
                break;
        }
    }

    if (m_paused)
    // get command from user
    {
        // update last location (used for line-stepping)
        m_ls_state = LS_NONE;
        m_ls_prev_address = staticExecutor.m_pc.m_pos;
        m_ls_prev_frame = staticExecutor.m_return_addresses.size();
        m_ls_prev_section_data = staticExecutor.m_pc.m_bytecode.m_data.get();

        // show current line
        std::cout << location_for_bytecode_stream(staticExecutor.m_pc, m_config.m_show_asm, true) << std::endl;

        while (true)
        {
            std::string input = ogm::interpreter::input();
            if (input == "")
            {
                input = m_prev_input;
            }
            else
            {
                m_prev_input = input;
            }

            DebuggerCommand command;
            parse_command(command, input);
    #define HANDLE(c) if (command.m_command == c)
            HANDLE("") { }
            else HANDLE("print")
            {
                cmd_print(command.m_arguments_str);
            }
            else HANDLE("info")
            {
                cmd_info(command.m_arguments_str);
            }
            else HANDLE("step")
            {
                if (!symbols)
                {
                    std::cout << "Cannot step in: no debug symbols for this section.\n";
                    std::cout << "Use \"step-instruction\" (or \"finish\" to exit this frame).\n";
                }
                else
                {
                    m_paused = false;
                    m_ls_state = LS_IN;
                    break;
                }
            }
            else HANDLE("step-instruction")
            {
                m_paused = false; // (not actually necessary -- would break on next step if paused)
                m_ls_state = LS_I_IN;
                break;
            }
            else HANDLE("next")
            {
                if (!symbols)
                {
                    std::cout << "Cannot step over: no debug symbols for this section.\n";
                    std::cout << "Use \"next-instruction\" (or \"finish\" to exit this frame).\n";
                }
                else
                {
                    m_paused = false;
                    m_ls_state = LS_OVER;
                    break;
                }
            }
            else HANDLE("next-instruction")
            {
                m_paused = false;
                m_ls_state = LS_I_OVER;
                break;
            }
            else HANDLE("to")
            {
                // TODO
                break;
            }
            else HANDLE("continue")
            {
                m_paused = false;
                break;
            }
            else HANDLE("finish")
            {
                m_paused = false;
                m_ls_state = LS_FINISH;
                break;
            }
            else HANDLE("out")
            {
                m_paused = false;
                m_ls_state = LS_OUT;
                break;
            }
            else HANDLE("finish-instruction")
            {
                m_paused = false;
                m_ls_state = LS_I_FINISH;
                break;
            }
            else HANDLE("out-instruction")
            {
                m_paused = false;
                m_ls_state = LS_I_OUT;
                break;
            }
            else HANDLE("up")
            {
                // TODO
            }
            else HANDLE("down")
            {
                // TODO
            }
            else HANDLE("break")
            {
                // TODO
            }
            else HANDLE("tbreak")
            {
                // TODO
            }
            else HANDLE("watch")
            {
                // TODO
            }
            else HANDLE("enable")
            {
                // TODO
            }
            else HANDLE("disable")
            {
                // TODO
            }
            else HANDLE("delete")
            {
                // TODO
            }
            else HANDLE("display")
            {
                // TODO
            }
            else HANDLE("bytecode")
            {
                // TODO
            }
            else HANDLE("list")
            {
                // TODO
            }
            else HANDLE("backtrace")
            {
                size_t i;
                // we skip 0 because it's a dummy entry.
                for (i = 1; i < staticExecutor.m_return_addresses.size(); ++i)
                {
                    std::cout << "  #" << i << ": " << location_for_bytecode_stream(staticExecutor.m_return_addresses.at(i), m_config.m_show_asm) << std::endl;
                }
                std::cout << "> #" << i << ": " << location_for_bytecode_stream(staticExecutor.m_pc, m_config.m_show_asm) << std::endl;
            }
            else HANDLE("config")
            {
                cmd_set(command.m_arguments);
            }
            else HANDLE("execute")
            {
                // TODO
            }
            else HANDLE("source")
            {
                // TODO
            }
            else HANDLE("define")
            {
                // TODO
            }
            else HANDLE("quit")
            {
                // TODO
            }
            else HANDLE("restart")
            {
                // TODO
            }
            else HANDLE("help")
            {
                cmd_help(command.m_arguments.empty() ? "" : command.m_arguments_str);
            }
            else
            {
                std::cout << "Unrecognized command \"" << command.m_command << "\". Try \"help\".\n";
            }
        }
    }
}

void Debugger::tick_trace(bytecode::BytecodeStream& in)
{
    const size_t ls_frame = staticExecutor.m_return_addresses.size();
    if (m_config.m_trace)
    // write trace data.
    {
        // differences between file-output and stdout trace is basically just
        // the location of newlines and some "TRACE" markers.
        bool write_file = !!m_trace_ofstream;
        auto& trace_stream = (write_file) ? *m_trace_ofstream : std::cout;

        // first tick for this trace.
        bool first_tick = !m_trace_prev_section_data;

        if (!first_tick)
        {
            // write addendums for previous line
            if (m_trace_addendum.length() > 0)
            {
                if (write_file)
                {
                    trace_stream << m_trace_addendum;
                }
                else
                {
                    trace_stream << "TRACE (RESULT)" << m_trace_addendum << std::endl;
                }
            }
            if (write_file) trace_stream << std::endl;
        }

        if (in.m_bytecode.m_data.get() != m_trace_prev_section_data || m_trace_prev_frame != ls_frame)
        {
            // frame has changed -- trace frame name.
            m_trace_prev_section_data = in.m_bytecode.m_data.get();
            m_trace_prev_frame = ls_frame;
            const bytecode::DebugSymbols* symbols = in.m_bytecode.m_debug_symbols.get();
            if (!write_file) trace_stream << "TRACE ";
            if (write_file && !first_tick) trace_stream << std::endl;
            for (size_t i = 0; i < ls_frame; ++i)
            {
                trace_stream << "  ";
            }
            if (symbols && symbols->m_name)
            {
                trace_stream << "@" << symbols->m_name << std::endl;
            }
            else
            {
                trace_stream << "@<anonymous>"  << std::endl;
            }
        }

        if (m_config.m_trace_stack_count > 0)
        {
            trace_stream << "STACK (" << staticExecutor.m_varStackIndex + 1 << "): ";
            bool first = true;
            for (size_t i = std::min<size_t>(m_config.m_trace_stack_count, staticExecutor.m_varStackIndex) - 1; i != std::numeric_limits<size_t>::max(); --i)
            {
                if (!first)
                {
                    trace_stream << ", ";
                }
                first = false;
                trace_stream << staticExecutor.peekRef(i);
            }
            trace_stream << std::endl;
        }

        if (!write_file) trace_stream << "TRACE ";
        // indent by return address stack size.
        for (size_t i = 0; i < ls_frame; ++i)
        {
            trace_stream << "  ";
        }

        {
            std::stringstream ss;
            ogm::bytecode::bytecode_dis(in, ss, standardLibrary, staticExecutor.m_frame.m_reflection, false, staticExecutor.m_pc.m_pos + 1);
            std::string s = ss.str();
            trim(s);
            trace_stream << s;
        }
        if (!write_file)
        {
            trace_stream << std::endl;
        }

        if (write_file)
        {
            std::flush(trace_stream);
        }
    }

    m_trace_addendum = "";
}

void Debugger::cmd_info_stack()
{
    std::cout << "Size of stack: " << staticExecutor.m_varStackIndex + 1 << " variables.\n";
    std::cout << "Local variables for this stack frame begin at index " << staticExecutor.m_locals_start << std::endl;
    std::cout << "Additionally, there are " << staticExecutor.m_return_addresses.size() << " values on the return address stack.\n";
    std::cout << std::endl;
    for (size_t i = 0; i <= staticExecutor.m_varStackIndex; ++i)
    {
        std::cout << i << ": " << staticExecutor.m_varStack[i];
        switch (i - staticExecutor.m_locals_start)
        {
            case 0:
                std::cout << " <- locals start here";
                break;
            case -1:
                std::cout << " <- previous locals index";
                break;
            case -2:
                // TODO: only variadic arguments get a count.
                std::cout << " <- argument count";
                break;
            default:
                break;
        }
        std::cout << std::endl;
    }
}

void Debugger::cmd_set(const std::vector<std::string>& args)
{
    if (args.size() == 0)
    {
        std::cout << "Usage is as follows.\n";
        std::cout << "Set: config <key> <value>\n";
        std::cout << "View: config <key>\n";
        std::cout << "To display a list of keys, try \"help config\"";
    }
    else if (args.size() == 1)
    {
        // TODO -- view
        if (args[0] == "show-asm")
        {
            std::cout << "show-asm is " << ((m_config.m_show_asm)
                ? "on"
                : "off");
            std::cout << std::endl;
        }
        else if (args[0] == "trace")
        {
        show_trace:
            std::cout << "tracing " << ((m_config.m_trace)
                ? "is on. "
                : "is off. ");
            if (m_config.m_trace_path != "")
            {
                std::cout << "Logging to " << m_config.m_trace_path;
            }
            std::cout << std::endl;
        }
    }
    else if (args.size() > 1)
    {
        if (args[0] == "show-asm")
        {
            if (args[1] == "on")
            {
                m_config.m_show_asm = true;
            }
            else if (args[1] == "off")
            {
                m_config.m_show_asm = false;
            }
            else goto err_value_1;

            std::cout << "show-asm is " << ((m_config.m_show_asm)
                ? "on"
                : "off");
            std::cout << std::endl;

            // show current line
            std::cout << location_for_bytecode_stream(staticExecutor.m_pc, m_config.m_show_asm, true) << std::endl;
            return;
        }
        else if (args[0] == "trace")
        {
            if (args[1] == "on")
            {
                m_config.m_trace = true;
            }
            else if (args[1] == "off")
            {
                m_config.m_trace = false;
            }
            else goto err_value_1;

            m_trace_prev_section_data = nullptr;
            if (m_trace_ofstream)
            {
                delete m_trace_ofstream;
            }

            if (args.size() > 2)
            {
                m_config.m_trace_path = args[2];
                m_trace_ofstream = new std::ofstream(m_config.m_trace_path, std::ios::out);
                if (!m_trace_ofstream->good())
                {
                    delete m_trace_ofstream;
                    m_trace_ofstream = nullptr;
                    std::cout << "ERROR: Cannot write to file \"" << m_config.m_trace_path << "\"; tracing to stdout instead.";
                    m_config.m_trace_path = "";
                }
            }
            else
            {
                m_config.m_trace_path = "";
                m_trace_ofstream = nullptr;
            }

            goto show_trace;
        }
        else if (args[0] == "trace-stack-count")
        {
            m_config.m_trace_stack_count = std::stoi(args[1]);
            std::cout << "stack trace count is " << m_config.m_trace_stack_count << std::endl;
            return;
        }
        else
        {
            std::cout << "unrecognized key.\n";
            return;
        }
        return;

    err_value_1:
        std::cout << "Invalid value \"" << args[1] << "\".\n";
        std::cout << "Try \"help set\"\n";
        return;

    err_value_2:
        std::cout << "Invalid value \"" << args[2] << "\".\n";
        std::cout << "Try \"help set\"\n";
        return;
    }


}

void Debugger::cmd_info_locals()
{
    const bytecode::DebugSymbols* symbols = staticExecutor.m_pc.m_bytecode.m_debug_symbols.get();
    if (symbols)
    {
        const auto& locals = symbols->m_namespace_local;
        if (staticExecutor.m_varStackIndex + 1 < staticExecutor.m_locals_start + locals.id_count())
        {
            std::cout << "Local variables for this stack frame have not yet been allocated.\n";
        }
        else
        {
            std::cout << "Local variables:\n";
            for (variable_id_t i = 0; i < locals.id_count(); ++i)
            {
                std::cout << std::setw(15) << locals.find_name(i) << std::setw(0) << ": " << staticExecutor.local(i) << std::endl;
            }
        }
    }
    else
    {
        std::cout << "No debug symbols found for this section.\n";
        std::cout << "You may be able to glean some information about the local variables by analyzing the stack.\n";
        std::cout << "Try \"info stack\".\n";
    }
}

void Debugger::cmd_info_instance()
{
    const Instance* instance = staticExecutor.m_self;
    const bytecode::ReflectionAccumulator* reflection = staticExecutor.m_frame.m_reflection;

    {
        std::cout << "Instance variables:\n";
        const SparseContiguousMap<variable_id_t, Variable>& map = instance->getVariableStore();
        for (size_t i = 0; i < map.count(); ++i)
        {
            variable_id_t id = map.get_ith_key(i);
            const Variable& v = map.get_ith_value(i);

            std::string id_s = "%" + std::to_string(id);
            if (reflection)
            {
                id_s = reflection->m_namespace_instance.find_name(id);
            }

            std::cout << "  " << std::setw(15) << id_s << std::setw(0) << ": " << v << std::endl;
        }
    }
}

void Debugger::cmd_info(std::string topic)
{
    if (topic == "")
    {
        std::cout << "Need to provide a topic for info. Try \"help info\".\n";
    }
    else if (topic == "instance")
    {
        cmd_info_instance();
    }
    else if (topic == "locals")
    {
        cmd_info_locals();
    }
    else if (topic == "stack")
    {
        cmd_info_stack();
    }
    else if (topic == "pc" || topic == "program counter")
    {
        std::cout << location_for_bytecode_stream(staticExecutor.m_pc, m_config.m_show_asm, false) << std::endl;
    }
    else if (topic == "flag" || topic == "flags")
    {
        std::cout << "status flags: "
            << ((staticExecutor.m_statusCond)
                ? "C"
                : "c")
            << ((staticExecutor.m_statusCOW)
                ? "x"
                : "X")
            << std::endl;

        if (staticExecutor.m_statusCond)
        {
            std::cout << "Condition is set; branches will be taken\n";
        }
        else
        {
            std::cout << "Condition unset; branches will not be taken.\n";
        }

        if (!staticExecutor.m_statusCOW)
        {
            std::cout << "Arrays will be edited without copying.\n";
        }
        else
        {
            std::cout << "Arrays will be copied on write.\n";
        }
    }
    else
    {
        std::cout << "Unrecognized topic \"" << topic << "\". Try \"help info\"\n";
    }
}

void Debugger::cmd_print(std::string expression)
{
    if (expression == "")
    {
        std::cout << "Error: need to provide an expression to print.\n";
    }
    else
    {
        expression = "show_debug_message(" + expression + ");";

        // parse expression
        ogm_ast_t* ast;
        try
        {
            ast = ogm_ast_parse(expression.c_str());
        }
        catch(const std::exception& e)
        {
            std::cout << "While parsing the provided expression, an error occurred:\n";
            std::cout << e.what() << std::endl;
            return;
        }

        if (!ast)
        {
            std::cout << "Unable to parse ast.\n";
            return;
        }

        // compile expression
        bytecode::Bytecode bytecode;
        try
        {
            bytecode::ProjectAccumulator acc{staticExecutor.m_frame.m_reflection, &staticExecutor.m_frame.m_assets, &staticExecutor.m_frame.m_bytecode, &ogm::interpreter::staticExecutor.m_frame.m_config};
            bytecode_generate(
                bytecode,
                {ast, 0, 0, "ogmdb anonymous bytecode section", expression.c_str()},
                ogm::interpreter::standardLibrary,
                &acc
            );
        }
        catch(const std::exception& e)
        {
            std::cout << "While compiling the provided expression, an error occurred:\n";
            std::cout << e.what() << std::endl;
            ogm_ast_free(ast);
            return;
        }
        ogm_ast_free(ast);
#if 0
        std::stringstream ss;
        bytecode_dis(bytecode::BytecodeStream(bytecode), ss, ogm::interpreter::standardLibrary, staticExecutor.m_frame.m_reflection);
        std::cout << ss.str();
#endif

        // execute expression.
        bool temp_paused = m_paused;
        try
        {
            m_paused = false;
            ogm::interpreter::execute_bytecode(bytecode);
            m_paused = temp_paused;
        }
        catch(const std::exception& e)
        {
            m_paused = temp_paused;
            std::cout << "While executing the provided expression, an error occurred:\n";
            std::cout << e.what() << std::endl;
        }
    }
}

void Debugger::cmd_help(std::string topic)
{
    if (topic == "")
    {
        std::cout <<
        "Type \"help <topic>\" for help with a particular command or topic.\n\n"
        "[p]rint       -- evaluate and display an expression, such as a variable name.\n"
        "[i]nfo        -- information on topics such as the list of variables or instances.\n"
        "[s]tep        -- take a step, entering any function calls if applicable.\n"
        "[n]next       -- take a step, skipping over any function calls if applicable.\n"
        "[c]ontinue    -- resumes execution.\n"
        "to            -- resumes execution until a particular line is reached.\n"
        "[f]inish      -- resume execution until return from this function.\n"
        "[b]reak       -- place a breakpoint.\n"
        "[tb]reak      -- place a temporary/one-shot breakpoint.\n"
        "[w]atch       -- watch a variable and break if it is accessed.\n"
        "[en]able      -- enable a disabled breakpoint or watchpoint.\n"
        "[dis]able     -- disable a breakpoint or watchpoint.\n"
        "[d]elete      -- delete a breakpoint or watchpoint.\n"
        "[disp]lay     -- print the given expression when paused.\n"
        "[l]ist        -- display the source code around the current line.\n"
        "config        -- views/sets debugger configuration values.\n"
        "e[x]ecute     -- runs the provided code snippet.\n"
        "., source     -- executes the provided ogmdb command file.\n"
        "quit          -- quits ogmdb.\n"
        "restart       -- starts over.\n"
        "[h]elp        -- provides insightful information on debugger usage.\n"
        "\n"
        "Additionally, help is available on the following topics:\n"
        "bytecode, stack\n"
        ;
    }
    else if (topic == "bytecode")
    {
        std::cout <<
        "bc, bytecode         -- display the bytecode around the current line.\n"
        "si, step-instruction -- take a bytecode step, entering any function calls as needed.\n"
        "ni, next-instruction -- take a bytecode step, entering any function calls as needed.\n"
        "fi, finish-instruction -- break immediately after exiting.\n"
        ;
    }
    else if (topic == "stack")
    {
        std::cout <<
        "bt, backtrace -- show the stack of function calls.\n"
        "up            -- go up a level in the stack.\n"
        "down          -- go down a level in the stack.\n"
        ;
    }
    else if (topic == "print")
    {
        std::cout <<
        "usage: print <GML expression>\n"
        ;
    }
    else if (topic == "config")
    {
        std::cout <<
        "usage: config <key> <value>\n"
        "       config <key>\n"
        "Keys are as follows:\n"
        "show-asm <on|off>\n"
        "trace    <on|off> [file]\n"
        ;
    }
    else
    {
        std::cout << "Unrecognized topic \"" << topic << "\". Type \"help\" for help.\n";
    }
}

}}
