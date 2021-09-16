#include "ogm/project/resource/ResourceObject.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"
#include "project/XMLError.hpp"

#include "cache.hpp"
#include "macro.hpp"

#include <string>
#include <cstdlib>
#include <map>

using namespace ogm;

namespace ogm::project {

ResourceObject::ResourceObject(const char* path, const char* name)
    : Resource(name)
    , m_path(path)
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
    else if (ends_with(m_path, ".yy"))
    {
        load_file_json();
    }
    else
    {
        throw ResourceError(1015, this, "Unrecognized file extension for object file: \"{}\"", m_path);
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
            throw ResourceError(1019, this, "Can't handle non-standard-library actions.");
        }

        std::string whoName = action.m_who_name;

        if (whoName == "" || whoName == "&lt;undefined&gt;"  || whoName == "<undefined>")
        {
            whoName = "self";
        }

        #define argexpect(k) if (action.m_arguments.size() != k) { throw ResourceError(1020, this, "expected " #k " arguments to action."); }
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
                    ss_event << "{ /* begin code action */\n";
                    const char* source = action.m_arguments[0].m_location.m_source;
                    ss_event << fmt::format(
                        "#line \"{}\" {} {}\n",
                        source ? source : "",
                        action.m_arguments[0].m_location.m_source_line,
                        action.m_arguments[0].m_location.m_source_column
                    );

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
            // TODO: use formatting better.
            throw ResourceError(1021, this, "{}",
                "Not sure how to handle action in " + m_path + ", event_type="
                + std::to_string(event.m_event_type) + ", enumb=" + std::to_string(event.m_enumb) + ", action kind="
                + std::to_string(action.m_kind) + ", id=" + std::to_string(action.m_id)
            );
        }
    }

    event.m_source = ss_event.str();
}

void ResourceObject::parse(const bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(PARSED)) return;
    // assign bytecode indices to events and parse AST.
    for (Event& event : m_events)
    {
        #ifdef CACHE_AST
        // check for cached compiled ast...
        bool cache_hit = false;
        std::string cache_path;
        if (acc.m_config->m_cache)
        {
            ogm::asset::DynamicEvent _event = static_cast<ogm::asset::DynamicEvent>(event.m_event_type);
            ogm::asset::DynamicSubEvent _subevent = static_cast<ogm::asset::DynamicSubEvent>(event.m_enumb);
            std::string event_name = get_event_name(_event, _subevent);
            cache_path = m_path + "." + event_name + ".ast.ogmc";
            ogm_ast* ast;
            cache_hit = cache_load(ast, cache_path, m_edit_time);
            if (cache_hit)
            {
                event.m_ast = std::unique_ptr<ogm_ast_t, ogm_ast_deleter_t>{ ast };
            }
        }

        if (!cache_hit)
        #endif
        {
            try
            {
                // assign code string for event
                assign_event_string(event);

                // parse code string
                event.m_ast = std::unique_ptr<ogm_ast_t, ogm_ast_deleter_t>{
                    ogm_ast_parse(
                        event.m_source.c_str(), ogm_ast_parse_flag_no_decorations
                    )
                };
            }
            catch (ogm::Error& e)
            {
                throw e.detail<resource_event>(event.m_ename);
            }

            #ifdef CACHE_AST
            if (acc.m_config->m_cache)
            {
                cache_write(event.m_ast.get(), cache_path);
            }
            #endif
        }
    }
}

void ResourceObject::assign_id(bytecode::ProjectAccumulator& acc)
{
    Resource::assign_id(acc);
    m_object_asset = acc.m_assets->add_asset<asset::AssetObject>(m_name.c_str(), &m_object_index);
}

void ResourceObject::precompile(bytecode::ProjectAccumulator& acc)
{
    if (mark_progress(PRECOMPILED)) return;
    std::string object_name = m_name;
    std::string _path = native_path(m_path);
    
    // look up ID references
    lookup_v2_id(acc, m_parent_name);
    lookup_v2_id(acc, m_sprite_name);
    lookup_v2_id(acc, m_mask_name);

    // assign bytecode indices to events.
    for (auto& event : m_events)
    {
        // look up ID reference
        lookup_v2_id(acc, event.m_ename);
        
        bytecode::DecoratedAST ast{ event.m_ast.get() };
        bytecode::bytecode_preprocess(ast, *acc.m_reflection);
        if (ast.m_argc != 0 && ast.m_argc != static_cast<uint8_t>(-1))
        {
            throw ResourceError(1022, this, "Arguments are not provided to events.");
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
            throw ResourceError(1022, this, "Cannot find sprite asset with name \"{}\"", sprite_name);
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
            throw ResourceError(1022, this, "Cannot find mask (sprite) asset with name \"{}\"", sprite_name);
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

void ResourceObject::compile(bytecode::ProjectAccumulator& acc)
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
            throw ResourceError(1022, this, "Cannot find parent (object) asset with name {}", m_parent_name);
        }
    }

    // compile events
    for (const Event& event : m_events)
    {
        ogm::asset::DynamicEvent _event = static_cast<ogm::asset::DynamicEvent>(event.m_event_type);
        ogm::asset::DynamicSubEvent _subevent = static_cast<ogm::asset::DynamicSubEvent>(event.m_enumb);
        std::string event_name = get_event_name(_event, _subevent);
        std::string combined_name = m_name + "#" + event_name;

        bytecode::Bytecode b;

        #ifdef CACHE_BYTECODE
        bool cache_hit = false;
        std::string cache_path;
        if (acc.m_config->m_cache)
        {
            cache_path = m_path + "." + event_name + ".ogmc";
            cache_hit = cache_load(b, acc, cache_path);
        }

        if (!cache_hit)
        #endif
        {
            try
            {
                ogm::bytecode::bytecode_generate(
                    {event.m_ast.get(), combined_name.c_str(), event.m_source.c_str()},
                    acc,
                    nullptr,
                    event.m_bytecode_index
                );
            }
            catch (Error& e)
            {
                throw e.detail<resource_event>(event_name);
            }

            #ifdef CACHE_BYTECODE
            if (acc.m_config->m_cache)
            {
                cache_write(acc.m_bytecode.get_bytecode(event.m_bytecode_index), acc, cache_path);
            }
            #endif
        }
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

}