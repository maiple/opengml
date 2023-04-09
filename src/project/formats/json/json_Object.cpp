#include "json_parse.hpp"

#include "ogm/project/resource/ResourceObject.hpp"
#include "ogm/asset/Event.hpp"

#include "ogm/sys/util_sys.hpp"

namespace ogm::project
{
void ResourceObject::load_file_json()
{
    std::string dir = path_directory(m_path);
    std::fstream ifs(m_path);
    
    if (!ifs.good()) throw ResourceError(ErrorCode::F::file, this, "Error parsing file \"{}\"", m_path);
    
    json j;
    ifs >> j;
    
    checkModelName(j, "Object");
    
    m_v2_id = j.at("id").get<std::string>();
    m_visible = j.at("visible").get<bool>();
    m_solid = j.at("solid").get<bool>();
    m_parent_name = j.at("parentObjectId").get<std::string>();
    m_mask_name = j.at("maskSpriteId").get<std::string>();
    m_sprite_name = j.at("spriteId").get<std::string>();
    
    if (j.find("eventslist") != j.end())
    {
        const json& events = j.at("eventList");
        for (const json& event : events)
        {
            Event& ev = m_events.emplace_back();
            ev.m_event_type = event.at("eventtype").get<int32_t>();
            ev.m_enumb = event.at("enumb").get<int32_t>();
            ev.m_ename = event.at("collisionObjectId").get<std::string>();
            asset::DynamicEvent de = static_cast<asset::DynamicEvent>(ev.m_event_type);

            // file name containing event source
            std::string ev_source_path = path_join(dir, get_event_name_broad(de) + "_" + std::to_string(ev.m_enumb) + ".gml");

            std::string source = read_file_contents(ev_source_path);

            // create an (unmarked) action in this event to contain the code
            Event::Action& a = ev.m_actions.emplace_back();

            // code action
            a.m_lib_id = 1;
            a.m_kind = 7;
            a.m_id = 603;
            a.m_arguments.emplace_back(1, source);
        }
    }
}
}