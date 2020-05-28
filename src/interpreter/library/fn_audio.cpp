#include "libpre.h"
    #include "fn_audio.h"
    #include "ogm/fn_ogmeta.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>
#include <cctype>
#include <cstdlib>

#ifdef OGM_SOLOUD
#include <soloud.h>
#include <soloud_wav.h>
#endif

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

namespace ogm::interpreter::audio
{
    
typedef int64_t sound_id_t;
typedef sound_id_t resource_id_t;

#ifdef OGM_SOLOUD
SoLoud::Soloud soloud;
#endif
    
// data for a sound which can be instantiated
struct resource_t
{
    #ifdef OGM_SOLOUD
    SoLoud::Wav m_sample;
    #endif
    
    // sound instances not known to have stopped.
    std::set<sound_id_t> m_instances;
    
    // AssetSound id
    resource_id_t m_asset_id = k_no_asset;
    
    // default volume
    float m_volume = 1.0;
    
    // default pan
    float m_pan = 0.0;
};
    
// data for a playing sound instance.
struct instance_t
{
    #ifdef OGM_SOLOUD
    // soloud handle
    int m_handle;
    #endif
    resource_id_t m_resource_id = -1;
};


std::map<resource_id_t, resource_t> g_resources;
std::map<sound_id_t, instance_t> g_instances;

bool g_init = false;
    
constexpr sound_id_t g_first_instance_id = 0x10000000000000;
sound_id_t g_next_instance_id = g_first_instance_id;

// returns true if this is a sound resource
bool is_resource(sound_id_t id)
{
    return id >= 0 && !!(frame.m_assets.get_asset<AssetSound*>(id));
}

// returns true if this is a sound instance.
bool is_instance(sound_id_t id)
{
    return id >= g_first_instance_id && id < g_next_instance_id; 
}

resource_t* get_resource_from_resource_id(sound_id_t id)
{
    #ifdef OGM_SOLOUD
    if (is_resource(id) && g_init)
    {
        auto iter = g_resources.find(id);
        if (iter == g_resources.end())
        {
            // add resource
            AssetSound* sound = frame.m_assets.get_asset<AssetSound*>(id);
            if (sound)
            {
                resource_t& resource = g_resources[id];
                resource.m_asset_id = id;
                resource.m_volume = sound->m_volume;
                resource.m_pan = sound->m_pan;
                resource.m_sample.load(sound->m_path.c_str());
                return &resource;
            }
        }
        return &iter->second;
    }
    #endif
    return nullptr;
}

instance_t* get_instance(sound_id_t id)
{
    if (is_instance(id) && g_init)
    {
        auto iter = g_instances.find(id);
        if (iter == g_instances.end())
        {
            return nullptr;
        }
        return &iter->second;
    }
    else return nullptr;
}

// retrieves resource id for the given resource id or sound id.
// (if it's already a resource id, the resource id will be returned.)
resource_id_t get_resource_id(sound_id_t id)
{
    if (is_resource(id)) return id;
    else if (is_instance(id))
    {
        instance_t* instance = get_instance(id);
        return instance->m_resource_id;
    }
    else
    {
        return -1;
    }
}

bool instance_is_playing(sound_id_t id)
{
    if (!g_init) return false;
    if (instance_t* instance = get_instance(id))
    {
        #ifdef OGM_SOLOUD
        return soloud.isValidVoiceHandle(instance->m_handle);
        #endif
    }
    return false;
}

bool instance_is_paused(sound_id_t id)
{
    if (!g_init) return false;
    if (instance_t* instance = get_instance(id))
    {
        #ifdef OGM_SOLOUD
        return soloud.getPause(instance->m_handle);
        #endif
    }
    return false;
}

bool instance_stop(sound_id_t id)
{
    if (!g_init) return false;
    if (instance_t* instance = get_instance(id))
    {
        #ifdef OGM_SOLOUD
        soloud.stop(instance->m_handle);
        #endif
    }
    return false;
}

bool instance_pause(sound_id_t id)
{
    if (!g_init) return false;
    if (instance_t* instance = get_instance(id))
    {
        #ifdef OGM_SOLOUD
        soloud.setPause(instance->m_handle, true);
        #endif
    }
    return false;
}

bool instance_resume(sound_id_t id)
{
    if (!g_init) return false;
    if (instance_t* instance = get_instance(id))
    {
        #ifdef OGM_SOLOUD
        soloud.setPause(instance->m_handle, false);
        #endif
    }
    return false;
}

double instance_get_position(sound_id_t id)
{
    if (!g_init) return 0;
    if (instance_t* instance = get_instance(id))
    {
        #ifdef OGM_SOLOUD
        return soloud.getStreamPosition(instance->m_handle);
        #endif
    }
    return 0;
}

void instance_set_position(sound_id_t id, double position)
{
    if (!g_init) return;
    if (instance_t* instance = get_instance(id))
    {
        #ifdef OGM_SOLOUD
        soloud.seek(instance->m_handle, position);
        #endif
    }
}

void instance_set_pitch(sound_id_t id, double pitch)
{
    if (!g_init) return;
    if (instance_t* instance = get_instance(id))
    {
        #ifdef OGM_SOLOUD
        soloud.setRelativePlaySpeed(instance->m_handle, pitch);
        #endif
    }
}

double instance_get_pitch(sound_id_t id)
{
    if (!g_init) return 0;
    if (instance_t* instance = get_instance(id))
    {
        #ifdef OGM_SOLOUD
        return soloud.getRelativePlaySpeed(instance->m_handle);
        #endif
    }
    return 0;
}

void instance_fade_volume(sound_id_t id, double volume, double time)
{
    volume = std::clamp<double>(volume, 0, 1);
    if (!g_init) return;
    if (instance_t* instance = get_instance(id))
    {
        #ifdef OGM_SOLOUD
        if (time > 0)
        {
            soloud.fadeVolume(instance->m_handle, volume, time);
        }
        else
        {
            soloud.setVolume(instance->m_handle, volume);
        }
        #endif
    }
}

double instance_get_volume(sound_id_t id)
{
    if (!g_init) return 0;
    if (instance_t* instance = get_instance(id))
    {
        #ifdef OGM_SOLOUD
        return soloud.getVolume(instance->m_handle);
        #endif
    }
    return 0;
}

}
using namespace ogm::interpreter::audio;

