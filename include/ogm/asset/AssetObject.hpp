#pragma once

#include "Asset.hpp"
#include "Event.hpp"

#include "ogm/common/error.hpp"

namespace ogm::asset {

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

    // instances of this class need to be registered as input listeners.
    bool m_input_listener = false;

    // instances of this class need to be registered as async listeners.
    bool m_async_listener = false;

private:
    // events (indexed according to StaticEvent)
    bytecode_index_t m_events_static[static_cast<size_t>(StaticEvent::COUNT)] = {
        k_no_event, k_no_event, k_no_event,
        k_no_event, k_no_event, k_no_event,
        k_no_event, k_no_event, k_no_event
    };

    std::map<DynamicEventPair, bytecode_index_t> m_events_dynamic;

    // this ogm_assert warns to remind the editor to initialize the above array
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
