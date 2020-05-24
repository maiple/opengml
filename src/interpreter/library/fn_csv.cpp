#include "libpre.h"
    #include "fn_csv.h"
    #include "fn_ds.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"

#include <rapidcsv.h>

#include <string>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame (staticExecutor.m_frame)
#define fs frame.m_fs

void ogm::interpreter::fn::load_csv(VO out, V csv)
{
    std::string csv_path = fs.resolve_file_path(csv.castCoerce<std::string>());
    
    // -1, -1 allows first row and column to be treated as data rather than headers.
    rapidcsv::Document doc(csv_path, rapidcsv::LabelParams(-1, -1));
    
    // TODO: error handling..?
    
    const size_t width = doc.GetRowCount();
    const size_t height = doc.GetColumnCount();
    
    Variable o;
    ds_grid_create(o, width, height);
    int32_t ds = o.castCoerce<int32_t>();
    o.cleanup();
    
    for (size_t i = 0; i < width; ++i)
    {
        size_t j = 0;
        for (const std::string& val : doc.GetRow<std::string>(i))
        {
            Variable s;
            if (is_real(val.c_str()))
            {
                char* endptr;
                s = strtod(val.c_str(), &endptr);
            }
            else
            {
                s = val;
            }
            ds_grid_set(o, ds, i, j, s);
            s.cleanup();
            ++j;
        }
    }
    
    out = ds;
}