void ogm::interpreter::fn::ogm_audio_init(VO out)
{
    #ifdef OGM_SOLOUD
    if (frame.m_data.m_sound_enabled)
    {
        auto result = soloud.init(
            SoLoud::Soloud::CLIP_ROUNDOFF,
            SoLoud::Soloud::MINIAUDIO
        );
        if (result == 0)
        {
            g_init = true;
            out = false;
            return;
        }
        else
        {
            std::cout << "Error " << result << " occurred initializing SoLoud (audio): " << soloud.getErrorString(result) << std::endl;
            out = true;
            return;
        }
    }
    #endif
    out = false;
}

void ogm::interpreter::fn::ogm_audio_deinit(VO out)
{
    #ifdef OGM_SOLOUD
    if (g_init)
    {
        soloud.deinit();
    }
    #endif
}

void ogm::interpreter::fn::audio_exists(VO out, V audio)
{
    sound_id_t id = audio.castCoerce<sound_id_t>();
    out = is_resource(id) || is_instance(id);
}

void ogm::interpreter::fn::audio_get_name(VO out, V audio)
{
    sound_id_t id = audio.castCoerce<sound_id_t>();
    resource_t* resource = get_resource_from_resource_id(get_resource_id(id));
    if (resource)
    {
        const char* name = frame.m_assets.get_asset_name(resource->m_asset_id);
        if (name)
        {
            out = std::string(name);
            return;
        }
    }
    
    // default
    out = std::string("");
}

void ogm::interpreter::fn::audio_play_sound(VO out, V audio, V p, V l)
{
    sound_id_t id = audio.castCoerce<sound_id_t>();
    resource_t* resource = get_resource_from_resource_id(id);
    if (resource && g_init)
    {
        real_t priority = p.castCoerce<real_t>();
        bool loop = l.cond();
        #ifdef OGM_SOLOUD
        // start sound paused
        int handle = soloud.play(
            resource->m_sample,
            resource->m_volume,
            resource->m_pan, 
            true // paused
        );
        
        // set properties on handle
        soloud.setLooping(handle, loop);
        
        // add to instance and resource maps
        instance_t& instance = g_instances[g_next_instance_id];
        instance.m_handle = handle;
        instance.m_resource_id = id;
        resource->m_instances.emplace(g_next_instance_id);
        
        // return and update id count
        out = static_cast<real_t>(g_next_instance_id);
        ++g_next_instance_id;
        
        // resume sound
        soloud.setPause(handle, false);
        return;
        #endif
    }
    
    // default
    out = -1.0;
}

