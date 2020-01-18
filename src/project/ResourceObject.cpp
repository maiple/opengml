#include "ogm/project/resource/ResourceObject.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"
#include "ogm/project/arf/arf_parse.hpp"

#include "macro.hpp"

#include <pugixml.hpp>
#include <string>
#include <cstdlib>
#include <map>

namespace
{
    static const std::map<std::string, std::pair<int32_t, int32_t>> k_name_map
    {
        { "create", {0, 0} },
        { "destroy", {1, 0} },
        { "step", {3, 0} },
        { "step_normal", {3, 0} },
        { "step_begin", {3, 1} },
        { "step_end", {3, 2} },
        { "game_start", {7, 2} },
        { "game_end", {7, 3} },
        { "room_start", {7, 4} },
        { "room_end", {7, 5} },
        { "animation_end", {7, 7} },
        { "draw", {8, 0} },
        { "draw_normal", {8, 0} },
        { "draw_begin", {8, 72} },
        { "draw_end", {8, 73} },
        { "draw_pre", {8, 76} },
        { "draw_post", {8, 77} },
        { "async_http", {7, 62} },
        { "async_network", {7, 68} },
    };

    std::pair<int32_t, int32_t> event_name_to_pair(const std::string_view name)
    {
        std::string _name{ name };
        _name = remove_prefix(std::string{ name }, "other_");

        auto iter = k_name_map.find(std::string{ _name });
        if (iter != k_name_map.end())
        {
            return iter->second;
        }

        if (starts_with(_name, "user"))
        {
            _name = remove_prefix(_name, "user");
            return { 7, 10 + std::stoi(_name) };
        }

        if (starts_with(_name, "alarm"))
        {
            _name = remove_prefix(std::string{ _name }, "alarm");
            return { 2, std::stoi(_name) };
        }

        throw MiscError("Unrecognized event name \"" + std::string{ name } + "\"");
    }
}

namespace ogm { namespace project {

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

ResourceObject::ResourceObject(const char* path, const char* name): m_path(path), m_name(name)
{ }

ResourceObject::~ResourceObject()
{ }

void ResourceObject::load_file()
{
    if (mark_progress(FILE_LOADED)) return;
    if (ends_with(m_path, ".gmx"))
    {
        load_file_xml();
    }
    else if (ends_with(m_path, ".ogm") || ends_with(m_path, ".arf"))
    {
        load_file_arf();
    }
    else
    {
        throw MiscError("Unrecognized file extension for object file " + m_path);
    }
}

void ResourceObject::load_file_arf()
{
    std::string _path = native_path(m_path);
    std::string file_contents = read_file_contents(_path.c_str());

    ARFSection object_section;

    try
    {
        arf_parse(arf_object_schema, file_contents.c_str(), object_section);
    }
    catch (std::exception& e)
    {
        throw MiscError(
            "Error parsing object file \"" + _path + "\": " + e.what()
        );
    }

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
            throw MiscError("Unspecified event.");
        }

        if (is_digits(event_section->m_details.at(0)))
        {
            e.m_event_type = std::stoi(event_section->m_details.at(0));
        }
        else
        {
            auto pair = event_name_to_pair(event_section->m_details.at(0));
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
                }
            }
        }
    }
}

