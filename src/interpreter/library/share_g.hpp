#pragma once

#include "ogm/interpreter/async/Async.hpp"

#include <vector>

// miscellaneous functions for exposing content between library implementation files.
namespace ogm::interpreter
{
    void async_update_audio(std::vector<std::unique_ptr<AsyncEvent>>&);
}
