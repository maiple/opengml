#include "libpre.h"
    #include "fn_audio.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>
#include "ogm/common/error.hpp"

#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

// dummy
void ogm::interpreter::fn::audio_sound_gain(VO out, V audio, V p, V l)
{ }

// dummy
void ogm::interpreter::fn::audio_play_sound(VO out, V audio, V p, V l)
{ }

void ogm::interpreter::fn::audio_is_playing(VO out, V audio)
{
    // TODO
    out = false;
}

void ogm::interpreter::fn::audio_play_sound(VO out, V audio)
{
    // TODO
    out = false;
}

void ogm::interpreter::fn::audio_stop_sound(VO out, V audio)
{
    // TODO
    out = false;
}

void ogm::interpreter::fn::audio_set_gain(VO out, V audio)
{
    // TODO
    out = false;
}

void ogm::interpreter::fn::play_sound(VO out, V audio)
{
    asset_index_t index = frame.get_asset_index_from_variable<asset::AssetSound>(audio);
    frame.m_display->play_sfx(index, false);
}
