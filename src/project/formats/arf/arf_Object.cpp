#include "ogm/project/arf/arf_parse.hpp"
#include "ogm/common/util.hpp"
#include "ogm/project/resource/ResourceObject.hpp"
#include "project/XMLError.hpp"

namespace ogm::project
{

namespace
{
    ARFSchema arf_object_schema
    {
        "object",
        ARFSchema::DICT,
        {
            "event",
            ARFSchema::TEXT,
            {
                "action",
                ARFSchema::DICT,
                {
                    "argument",
                    ARFSchema::DICT
                }
            }
        }
    };
}

void ResourceObject::load_file_arf()
{
    std::string _path = native_path(m_path);
    std::string file_contents = read_file_contents(_path.c_str());
    m_edit_time = get_file_write_time(_path);

    ARFSection object_section;

    arf_parse(arf_object_schema, file_contents.c_str(), object_section);

    // defaults:
    m_depth = 0;
    m_visible = true;
    m_solid = false;
    m_persistent = false;
    m_parent_name = "";
    m_sprite_name = "";
    m_mask_name = "";

    // FIXME: replace with `object_section.get_value()`
    // depth
    auto iter = object_section.m_dict.find("depth");
    if (iter != object_section.m_dict.end())
    {
        m_depth = std::stod(iter->second);
    }

    // visible
    iter = object_section.m_dict.find("visible");
    if (iter != object_section.m_dict.end())
    {
        m_visible = iter->second != "0";
    }

    // solid
    iter = object_section.m_dict.find("solid");
    if (iter != object_section.m_dict.end())
    {
        m_solid = iter->second != "0";
    }

    // persistent
    iter = object_section.m_dict.find("persistent");
    if (iter != object_section.m_dict.end())
    {
        m_persistent = iter->second != "0";
    }

    // parent
    iter = object_section.m_dict.find("parent");
    if (iter != object_section.m_dict.end())
    {
        m_parent_name = iter->second;
    }

    iter = object_section.m_dict.find("parentName");
    if (iter != object_section.m_dict.end())
    {
        m_parent_name = iter->second;
    }

    // sprite
    iter = object_section.m_dict.find("sprite");
    if (iter != object_section.m_dict.end())
    {
        m_sprite_name = iter->second;
    }

    iter = object_section.m_dict.find("spriteName");
    if (iter != object_section.m_dict.end())
    {
        m_sprite_name = iter->second;
    }

    // mask
    iter = object_section.m_dict.find("mask");
    if (iter != object_section.m_dict.end())
    {
        m_mask_name = iter->second;
    }

    iter = object_section.m_dict.find("maskName");
    if (iter != object_section.m_dict.end())
    {
        m_mask_name = iter->second;
    }

    // events
    for (ARFSection* event_section : object_section.m_sections)
    {
        ogm_assert(event_section->m_name == "event");

        Event& e = m_events.emplace_back();

        if (event_section->m_details.size() == 0)
        {
            throw ResourceError(1017, this, "Unspecified event.");
        }

        if (is_digits(event_section->m_details.at(0)))
        {
            e.m_event_type = std::stoi(event_section->m_details.at(0));
        }
        else
        {
            auto pair = asset::event_name_to_pair(event_section->m_details.at(0));
            e.m_event_type = pair.first;
            e.m_enumb = pair.second;
        }

        if (event_section->m_details.size() > 1)
        {
            if (is_digits(event_section->m_details.at(1)))
            {
                e.m_enumb = std::stoi(event_section->m_details.at(1));
            }
            else
            {
                e.m_ename = event_section->m_details.at(1);
            }
        }

        if (!is_whitespace(event_section->m_text))
        {
            // unmarked action
            Event::Action& a = e.m_actions.emplace_back();

            // code action
            a.m_lib_id = 1;
            a.m_kind = 7;
            a.m_id = 603;
            a.m_arguments.emplace_back(1, event_section->m_text);
            a.m_arguments.back().m_location = 
                get_location_from_offset_in_file(
                    event_section->m_content_offset,
                    file_contents.c_str(),
                    m_path.c_str()
                );
        }

        // process actions
        for (ARFSection* as : event_section->m_sections)
        {
            Event::Action& a = e.m_actions.emplace_back();

            if (as->m_details.empty())
            {
                // code action
                a.m_lib_id = 1;
                a.m_kind = 7;
                a.m_id = 603;
                a.m_arguments.emplace_back(1, as->m_text);
                a.m_arguments.back().m_location = 
                get_location_from_offset_in_file(
                    as->m_content_offset,
                    file_contents.c_str(),
                    m_path.c_str()
                );
            }
            else
            {
                a.m_who_name = as->get_value(
                    "whoName",
                    as->get_value("who_name", "")
                );
                a.m_function_name = as->get_value(
                    "functionName",
                    as->get_value("function_name", "")
                );
                a.m_relative = as->get_value("relative", "0") != "0";
                a.m_use_relative = as->get_value("use_relative",
                    as->get_value("relative", "0")
                ) != "0";
                a.m_is_question = as->get_value("is_question", "0") != "0";
                a.m_use_apply_to = as->get_value("use_apply_to", "-1") != "0";
                a.m_is_not = as->get_value("is_not", "0") != "0";
                a.m_exetype = std::stoi(as->get_value("m_exetype", "1"));

                // arguments
                for (ARFSection* args : as->m_sections)
                {
                    Event::Action::Argument& arg = a.m_arguments.emplace_back();
                    arg.m_kind = std::stoi(args->get_value("kind", "1"));
                    arg.m_string = std::stoi(args->get_value("string", "1"));
                    arg.m_location = 
                    get_location_from_offset_in_file(
                        args->m_content_offset,
                        file_contents.c_str(),
                        m_path.c_str()
                    );
                }
            }
        }
    }
}
}