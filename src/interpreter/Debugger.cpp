#include "ogm/interpreter/Debugger.hpp"

#include "ogm/interpreter/StandardLibrary.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "./cli_input.hpp"

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
// Error when executing inline code from debugger.
struct InlineError : public std::exception
{
    std::string m_what;
    const char* what() const noexcept override
    {
        return m_what.c_str();
    }

    InlineError(std::string s)
        : m_what(s)
    { }
};

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
    EXPAND("li", "list-instruction");
    EXPAND("look-instruction", "list-instruction");
    EXPAND("bc", "bytecode");
    EXPAND("d", "delete");
    EXPAND("disp", "display");
    EXPAND("udisp", "undisplay");
    EXPAND("undisp", "undisplay");
    EXPAND("h", "help");
    EXPAND("'h'", "help");
    EXPAND("\"h\"", "help");
    EXPAND("q", "quit");
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

// input debugger
Debugger* g_rl_debugger = nullptr;

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
    ogm_assert(g_signal_debugger == this);
    g_signal_debugger = nullptr;
    std::signal(SIGINT, SIG_DFL);
}

// retrives source code line
std::string Debugger::source_line(const char* source, size_t _line)
{
    return nth_line(source, _line);
}

std::string Debugger::location_for_bytecode_stream(const ogm::interpreter::bytecode::BytecodeStream& bytecode, bool show_asm, bool show_source)
{
    std::stringstream ss;
    const bytecode::DebugSymbols* symbols = bytecode.m_bytecode.m_debug_symbols.get();
    if (symbols && symbols->m_name.length())
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
        size_t pos = bytecode.m_pos;
        if (pos == 0 && !symbols->m_source_map.get_location_at(pos, range))
        // work around for start-of-function "all" call being off-line.
        {
            pos += 5;
        }

        if (symbols->m_source_map.get_location_at(pos, range))
        {
        setline:
            ss << "line " << (range.m_source_start.m_line + 1);
            show_pc = bytecode.m_pos != range.m_address_start;
            if (symbols->m_source.length())
            {
                line = source_line(symbols->m_source.c_str(), range.m_source_start.m_line);
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

void Debugger::trace_addendum(const std::string& message)
{
    if (m_config.m_trace_addendums)
    {
        m_trace_addendum += "; " + message;
    }
}

void Debugger::break_execution()
{
    m_paused = true;
    m_ls_state = LS_NONE;
}

namespace
{
    char **
    debug_completer(const char* text, int start, int end)
    {
        if (g_rl_debugger) return g_rl_debugger->autocomplete(text, start, end);
        return nullptr;
    }
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
    if (!m_paused && m_ls_state != LS_NONE)
    {
        // determine if we are on an "exact line", i.e. if the bytecode address
        // lies exactly on the start of a statement.
        // These are the lines that s, n, f, etc. will end on.
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
                // FIXME: consecutive (horizontal) frames of the same
                // bytecode section will be skipped.
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

    // check suspend points
    tick_suspendpoints(in);

    if (m_paused)
    // get command from user
    {
        cache_stack_frames();

        // update last location (used for line-stepping)
        m_ls_state = LS_NONE;

        // show current line
        std::cout << location_for_bytecode_stream(staticExecutor.m_pc, m_config.m_show_asm, true) << std::endl;

        while (true)
        {
            void (a)(void);
            g_rl_debugger = this;
            std::string input;
            if (m_commands_queue.empty())
            {
                input = ogm::interpreter::input(
                    "(ogmdb) ",
                    debug_completer
                );
            }
            else
            {
                input = m_commands_queue.front();
                m_commands_queue.pop();
            }
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
                if (!cmd_break(command.m_arguments, true))
                {
                    m_paused = false;
                }
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
                if (m_current_frame < m_frames.size() - 1)
                {
                    cmd_frame_shift(m_current_frame + 1);
                }
                else
                {
                    if (m_stack_broken)
                    {
                        std::cout << "Stack is broken past this point." << std::endl;
                    }
                    else
                    {
                        std::cout << "Oldest (top-most) stack frame selected." << std::endl;
                    }
                }
            }
            else HANDLE("down")
            {
                if (m_current_frame > 0)
                {
                    cmd_frame_shift(m_current_frame - 1);
                }
                else
                {
                    std::cout << "Newest (bottom-most) stack frame selected." << std::endl;
                }
            }
            else HANDLE("frame")
            {
                if (command.m_arguments.size() > 0)
                {
                    if (is_digits(command.m_arguments.at(0)))
                    {
                        cmd_frame_shift(std::stoi(command.m_arguments.at(0)) + 1);
                    }
                }
                else
                {
                    // display stack frame.
                    cmd_frame_shift(m_current_frame);
                }
            }
            else HANDLE("break")
            {
                cmd_break(command.m_arguments, false);
            }
            else HANDLE("tbreak")
            {
                cmd_break(command.m_arguments, true);
            }
            else HANDLE("watch")
            {
                cmd_watch(command.m_arguments);
            }
            else HANDLE("enable")
            {
                cmd_enable(command.m_arguments, 2);
            }
            else HANDLE("disable")
            {
                cmd_enable(command.m_arguments, 1);
            }
            else HANDLE("delete")
            {
                cmd_enable(command.m_arguments, 0);
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
                cmd_list(&command.m_arguments, false);
            }
            else HANDLE("list-instruction")
            {
                cmd_list(&command.m_arguments, true);
            }
            else HANDLE("backtrace")
            {
                size_t i;
                // we skip 0 because it's a dummy entry.
                for (size_t i = 0; i < m_frames.size(); ++i)
                {
                    if (i == m_current_frame)
                    {
                        std::cout << ">";
                    }
                    else
                    {
                        std::cout << " ";
                    }
                    std::cout << " #" << i + 1 << ": "
                        << location_for_bytecode_stream(
                                get_frame_pc(i),
                                m_config.m_show_asm
                           )
                        << std::endl;
                }
                if (m_stack_broken)
                {
                    std::cout << "(Stack broken beyond this point.)" << std::endl;
                }
            }
            else HANDLE("config")
            {
                cmd_set(command.m_arguments);
            }
            else HANDLE("execute")
            {
                std::stringstream ss;
                for (const std::string& s : command.m_arguments)
                {
                    ss << " " << s;
                }
                string_execute_inline(ss.str());
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
                std::string input = ogm::interpreter::input("Quitting. Confirm? [y/n] ");
                trim(input);
                if (input == "y" || input == "Y")
                {
                    exit(0);
                }
                std::cout << "Cancelled.\n";
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
        }  // while (true)

        // update line-step information
        m_ls_prev_address = get_frame_pc().m_pos;
        m_ls_prev_frame = staticExecutor.m_return_addresses.size() - m_current_frame;
        m_ls_prev_section_data = get_frame_pc().m_bytecode.m_data.get();

        // if continuing
        if (!m_paused)
        {
            // return view to recent end of stack.
            m_current_frame = 0;
        }
    }
}

// stop if a suspendpoint is triggered.
void Debugger::tick_suspendpoints(bytecode::BytecodeStream& in)
{
    if (m_executing_inline) return;
    size_t i = 0;
    for (SuspendPoint& sp : m_suspend_points)
    {
        ++i;
        if (!sp.m_active) continue;

        if (sp.m_type == SuspendPoint::BREAK)
        {
            if (sp.m_breakpoint->m_pc == in)
            {
                if (sp.m_conditional)
                {
                    try
                    {
                        execute_inline(sp.m_condition_bc);

                        // check status flag
                        if (staticExecutor.m_statusCond)
                        {
                            continue;
                        }
                    }
                    catch (...)
                    {
                        // ignore
                        continue;
                    }
                }
                std::cout << "\nbreakpoint " << i << " reached.\n";
                m_paused = true;
                if (sp.m_temporary)
                {
                    sp.set_deleted();
                }
                break;
            }
        }
        else if (sp.m_type == SuspendPoint::WATCH && !sp.m_watchpoint->m_is_location
            && (!sp.m_watchpoint->m_scoped || sp.m_watchpoint->m_expression_scope.m_data == in.m_bytecode.m_data))
        {
            // calculate new value
            Variable v;
            if (--sp.m_watchpoint->m_eval_ticks > 0)
            {
                continue;
            }

            sp.m_watchpoint->m_eval_ticks = sp.m_watchpoint->m_eval_interval;
            bool error=false;
            sp.m_watchpoint->m_last_evaluation = staticExecutor.m_pc;
            try
            {
                execute_inline(sp.m_watchpoint->m_expression, &v);
            }
            catch (const InlineError& e)
            {
                std::cout << "An exception occurred while evaluating watchpoint " << i << ":\n";
                std::cout << e.what();
            }
            if (!error && v == sp.m_watchpoint->m_current_value)
            // no change occurred.
            {
                v.cleanup();
                continue;
            }

            // watchpoint triggered.
            if (!error)
            {
                std::cout << "\nwatchpoint " << i << " triggered.\n";
                std::cout << "Previous value: " << sp.m_watchpoint->m_current_value << std::endl;
                std::cout << "  Which was evaluated at " << location_for_bytecode_stream(sp.m_watchpoint->m_last_evaluation, false) << std::endl;
                std::cout << "New value: " << v << std::endl;
                sp.m_watchpoint->m_current_value.cleanup();
                sp.m_watchpoint->m_current_value.copy(v);
            }
            v.cleanup();
            m_paused = true;
            if (sp.m_temporary)
            {
                sp.set_deleted();
            }
            break;
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

        // first tick since trace config enabled.
        bool first_tick = !m_trace_prev_section_data;

        if (!m_config.m_trace_only_section_names)
        {
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

            if (symbols && symbols->m_name.length())
            {
                trace_stream << "@" << symbols->m_name;
            }
            else
            {
                trace_stream << "@<anonymous>";
            }

            if (!m_config.m_trace_only_section_names)
            {
                trace_stream << std::endl;
            }
        }

        if (!m_config.m_trace_only_section_names)
        {
            if (m_config.m_trace_stack_count > 0)
            {
                trace_stream << "STACK (" << staticExecutor.m_varStackIndex + 1 << "): ";
                bool first = true;
                for (size_t i = std::min<size_t>(staticExecutor.m_varStackIndex, m_config.m_trace_stack_count) + 1; i > 0 ; --i)
                {
                    if (!first)
                    {
                        trace_stream << ", ";
                    }
                    first = false;
                    trace_stream << staticExecutor.peekRef(i - 1);
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
        }

        if (write_file)
        {
            std::flush(trace_stream);
        }
    }

    if (m_trace_addendum.length() > 0)
    {
        m_trace_addendum = "";
    }
}

void Debugger::cmd_info_suspendpoints()
{
    size_t i = 0;
    bool any = false;
    for (const SuspendPoint& sp : m_suspend_points)
    {
        ++i;
        if (sp.m_type == SuspendPoint::DELETED)
        {
            continue;
        }
        any = true;
        std::cout << " ";
        if (sp.m_active)
        {
            std::cout << "*";
        }
        else
        {
            std::cout << " ";
        }
        std::cout << i << ", ";
        if (sp.m_temporary)
        {
            std::cout << "temporary ";
        }
        if (sp.m_conditional)
        {
            std::cout << "conditional ";
        }
        switch(sp.m_type)
        {
        case SuspendPoint::BREAK:
            std::cout << "breakpoint at ";
            std::cout << location_for_bytecode_stream(sp.m_breakpoint->m_pc, false, false);
            break;
        case SuspendPoint::WATCH:
            std::cout << "watchpoint on: ";
            std::cout << sp.m_watchpoint->m_description;
            std::cout << "\n" << "  current value: " << sp.m_watchpoint->m_current_value;
            break;
        default:
            std::cout << " <unknown> ";
        }
        std::cout << std::endl;
        if (sp.m_conditional)
        {
            std::cout << "    if:" << sp.m_condition_str << std::endl;
        }
    }

    if (!any)
    {
        std::cout << "No suspend points presently exist.\n";
    }
}

void Debugger::cmd_info_stack()
{
    std::cout << "Size of stack: " << staticExecutor.m_varStackIndex + 1 << " variables.\n";
    std::cout << "Local variables for this stack frame begin at index " << staticExecutor.m_locals_start << std::endl;
    std::cout << "Additionally, there are " << staticExecutor.m_return_addresses.size() << " values on the return address stack.\n";
    std::cout << std::endl;
    for (int32_t i = 0; i <= staticExecutor.m_varStackIndex; ++i)
    {
        std::cout << i << ": " << staticExecutor.m_varStack[i];
        for (size_t j = 0; j < m_frames.size(); ++j)
        {
            switch (static_cast<int32_t>(i - m_frames.at(j).m_locals_start))
            {
                case 0:
                    std::cout << " <- stack frame start (#" << (j + 1) << ")";
                    break;
                case -1:
                    std::cout << " <- stack frame return (#" << (j + 1) << ")";;
                    break;
                case -2:
                    // TODO: only variadic arguments get a count.
                    std::cout << " <- argument count (#" << (j + 1) << ")";;
                    break;
                default:
                    break;
            }
        }
        std::cout << std::endl;
    }
}

std::string Debugger::list_source(ogm::bytecode::Bytecode& bc, size_t range_min, size_t range_max)
{
    ogm_assert(bc.m_debug_symbols);
    ogm_assert(bc.m_debug_symbols->m_source.length());
    size_t gutter_width = std::to_string(range_max).length() + 3;
    std::vector<std::string> lines;
    split(lines, bc.m_debug_symbols->m_source, "\n", false);

    std::stringstream out;

    // determine program counter line
    int32_t pc_line = -1;
    bool approx_pc = false;
    if (bc.m_data == get_frame_pc().m_bytecode.m_data)
    {
        bytecode::DebugSymbolSourceMap::Range range;
        if (bc.m_debug_symbols->m_source_map.get_location_at(get_frame_pc().m_pos, range))
        {
            pc_line = range.m_source_start.m_line;

            // check if not exactly at start of range
            if (range.m_address_start != get_frame_pc().m_pos)
            {
                approx_pc = true;
            }
        }
    }

    // determine breakpoint locations
    std::vector<size_t> breakpoint_lines;
    // breakpoints
    for (const SuspendPoint& sus : m_suspend_points)
    {
        if (sus.m_type == SuspendPoint::BREAK && sus.m_active)
        {
            if (sus.m_breakpoint->m_pc.m_bytecode.m_data == bc.m_data)
            {
                bytecode::DebugSymbolSourceMap::Range range;
                if (bc.m_debug_symbols->m_source_map.get_location_at(sus.m_breakpoint->m_pc.m_pos, range));
                breakpoint_lines.push_back(range.m_source_start.m_line);
            }
        }
    }

    // list lines
    for (size_t i = range_min; i <= range_max && i < lines.size(); ++i)
    {
        std::string preamble = "  ";

        // program counter
        if (i == pc_line)
        {
            if (approx_pc)
            {
                preamble[0] = '~';
            }
            else
            {
                preamble[0] = '-';
            }
            preamble[1] = '>';
        }
        if (std::find(breakpoint_lines.begin(), breakpoint_lines.end(), i) != breakpoint_lines.end())
        {
            // determine which of the 2 characters shows the breakpoint
            size_t z = 0;
            if (approx_pc && i == pc_line)
            {
                z = 1;
            }

            // put breakpoint symbol in.
            preamble[z] = '*';
        }

        preamble += std::to_string(i + 1) + " ";
        while (preamble.length() < gutter_width)
        {
            preamble += ' ';
        }
        out << preamble;
        out << lines.at(i);
        out << std::endl;
    }

    return out.str();
}

std::string Debugger::list_instructions(ogm::bytecode::Bytecode& bc, size_t range_min, size_t range_max, std::vector<bytecode::DisassembledBytecodeInstruction>* dis)
{
    std::stringstream out;

    // disassemble if not provided
    std::vector<bytecode::DisassembledBytecodeInstruction> tmp;
    if (!dis)
    {
        dis = &tmp;
        bytecode_dis(bc, *dis, staticExecutor.m_library);
    }

    if (dis->size() == 0)
    {
        return "";
    }

    size_t gutter_width = std::to_string(dis->at(std::min(dis->size(), range_max)).m_address).length() + 3;

    // list lines
    for (size_t i = range_min; i <= range_max && i < dis->size(); ++i)
    {
        bytecode::DisassembledBytecodeInstruction& dbi = dis->at(i);
        std::string preamble = "  ";

        // program counter
        if (bc.m_data == get_frame_pc().m_bytecode.m_data && dbi.m_address == get_frame_pc().m_pos)
        {
            preamble[0] = '-';
            preamble[1] = '>';
        }

        // breakpoints
        for (const SuspendPoint& sus : m_suspend_points)
        {
            if (sus.m_type == SuspendPoint::BREAK && sus.m_active)
            {
                if (sus.m_breakpoint->m_pc.m_bytecode.m_data == bc.m_data)
                {
                    if (sus.m_breakpoint->m_pc.m_pos == i)
                    {
                        preamble[0] = '*';
                        break;
                    }
                }
            }
        }

        preamble += std::to_string(dbi.m_address) + " ";
        while (preamble.length() < gutter_width)
        {
            preamble += ' ';
        }
        out << preamble;
        out << get_opcode_string(dbi.m_op);
        out << dbi.m_immediate;
        out << std::endl;
    }

    return out.str();
}

void Debugger::cmd_list(const std::vector<std::string>* args, bool instruction_level)
{
    BytecodeStream& pc = get_frame_pc();
    std::vector<std::string> commasplit;
    std::vector<DisassembledBytecodeInstruction> outInstructions;

    if (!pc.m_bytecode.m_debug_symbols || !pc.m_bytecode.m_debug_symbols->m_source.length())
    {
        instruction_level = true;
    }

    // number of lines in what to display
    size_t source_len;
    size_t default_line = 0;
    if (instruction_level)
    {
        bytecode_dis(pc.m_bytecode, outInstructions, staticExecutor.m_library);
        source_len = outInstructions.size();
        for (const DisassembledBytecodeInstruction& instr : outInstructions)
        {
            if (instr.m_address == pc.m_pos)
            {
                break;
            }
            ++default_line;
        }
    }
    else
    {
        source_len = num_lines(pc.m_bytecode.m_debug_symbols->m_source.c_str());
        bytecode::DebugSymbolSourceMap::Range range;
        if (pc.m_bytecode.m_debug_symbols->m_source_map.get_location_at(pc.m_pos, range))
        {
            default_line = range.m_source_start.m_line;
        }
    }

    // parse args
    size_t range_min = 0;
    size_t range_max = 10;
    size_t radius = 5;
    if (args->size() == 1)
    {
        split(commasplit, args->at(0));
        if (commasplit.size() == 2)
        {
            // actually two arguments separated by comma.
            args = &commasplit;
        }
        else
        {
            // honest 1 argument
            radius = std::max(0, std::atoi(args->at(0).c_str()));
        }
    }
    if (args->size() == 2)
    {
        // parse start and end.
        std::vector<size_t> v;
        for (const std::string& arg : *args)
        {
            if (arg.length() == 0)
            {
                goto show_usage;
            }
            else
            {
                v.emplace_back();
                bool relative;
                if (starts_with(arg, "-"))
                {
                    relative = true;
                }
                else if (starts_with(arg, "+"))
                {
                    relative = true;
                }
                else
                {
                    relative = false;
                }
                int32_t offset = std::atoi(arg.c_str());
                if (relative)
                {
                    v.back() = std::max<int32_t>(std::min<int32_t>(default_line + offset, source_len - 1), 0);
                }
                else
                {
                    v.back() = offset;
                }
            }
            if (v.back() < v.front())
            {
                std::swap(v.back(), v.front());
            }

            range_min = v.front();
            range_max = v.back();
        }
    }
    else if (args->size() < 2)
    {
        bool capped_min = false;
        bool capped_max = false;
        if (default_line < radius)
        {
            capped_min = true;
        }
        if (default_line + radius >= source_len)
        {
            capped_max = true;
        }
        if (capped_min && capped_max)
        {
            range_max = source_len;
        }
        else if (capped_min)
        {
            range_max = 2 * radius;
        }
        else if (capped_max)
        {
            range_max = source_len - 1;
            range_min = source_len - 1 - 2 * radius;
        }
        else
        {
            range_min = default_line - radius;
            range_max = default_line + radius;
        }
    }
    else
    {
        goto show_usage;
    }

    if (instruction_level)
    {
        std::cout << list_instructions(pc.m_bytecode, range_min, range_max, &outInstructions);
    }
    else
    {
        std::cout << list_source(pc.m_bytecode, range_min, range_max);
    }

    return;
show_usage:
    std::cout << "usage:\n\n";
    std::cout << "  list\n";
    std::cout << "  list all\n";
    std::cout << "  list <radius>\n";
    std::cout << "  list <start>,<end>\n";
    std::cout << "  list <+|-><rel. start>,<+|-><rel. end>\n\n";
    std::cout << "ex: list -16,+34\n";
}

void Debugger::cmd_enable(const std::vector<std::string>& args, int32_t mode)
{
    for (const std::string& arg : args)
    {
        size_t i = atoi(arg.c_str()) - 1;
        if (i < m_suspend_points.size())
        {
            SuspendPoint& sp = m_suspend_points.at(i);
            if (sp.m_type != SuspendPoint::DELETED)
            {
                switch (mode)
                {
                case 0:
                    sp.m_type = SuspendPoint::DELETED;
                    std::cout << "Deleted ";
                    break;
                case 1:
                    if (!sp.m_active) continue;
                    sp.m_active = false;
                    std::cout << "Disabled ";
                    break;
                case 2:
                    if (sp.m_active) continue;
                    sp.m_active = true;
                    std::cout << "Enabled ";
                    break;
                default:
                    ogm_assert(false);
                }

                std::cout << "suspend point " << (i + 1) << std::endl;
            }
        }
    }
}

bool Debugger::cmd_watch(const std::vector<std::string>& args, bool temporary)
{
    std::stringstream ss;
    bool location = false;
    bool expression = false;
    bool read = false;
    bool write = false;
    bool scoped = true;
    uint64_t interval = 1;
    bool reading_expression = false;
    for (const std::string& s : args)
    {
        if (!reading_expression)
        {
            if (s == "-e")
            {
                expression = true;
            }
            else if (s == "-l")
            {
                location = true;
            }
            else if (s == "-r")
            {
                read = true;
            }
            else if (s == "-w")
            {
                write = true;
            }
            else if (s == "-a")
            {
                scoped = false;
            }
            else if (starts_with(s, "-n="))
            {
                interval = std::atoi(s.c_str() + 3);
            }
            else
            {
                reading_expression = true;
            }
        }
        if (reading_expression)
        {
            ss << s << " ";
        }
    }

    if (!location && !expression)
    {
        std::cout << "watchpoint must be either location (-l) or expression (-e)." << std::endl;
        return false;
    }

    if (location && expression)
    {
        std::cout << "watchpoint cannot be both location (-l) and expression (-e)." << std::endl;
        return false;
    }

    if (location && !(read || write))
    {
        std::cout << "location watchpoint must at least be either read (-r) or write (-w)." << std::endl;
        return false;
    }

    if (location && (!scoped || 1 != interval))
    {
        std::cout << "Flags -a, -n are not applicable to location watchpoints." << std::endl;
        return false;
    }

    if (location)
    {
        std::cout << "location watchpoint not yet implemented." << std::endl;
        return false;
    }

    if (expression && (read || write))
    {
        std::cout << "Flags -r and -w are not applicable to expression watchpoints." << std::endl;
        return false;
    }

    if (interval == 0)
    {
        std::cout << "interval must be at least 1.\n";
        return false;
    }

    std::string description = ss.str();
    m_suspend_points.emplace_back();
    SuspendPoint& sp = m_suspend_points.back();
    sp.m_type = SuspendPoint::WATCH;
    sp.m_temporary = temporary;
    sp.m_watchpoint = new SuspendPoint::WatchPoint();
    sp.m_watchpoint->m_description = description;
    sp.m_watchpoint->m_is_location = location;
    sp.m_watchpoint->m_eval_interval = interval;
    sp.m_watchpoint->m_eval_ticks = 0;
    sp.m_watchpoint->m_scoped = scoped;
    if (expression)
    {
        ogm_assert(!location);
        sp.m_watchpoint->m_expression_scope = get_frame_pc().m_bytecode;
        try
        {
            sp.m_watchpoint->m_expression = compile_inline("return " + description, get_frame_pc().m_bytecode, 1, !scoped);
        }
        catch (const InlineError& e)
        {
            std::cout << "Error compiling expression." << std::endl;
            std::cout << e.what();
            m_suspend_points.pop_back();
            return false;
        }

        // execute the expression to get initial value
        ogm_assert(sp.m_watchpoint->m_expression.m_retc == 1);
        try
        {
            execute_inline(sp.m_watchpoint->m_expression, &sp.m_watchpoint->m_current_value);
        }
        catch (const InlineError& e)
        {
            std::cout << "An error occurred while determining the value of the expression.\n";
            std::cout << e.what();
            m_suspend_points.pop_back();
            return false;
        }
        std::cout << "Current value is " << sp.m_watchpoint->m_current_value << std::endl;
        sp.m_watchpoint->m_last_evaluation = staticExecutor.m_pc;
    }
    else
    {
        // location
        ogm_assert(location);
        ogm_assert(false);
    }
    return true;
}

bool Debugger::cmd_break(const std::vector<std::string>& args, bool temporary)
{
    {
        ogm::bytecode::BytecodeStream pc;
        if (args.size() == 0)
        {
            // break at current location.
            pc = get_frame_pc();
        }
        else
        {
            // break at specified location.
            std::vector<std::string> location_arg;
            split(location_arg, args.at(0), ":");
            if (location_arg.size() != 2 && location_arg.size() != 1)
            {
                goto usage;
            }
            bool has_section = location_arg.size() == 2 || !is_digits(location_arg.at(0));
            std::string line_info;
            if (has_section)
            {
                // find section
                if (!staticExecutor.m_frame.m_bytecode.get_bytecode_from_section_name(
                    location_arg.at(0).c_str(), pc.m_bytecode)
                )
                {
                    goto no_section;
                }
                if (location_arg.size() > 1)
                {
                    line_info = location_arg.at(1);
                }
                else
                {
                    line_info = "$5";
                }
            }
            else
            {
                pc.m_bytecode = get_frame_pc().m_bytecode;
                line_info = location_arg.at(0);
            }
            bool is_pc = starts_with(line_info, "$");
            uint32_t location;
            if (is_pc)
            {
                // pc count; +1 to start from after the '$'
                location = std::atoi(line_info.c_str() + 1);
            }
            else
            {
                // convert to 0-indexed lines
                location = std::atoi(line_info.c_str()) - 1;
            }

            // find position
            if (is_pc)
            {
                pc.m_pos = location;
            }
            else
            {
                bytecode::DebugSymbolSourceMap::Range range;
                if (pc.m_bytecode.m_debug_symbols->m_source_map.get_location_at(
                    ogm_ast_line_column_t{static_cast<int>(location), 0}, range, true)
                )
                {
                    pc.m_pos = range.m_address_start;
                }
                else
                {
                    goto no_line;
                }
            }
        }

        // add breakpoint
        m_suspend_points.emplace_back();
        SuspendPoint& sp = m_suspend_points.back();
        sp.m_type = SuspendPoint::BREAK;
        sp.m_temporary = temporary;
        sp.m_breakpoint = new SuspendPoint::BreakPoint();
        sp.m_breakpoint->m_pc = pc;

        // conditional breakpoint
        if (args.size() > 1 && starts_with(args.at(1), "if"))
        {
            // determine condition code
            std::stringstream condition;
            condition << "if (";
            condition << (args.at(1).c_str() + 2);
            for (auto iter = args.begin() + 2; iter != args.end(); ++iter)
            {
                condition << " " << *iter;
            }
            sp.m_condition_str = condition.str().substr(4);
            condition << ") { }";

            // compile  condition
            try
            {
                sp.m_condition_bc = compile_inline(condition.str(), sp.m_breakpoint->m_pc.m_bytecode);
            }
            catch (const InlineError& e) // for some reason this doesn't compile?
            {
                std::cout << "Condition is invalid.";
                std::cout << e.what();
                m_suspend_points.pop_back();
                return true;
            }

            sp.m_conditional = true;
            std::cout<< "Conditional breakpoint";
        }
        else
        {
            sp.m_conditional = false;
            std::cout << "Breakpoint";
        }
        std::cout << " " << m_suspend_points.size() <<  " placed at " << location_for_bytecode_stream(pc, false) << std::endl;
        return false;
    }
usage:
    {
        std::cout << "usage:\n\n";
        std::cout << "  break section:[line|$pc] [if <condition>]\n";
        std::cout << "ex. `break object0#collision#object1:32 if x > 60`";
    }
    return true;
no_section:
    {
        std::cout << "specified section not found.\n";
    }
    return true;
no_line:
    {
        std::cout << "specified line has no active code.\n";
    }
    return true;
}

void Debugger::set_tracing_addendums()
{
    m_config.m_trace_addendums = m_config.m_trace && !m_config.m_trace_only_section_names;
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
            if (m_config.m_trace_only_section_names)
            {
                std::cout << " (section names only)";
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
            std::cout << location_for_bytecode_stream(get_frame_pc(), m_config.m_show_asm, true) << std::endl;
            return;
        }
        else if (args[0] == "trace")
        {
            if (args[1] == "on")
            {
                m_config.m_trace = true;
                set_tracing_addendums();
            }
            else if (args[1] == "off")
            {
                m_config.m_trace = false;
                set_tracing_addendums();
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
        else if (args[0] == "trace-only-section-names")
        {
            if (args[1] == "on")
            {
                m_config.m_trace_only_section_names = true;
                m_config.m_trace = true;
                std::cout << "Tracing section names only." << std::endl;
            }
            else if (args[1] == "off")
            {
                m_config.m_trace_only_section_names = false;
                std::cout << "Tracing all." << std::endl;
            }
            else goto err_value_1;
            set_tracing_addendums();
            return;
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
    const bytecode::DebugSymbols* symbols = get_frame_pc().m_bytecode.m_debug_symbols.get();
    if (symbols)
    {
        const auto& locals = symbols->m_namespace_local;
        if (staticExecutor.m_varStackIndex + 1 < m_frames.at(m_current_frame).m_locals_start + locals.id_count())
        {
            std::cout << "Local variables for this stack frame have not yet been allocated.\n";
        }
        else
        {
            std::cout << "Local variables:\n";
            for (variable_id_t i = 0; i < locals.id_count(); ++i)
            {
                std::cout << std::setw(15) << locals.find_name(i) << std::setw(0) << ": "
                    << staticExecutor.m_varStack[m_frames.at(m_current_frame).m_locals_start + i] << std::endl;
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

void Debugger::cmd_info_ds(const std::vector<std::string>& args)
{
    if (args.size() != 2)
    {
        std::cout << "Usage: info ds <type> <expression>\n";
        std::cout << "Example: info ds list other.MyList\n";
    }
    else
    {
        std::string type = args[0];
        std::string expression = args[1];
        string_execute_inline("ogm_ds_info(ds_type_" + type + ", " + expression + ");");
    }
}

void Debugger::cmd_info_buffer(const std::vector<std::string>& args)
{
    if (args.size() == 0)
    {
        std::cout << "Usage: info buffer <type> <expression>\n";
        std::cout << "Example: info buffer other.MyBuffer\n";
    }
    else
    {
        std::string expression = join(args);
        string_execute_inline("ogm_buffer_info(" + expression + ");");
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
    else if (starts_with(topic, "ds ") || topic == "ds")
    {
        std::vector<std::string> arguments;
        split(arguments, topic);
        arguments.erase(arguments.begin(), arguments.begin() + 1);
        cmd_info_ds(arguments);
    }
    else if (starts_with(topic, "buffer ") || starts_with(topic, "buf ") || topic == "buffer" || topic == "buf")
    {
        std::vector<std::string> arguments;
        split(arguments, topic);
        arguments.erase(arguments.begin(), arguments.begin() + 1);
        cmd_info_buffer(arguments);
    }
    else if (topic == "pc" || topic == "program counter")
    {
        std::cout << location_for_bytecode_stream(staticExecutor.m_pc, m_config.m_show_asm, false) << std::endl;
    }
    else if (topic == "b" || topic == "breakpoint" || topic == "breakpoints" || topic == "suspendpoints" || topic == "spoints" || topic == "suspoints")
    {
        cmd_info_suspendpoints();
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

bytecode::Bytecode Debugger::compile_inline(std::string code, const ogm::bytecode::Bytecode& host, uint8_t retc, bool unscoped)
{
    // parse expression
    ogm_ast_t* ast;
    try
    {
        ast = ogm_ast_parse(code.c_str());
    }
    catch(const std::exception& e)
    {
        std::stringstream s;
        s << "While parsing the provided expression, an error occurred:\n";
        s << e.what() << std::endl;
        throw InlineError{ s.str() };
    }

    if (!ast)
    {
        throw InlineError{ "Unable to parse ast.\n" };
    }

    // compile expression
    bytecode::Bytecode bytecode;
    try
    {
        GenerateConfig cfg;
        cfg.m_return_is_suspend = true; // don't return -- this would pop the local variables.

        // unless unscoped is true (default false), prevent the compiler from
        // creating new local variables.
        cfg.m_no_locals = unscoped;
        if (host.m_debug_symbols && !unscoped)
        {
            // use host's local variables.
            cfg.m_existing_locals_namespace = &host.m_debug_symbols->m_namespace_local;
        }
        bytecode::ProjectAccumulator acc{staticExecutor.m_frame.m_reflection, &staticExecutor.m_frame.m_assets, &staticExecutor.m_frame.m_bytecode, &ogm::interpreter::staticExecutor.m_frame.m_config};
        bytecode_generate(
            bytecode,
            {ast, "ogmdb anonymous bytecode section", code.c_str(), retc, 0},
            staticExecutor.m_library,
            &acc,
            &cfg
        );
    }
    catch(const std::exception& e)
    {
        std::stringstream s;
        s << "While compiling the provided expression, an error occurred.\n";
        s << e.what() << std::endl;
        ogm_ast_free(ast);
        throw InlineError{ s.str() };
    }
    ogm_ast_free(ast);

#if 0
    std::stringstream ss;
    bytecode_dis(bytecode::BytecodeStream(bytecode), ss, staticExecutor.m_library, staticExecutor.m_frame.m_reflection);
    std::cout << ss.str();
#endif

return bytecode;
}

void Debugger::execute_inline(const bytecode::Bytecode& bytecode, interpreter::Variable* outv)
{
    // execute expression.
    bool temp_paused = m_paused;
    m_paused = false;
    m_ls_state = LS_NONE;
    m_executing_inline = true;

    // we do some sketchiness with the stack here to
    // allow using the same local variables as the host, and
    // to fix the stack if an error occurs.
    staticExecutor.m_return_addresses.push_back(staticExecutor.m_pc);
    size_t pre_locals_start = staticExecutor.m_locals_start;
    // use the local start index associated with the presently-selected frame.
    staticExecutor.m_locals_start = m_frames.at(m_current_frame).m_locals_start;
    size_t pre_varStackIndex = staticExecutor.m_varStackIndex;
    try
    {
        // run the bytecode
        staticExecutor.m_pc = bytecode;

        // skip the starting `all` call.
        if (*bytecode.m_data == bytecode::opcode::all)
        {
            staticExecutor.m_pc.m_pos = 5;
        }

        // run the bytecode with the same locals as outer stack.
        ogm::interpreter::execute_resume();

        // after suspending, read variables from stack.
        if (bytecode.m_retc != 0)
        {
            ogm_assert(outv);
            for (size_t i = 0; i < bytecode.m_retc; --i)
            {
                outv[bytecode.m_retc - i - 1] = std::move(staticExecutor.pop());
            }
        }
    }
    catch(const std::exception& e)
    {
        // resume normal execution
        m_executing_inline = false;
        staticExecutor.m_pc = staticExecutor.m_return_addresses.back();
        staticExecutor.m_return_addresses.pop_back();
        staticExecutor.m_locals_start = pre_locals_start;
        m_paused = temp_paused;

        while (staticExecutor.m_varStackIndex > pre_varStackIndex)
        {
            staticExecutor.pop().cleanup();
        }

        // process exception
        std::stringstream s;
        s << "While executing the provided expression, an error occurred.\n";
        s << e.what() << std::endl;
        throw InlineError{ s.str() };
    }

    ogm_assert(staticExecutor.m_varStackIndex == pre_varStackIndex);

    // resume normal execution
    m_executing_inline = false;
    staticExecutor.m_pc = staticExecutor.m_return_addresses.back();
    staticExecutor.m_return_addresses.pop_back();
    staticExecutor.m_locals_start = pre_locals_start;
    m_paused = temp_paused;
}

void Debugger::string_execute_inline(std::string code)
{
    try
    {
        execute_inline(compile_inline(code, get_frame_pc().m_bytecode));
    }
    catch (const InlineError& e)
    {
        std::cout << e.what();
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

        string_execute_inline(expression);
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
    else if (topic == "info")
    {
        std::cout <<
        "Usage: info <topic>\n"
        "<topic> can be any of the following:\n\n"
        "b, breakpoint -- display all watchpoints and breakpoints.\n"
        "instance      -- show details of the current instance, including instance variables.\n"
        "locals        -- list local variables.\n"
        "stack         -- list what data is stored on the stack.\n"
        "pc            -- display the current program counter location.\n"
        ;
    }
    else if (topic == "print")
    {
        std::cout <<
        "usage: print <GML expression>\n"
        ;
    }
    else if (topic == "watch")
    {
        std::cout <<
        "[Not implemented] To watch a location for read and/or write:\n"
        "  watch -l -r -w <lvalue>\n"
        "To monitor a particular expression anytime execution is within this code section:\n"
        "  watch -e <expression>\n"
        "To monitor an expression from all frames (cannot use local variables):\n"
        "  watch -e -a <expression>\n"
        "  expression watchpoints are evaluated every cycle (or every cycle when within the\nrelevant code section), so this makes them very expensive.\n"
        "  Processing time can be reduced by evaluating only every n cycles (-n=<number>)"
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
        "trace-stack-count <amount>\n"
        ;
    }
    else
    {
        std::cout << "Unrecognized topic \"" << topic << "\". Type \"help\" for help.\n";
    }
}

void Debugger::cmd_frame_shift(int32_t frame_dst)
{
    // validate input
    if (frame_dst < 0 || frame_dst >= m_frames.size())
    {
        std::cout << "frame number out of bounds." << std::endl;
        if (frame_dst > 0)
        {
            std::cout << "Highest frame is #" << m_frames.size() << std::endl;
        }

        return;
    }

    // change frame
    m_current_frame = frame_dst;

    // display frame
    std::cout << "> #" << m_current_frame + 1 << ": "
        << location_for_bytecode_stream(
                get_frame_pc(),
                m_config.m_show_asm
            )
        << std::endl;
}

ogm::bytecode::BytecodeStream& Debugger::get_frame_pc()
{
    return get_frame_pc(m_current_frame);
}

ogm::bytecode::BytecodeStream& Debugger::get_frame_pc(size_t frame_index)
{
    StackFrame& sf = m_frames.at(frame_index);
    size_t ra_index = sf.m_return_address_index;
    if (sf.m_return_address_index >= staticExecutor.m_return_addresses.size())
    {
        return staticExecutor.m_pc;
    }

    return staticExecutor.m_return_addresses.at(ra_index);
}

void Debugger::cache_stack_frames()
{
    int32_t ra_index = staticExecutor.m_return_addresses.size();
    int32_t local_start = staticExecutor.m_locals_start;

    // put 1st frame on stack frame cache.
    m_frames.clear();
    m_frames.emplace_back(local_start, ra_index);
    m_stack_broken = false;

    // read locals start.

    while (local_start > 0)
    {
        const Variable& localsv = staticExecutor.m_varStack[local_start - 1];

        if (!localsv.is_integral() || ra_index <= 0)
        {
            m_stack_broken = true;
            break;
        }

        size_t new_start = localsv.castCoerce<size_t>();
        if (new_start >= local_start)
        {
            m_stack_broken = true;
            break;
        }

        if (new_start <= 0) break;

        m_frames.emplace_back(new_start, --ra_index);
        local_start = new_start;
    }
}

namespace
{
    inline char* duplicate_string(const char* c)
    {
        size_t len = strlen(c) + 1;
        char* _c = (char*)malloc(len);
        memcpy(_c, c, len);
        return _c;
    }
}

char** Debugger::autocomplete(const char*, int start, int end)
{
    const char* text = get_rl_buffer();
    std::vector<std::string_view> parsed;
    split(parsed, std::string_view(text, start));

    std::vector<char*> suggestions;
    std::string_view sv = std::string_view(text + start, end - start);

    #define TRY_MATCH(str) if (starts_with(str, sv)) suggestions.push_back(duplicate_string(str))

    if (parsed.empty())
    {
        // first word -- it's a command.
        TRY_MATCH("print");
        TRY_MATCH("execute");
        TRY_MATCH("info");
        TRY_MATCH("backtrace");
        TRY_MATCH("next");
        TRY_MATCH("step");
        TRY_MATCH("continue");
        TRY_MATCH("finish");
        TRY_MATCH("out");
        TRY_MATCH("break");
        TRY_MATCH("watch");
        TRY_MATCH("look");
        TRY_MATCH("list");
        TRY_MATCH("delete");
        TRY_MATCH("display");
        TRY_MATCH("udisplay");
        TRY_MATCH("help");
        TRY_MATCH("quit");
    }
    else if (parsed.at(0) == "b" || parsed.at(0) == "break")
    {
        bytecode_index_t bci = 0;
        while (staticExecutor.m_frame.m_bytecode.has_bytecode_NOMUTEX(bci))
        {
            Bytecode b = staticExecutor.m_frame.m_bytecode.get_bytecode_NOMUTEX(bci);
            if (b.m_debug_symbols)
            {
                const std::string& fullsec = b.m_debug_symbols->m_name;
                auto tagpos = fullsec.find("#");
                if (tagpos != std::string_view::npos)
                {
                    std::string subsec = fullsec.substr(0, tagpos);
                    TRY_MATCH(subsec.c_str());
                    TRY_MATCH(fullsec.c_str());
                }
                else
                {
                    TRY_MATCH(fullsec.c_str());
                }
            }
            ++bci;
        }
    }

    if (suggestions.empty())
    {
        return nullptr;
    }

    // create rl array from vector.
    // ostensibly, readline will delete this array and its contents later.
    char** suggestions_list = (char**)malloc(sizeof(char*) * (suggestions.size() + 2));

    std::string_view prefix = suggestions.at(0); // maximal prefix
    for (size_t i = 0; i < suggestions.size(); ++i)
    {
        suggestions_list[i + 1] = suggestions.at(i);
        prefix = common_substring(
             prefix, suggestions.at(i)
        );
    }
    suggestions_list[0] = duplicate_string(std::string{ prefix }.c_str());
    suggestions_list[suggestions.size() + 1] = nullptr;

    return suggestions_list;
}

}}
