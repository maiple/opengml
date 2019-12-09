#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace ogm { namespace project {

class Resource
{
public:
    virtual void load_file() { };
    virtual void parse() { };
    virtual const char* get_name() { return "<unknown resource>"; }
    // TODO: add precompile and compile
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
  Resource* get(std::vector<std::string>& init_files);
private:
  // pointer to resource (if realized)
  Resource* m_ptr;
  ResourceType m_type;

  // path to resource (to construct if necessary)
  std::string m_path;
  std::string m_name;
};

// resource name -> RTE -> resource
typedef std::unordered_map<std::string, ResourceTableEntry> ResourceTable;

}}
