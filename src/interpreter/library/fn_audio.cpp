#include "libpre.h"
    #include "fn_audio.h"
    #include "fn_buffer.h"
    #include "ogm/fn_ogmeta.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/BufferManager.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>
#include <cctype>
#include <cstdlib>

#ifdef OGM_SOLOUD
#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>
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
    std::unique_ptr<SoLoud::AudioSource> m_sample;
    real_t m_length = 0.0;
    #endif
    
    // sound instances not known to have stopped.
    std::set<sound_id_t> m_instances;
    
    // AssetSound id
    resource_id_t m_asset_id = k_no_asset;
    
    // is an audio buffer sound?
    bool m_is_buffer = false;
    
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

constexpr sound_id_t g_first_dynamic_resource_id = g_first_instance_id >> 4;
sound_id_t g_next_dynamic_resource_id = g_first_dynamic_resource_id;

// returns true if this is a sound resource
bool is_resource(sound_id_t id)
{
    if (id < 0) return false;
    if (id >= g_first_dynamic_resource_id && id < g_next_dynamic_resource_id) return true;
    if (!!frame.m_assets.get_asset<AssetSound*>(id)) return true;
    return false;
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
                SoLoud::AudioSource* audio_source;
                if (sound->m_streamed)
                {
                    auto* source = new SoLoud::WavStream();
                    source->load(sound->m_path.c_str());
                    resource.m_length = source->getLength();
                    audio_source = source;
                }
                else
                {
                    auto* source = new SoLoud::Wav();
                    source->load(sound->m_path.c_str());
                    resource.m_length = source->getLength();
                    audio_source = source;
                }
                resource.m_sample = std::unique_ptr<SoLoud::AudioSource>(
                    audio_source
                );
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
    const sound_id_t id = audio.castCoerce<sound_id_t>();
    out = is_resource(id) || is_instance(id);
}

void ogm::interpreter::fn::audio_get_name(VO out, V audio)
{
    const sound_id_t id = audio.castCoerce<sound_id_t>();
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
    const sound_id_t id = audio.castCoerce<sound_id_t>();
    resource_t* resource = get_resource_from_resource_id(id);
    if (resource && g_init)
    {
        const real_t priority = p.castCoerce<real_t>();
        const bool loop = l.cond();
        #ifdef OGM_SOLOUD
        // start sound paused
        if (resource->m_sample)
        {
            const int handle = soloud.play(
                *resource->m_sample,
                resource->m_volume,
                resource->m_pan, 
                true // paused
            );
            
            // set properties on handle
            if (loop)
            {
                soloud.setLooping(handle, loop);
            }
            
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
        }
        #endif
    }
    
    // default
    out = -1.0;
}

namespace
{
    // iterates through matching sounds, applying functionality per sound
    // cb:
    //   - takes an instance id
    //   - io: a variable to set as a return value
    //   - return non-zero to end traversal
    void per_instance(sound_id_t id, VO out, const std::function<int(instance_id_t, VO)>& cb)
    {
        if (is_instance(id) && instance_is_playing(id))
        {
            if (cb(id, out))
            {
                return;
            }
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
                        if (cb(sound_id, out))
                        {
                            return;
                        }
                    }
                    else
                    {
                        // sneak in some clean-up, we don't need this sound in the set as it is no longer playing.
                        resource->m_instances.erase(sound_id);
                    }
                }
            }
        }
    }
}

void ogm::interpreter::fn::audio_is_playing(VO out, V audio)
{
    const sound_id_t id = audio.castCoerce<sound_id_t>();
    out = false; // default value
    
    // find any single running instance.
    per_instance(id, out, [](sound_id_t, VO out) -> int {
        out = true;
        
        // exit
        return 1;
    });
}

void ogm::interpreter::fn::audio_is_paused(VO out, V audio)
{
    const sound_id_t id = audio.castCoerce<sound_id_t>();
    out = false; // default value
    
    // find any single paused instance.
    per_instance(id, out, [](sound_id_t id, VO out) -> int {
        if (instance_is_paused(id))
        {
            out = true;
            
            // exit
            return 1;
        }
        
        // continue
        return 0;
    });
}

void ogm::interpreter::fn::audio_stop_sound(VO out, V audio)
{
    const sound_id_t id = audio.castCoerce<sound_id_t>();
       
    per_instance(id, out, [](sound_id_t id, VO out) -> int {
        instance_stop(id);
        return 0;
    });
}

void ogm::interpreter::fn::audio_pause_sound(VO out, V audio)
{
    const sound_id_t id = audio.castCoerce<sound_id_t>();
        
    per_instance(id, out, [](sound_id_t id, VO out) -> int {
        instance_pause(id);
        return 0;
    });
}

void ogm::interpreter::fn::audio_resume_sound(VO out, V audio)
{
    const sound_id_t id = audio.castCoerce<sound_id_t>();
        
    per_instance(id, out, [](sound_id_t id, VO out) -> int {
        instance_resume(id);
        return 0;
    });
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
    const sound_id_t id = audio.castCoerce<sound_id_t>();
    const real_t position = pos.castCoerce<real_t>();
        
    per_instance(id, out, [position](sound_id_t id, VO out) -> int {
        instance_set_position(id, position);
        return 0;
    });
}