void ResourceObject::load_file_xml()
{
    std::string _path = native_path(m_path);
    std::string file_contents = read_file_contents(_path.c_str());

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(
        file_contents.c_str(),
        pugi::parse_default | pugi::parse_escapes | pugi::parse_comments
    );

    if (!result)
    {
        throw MiscError("Error parsing object file: " + _path);
    }

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

        ev.m_event_type = static_cast<size_t>(stoi(event_type));
        ev.m_enumb = (enumb == "") ? 0 : static_cast<size_t>(stoi(enumb));

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

void ResourceObject::assign_event_string(Event& event)
{
    // construct source code string
    size_t pending_end_parens = 0;
    std::vector<size_t> stalled_end_parens;

    bool skip_end_paren = false;

    std::stringstream ss_event;

    for (Event::Action& action : event.m_actions)
    {
        if (action.m_lib_id != 1)
        {
            throw MiscError("Can't handle non-standard-library functions.");
        }

        std::string whoName = action.m_who_name;

        if (whoName == "" || whoName == "&lt;undefined&gt;"  || whoName == "<undefined>")
        {
            whoName = "self";
        }

        #define argexpect(k) if (action.m_arguments.size() != k) { throw MiscError("expected " #k " arguments to action."); }
        #define whoif() if (whoName != "self") \
        { \
            ss_event << "with (" << whoName << ") "; \
        }

        switch(action.m_kind)
        {
        case 0:
            switch (action.m_id)
            {
            case 605:
                //comment
                skip_end_paren = true;
                break;
            default:
                {
                    if (action.m_function_name == "") goto error;
                    if (action.m_is_question)
                    {
                        ss_event << "var _ogm_result = 0;\n";
                    }
                    whoif();
                    if (action.m_is_question)
                    {
                        skip_end_paren = true;
                        ss_event << "_ogm_result = (";
                    }
                    ss_event << action.m_function_name << "(";
                    bool first = true;
                    for (Event::Action::Argument& argument : action.m_arguments)
                    {
                        if (argument.m_kind != 1 && argument.m_string.length() == 0)
                        {
                            continue;
                        }

                        if (!first)
                        {
                            ss_event << ", ";
                        }

                        if (argument.m_kind == 1)
                        {
                            // put quotes around string arg.
                            ss_event << '"' << argument.m_string << '"';
                        }
                        else
                        {
                            ss_event << argument.m_string;
                        }
                        first = false;
                    }
                    ss_event << ")";
                    if (action.m_is_question)
                    {
                        ss_event << ")\nif (!is_undefined(_ogm_result) && _ogm_result)\n{";
                        ++pending_end_parens;
                    }
                    else
                        ss_event << ";";
                    ss_event << "\n";
                    break;
                }
                break;
            }
            break;
        case 1:
            switch (action.m_id)
            {
                case 422:
                    ss_event << "{ // begin code block\n";
                    if (pending_end_parens > 0)
                        pending_end_parens--;
                    stalled_end_parens.push_back(pending_end_parens);
                    pending_end_parens = 0;
                    break;
                default:
                    goto error;
            }
            break;
        case 2:
            switch (action.m_id)
            {
                case 424:
                    ss_event << "}  // end code block\n";
                    if (stalled_end_parens.size() != 0) // check in case D&D is messed up
                    {
                        pending_end_parens = stalled_end_parens.back();
                        stalled_end_parens.pop_back();
                    }
                    break;
                default:
                    goto error;
            }
            break;
        case 6:
            switch (action.m_id)
            {
            case 611:
                {
                    argexpect(2)
                    whoif();
                    ss_event
                        << action.m_arguments[0].m_string
                        << " = ";
                    if (action.m_arguments[1].m_kind == 1)
                    {
                        // put quotes around string arg.
                        ss_event << '"';
                    }

                    ss_event << action.m_arguments[1].m_string << ";\n";

                    if (action.m_arguments[1].m_kind == 1)
                    {
                        ss_event << '"';
                    }
                }
                break;
            default:
                goto error;
            }
            break;
        case 7:
            switch (action.m_id)
            // code block
            {
            case 603:
                {
                    argexpect(1)
                    whoif();
                    ss_event << "{ /* begin code action */\n\n";

                    // read
                    std::string raw_code = action.m_arguments[0].m_string;

                    ss_event << raw_code;

                    // in addition to marking the end of the code action,
                    // the comment has the added effect of terminating any
                    // stray unended multiline comment.
                    ss_event << "\n/* end code action */\n}\n";
                }
                break;
            default:
                goto error;
            }
            break;
        default:
            goto error;
        }

        for (size_t i = 0; i < pending_end_parens; ++i)
        {
            ss_event << "}\n";
        }

        pending_end_parens = 0;

        if (false)
        {
        error:
            throw MiscError(
                "Not sure how to handle action in " + m_path + ", event_type="
                + std::to_string(event.m_event_type) + ", enumb=" + std::to_string(event.m_enumb) + ", action kind="
                + std::to_string(action.m_kind) + ", id=" + std::to_string(action.m_id)
            );
        }
    }

    event.m_source = ss_event.str();
}

void ResourceObject::parse()
{
    if (mark_progress(PARSED)) return;
    // assign bytecode indices to events and parse AST.
    for (Event& event : m_events)
    {
        try
        {
            // assign code string for event
            assign_event_string(event);

            // parse code string
            event.m_ast = ogm_ast_parse(
                event.m_source.c_str(), ogm_ast_parse_flag_no_decorations
            );
        }
        catch (const MiscError& e)
        {
            std::cout << "Error while parsing object " << m_name << " event " << event << "\n";
            throw e;
        }
        catch (const ParseError& e)
        {
            std::cout << "Error while parsing object " << m_name << " event " << event << "\n";
            throw e;
        }
    }
}

void ResourceObject::assign_id(bytecode::ProjectAccumulator& acc)
{
    m_object_asset = acc.m_assets->add_asset<asset::AssetObject>(m_name.c_str(), &m_object_index);
}

void ResourceObject::precompile(bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(PRECOMPILED)) return;
    std::string object_name = m_name;
    std::string _path = native_path(m_path);

    // assign bytecode indices to events.
    for (auto& event : m_events)
    {
        bytecode::DecoratedAST ast{ event.m_ast };
        bytecode::bytecode_preprocess(ast, *acc.m_reflection);
        if (ast.m_argc != 0 && ast.m_argc != static_cast<uint8_t>(-1))
        {
            throw MiscError("Arguments are not provided to events.");
        }
        event.m_bytecode_index = acc.next_bytecode_index();

        // has an input-listening event?
        if (
            static_cast<asset::DynamicEvent>(event.m_event_type) == asset::DynamicEvent::KEYBOARD   ||
            static_cast<asset::DynamicEvent>(event.m_event_type) == asset::DynamicEvent::MOUSE      ||
            static_cast<asset::DynamicEvent>(event.m_event_type) == asset::DynamicEvent::KEYPRESS   ||
            static_cast<asset::DynamicEvent>(event.m_event_type) == asset::DynamicEvent::KEYRELEASE
        )
        {
            // mark that instances must be registered as input listener
            m_object_asset->m_input_listener = true;
        }

        // has an async event
        if (
            static_cast<asset::DynamicEvent>(event.m_event_type) == asset::DynamicEvent::OTHER
            && event.m_enumb >= static_cast<int32_t>(asset::DynamicSubEvent::OTHER_ASYNC_IMAGE)
        )
        {
            // mark that instances must be registered as input listener
            m_object_asset->m_async_listener = true;
        }
    }

    // sprite
    std::string sprite_name = m_sprite_name;
    if (sprite_name != "" && sprite_name != "<undefined>")
    {
        asset_index_t sprite_asset_index;
        asset::Asset* spr = acc.m_assets->get_asset(sprite_name.c_str(), sprite_asset_index);
        if (spr)
        {
            m_object_asset->m_init_sprite_index = sprite_asset_index;
        }
        else
        {
            throw MiscError("Cannot find sprite asset with name " + sprite_name);
        }
    }

    // mask
    std::string mask_name = m_mask_name;
    if (mask_name != "" && mask_name != "<undefined>")
    {
        asset_index_t sprite_asset_index;
        asset::Asset* spr = acc.m_assets->get_asset(mask_name.c_str(), sprite_asset_index);
        if (spr)
        {
            m_object_asset->m_init_mask_index = sprite_asset_index;
        }
        else
        {
            throw MiscError("Cannot find mask (sprite) asset with name " + sprite_name);
        }
    }

    // defer parent asset lookup to later.
    // (b/c we're still parsing objects)

    // other properties
    m_object_asset->m_init_depth = m_depth;
    m_object_asset->m_init_visible = m_visible;
    m_object_asset->m_init_solid = m_solid;
    m_object_asset->m_init_persistent = m_persistent;
}

