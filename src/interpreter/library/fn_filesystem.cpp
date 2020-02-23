#include "libpre.h"
    #include "fn_filesystem.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"

#include <string>

#include <cctype>
#include <cstdlib>

#ifdef NATIVE_FILE_DIALOG
extern "C" {
#include <nfd.h>
}
#endif

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h>
#endif

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame (staticExecutor.m_frame)

void ogm::interpreter::fn::file_exists(VO out, V f)
{
    out = frame.m_fs.file_exists(f.castCoerce<std::string>());
}

void ogm::interpreter::fn::file_text_open_read(VO out, V f)
{
    out = static_cast<real_t>(frame.m_fs.open_file<FileAccessType::read>(f.castCoerce<std::string>()));
}

void ogm::interpreter::fn::file_text_open_write(VO out, V f)
{
    out = static_cast<real_t>(frame.m_fs.open_file<FileAccessType::write>(f.castCoerce<std::string>()));
}

void ogm::interpreter::fn::file_text_open_append(VO out, V f)
{
    out = static_cast<real_t>(frame.m_fs.open_file<FileAccessType::append>(f.castCoerce<std::string>()));
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

void ogm::interpreter::fn::file_text_writeln(VO out, V f)
{
    // TODO error-check index valid and correct access type
    FileHandle& fh = frame.m_fs.get_file_handle(f.castCoerce<std::int32_t>());
    (*fh.m_ofstream) << "\n";
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

void ogm::interpreter::fn::getv::working_directory(VO out)
{
    // TODO: confirm
    out = "." + std::string(1, PATH_SEPARATOR);
}

namespace
{
    std::string translate_filter(V filter)
    {
        std::string _filter = filter.castCoerce<std::string>();
        _filter = replace_all(_filter, ";", ",");
        _filter = replace_all(_filter, "|", ";");
        return _filter;
    }
}

#ifdef _WIN32
namespace
{
    char fbuff[MAX_PATH];
    char fbuff2[MAX_PATH];

    const char* open_dialog(bool save, const char* filter, const char* name, const char* dir=nullptr, const char* caption=nullptr)
    {
        OPENFILENAME ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFile = &fbuff[0];
        strcpy(fbuff, name);
        ofn.nMaxFile = sizeof(fbuff);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 0;
        ofn.lpstrFileTitle = &fbuff2[0];
        strcpy(fbuff2, name);
        ofn.nMaxFileTitle = sizeof(fbuff2);
        ofn.lpstrInitialDir = dir;
        ofn.lpstrTitle = caption;
        ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;

        if (save)
        {
            if (GetSaveFileName( &ofn ))
            {
                return ofn.lpstrFile;
            }
        }
        else
        {
            if (GetOpenFileName( &ofn ))
            {
                return ofn.lpstrFile;
            }
        }

        return "";
    }
}
#endif

// TODO: support fname and caption options.
void ogm::interpreter::fn::get_open_filename(VO out, V filter, V fname)
{
    #ifdef NATIVE_FILE_DIALOG
    std::string _filter = translate_filter(filter);
    char* outPath = nullptr;
    NFD_OpenDialog(_filter.c_str(), nullptr, &outPath);
    out = std::string(outPath);
    free(outPath);
    #elif defined(_WIN32)
    out = open_dialog(
        false,
        filter.castCoerce<std::string>().c_str(),
        fname.castCoerce<std::string>().c_str()
    );
    #else
    out = "";
    #endif
}

void ogm::interpreter::fn::get_save_filename(VO out, V filter, V fname)
{
    #ifdef NATIVE_FILE_DIALOG
    std::string _filter = translate_filter(filter);
    char* outPath = nullptr;
    NFD_SaveDialog(_filter.c_str(), nullptr, &outPath);
    out = std::string(outPath);
    free(outPath);
    #elif defined(_WIN32)
    out = open_dialog(
        true,
        filter.castCoerce<std::string>().c_str(),
        fname.castCoerce<std::string>().c_str()
    );
    #else
    out = "";
    #endif
}

void ogm::interpreter::fn::get_open_filename_ext(VO out, V filter, V fname, V dir, V caption)
{
    #ifdef NATIVE_FILE_DIALOG
    std::string _filter = translate_filter(filter);
    char* outPath = nullptr;
    NFD_OpenDialog(_filter.c_str(), dir.castCoerce<std::string>().c_str(), &outPath);
    out = std::string(outPath);
    free(outPath);
    #elif defined(_WIN32)
    out = open_dialog(
        false,
        filter.castCoerce<std::string>().c_str(),
        fname.castCoerce<std::string>().c_str(),
        dir.castCoerce<std::string>().c_str(),
        caption.castCoerce<std::string>().c_str()
    );
    #else
    out = "";
    #endif
}

void ogm::interpreter::fn::get_save_filename_ext(VO out, V filter, V fname,  V dir, V caption)
{
    #ifdef NATIVE_FILE_DIALOG
    std::string _filter = translate_filter(filter);
    char* outPath = nullptr;
    NFD_SaveDialog(_filter.c_str(), dir.castCoerce<std::string>().c_str(), &outPath);
    out = std::string(outPath);
    free(outPath);
    #elif defined(_WIN32)
    out = open_dialog(
        true,
        filter.castCoerce<std::string>().c_str(),
        fname.castCoerce<std::string>().c_str(),
        dir.castCoerce<std::string>().c_str(),
        caption.castCoerce<std::string>().c_str()
    );
    #else
    out = "";
    #endif
}