void ogm::interpreter::fn::audio_sound_pitch(VO out, V audio, V vpitch)
{
    const sound_id_t id = audio.castCoerce<sound_id_t>();
    real_t pitch = std::clamp<real_t>(vpitch.castCoerce<real_t>(), 1/256.0, 256.0);
    
    per_instance(id, out, [pitch](sound_id_t id, VO out) -> int {
        instance_set_pitch(id, pitch);
        return 0;
    });
}

void ogm::interpreter::fn::audio_sound_get_pitch(VO out, V audio)
{
    const sound_id_t id = audio.castCoerce<sound_id_t>();
    out = 0.0;
    per_instance(id, out, [](sound_id_t id, VO out) -> int {
        out = instance_get_pitch(id);
        return 1;
    });
}

void ogm::interpreter::fn::audio_sound_gain(VO out, V audio, V vgain, V vtime)
{
    const sound_id_t id = audio.castCoerce<sound_id_t>();
    const real_t volume = vgain.castCoerce<real_t>();
    const real_t time = vtime.castCoerce<real_t>();
    per_instance(id, out, [&volume, &time](sound_id_t id, VO out) -> int {
        instance_fade_volume(id, volume, time);
        return 0;
    });
}

void ogm::interpreter::fn::audio_sound_get_gain(VO out, V audio)
{
    const sound_id_t id = audio.castCoerce<sound_id_t>();
    out = 0.0;
    per_instance(id, out, [](sound_id_t id, VO out) -> int {
        out = instance_get_volume(id);
        return 1;
    });
}

void ogm::interpreter::fn::audio_sound_get_track_position(VO out, V audio)
{
    const sound_id_t id = audio.castCoerce<sound_id_t>();
    if (is_instance(id))
    {
        if (instance_t* instance = get_instance(id))
        {
            out = instance_get_position(id);
            return;
        }
    }
    // TODO: confirm...
    // apparently this shouldn't be able to be run on instances-per-resource.
    out = 0.0;
}

void ogm::interpreter::fn::audio_master_gain(VO out, V vgain)
{
    const real_t volume = std::clamp<real_t>(vgain.castCoerce<real_t>(), 0, 1);
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
    const sound_id_t id = audio.castCoerce<sound_id_t>();
    if (resource_t* resource = get_resource_from_resource_id(get_resource_id(id)))
    {
        #ifdef OGM_SOLOUD
        if (resource->m_sample)
        {
            out = resource->m_length;
            return;
        }
        #endif
    }
    out = 0.0;
}

void ogm::interpreter::fn::audio_system(VO out)
{
    // new system.
    out = static_cast<real_t>(constant::audio_new_system);
}

// audio buffers

void ogm::interpreter::fn::audio_create_buffer_sound(VO out, V vbuffer, V vfmt, V vsample_rate, V voffset, V vlength, V vchannels)
{
    Buffer& buffer = frame.m_buffers.get_buffer(vbuffer.castCoerce<buffer_id_t>());
    const bool is_s16 = vfmt.castCoerce<real_t>() == constant::buffer_s16;
    const bool is_u8 = vfmt.castCoerce<real_t>() == constant::buffer_u8;
    if (!(is_s16 ^ is_u8))
    {
        throw MiscError("Buffer format must be either buffer_s16 or buffer_u8.");
    }
    real_t sample_rate = vsample_rate.castCoerce<real_t>();
    int32_t offset = voffset.castCoerce<int32_t>();
    int32_t length = vlength.castCoerce<int32_t>();
    int32_t channels = vchannels.castCoerce<int32_t>();
    
    if (length < 0) throw MiscError("length must be >= 0");
    if (offset < 0) throw MiscError("offset must be >= 0");
    
    size_t ulength = static_cast<size_t>(length);
    size_t uoff = static_cast<size_t>(offset);
    
    if (ulength + uoff > buffer.size())
    {
        throw MiscError("length + offset exceeds buffer size.");
    }
    
    // create sound resource
    sound_id_t id = g_next_dynamic_resource_id++;
    resource_t& resource = g_resources[id];
    resource.m_is_buffer = true;
    
    // set sample data to buffer data
    // (TODO: stream from buffer instead using custom audio source)
    #ifdef OGM_SOLOUD
    unsigned char* data = buffer.get_data() + uoff;
    std::unique_ptr<SoLoud::Wav> wav{ new SoLoud::Wav() };
    if (is_u8)
    {
        wav->loadRawWave8(data, ulength, sample_rate, channels);
    }
    else if (is_s16)
    {
        wav->loadRawWave16(reinterpret_cast<int16_t*>(data), ulength, sample_rate, channels);
    }
    else
    {
        ogm_assert(false);
    }
    
    resource.m_length = length / sample_rate;
    resource.m_sample = std::move(wav);
    #endif
    
    out = static_cast<real_t>(id);
}

void ogm::interpreter::fn::audio_free_buffer_sound(VO out, V audio)
{
    const sound_id_t id = audio.castCoerce<sound_id_t>();
    auto iter = g_resources.find(id);
    if (iter != g_resources.end())
    {
        if (iter->second.m_is_buffer)
        {
            g_resources.erase(iter);
        }
    }
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
