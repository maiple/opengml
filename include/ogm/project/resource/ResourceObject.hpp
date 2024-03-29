#pragma once

#include "Resource.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/bytecode/bytecode.hpp"

#include <string>
#include <map>
#include <memory>

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
        struct Action
        {
            struct Argument
            {
                int32_t m_kind;
                std::string m_string;

                // location in file.
                ogm_location_t m_location;

                Argument()
                { }

                Argument(int32_t kind, std::string string)
                    : m_kind(kind)
                    , m_string(string)
                { }
            };

            std::vector<Argument> m_arguments;

            std::string m_who_name;
            std::string m_function_name;

            bool m_use_relative : 1;
            bool m_is_question : 1;
            bool m_use_apply_to : 1;
            bool m_relative : 1;
            bool m_is_not : 1;

            int32_t m_exetype;
            int32_t m_kind;
            int32_t m_id;
            int32_t m_lib_id;
        };

        int32_t m_event_type;
        int32_t m_enumb;
        std::string m_ename;

        // bytecode definition within the event.
        bytecode_index_t m_bytecode_index;
        std::vector<Action> m_actions;

        // the following will be set only if code was parsed
        // (i.e. after calling ResourceObject::parse)
        std::string m_source;
        std::unique_ptr<ogm_ast_t, ogm_ast_deleter_t> m_ast;

        friend std::ostream& operator<<(std::ostream& s, Event& e)
        {
            return s << e.m_event_type << ":" << e.m_enumb;
        }
    };

public:
    ResourceObject(const char* path, const char* name);
    ~ResourceObject();

    void load_file() override;
    void parse(const bytecode::ProjectAccumulator& acc) override;
    void assign_id(bytecode::ProjectAccumulator&) override;
    void precompile(bytecode::ProjectAccumulator&) override;
    void compile(bytecode::ProjectAccumulator&) override;
    const char* get_type_name() override { return "object"; };

    std::string m_path;

    // data fields
    real_t m_depth;
    bool m_visible;
    bool m_solid;
    bool m_persistent;
    std::string m_parent_name;
    std::string m_sprite_name;
    std::string m_mask_name;
    std::vector<Event> m_events;

    // only if compiled:
    asset::AssetObject* m_object_asset;
    asset_index_t m_object_index;

private:
    uint64_t m_edit_time;

private:
    void load_file_xml();
    void load_file_arf();
    void load_file_json();

    // assembles code string for the given event.
    void assign_event_string(Event&);
};

}}
