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
private:
    // how compiled is this?
    ResourceProgress m_progress = NO_PROGRESS;

protected:    
    Resource(const std::string& name)
        : m_name(name)
    { }
    
public:
    // resource name
    std::string m_name;
    
    // optional
    std::string m_v2_id;
    
public:
    virtual void load_file() { };
    virtual void parse(const bytecode::ProjectAccumulator&) { };
    virtual void assign_id(bytecode::ProjectAccumulator&);
    virtual void precompile(bytecode::ProjectAccumulator&)  { }
    virtual void compile(bytecode::ProjectAccumulator&)     { }
    virtual const char* get_name() { return "<unknown resource>"; }
    // TODO: add precompile and compile

    // re-loads file (e.g. if edited)
    inline void reload_file()
    {
        m_progress = NO_PROGRESS;
        load_file();
    }

    // saves to disk
    // returns false on failure.
    inline virtual bool save_file()
    {
        return false;
    }

protected:
    // applies progress marker, returns true if already applied.
    bool mark_progress(ResourceProgress rp);
    
    // returns true if the string matches v2 id format. (XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX)
    static bool is_id(const std::string& id);
    
    // stores an id-to-name map if id matches the id format
    // returns true if it was stored.
    static bool store_v2_id(bytecode::ProjectAccumulator&, const std::string& id, const std::string& name);
    
    // replaces the given reference with what its id maps to, if the id is in the id map.
    // returns true if a replacement occurred.
    static bool lookup_v2_id(bytecode::ProjectAccumulator&, std::string& io_reference);
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
