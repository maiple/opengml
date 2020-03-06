#include "libpre.h"
    #include "fn_game.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/interpreter/Executor.hpp"

#include <string>
#include <iostream>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

void ogm::interpreter::fn::game_end(VO out)
{
    frame.m_data.m_prg_end = true;
}