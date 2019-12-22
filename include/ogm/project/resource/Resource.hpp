#pragma once

#include <string>
#include <unordered_map>
#include <vector>

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
    virtual void parse() { };
    virtual const char* get_name() { return "<unknown resource>"; }
    // TODO: add precompile and compile

    // re-loads file (i.e. if edited)
    void reload_file()
    {
        m_progress = NO_PROGRESS;
        load_file();
    }

    // saves to disk
    // returns false on failure.
    virtual bool save()
    {
        return false;
    }

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

struct ResourceTableEntry {
  ResourceTableEntry(ResourceType, const char* path, const char* name);
  ResourceTableEntry(ResourceType, Resource* m_ptr);
  ResourceTableEntry(const ResourceTableEntry&);
  ResourceTableEntry() {};
  Resource* get(std::vector<std::string>* init_files=nullptr);

  ResourceType m_type;

private:
  // path to resource (to construct if necessary)
  std::string m_path;
  std::string m_name;

  // pointer to resource (if realized)
  Resource* m_ptr;
};

// resource name -> RTE -> resource
typedef std::unordered_map<std::string, ResourceTableEntry> ResourceTable;

}}
