#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/ds/List.hpp"
#include "ogm/interpreter/Executor.hpp"

#include <string>
#include "ogm/common/error.hpp"
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame


void ogm::interpreter::fn::ds_exists(VO out, V id, V type)
{
    ds_index_t did = id.castCoerce<ds_index_t>();
    size_t dtype = type.castCoerce<size_t>();
    switch (dtype)
    {
    case 0:
        out = frame.m_ds_map.ds_exists(did);
        break;
    case 1:
        out = frame.m_ds_list.ds_exists(did);
        break;
    case 2:
        out = frame.m_ds_stack.ds_exists(did);
        break;
    case 3:
        out = frame.m_ds_grid.ds_exists(did);
        break;
    case 4:
        out = frame.m_ds_queue.ds_exists(did);
        break;
    case 5:
        out = frame.m_ds_priority.ds_exists(did);
        break;
    default:
        out = false;
        break;
    }
}
