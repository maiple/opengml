#ifndef STREAM_MACRO
#define STREAM_MACRO

#include "bytecode.hpp"

#include <sstream>

#define write(out, a) out.write((char*)(void*) &a, sizeof(decltype( a )))
#define write_op(out, op) {char __a = (char) op; write(out, __a);}
#define write_at(out, a, address) {bytecode_address_t __l = out.tellp(); out.seekp(address); write(out, a); out.seekp(__l);}
#define write_op_at(out, op, address) {bytecode_address_t __l = out.tellp(); out.seekp(address); write_op(out, op); out.seekp(__l);}

#define read(in, a) in.read<>(a)
#define read_op(in, op) {char __a; read(in, __a); op = static_cast<decltype( op )>(__a);}

static void write_ldi_string(std::ostream& out, const char* c, bool quotes=false)
{
    using namespace ogm::bytecode;
    write_op(out, opcode::ldi_string);
    int32_t len=1;
    bytecode_address_t lenloc = out.tellp();
    write(out, len);
    for (size_t i = quotes; c[i + quotes] != 0; i++)
    {
        out << c[i];
        len++;
    }
    out << (char) 0;
    write_at(out, len, lenloc);
}


#endif