void ogm::interpreter::fn::audio_is_playing(VO out, V audio)
{
    sound_id_t id = audio.castCoerce<sound_id_t>();
    if (is_instance(id))
    {
        out = instance_is_playing(id);
        return;
    }
    else if (is_resource(id))
    {
        if (resource_t* resource = get_resource_from_resource_id(id))
        {
            auto instances = resource->m_instances;
            for (sound_id_t sound_id : instances)
            {
                if (instance_is_playing(sound_id))
                {
                    out = true;
                    return;
                }
                else
                {
                    resource->m_instances.erase(sound_id);
                }
            }
        }
    }
    out = false;
}

void ogm::interpreter::fn::audio_is_paused(VO out, V audio)
{
    sound_id_t id = audio.castCoerce<sound_id_t>();
    if (is_instance(id))
    {
        out = instance_is_paused(id);
        return;
    }
    else if (is_resource(id))
    {
        if (resource_t* resource = get_resource_from_resource_id(id))
        {
            auto instances = resource->m_instances;
            for (sound_id_t sound_id : instances)
            {
                if (instance_is_playing(sound_id))
                {
                    if (instance_is_paused(sound_id))
                    {
                        out = true;
                        return;
                    }
                }
                else
                {
                    resource->m_instances.erase(sound_id);
                }
            }
        }
    }
    out = false;
}

void ogm::interpreter::fn::audio_stop_sound(VO out, V audio)
{
    sound_id_t id = audio.castCoerce<sound_id_t>();
    if (is_instance(id))
    {
        if (instance_t* instance = get_instance(id))
        {
            instance_stop(id);
        }
    }
    else if (is_resource(id))
    {
        if (resource_t* resource = get_resource_from_resource_id(id))
        {
            for (sound_id_t sound_id : resource->m_instances)
            {
                instance_stop(sound_id);
            }
            resource->m_instances.clear();
        }
    }
}

void ogm::interpreter::fn::audio_pause_sound(VO out, V audio)
{
    sound_id_t id = audio.castCoerce<sound_id_t>();
    if (is_instance(id))
    {
        if (instance_t* instance = get_instance(id))
        {
            instance_pause(id);
        }
    }
    else if (is_resource(id))
    {
        if (resource_t* resource = get_resource_from_resource_id(id))
        {
            for (sound_id_t sound_id : resource->m_instances)
            {
                instance_pause(sound_id);
            }
        }
    }
}

void ogm::interpreter::fn::audio_resume_sound(VO out, V audio)
{
    sound_id_t id = audio.castCoerce<sound_id_t>();
    if (is_instance(id))
    {
        if (instance_t* instance = get_instance(id))
        {
            instance_resume(id);
        }
    }
    else if (is_resource(id))
    {
        if (resource_t* resource = get_resource_from_resource_id(id))
        {
            for (sound_id_t sound_id : resource->m_instances)
            {
                instance_resume(sound_id);
            }
        }
    }
}

void ogm::interpreter::fn::audio_stop_all(VO out)
{
    for (auto& [resource_id, resource] : g_resources)
    {
        audio_stop_sound(out, resource_id);
    }
}

void ogm::interpreter::fn::audio_pause_all(VO out)
{
    for (auto& [resource_id, resource] : g_resources)
    {
        audio_pause_sound(out, resource_id);
    }
}

void ogm::interpreter::fn::audio_resume_all(VO out)
{
    for (auto& [resource_id, resource] : g_resources)
    {
        audio_resume_sound(out, resource_id);
    }
}

void ogm::interpreter::fn::audio_sound_set_track_position(VO out, V audio, V pos)
{
    sound_id_t id = audio.castCoerce<sound_id_t>();
    real_t position = pos.castCoerce<real_t>();
    if (is_instance(id))
    {
        if (instance_t* instance = get_instance(id))
        {
            instance_set_position(id, position);
        }
    }
    else if (is_resource(id))
    {
        if (resource_t* resource = get_resource_from_resource_id(id))
        {
            for (sound_id_t sound_id : resource->m_instances)
            {
                instance_set_position(sound_id, position);
            }
        }
    }
}

void ogm::interpreter::fn::audio_sound_pitch(VO out, V audio, V vpitch)
{
    sound_id_t id = audio.castCoerce<sound_id_t>();
    real_t pitch = std::clamp<real_t>(vpitch.castCoerce<real_t>(), 1/256.0, 256.0);
    if (is_instance(id))
    {
        if (instance_t* instance = get_instance(id))
        {
            instance_set_pitch(id, pitch);
        }
    }
    else if (is_resource(id))
    {
        if (resource_t* resource = get_resource_from_resource_id(id))
        {
            for (sound_id_t sound_id : resource->m_instances)
            {
                instance_set_pitch(sound_id, pitch);
            }
        }
    }
}

