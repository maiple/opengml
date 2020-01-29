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

#define dsgm staticExecutor.m_frame.m_ds_grid

void ogm::interpreter::fn::ds_grid_create(VO out, V w, V h)
{
    uint32_t width = w.castCoerce<uint32_t>();
    uint32_t height = h.castCoerce<uint32_t>();
    size_t index = dsgm.ds_new(width, height);

    DSGrid& grid = dsgm.ds_get(index);
    grid.m_data.reserve(width);
    for (size_t i = 0; i < width; ++i)
    {
        grid.m_data.emplace_back();
        grid.m_data.back().reserve(height);
        for (size_t j = 0; j < height; ++j)
        {
            grid.m_data.back().emplace_back();
            grid.m_data.back().back() = 0;
        }
    }

    out = static_cast<real_t>(index);
}

void ogm::interpreter::fn::ds_grid_destroy(VO out, V i)
{
    ds_index_t index = i.castCoerce<ds_index_t>();
    if (!dsgm.ds_exists(index))
    {
        throw MiscError("Attempted to destroy non-existent grid datastructure.");
    }

    // clean up contents
    DSGrid& grid = dsgm.ds_get(index);
    for (auto& vec : grid.m_data)
    {
        for (Variable& v : vec)
        {
            v.cleanup();
        }
    }

    dsgm.ds_delete(index);
}

void ogm::interpreter::fn::ds_grid_width(VO out, V i)
{
    ds_index_t index = i.castCoerce<ds_index_t>();
    if (!dsgm.ds_exists(index))
    {
        throw MiscError("Attempted to destroy non-existent grid datastructure.");
    }
    DSGrid& grid = dsgm.ds_get(index);

    out = static_cast<real_t>(grid.m_width);
}

void ogm::interpreter::fn::ds_grid_height(VO out, V i)
{
    ds_index_t index = i.castCoerce<ds_index_t>();
    if (!dsgm.ds_exists(index))
    {
        throw MiscError("Attempted to destroy non-existent grid datastructure.");
    }
    DSGrid& grid = dsgm.ds_get(index);

    out = static_cast<real_t>(grid.m_height);
}

void ogm::interpreter::fn::ds_grid_get(VO out, V i, V _x, V _y)
{
    ds_index_t index = i.castCoerce<ds_index_t>();
    if (!dsgm.ds_exists(index))
    {
        throw MiscError("Attempted to destroy non-existent grid datastructure.");
    }
    DSGrid& grid = dsgm.ds_get(index);

    size_t x = _x.castCoerce<size_t>();
    size_t y = _y.castCoerce<size_t>();
    if (x >= grid.m_data.size() || y >= grid.m_data.at(x).size())
    {
        // TODO: *carefully* approve this value.
        out = -1;
        return;
    }

    out.copy(grid.m_data.at(x).at(y));
}

void ogm::interpreter::fn::ds_grid_set(VO out, V i, V _x, V _y, V val)
{
    ds_index_t index = i.castCoerce<ds_index_t>();
    if (!dsgm.ds_exists(index))
    {
        throw MiscError("Attempted to destroy non-existent grid datastructure.");
    }
    DSGrid& grid = dsgm.ds_get(index);

    size_t x = _x.castCoerce<size_t>();
    size_t y = _y.castCoerce<size_t>();
    if (x >= grid.m_data.size() || y >= grid.m_data.at(x).size())
    {
        throw MiscError("Non-existent entry in grid.");
    }

    Variable& v = grid.m_data.at(x).at(y);
    v.copy(val);
    v.make_root();
}

void ogm::interpreter::fn::ds_grid_add(VO out, V i, V _x, V _y, V val)
{
    ds_index_t index = i.castCoerce<ds_index_t>();
    if (!dsgm.ds_exists(index))
    {
        throw MiscError("Attempted to destroy non-existent grid datastructure.");
    }
    DSGrid& grid = dsgm.ds_get(index);

    size_t x = _x.castCoerce<size_t>();
    size_t y = _y.castCoerce<size_t>();
    if (x >= grid.m_data.size() || y >= grid.m_data.at(x).size())
    {
        throw MiscError("Non-existent entry in grid.");
    }

    grid.m_data.at(x).at(y) += val;
    // TODO: consider another make_root, just in case += somehow casts to array..?
}
