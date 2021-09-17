#pragma once

#include "ogm/bytecode/bytecode.hpp"
#include "ogm/common/error.hpp"
#include "ogm/sys/util_sys.hpp"

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
    virtual ~Resource() {};
    virtual void load_file() { };
    virtual void parse(const bytecode::ProjectAccumulator&) { };
    virtual void assign_id(bytecode::ProjectAccumulator&);
    virtual void precompile(bytecode::ProjectAccumulator&)  { }
    virtual void compile(bytecode::ProjectAccumulator&)     { }
    virtual const char* get_type_name()=0;
    virtual const char* get_name() { return m_name.c_str(); }
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
    static bool is_valid_v2_id(const std::string& id);
    
    // stores an id-to-name map if id matches the id format
    // returns true if it was stored.
    static bool store_v2_id(bytecode::ProjectAccumulator&, const std::string& id, const std::string& name);
    
    // replaces the given reference with what its id maps to, if the id is in the id map.
    // returns true if a replacement occurred.
    static bool lookup_v2_id(bytecode::ProjectAccumulator&, std::string& io_reference);
};

class ResourceError : public ProjectError
{
    public:
    template<typename... P>
    ResourceError(error_code_t error_code, Resource* resource, const char* fmt, const P&... args)
        : ProjectError(error_code, fmt, args...)
    {
        detail<resource_type>(resource->get_type_name());
        detail<resource_name>(resource->get_name());
    }
};


enum ResourceType {
  SPRITE,
  SOUND,
  BACKGROUND, // aka 'tileset' in v2
  PATH,
  SCRIPT,
  SHADER,
  FONT,
  TIMELINE,
  OBJECT,
  ROOM,
  
  // the following resources have no associated asset type.
  AUDIOGROUP,
  CONSTANT,
  NONE
};

extern const char* RESOURCE_TYPE_NAMES[NONE];
extern const char* RESOURCE_TREE_NAMES[NONE];

template<typename ResourceType>
std::function<std::unique_ptr<Resource>()>
construct_resource(const std::string& path, const std::string& name)
{
    return [path, name]()
    {
        return std::make_unique<ResourceType>
            (path.c_str(), name.c_str());
    };
}

}}