namespace
{
    std::string get_event_name_enum(size_t etype, size_t enumb, ogm::asset::DynamicEvent& event, ogm::asset::DynamicSubEvent& subevent)
    {
        using namespace ogm::asset;
        event = static_cast<DynamicEvent>(etype);
        subevent = static_cast<DynamicSubEvent>(enumb);

        switch (event)
        {
        case DynamicEvent::CREATE:
            return "create";
        case DynamicEvent::DESTROY:
            return "destroy";
        case DynamicEvent::ALARM:
            return "alarm" + std::to_string(static_cast<int32_t>(subevent));
        case DynamicEvent::STEP:
            switch (subevent)
            {
            case DynamicSubEvent::STEP_NORMAL:
                return "step";
            case DynamicSubEvent::STEP_BEGIN:
                return "step_begin";
            case DynamicSubEvent::STEP_END:
                return "step_end";
            case DynamicSubEvent::STEP_BUILTIN:
                return "step_builtin";
            }
            break;
        case DynamicEvent::KEYBOARD:
            return "key" + std::to_string(static_cast<int32_t>(subevent));
        case DynamicEvent::MOUSE:
            return "mouse" + std::to_string(static_cast<int32_t>(subevent)); // TODO
        case DynamicEvent::OTHER:
            switch (subevent)
            {
            case DynamicSubEvent::OTHER_GAMESTART:
                return "other_game_start";
            case DynamicSubEvent::OTHER_GAMEEND:
                return "other_game_end";
            case DynamicSubEvent::OTHER_ROOMSTART:
                return "other_room_start";
            case DynamicSubEvent::OTHER_ROOMEND:
                return "other_room_end";
            default:
                if (static_cast<int32_t>(subevent) >= static_cast<int32_t>(DynamicSubEvent::OTHER_USER0))
                {
                    int32_t ue = static_cast<int32_t>(subevent) - static_cast<int32_t>(DynamicSubEvent::OTHER_USER0);
                    return "other_user" + std::to_string(ue);
                }
            }
            break;
        case DynamicEvent::DRAW:
            switch (subevent)
            {
                case DynamicSubEvent::DRAW_NORMAL:
                    return "draw";
                case DynamicSubEvent::DRAW_BEGIN:
                    return "draw_begin";
                case DynamicSubEvent::DRAW_END:
                    return "draw_end";
                case DynamicSubEvent::DRAW_PRE:
                    return "draw_pre";
                case DynamicSubEvent::DRAW_POST:
                    return "draw_post";
            }
            break;
        case DynamicEvent::KEYPRESS:
            return "keypress" + std::to_string(static_cast<int32_t>(subevent)); // TODO
        case DynamicEvent::KEYRELEASE:
            return "keyrelease" + std::to_string(static_cast<int32_t>(subevent)); // TODO
        default:
            break;
        }
        return "unknown-" + std::to_string(static_cast<int32_t>(event)) + "-" + std::to_string(static_cast<int32_t>(subevent));
    }
}

