#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

#include <string>
#include <cassert>
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogmi;
using namespace ogmi::fn;

#define frame staticExecutor.m_frame

void ogmi::fn::audio_is_playing(VO out, V audio)
{
    std::cout << "WARNING: unimplemented dummy: audio_is_playing" << std::endl;
    out = false;
}