void ogm::interpreter::fn::audio_sound_get_pitch(VO out, V audio)
{
    sound_id_t id = audio.castCoerce<sound_id_t>();
    if (is_instance(id))
    {
        out = instance_get_pitch(id);
        return;
    }
    else if (is_resource(id))
    {
        if (resource_t* resource = get_resource_from_resource_id(id))
        {
            auto instances = resource->m_instances;
            for (sound_id_t sound_id : instances)
            {
                if (instance_is_playing(sound_id))
                {
                    out = instance_get_pitch(sound_id);
                    return;
                }
                else
                {
                    resource->m_instances.erase(sound_id);
                }
            }
        }
    }
    out = 0.0;
}

void ogm::interpreter::fn::audio_sound_gain(VO out, V audio, V vgain, V vtime)
{
    sound_id_t id = audio.castCoerce<sound_id_t>();
    real_t volume = vgain.castCoerce<real_t>();
    real_t time = vtime.castCoerce<real_t>();
    if (is_instance(id))
    {
        if (instance_t* instance = get_instance(id))
        {
            instance_fade_volume(id, volume, time);
        }
    }
    else if (is_resource(id))
    {
        if (resource_t* resource = get_resource_from_resource_id(id))
        {
            for (sound_id_t sound_id : resource->m_instances)
            {
                instance_fade_volume(sound_id, volume, time);
            }
        }
    }
}

void ogm::interpreter::fn::audio_sound_get_gain(VO out, V audio)
{
    sound_id_t id = audio.castCoerce<sound_id_t>();
    if (is_instance(id))
    {
        out = instance_get_volume(id);
        return;
    }
    else if (is_resource(id))
    {
        if (resource_t* resource = get_resource_from_resource_id(id))
        {
            auto instances = resource->m_instances;
            for (sound_id_t sound_id : instances)
            {
                if (instance_is_playing(sound_id))
                {
                    out = instance_get_volume(sound_id);
                    return;
                }
                else
                {
                    resource->m_instances.erase(sound_id);
                }
            }
        }
    }
    out = 0.0;
}

void ogm::interpreter::fn::audio_sound_get_track_position(VO out, V audio)
{
    sound_id_t id = audio.castCoerce<sound_id_t>();
    if (is_instance(id))
    {
        if (instance_t* instance = get_instance(id))
        {
            out = instance_get_position(id);
            return;
        }
    }
    out = 0.0;
}

void ogm::interpreter::fn::audio_master_gain(VO out, V vgain)
{
    real_t volume = std::clamp<real_t>(vgain.castCoerce<real_t>(), 0, 1);
    #ifdef OGM_SOLOUD
    soloud.setGlobalVolume(volume);
    #endif
}

void ogm::interpreter::fn::audio_channel_num(VO out, V vchannels)
{
    int32_t channels = std::max<int32_t>(0, vchannels.castCoerce<int32_t>());
    #ifdef OGM_SOLOUD
    soloud.setMaxActiveVoiceCount(channels);
    #endif
}

void ogm::interpreter::fn::audio_sound_length(VO out, V audio)
{
    sound_id_t id = audio.castCoerce<sound_id_t>();
    if (resource_t* resource = get_resource_from_resource_id(get_resource_id(id)))
    {
        #ifdef OGM_SOLOUD
        out = static_cast<real_t>(resource->m_sample.getLength());
        #endif
    }
    out = 0.0;
}

void ogm::interpreter::fn::audio_system(VO out)
{
    // new system.
    out = 1.0;
}

////////// sound_*

void ogm::interpreter::fn::sound_play(VO out, V audio)
{
    audio_play_sound(out, audio, 0, false);
}

void ogm::interpreter::fn::sound_exists(VO out, V audio)
{
    audio_exists(out, audio);
}

void ogm::interpreter::fn::sound_get_name(VO out, V audio)
{
    audio_get_name(out, audio);
}

void ogm::interpreter::fn::sound_loop(VO out, V audio)
{
    audio_play_sound(out, audio, 0, true);
}

void ogm::interpreter::fn::sound_stop(VO out, V audio)
{
    audio_stop_sound(out, audio);
}

void ogm::interpreter::fn::sound_stop_all(VO out)
{
    audio_stop_all(out);
}

void ogm::interpreter::fn::sound_is_playing(VO out, V audio)
{
    audio_is_playing(out, audio);
}