void ResourceObject::compile(bytecode::ProjectAccumulator& acc, const bytecode::Library* library)
{
    if (mark_progress(COMPILED)) return;
    std::string object_name = m_name;

    // set parent
    if (m_parent_name != "" && m_parent_name != "<undefined>" && m_parent_name != "self")
    {
        asset_index_t object_asset_index;
        asset::Asset* parent = acc.m_assets->get_asset(m_parent_name.c_str(), object_asset_index);
        if (parent)
        {
            m_object_asset->m_parent = object_asset_index;
        }
        else
        {
            throw MiscError("Cannot find parent (object) asset with name " + m_parent_name);
        }
    }

    // compile events
    for (const Event& event : m_events)
    {
        ogm::asset::DynamicEvent _event;
        ogm::asset::DynamicSubEvent _subevent;
        std::string event_name = get_event_name_enum(event.m_event_type, event.m_enumb, _event, _subevent);
        std::string combined_name = object_name + "#" + event_name;

        bytecode::Bytecode b;
        try
        {
            ogm::bytecode::bytecode_generate(
                b,
                {event.m_ast, combined_name.c_str(), event.m_source.c_str()},
                library, &acc);
        }
        catch (std::exception& e)
        {
            throw MiscError("Error while compiling " + combined_name + ": " + e.what());
        }
        acc.m_bytecode->add_bytecode(event.m_bytecode_index, std::move(b));
        ogm_ast_free(event.m_ast);
        m_object_asset->dynamic_event(_event, _subevent) = event.m_bytecode_index;
    }

    m_events.clear();
}

#if 0
std::string ResourceObject::beautify(BeautifulConfig bc, bool dry) {
  std::string _path = native_path(path);
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_file(_path.c_str(), pugi::parse_default | pugi::parse_escapes | pugi::parse_comments);

  std::cout<<"beautify "<<_path<<std::endl;

  pugi::xml_node node_object = doc.child("object");
  pugi::xml_node node_events = node_object.child("events");
  // if no events, skip
  if (node_events.child("event").empty()) {
    return read_file_contents(path);
  }

  for (pugi::xml_node event : node_events.children("event")) {
    std::string event_type = event.attribute("eventtype").value();
    std::string enumb = event.attribute("enumb").value();
    int action_n = 0;
    for (pugi::xml_node action: event.children("action")) {
      // is code event:
      if (action.child("kind").text().get() == std::string("7") &&
          action.child("id").text().get() == std::string("603")) {
        std::string descriptor = _path + ", event type " + event_type + ", enumb " + enumb;
        std::cout<<"beautify "<<descriptor<<std::endl;
        // read
        auto node_code = action.child("arguments").child("argument").child("string");
        std::string raw_code = node_code.text().get();

        // skip @noformat
        if (raw_code.find("@noformat") != std::string::npos)
        {
          continue;
        }

        // test
        std::stringstream ss(raw_code);
        if (perform_tests(ss, bc))
          throw TestError("Error while testing " + descriptor);

        // beautify
        Parser p(raw_code);
        Production* syntree = p.parse();
        std::string beautiful = syntree->beautiful(bc).to_string(bc)+"\n";
        delete(syntree);

        if (!dry) {
          node_code.text() = beautiful.c_str();
        }
      }
      action_n ++;
    }
  }

  // reformatted object to string
  std::stringstream ssf;
  doc.save(ssf, "  ", pugi::format_default | pugi::format_no_empty_element_tags | pugi::format_no_declaration);
  std::string sf(ssf.str());

  // reformat pernicious <PhysicsShapePoints/> tag
  auto psp_index = sf.rfind("<PhysicsShapePoints>");
  if (psp_index != std::string::npos) {
    std::string sf_last(sf.substr(psp_index, sf.size() - psp_index));
    sf_last = replace_all(sf_last,"<PhysicsShapePoints></PhysicsShapePoints>","<PhysicsShapePoints/>");

    sf = sf.substr(0,psp_index) + sf_last;
  }

  // output beautified text
  if (!dry) {
    std::ofstream out(_path.c_str());
    out << sf;
  }
  return sf;
}

#endif

}}
