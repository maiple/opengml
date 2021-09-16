#include "ogm/project/resource/ResourceObject.hpp"
#include "project/XMLError.hpp"
#include <pugixml.hpp>

namespace ogm::project
{
void ResourceObject::load_file_xml()
{
    std::string _path = native_path(m_path);
    std::string file_contents = read_file_contents(_path.c_str());
    m_edit_time = get_file_write_time(_path);

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(
        file_contents.c_str(),
        pugi::parse_default | pugi::parse_escapes | pugi::parse_comments
    );

    check_xml_result<ResourceError>(1062, result, _path.c_str(), "Error parsing object file: " + _path + "\n" + result.description(), this);

    pugi::xml_node node_object = doc.child("object");

    // read properties
    m_depth = atof(node_object.child("depth").text().get());
    m_visible = atoi(node_object.child("visible").text().get()) != 0;
    m_solid = atoi(node_object.child("solid").text().get()) != 0;
    m_persistent = atoi(node_object.child("persistent").text().get()) != 0;
    m_parent_name = node_object.child("parentName").text().get();
    m_sprite_name = node_object.child("spriteName").text().get();
    m_mask_name = node_object.child("maskName").text().get();

    trim(m_parent_name);
    trim(m_sprite_name);
    trim(m_parent_name);

    // aggregate events
    pugi::xml_node node_events = node_object.child("events");
    for (pugi::xml_node event: node_events.children("event"))
    {
        Event& ev = m_events.emplace_back();

        std::string event_type = event.attribute("eventtype").value();
        std::string enumb = event.attribute("enumb").value();

        ev.m_event_type = static_cast<int32_t>(stoi(event_type));
        ev.m_enumb = (enumb == "") ? 0 : static_cast<int32_t>(stoi(enumb));

        for (pugi::xml_node action: event.children("action"))
        {
            Event::Action& a = ev.m_actions.emplace_back();

            std::string action_kind = action.child("kind").text().get();
            std::string action_id = action.child("id").text().get();
            std::string action_libid = action.child("libid").text().get();

            a.m_who_name = action.child("whoName").text().get();

            a.m_function_name = action.child("functionname").text().get();

            a.m_use_relative = action.child("userelative").text().get() != "0";
            a.m_is_question = action.child("isquestion").text().get() != "0";
            a.m_use_apply_to = action.child("useapplyto").text().get() != "0";
            a.m_exetype = std::atoi(action.child("exetype").text().get());
            a.m_relative = action.child("relative").text().get() != "0";
            a.m_is_not = action.child("isnot").text().get() != "0";

            a.m_kind = std::atoi(action_kind.c_str());
            a.m_id = std::atoi(action_id.c_str());
            a.m_lib_id = std::atoi(action_libid.c_str());

            for (pugi::xml_node argument: action.child("arguments").children("argument"))
            {
                Event::Action::Argument& arg = a.m_arguments.emplace_back();
                arg.m_location = get_location_from_offset_in_file(
                    argument.offset_debug(),
                    file_contents.c_str(),
                    m_path.c_str()
                );
                arg.m_kind = std::atoi(argument.child("kind").text().get());
                if (argument.child("path"))
                {
                    arg.m_string = argument.child("path").text().get();
                }
                else if (argument.child("object"))
                {
                    arg.m_string = argument.child("object").text().get();
                }
                else if (argument.child("script"))
                {
                    arg.m_string = argument.child("script").text().get();
                }
                else
                {
                    arg.m_string = argument.child("string").text().get();
                }
            }
        }
    }
}
}