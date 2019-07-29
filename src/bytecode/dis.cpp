#include "ogm/bytecode/bytecode.hpp"
#include "ogm/bytecode/Library.hpp"
#include "ogm/bytecode/stream_macro.hpp"

#include "ogm/common/error.hpp"

#include <cstddef>

namespace ogm { namespace bytecode {

typedef int32_t bytecode_address_t;

const char* opcode::opcode_string[] =
{
    "ldi_false",
    "ldi_true",
    "ldi_undef",
    "ldi_f32",
    "ldi_f64",
    "ldi_s32",
    "ldi_u64",
    "ldi_string",
    "inc",
    "dec",
    "incl",
    "decl",
    "add2",
    "sub2",
    "mult2",
    "fdiv2",
    "idiv2",
    "mod2",
    "lsh2",
    "rsh2",
    "gt",
    "lt",
    "gte",
    "lte",
    "eq",
    "neq",
    "bland",
    "blor",
    "blxor",
    "band",
    "bor",
    "bxor",
    "bnot",
    "cond",
    "ncond",
    "pcond",
    "sfx",
    "ufx",
    "all",
    "stl",
    "ldl",
    "sts",
    "lds",
    "sto",
    "ldo",
    "stg",
    "ldg",
    "stt",
    "ldt",
    "stp",
    "ldp",
    "stla",
    "ldla",
    "stsa",
    "ldsa",
    "stoa",
    "ldoa",
    "stga",
    "ldga",
    "stpa",
    "ldpa",
    "pop",
    "dup",
    "dup2",
    "dup3",
    "dupn",
    "nat",
    "wti",
    "wty",
    "wtd",
    "jmp",
    "bcond",
    "call",
    "ret",
    "sus",
    "nop",
    "eof",
};

const char* opcode::get_opcode_string(opcode::opcode_t op)
{
    if (static_cast<uint32_t>(op) <= opcode::eof)
    {
        return opcode::opcode_string[op];
    }
    else
    {
        return "?";
    }
}

template<bool porcelain>
void instruction_dis(bytecode::BytecodeStream& in, opcode::opcode_t op, std::ostream& out, const Library* library = nullptr, const ReflectionAccumulator* accumulator = nullptr)
{
    const bytecode::DebugSymbols* symbols = in.m_bytecode.m_debug_symbols.get();
    using namespace opcode;
    switch(op)
    {
    case ldi_false:
    case ldi_true:
    case ldi_undef:
        break;
    case ldi_f32:
        {
            float immf;
            read(in, immf);
            if (!porcelain)
            {
                out << " ";
            }
            out << immf;
        }
        break;
    case ldi_f64:
        {
            double immf;
            read(in, immf);
            if (!porcelain)
            {
                out << " ";
            }
            out << immf;
        }
        break;
    case ldi_s32:
        {
            int32_t immi;
            read(in, immi);
            if (!porcelain)
            {
                out << " ";
            }
            out << immi;
        }
        break;
    case ldi_u64:
        {
            uint64_t immu;
            read(in, immu);
            if (!porcelain)
            {
                out << " ";
            }
            out << immu;
        }
        break;
    case ldi_string:
        {
            std::string imms;
            size_t i = 0;
            while (true)
            {
                char c;
                read(in, c);
                if (c == 0)
                {
                    break;
                }
                imms.push_back(c);
            }
            if (!porcelain)
            {
                out << " \"";
            }
            out << imms;
            if (!porcelain)
            {
                out << "\"";
            }
        }
        break;
    case inc:
    case dec:
        break;
    case incl:
    case decl:
        {
            uint32_t var_id;
            read(in, var_id);
            if (!porcelain)
            {
                out << " $";
            }
            out << (int) var_id;
        }
        break;
    case add2:
    case sub2:
    case mult2:
    case fdiv2:
    case idiv2:
    case mod2:
    case lsh2:
    case rsh2:
    case lt:
    case lte:
    case gt:
    case gte:
    case eq:
    case neq:
    case bland:
    case blor:
    case blxor:
    case band:
    case bor:
    case bxor:
    case bnot:
    case cond:
    case ncond:
    case pcond:
        break;
    case all:
        {
            uint32_t id;
            read(in, id);
            if (!porcelain)
            {
                out << " ";
            }
            out << (int)id;
            break;
        }
    case stl:
    case ldl:
    case stla:
    case ldla:
        {
            uint32_t id;
            read(in, id);
            if (!porcelain)
            {
                out << " $";
            }
            out << (int)id;

            // check if there is a debug symbol name mapping for this variable.
            if (symbols && symbols->m_namespace_local.has_name(id) && !porcelain)
            {
                out << " (" << symbols->m_namespace_local.find_name(id) << ")";
            }
        }
        break;
    case sts:
    case lds:
    case sto:
    case ldo:
    case stsa:
    case ldsa:
    case stoa:
    case ldoa:
        {
            variable_id_t id;
            read(in, id);
            if (!porcelain)
            {
                out << " %";
            }

            out << id;

            // check if there is a name mapping for this variable.
            if (accumulator && accumulator->m_namespace_instance.has_name(id) && !porcelain)
            {
                out << " (" << accumulator->m_namespace_instance.find_name(id) << ")";
            }
        }
        break;
    case stg:
    case ldg:
    case stga:
    case ldga:
        {
            variable_id_t id;
            read(in, id);
            if (!porcelain)
            {
                out << " @" << id;
            }

            out << id;

            // check if there is a name mapping for this variable.
            if (accumulator && accumulator->m_namespace_global.has_name(id) &&! porcelain)
            {
                out << " (" << accumulator->m_namespace_global.find_name(id) << ")";
            }
        }
        break;
    case stt:
    case ldt:
    case stp:
    case ldp:
    case stpa:
    case ldpa:
        {
            variable_id_t id;
            read(in, id);
            if (!porcelain)
            {
                out << " %*";
            }
            out << id;
        }
        break;
    case pop:
    case dup:
    case dup2:
    case dup3:
        break;
    case dupn:
        {
            uint8_t c;
            read(in, c);
            if (!porcelain)
            {
                out << " ";
            }
            out << c;
        }
        break;
    case nat:
        {
            // look up name for address
            if (!porcelain)
            {
                out << " ";
            }
            if (!library->dis_function_name(in, out))
            {
                throw MiscError("cannot parse function call in bytecode.");
            }
        }
        break;
    case wti:
    case wty:
        break;
    case jmp:
    case bcond:
        {
            bytecode_address_t address;
            read(in, address);
            if (!porcelain)
            {
                out << " .";
            }
            out << address;
        }
        break;
    case call:
        {
            bytecode_index_t bsecid;
            uint8_t argc;
            read(in, bsecid);
            read(in, argc);
            if (!porcelain)
            {
                out << " :";
            }
            out << bsecid;
            if (!porcelain)
            {
                if (argc != 0)
                {
                    out << "(" << (int32_t)argc << ")";
                }
            }
            else
            {
                out << ":" << argc;
            }
        }
    break;
    case ret:
        {
            uint8_t count;
            read(in, count);
            if (!porcelain)
            {
                out << " ";
            }
            out << (int)count;
        }
        break;
    case sus:
        break;
    case nop:
        break;
    case eof:
        break;
    }
}

void bytecode_dis(bytecode::BytecodeStream in, std::vector<DisassembledBytecodeInstruction>& outInstructions, const Library* library)
{
    using namespace opcode;
    while (true)
    {
        auto pc = in.tellg();
        opcode_t op;
        read_op(in, op);
        std::stringstream ss;
        instruction_dis<true>(in, op, ss, library);
        outInstructions.emplace_back(op, pc, ss.str());
        if (op == eof)
        {
            break;
        }
    }
}

void bytecode_dis(bytecode::BytecodeStream in, std::ostream& out, const Library* library, const ReflectionAccumulator* accumulator, bool show_source_inline, size_t end_pos)
{
    using namespace opcode;
    out.setf(std::ios::hex);
    out.unsetf(std::ios::showbase);
    while (true)
    {
        auto pc = in.tellg();
        if (pc >= end_pos) {
            return;
        }

        if (show_source_inline && in.m_bytecode.m_debug_symbols && in.m_bytecode.m_debug_symbols->m_source)
        {
            const DebugSymbolSourceMap& map = in.m_bytecode.m_debug_symbols->m_source_map;
            std::vector<DebugSymbolSourceMap::Range> ranges;
            map.get_locations_at(pc, ranges);

            for (const DebugSymbolSourceMap::Range& range : ranges)
            {
                const ogm_ast_line_column lc = range.m_source_start;
                if (pc != range.m_address_start)
                {
                    // only show range on the first bytecode address line it applies to
                    continue;
                }
                const ogm_ast_line_column lce = range.m_source_end;
                const ogm_ast_line_column line_start{ lc.m_line, 0 };
                const char* source_pos = get_string_position_line_column(in.m_bytecode.m_debug_symbols->m_source, line_start.m_line, line_start.m_column);
                if (source_pos)
                {
                    while (*source_pos)
                    {
                        out << *source_pos;
                        if (*source_pos == '\n')
                        {
                            break;
                        }

                        ++source_pos;
                    }

                    // ^ location
                    for (size_t i = 0; i < lc.m_column - 1; ++i)
                    {
                        out << ' ';
                    }
                    if (lce.m_line != lc.m_line)
                    // multiple lines
                    {
                        out << "^... +" << (lce.m_line - lc.m_line) << " lines";
                    }
                    else
                    // single line
                    {
                        for (size_t i = 0; i < std::max(1, lce.m_column - lc.m_column); ++i)
                        {
                            out << '^';
                        }
                    }
                    out << std::endl;
                }
                else
                {
                    out << "<cannot find source mapping for line: " << lc.m_line << ", column: "
                        << lc.m_column << ">\n";
                }
            }
        }

        opcode_t op;
        read_op(in, op);
        out << pc;
        out << "  ";
        out << get_opcode_string(op);
        instruction_dis<false>(in, op, out, library, accumulator);
        out << std::endl;
        if (op == eof)
        {
            break;
        }
    }
}

}
}
