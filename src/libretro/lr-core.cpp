#include "libretro.h"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/Instance.hpp"

#include <memory>

RETRO_API unsigned retro_api_version(void)                                {return RETRO_API_VERSION;}

retro_environment_t retro_environment     = [](unsigned, void *)                        {return false;}
retro_video_refresh_t retro_video_refresh = [](const void*, unsigned, unsigned, size_t) {}
retro_audio_sample_t retro_audio_sample   = [](int16_t, int16_t)                        {}
retro_audio_sample_batch_t retro_audio_sample_batch = [](int16_t*, size_t)              {return 0;}
retro_input_poll_t retro_input_poll       = [](){}
retro_input_state_t retro_input_state     = [](unsigned, unsigned, unsigned, unsigned)  {return 0;}

RETRO_API void retro_set_environment(retro_environment_t f)               {retro_environment = f;}
RETRO_API void retro_set_video_refresh(retro_video_refresh_t f)           {retro_video_refresh = f;}
RETRO_API void retro_set_audio_sample(retro_audio_sample_t f)             {retro_audio_sample = f;}
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t f) {retro_audio_sample_batch = f;}
RETRO_API void retro_set_input_poll(retro_input_poll_t f)                 {retro_input_poll = f;}
RETRO_API void retro_set_input_state(retro_input_state_t f)               {retro_input_state = f;}

namespace
{
    enum {
        STATE_UNINIT, // has not yet initialized
        STATE_LOADED, // first frame only
        STATE_SUSPENDED, // when running
        STATE_IN_PROGRESS,
        STATE_COMPLETE, // game finished
    } g_state = STATE_UNINIT;
    
    std::unique_ptr<ogm::interpreter::Instance> anonymous
}

RETRO_API void retro_init(void)
{
    g_state = STATE_UNINIT;
}
RETRO_API void retro_deinit(void)
{
    g_state = STATE_UNINIT;
}

#ifndef VERSION
    #define VERSION OpenGML (development build)
#endif

#define _STR(A) #A
#define STR(A) _STR(A)
#define VERSION_STR STR(VERSION)

RETRO_API void retro_get_system_info(struct retro_system_info* info)
{
    // note that we allow the frontend to unzip .yyz and .gmz files
    // TODO: would it be preferable that we unzip them instead?
    
    info->library_name = "OpenGML";
    info->library_version = VERSION_STR;
    info->valid_extensions = "yyp|project.gmx|project.ogm|project.arf";
    info->need_fullpath = true;
    info->block_extract = false;
}

RETRO_API bool retro_load_game(const struct retro_game_info* info)
{
    // can only load by path
    if (!info) return false;
    if (info->data) return false;
    if (!info->path) return false;
    
    try
    {
        // TODO: this is a lot of junk that is required. Let's make it not required.
        ReflectionAccumulator reflection;
        ogm::interpreter::standardLibrary->reflection_add_instance_variables(reflection);
        ogm::interpreter::staticExecutor.m_frame.m_reflection = &reflection;
        ogm::bytecode::BytecodeTable& bytecode = ogm::interpreter::staticExecutor.m_frame.m_bytecode;

        // reserve bytecode slots. (It can expand if needed.)
        bytecode.reserve(4096);

        ogm::interpreter::staticExecutor.m_frame.m_data.m_sound_enabled = sound;
        
        ogm::project::Project project(info->path);
        project.process();
        ogm::bytecode::ProjectAccumulator acc{
            ogm::interpreter::standardLibrary,
            ogm::interpreter::staticExecutor.m_frame.m_reflection,
            &ogm::interpreter::staticExecutor.m_frame.m_assets,
            &ogm::interpreter::staticExecutor.m_frame.m_bytecode,
            &ogm::interpreter::staticExecutor.m_frame.m_config
        };
        if (!project.build(acc))
        {
            return false;
        }
        
        ogm::interpreter::staticExecutor.m_frame.m_fs.set_included_directory(acc.m_included_directory);
        
        // TODO: get rid of "anonymous" instance, replace with global instance.
        anonymous.reset(new ogm::interpreter::Instance());
        anonymous->m_data.m_frame_owner = &ogm::interpreter::staticExecutor.m_frame;
        ogm::interpreter::staticExecutor.m_library = ogm::interpreter::standardLibrary;
        ogm::interpreter::staticExecutor.m_self = anonymous.get();
        
        g_state = STATE_LOADED;
        
        return true;
    }
    catch (...)
    {
        g_state = STATE_UNINIT;
        return false;
    }
}

RETRO_API void retro_unload_game()
{
    g_state = STATE_UNINIT;
    anonymous.reset();
}
 
RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device)
{
    // TODO
}

RETRO_API void retro_get_system_av_info(struct retro_system_av_info* info)
{
    // TODO
    info->geometry->base_width = 800;
    info->geometry->base_height = 600;
    info->geometry->max_width = 800;
    info->geometry->max_height = 600;
    info->geometry->aspect_ratio = -1.0f;
    info->timing->fps = 60.0;
    info->timing->sample_rate = 44100.0;
}

RETRO_API void retro_run(struct retro_system_av_info* info)
{
    // state must be either 'loaded' or 'suspended' to run a frame.
    if (g_state != STATE_LOADED && g_state != STATE_SUSPENDED)
    {
        return;
    }
    
    const bool first_frame = g_state == STATE_LOADED;
    g_state = STATE_IN_PROGRESS;
    const bool suspended = (first_frame)
        ? ogm::interpreter::execute_bytecode(0)
        : ogm::interpreter::resume_execution();
    
    g_state = (suspended)
        ? STATE_SUSPENDED
        : STATE_COMPLETE;
}

RETRO_API size_t retro_serialize_size(void)
{
    return 0;
}

RETRO_API bool retro_serialize(void *data, size_t size)
{
    // TODO
    return false;
}

RETRO_API bool retro_unserialize(const void *data, size_t size)
{
    // TODO
    return false;
}