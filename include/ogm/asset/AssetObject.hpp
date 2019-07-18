#ifndef OGM_PROJECT_OBJECT_HPP
#define OGM_PROJECT_OBJECT_HPP

#include "Asset.hpp"

#include <cassert>

namespace ogm
{
namespace asset
{

constexpr bytecode_index_t k_no_event = -1;

enum class StaticEvent
{
    CREATE,
    DESTROY,
    // alarm events are purely dynamic
    STEP_BEGIN,
    STEP,
    STEP_END,
    STEP_BUILTIN,
    // collision is dynamic
    // io is dyanimc
    // other is dynamic
    DRAW_BEGIN,
    DRAW,
    DRAW_END,
    // remaining draw events are dynamic
    // async is dynamic
    COUNT
};

enum class DynamicEvent
{
    CREATE = 0,
    DESTROY = 1,
    ALARM = 2,
    STEP = 3,
    COLLISION = 4,
    KEYBOARD = 5,
    MOUSE = 6,
    OTHER = 7,
    DRAW = 8,
};

enum class DynamicSubEvent
{
    NO_SUB = 0,
    STEP_NORMAL = 0,
    STEP_END = 1,
    STEP_BEGIN = 2,
    STEP_BUILTIN = 135,
    DRAW_NORMAL = 0,
    DRAW_END = 1,
    DRAW_BEGIN = 2,
    OTHER_GAMESTART = 2,
    OTHER_GAMEEND = 3,
    OTHER_ROOMSTART = 4,
    OTHER_ROOMEND = 5,
    OTHER_USER0 = 10,
    OTHER_USER1 = 11,
    OTHER_USER2 = 12,
    OTHER_USER3 = 13,
    OTHER_USER4 = 14,
    OTHER_USER5 = 15,
    OTHER_USER6 = 16,
    OTHER_USER7 = 17,
    OTHER_USER8 = 18,
    OTHER_USER9 = 19,
    OTHER_USER10 = 20,
    OTHER_USER11 = 21,
    OTHER_USER12 = 22,
    OTHER_USER13 = 23,
    OTHER_USER14 = 24,
    OTHER_USER15 = 25,
};

// converts dynamic event to a static event.
inline bool event_dynamic_to_static(DynamicEvent ev, DynamicSubEvent sev, StaticEvent& out_se)
{
    switch (ev)
    {
    case DynamicEvent::CREATE:
        out_se = StaticEvent::CREATE;
        return true;
    case DynamicEvent::DESTROY:
        out_se = StaticEvent::DESTROY;
        return true;
    case DynamicEvent::STEP:
        switch (sev)
        {
        case DynamicSubEvent::STEP_NORMAL:
            out_se = StaticEvent::STEP;
            return true;
        case DynamicSubEvent::STEP_BUILTIN:
            out_se = StaticEvent::STEP_BUILTIN;
            return true;
        case DynamicSubEvent::STEP_BEGIN:
            out_se = StaticEvent::STEP_END;
            return true;
        case DynamicSubEvent::STEP_END:
            out_se = StaticEvent::STEP_BEGIN;
            return true;
        default:
            break;
        }
    case DynamicEvent::DRAW:
        switch (sev)
        {
        case DynamicSubEvent::DRAW_NORMAL:
            out_se = StaticEvent::DRAW;
            return true;
        default:
            // TODO: implement these
            break;
        }
    default:
        break;
    }
    return false;
}

class AssetObject : public Asset
{
public:
    asset_index_t m_parent = -1;
    asset_index_t m_init_sprite_index = -1;
    asset_index_t m_init_mask_index = -1;
    double m_init_depth = 0;
    bool m_init_persistent = false;
    bool m_init_solid = false;
    bool m_init_visible = true;

private:
    // events (indexed according to StaticEvent)
    bytecode_index_t m_events_static[static_cast<size_t>(StaticEvent::COUNT)] = {
        k_no_event, k_no_event, k_no_event,
        k_no_event, k_no_event, k_no_event,
        k_no_event, k_no_event, k_no_event
    };

    std::map<std::pair<DynamicEvent, DynamicSubEvent>, bytecode_index_t> m_events_dynamic;

    // this assert warns to remind the editor to initialize the above array
    // up to StaticEvent::COUNT.
    // There is nothing else magical about the particular integral value.
    static_assert(static_cast<size_t>(StaticEvent::COUNT) == 9,
        "m_events_static incorrectly initialized.");

public:
    bytecode_index_t& static_event(StaticEvent event)
    {
        return m_events_static[static_cast<size_t>(event)];
    }

    bytecode_index_t static_event(StaticEvent event) const
    {
        return m_events_static[static_cast<size_t>(event)];
    }

    inline bool has_dynamic_event(DynamicEvent event, DynamicSubEvent subevent) const
    {
        // first check static events.
        StaticEvent ev;
        if (event_dynamic_to_static(event, subevent, ev))
        {
            return static_event(ev) != k_no_event;
        }

        // then check for dynamics
        if (m_events_dynamic.find(std::pair<DynamicEvent, DynamicSubEvent>(event, subevent)) != m_events_dynamic.end())
        {
            return m_events_dynamic.at(std::pair<DynamicEvent, DynamicSubEvent>(event, subevent)) != k_no_event;
        }
        return false;
    }

    bytecode_index_t& dynamic_event(DynamicEvent event, DynamicSubEvent subevent)
    {
        // first check static events.
        StaticEvent ev;
        if (event_dynamic_to_static(event, subevent, ev))
        {
            return static_event(ev);
        }

        // default initialize to k_no_event.
        if (m_events_dynamic.find(std::pair<DynamicEvent, DynamicSubEvent>(event, subevent)) == m_events_dynamic.end())
        {
            m_events_dynamic[std::pair<DynamicEvent, DynamicSubEvent>(event, subevent)] = k_no_event;
        }
        return m_events_dynamic[std::pair<DynamicEvent, DynamicSubEvent>(event, subevent)];
    }

    bytecode_index_t dynamic_event(DynamicEvent event, DynamicSubEvent subevent) const
    {
        // first check static events.
        StaticEvent ev;
        if (event_dynamic_to_static(event, subevent, ev))
        {
            return static_event(ev);
        }
        return m_events_dynamic.at(std::pair<DynamicEvent, DynamicSubEvent>(event, subevent));
    }
};

}
}

#endif
