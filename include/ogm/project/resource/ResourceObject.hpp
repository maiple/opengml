#pragma once

#include "Resource.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/bytecode/bytecode.hpp"

#include <string>
#include <map>

// foreward declarations
namespace pugi
{
    class xml_document;
}

namespace ogm { namespace project {

class ResourceObject : public Resource
{
    struct Event
    {
        size_t m_event_type;
        size_t m_enumb;
        bytecode_index_t m_bytecode_index;

        std::string m_source;
        ogm_ast_t* m_ast;
    };
public:
    ResourceObject(const char* path, const char* name);
    ~ResourceObject();

    void load_file() override;
    void parse() override;
    void precompile(bytecode::ProjectAccumulator&);
    void compile(bytecode::ProjectAccumulator&, const bytecode::Library* library);

    asset::AssetObject* m_object_asset;
    asset_index_t m_object_index;
    std::string m_path;
    std::string m_name;
    std::string m_parent_name;
    std::vector<Event> m_events;
    std::string m_file_contents;
    pugi::xml_document* const m_doc;
};

}}
