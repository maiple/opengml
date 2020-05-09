#pragma once

#include "ogm/bytecode/bytecode.hpp"

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace ogm { namespace project {

enum ResourceProgress
{
    NO_PROGRESS = 0,
    FILE_LOADED = 1 << 0,
    PARSED      = 1 << 1,
    PRECOMPILED = 1 << 2,
    COMPILED    = 1 << 3,
};

class Resource
{
    // how compiled is this?
    ResourceProgress m_progress = NO_PROGRESS;
public:
    virtual void load_file() { };
    virtual void parse(const bytecode::ProjectAccumulator&) { };
    virtual void assign_id(bytecode::ProjectAccumulator&)   { }
    virtual void precompile(bytecode::ProjectAccumulator&)  { }
    virtual void compile(bytecode::ProjectAccumulator&)     { }
    virtual const char* get_name() { return "<unknown resource>"; }
    // TODO: add precompile and compile

    // re-loads file (e.g. if edited)
    void reload_file()
    {
        m_progress = NO_PROGRESS;
        load_file();
    }

    // saves to disk
    // returns false on failure.
    virtual bool save_file()
    {
        return false;
    }

    virtual ~Resource() {}

protected:
    // applies progress marker, returns true if already applied.
    bool mark_progress(ResourceProgress rp)
    {
        if (m_progress & rp)
        {
            return true;
        }
        m_progress = static_cast<ResourceProgress>(rp | m_progress);
        return false;
    }
};

enum ResourceType {
  SPRITE,
  SOUND,
  BACKGROUND,
  PATH,
  SCRIPT,
  SHADER,
  FONT,
  TIMELINE,
  OBJECT,
  ROOM,
  CONSTANT,
  NONE
};

extern const char* RESOURCE_TYPE_NAMES[NONE];

extern const char* RESOURCE_TREE_NAMES[NONE];

}}
