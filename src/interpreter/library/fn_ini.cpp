#include "libpre.h"
    #include "fn_ini.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"

#include <string>
#include <cstdlib>
#include <simpleini/SimpleIni.h>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame (staticExecutor.m_frame)

namespace
{
    // only ONE ini file allowed at a time
    CSimpleIniA* g_ini = nullptr;
    std::string g_strdata;
    std::string g_path;
    bool g_is_file;
    bool g_dirty;
}

#define CHECK_INI if (!g_ini) throw MiscError("No ini file loaded")

void ogm::interpreter::fn::ini_open(VO out, V vpath)
{
    if (g_ini) throw MiscError("Ini file is already open");
    std::string _path = vpath.castCoerce<std::string>();
    g_path = frame.m_fs.resolve_file_path(_path);
    g_is_file = true;
    g_dirty = false;
    g_ini = new CSimpleIniA();
    g_ini->SetUnicode(); //TODO: needed?
    g_ini->LoadFile(g_path.c_str());
}

void ogm::interpreter::fn::ini_open_from_string(VO out, V str)
{
    if (g_ini) throw MiscError("Ini file is already open");
    g_strdata = str.castCoerce<std::string>();
    g_is_file = false;
    g_dirty = false;
    g_ini->LoadData(g_strdata.c_str(), g_strdata.size());
}

void ogm::interpreter::fn::ini_close(VO out)
{
    CHECK_INI;

    g_ini->Save(g_strdata);
    if (g_is_file && g_dirty)
    {
        // save to file path.
        g_ini->SaveFile(g_path.c_str());
    }
    out = g_strdata;
    delete g_ini;
    g_ini = nullptr;
}

void ogm::interpreter::fn::ini_write_string(VO out, V section, V key, V value)
{
    CHECK_INI;
    g_dirty = true;
    std::string _sec = section.castCoerce<std::string>();
    std::string _key = key.castCoerce<std::string>();
    std::string _v = value.castCoerce<std::string>();
    g_ini->SetValue(
        _sec.c_str(), _key.c_str(), _v.c_str()
    );
}

void ogm::interpreter::fn::ini_read_string(VO out, V section, V key, V d)
{
    CHECK_INI;
    std::string _sec = section.castCoerce<std::string>();
    std::string _key = key.castCoerce<std::string>();
    std::string _vdef = d.castCoerce<std::string>();
    const char* value = g_ini->GetValue(
        _sec.c_str(), _key.c_str(), _vdef.c_str()
    );
    if (value)
    {
        out = std::string(value);
    }
    else
    {
        out = "";
    }
}

void ogm::interpreter::fn::ini_write_real(VO out, V section, V key, V value)
{
    CHECK_INI;
    g_dirty = true;
    std::string _sec = section.castCoerce<std::string>();
    std::string _key = key.castCoerce<std::string>();
    real_t v = value.castCoerce<real_t>();
    g_ini->SetValue(
        _sec.c_str(), _key.c_str(), std::to_string(v).c_str()
    );
}

void ogm::interpreter::fn::ini_read_real(VO out, V section, V key, V d)
{
    CHECK_INI;
    std::string _sec = section.castCoerce<std::string>();
    std::string _key = key.castCoerce<std::string>();
    real_t _def = d.castCoerce<real_t>();
    const char* value = g_ini->GetValue(
        _sec.c_str(), _key.c_str(), nullptr
    );
    
    if (value == nullptr)
    {
        out = _def;
        return;
    }
    
    try
    {
        real_t r = std::stod(value);
        out = r;
    }
    catch (...)
    {
        // backup in case no string was found.
        out = _def;
    }
}

void ogm::interpreter::fn::ini_section_exists(VO out, V section)
{
    CHECK_INI;
    out = g_ini->GetSectionSize(section.castCoerce<std::string>().c_str()) >= 0;
}

void ogm::interpreter::fn::ini_key_exists(VO out, V section, V key)
{
    CHECK_INI;
    out = false;
    std::string sec = section.castCoerce<std::string>();
    if (g_ini->GetSectionSize(sec.c_str()) >= 0)
    {
        out = g_ini->GetValue(sec.c_str(), key.castCoerce<std::string>().c_str()) != nullptr;
    }
}

void ogm::interpreter::fn::ini_section_delete(VO out, V section)
{
    CHECK_INI;
    g_dirty = true;
    out = g_ini->Delete(section.castCoerce<std::string>().c_str(), nullptr);
}

void ogm::interpreter::fn::ini_key_delete(VO out, V section, V key)
{
    g_dirty = true;
    CHECK_INI;
    out = false;
    std::string sec = section.castCoerce<std::string>();
    out = g_ini->Delete(
        section.castCoerce<std::string>().c_str(),
        key.castCoerce<std::string>().c_str()
    );
}
