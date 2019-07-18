#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"

#include <string>
#include <cassert>
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogmi;
using namespace ogmi::fn;

#define frame staticExecutor.m_frame

void ogmi::fn::event_perform_object(VO out, V object, V ev, V subev)
{
    // change event
    AssetObject* a = frame.get_asset_from_variable<AssetObject>(object);
    assert(a);
    DynamicEvent de = static_cast<DynamicEvent>(ev.castCoerce<size_t>());
    DynamicSubEvent dse = static_cast<DynamicSubEvent>(subev.castCoerce<size_t>());

    bytecode_index_t bytecode_index= frame.get_dynamic_event_bytecode(a, de, dse);
    if (bytecode_index != k_no_bytecode)
    {
        Bytecode b = frame.m_bytecode.get_bytecode(bytecode_index);
        Frame::EventContext e = frame.m_data.m_event_context;
        frame.m_data.m_event_context.m_event = de;
        frame.m_data.m_event_context.m_sub_event = dse;
        frame.m_data.m_event_context.m_object = object.castCoerce<asset_index_t>();
        execute_bytecode(b);
        // restore previous event.
        frame.m_data.m_event_context = e;
    }


}

void ogmi::fn::event_perform(VO out, V ev, V subev)
{
    Variable object_asset_index = static_cast<uint64_t>(staticExecutor.m_self->m_data.m_object_index);
    ogmi::fn::event_perform_object(
        out,
        object_asset_index,
        ev,
        subev
    );
}

void ogmi::fn::event_user(VO out, V vn)
{
    size_t n = vn.castCoerce<size_t>();
    Variable e = static_cast<size_t>(DynamicEvent::OTHER);
    Variable se = n + static_cast<size_t>(DynamicSubEvent::OTHER_USER0);
    ogmi::fn::event_perform(out, e, se);
    e.cleanup();
    se.cleanup();
}

void ogmi::fn::event_inherited(VO out)
{
    // we use the event context rather than self->m_data.m_object_instance.
    // This is so that if the parent object calls event_inherited(), it will
    // go onto the parent's parent instead.
    asset_index_t object_index = frame.m_data.m_event_context.m_object;
    const asset_index_t prev_object_index = object_index;
    if (AssetObject* object = frame.m_assets.get_asset<AssetObject*>(object_index))
    {
        asset_index_t parent_index = object->m_parent;
        if (AssetObject* parent = frame.m_assets.get_asset<AssetObject*>(parent_index))
        {
            DynamicEvent de = static_cast<DynamicEvent>(frame.m_data.m_event_context.m_event);
            DynamicSubEvent dse = static_cast<DynamicSubEvent>(frame.m_data.m_event_context.m_sub_event);
            bytecode_index_t bytecode_index= frame.get_dynamic_event_bytecode(parent, de, dse);
            if (bytecode_index != k_no_bytecode)
            {
                Bytecode b = frame.m_bytecode.get_bytecode(bytecode_index);
                frame.m_data.m_event_context.m_object = parent_index;
                execute_bytecode(b);
                frame.m_data.m_event_context.m_object = prev_object_index;
            }
        }
    }
}
