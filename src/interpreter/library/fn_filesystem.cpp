#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"

#include <string>
#include <cassert>
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame (staticExecutor.m_frame)

void ogm::interpreter::fn::file_exists(VO out, V f)
{
    out = frame.m_fs.file_exists(f.castCoerce<std::string>());
}

void ogm::interpreter::fn::file_text_open_read(VO out, V f)
{
    out = frame.m_fs.open_file<FileAccessType::read>(f.castCoerce<std::string>());
}

void ogm::interpreter::fn::file_text_open_write(VO out, V f)
{
    out = frame.m_fs.open_file<FileAccessType::write>(f.castCoerce<std::string>());
}

void ogm::interpreter::fn::file_text_open_append(VO out, V f)
{
    out = frame.m_fs.open_file<FileAccessType::append>(f.castCoerce<std::string>());
}

void ogm::interpreter::fn::file_text_read_real(VO out, V f)
{
    // TODO error-check index valid and correct access type
    FileHandle& fh = frame.m_fs.get_file_handle(f.castCoerce<std::int32_t>());
    real_t r;
    (*fh.m_ifstream) >> r;
    out = r;
}

void ogm::interpreter::fn::file_text_read_string(VO out, V f)
{
    // TODO error-check index valid and correct access type
    FileHandle& fh = frame.m_fs.get_file_handle(f.castCoerce<std::int32_t>());
    std::string r;
    (*fh.m_ifstream) >> r;
    out = r;
}

void ogm::interpreter::fn::file_text_readln(VO out, V f)
{
    // TODO error-check index valid and correct access type
    FileHandle& fh = frame.m_fs.get_file_handle(f.castCoerce<std::int32_t>());
    std::string r;
    std::getline(*(fh.m_ifstream), r);
    out = r;
}

void ogm::interpreter::fn::file_text_write_real(VO out, V f, V v)
{
    // TODO error-check index valid and correct access type
    FileHandle& fh = frame.m_fs.get_file_handle(f.castCoerce<std::int32_t>());
    real_t r = v.castCoerce<real_t>();
    (*fh.m_ofstream) << r;
}

void ogm::interpreter::fn::file_text_write_string(VO out, V f, V v)
{
    // TODO error-check index valid and correct access type
    FileHandle& fh = frame.m_fs.get_file_handle(f.castCoerce<std::int32_t>());
    std::string s = v.castCoerce<string_t>();
    (*fh.m_ofstream) << s;
}

void ogm::interpreter::fn::file_text_eoln(VO out, V f)
{
    // TODO error-check index valid and correct access type
    FileHandle& fh = frame.m_fs.get_file_handle(f.castCoerce<std::int32_t>());
    out = (fh.m_ifstream->peek()) == '\n' || fh.m_ifstream->eof();
}

void ogm::interpreter::fn::file_text_eof(VO out, V f)
{
    // TODO error-check index valid and correct access type
    FileHandle& fh = frame.m_fs.get_file_handle(f.castCoerce<std::int32_t>());
    out = fh.m_ifstream->eof();
}


void ogm::interpreter::fn::file_text_close(VO out, V f)
{
    // TODO error-check index valid and correct access type
    frame.m_fs.close_file(f.castCoerce<std::int32_t>());
}
