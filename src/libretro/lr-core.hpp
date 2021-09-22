#include "libretro.h"
#include <cassert>

external retro_environment_t retro_environment;
external retro_video_refresh_t retro_video_refresh;
external retro_audio_sample_t retro_audio_sample;
external retro_audio_sample_batch_t retro_audio_sample_batch;
external retro_input_poll_t retro_input_poll;
external retro_input_state_t retro_input_state;