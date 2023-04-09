#include "ogm/asset/Event.hpp"
#include "ogm/common/util.hpp"
#include <cassert>

namespace ogm::asset
{
const std::map<std::string, std::pair<unsigned char, unsigned char>> k_name_map
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
    { "async_audio_recording", {7, 73} },
    { "async_audio_playback", {7, 74} },
    { "async_system", {7, 75} },
};

std::pair<unsigned char, unsigned char> event_name_to_pair(const std::string_view name)
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

    throw MiscError("Unrecognized event name \"{}\"" + std::string{ name } + "\"");
}

static const char* EVENT_NAMES[]
{
    "create",
    "destroy",
    "alarm",
    "step",
    "collision",
    "keyboard",
    "mouse",
    "other",
    "draw",
    "keypress",
    "keyrelease",
};

std::string get_event_name_broad(ogm::asset::DynamicEvent event)
{
    size_t event_int = static_cast<size_t>(event);
    assert(event_int < sizeof(EVENT_NAMES) / sizeof(EVENT_NAMES[0]));
    return EVENT_NAMES[event_int];
}

std::string get_event_name(ogm::asset::DynamicEvent event, ogm::asset::DynamicSubEvent subevent)
{
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