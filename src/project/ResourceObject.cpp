#include "ogm/project/resource/ResourceObject.hpp"
#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"
#include "macro.hpp"

#include <pugixml.hpp>
#include <string>
#include <cstdlib>

namespace ogm { namespace project {

ResourceObject::ResourceObject(const char* path, const char* name): m_path(path), m_name(name), m_doc(new pugi::xml_document())
{ }

ResourceObject::~ResourceObject()
{
    delete m_doc;
}

void ResourceObject::load_file()
{
    std::string _path = native_path(m_path);
    m_file_contents = read_file_contents(_path.c_str());

    pugi::xml_document& doc = *m_doc;
    pugi::xml_parse_result result = doc.load_string(m_file_contents.c_str(), pugi::parse_default | pugi::parse_escapes | pugi::parse_comments);

    if (!result)
    {
        throw MiscError("Error parsing object file: " + _path);
    }

    pugi::xml_node node_object = doc.child("object");

    // parse events
    pugi::xml_node node_events = node_object.child("events");
    for (pugi::xml_node event: node_events.children("event"))
    {
        std::stringstream ss_event;
        std::string event_type = event.attribute("eventtype").value();
        std::string enumb = event.attribute("enumb").value();
        for (pugi::xml_node action: event.children("action"))
        {
            #ifdef VERBOSE_COMPILE_LOG
            std::cout << "precompiling " << object_name << "#" << event_type << "_" << enumb << std::endl;
            #endif
            // is code action:
            std::string action_kind = action.child("kind").text().get();
            std::string action_id = action.child("id").text().get();
            if (action_kind == "0" && action_id == "605")
            // comment
            {
                continue;
            }
            else if (action_kind == "7" && action_id == "603")
            // code block
            {
                std::string whoName = action.child("whoName").text().get();
                if (whoName != "self")
                {
                    ss_event << "with (" << whoName << ") ";
                }
                ss_event << "{ /* begin code action */\n\n";

                // read
                auto node_code = action.child("arguments").child("argument").child("string");
                std::string raw_code = node_code.text().get();

                ss_event << raw_code;

                // in addition to marking the end of the code action,
                // the comment has the added effect of terminating any
                // stray unended multiline comment.
                ss_event << "\n/* end code action */\n}\n";
            }
            else
            {
                throw MiscError("Not sure how to handle action in " + m_path + ", event_type=" + event_type + ", enumb=" + enumb + ", action kind=" + action_kind + ", id=" + action_id);
            }
        }

        std::string event_str = ss_event.str();

        m_events.push_back({stoi(event_type), (enumb == "") ? 0 : stoi(enumb), 0, event_str, nullptr});
    }
}

void ResourceObject::parse()
{
    // assign bytecode indices to events and parse AST.
    for (auto& event : m_events)
    {
        event.m_ast = ogm_ast_parse(event.m_source.c_str());
    }
}

void ResourceObject::precompile(bytecode::ProjectAccumulator& acc)
{
    std::string object_name = m_name;
    m_object_asset = acc.m_assets->add_asset<asset::AssetObject>(object_name.c_str(), &m_object_index);
    std::string _path = native_path(m_path);

    // assign bytecode indices to events.
    for (auto& event : m_events)
    {
        uint8_t retc, argc;
        bytecode::bytecode_preprocess(*event.m_ast, retc, argc, *acc.m_reflection);
        if (argc != 0 && argc != static_cast<uint8_t>(-1))
        {
            throw MiscError("Arguments are not provided to events.");
        }
        event.m_bytecode_index = acc.next_bytecode_index();
    }

    pugi::xml_document& doc = *m_doc;

    pugi::xml_node node_object = doc.child("object");

    // initial properties

    // sprite
    std::string sprite_name = node_object.child("spriteName").text().get();
    trim(sprite_name);
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
    std::string mask_name = node_object.child("maskName").text().get();
    trim(mask_name);
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

    // parent (defer asset lookup to later)
    m_parent_name = node_object.child("parentName").text().get();
    trim(m_parent_name);

    // other properties
    m_object_asset->m_init_depth = atof(node_object.child("depth").text().get());
    m_object_asset->m_init_visible = atoi(node_object.child("visible").text().get());
    m_object_asset->m_init_solid = atoi(node_object.child("solid").text().get());
    m_object_asset->m_init_persistent = atoi(node_object.child("persistent").text().get());
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
        case DynamicEvent::STEP:
            switch (subevent)
            {
            case DynamicSubEvent::STEP_NORMAL:
                return "step_normal";
            case DynamicSubEvent::STEP_BEGIN:
                return "step_begin";
            case DynamicSubEvent::STEP_END:
                return "step_end";
            case DynamicSubEvent::STEP_BUILTIN:
                return "step_builtin";
            }
            break;
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
                if (static_cast<int32_t>(subevent) > static_cast<int32_t>(DynamicSubEvent::OTHER_USER0))
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
                    return "draw_normal";
            }
        }
        return "<unknown event>";
    }
}

void ResourceObject::compile(bytecode::ProjectAccumulator& acc, const bytecode::Library* library)
{
    std::string object_name = remove_suffix(path_leaf(m_path), ".object.gmx");
    ogm::asset::AssetObject* obj = m_object_asset;

    // set parent
    if (m_parent_name != "" && m_parent_name != "<undefined>")
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

        #ifdef VERBOSE_COMPILE_LOG
        std::cout << "compiling " << combined_name << std::endl;
        #endif

        bytecode::Bytecode b;
        ogm::bytecode::bytecode_generate(
            b,
            {event.m_ast, 0, 0, combined_name.c_str(), event.m_source.c_str()},
            library, &acc);
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

  for (pugi::xml_node event: node_events.children("event")) {
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
