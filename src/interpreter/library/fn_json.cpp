#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <locale>
#include <cctype>
#include <cstdlib>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame (staticExecutor.m_frame)

#define dsmm staticExecutor.m_frame.m_ds_map
#define dslm staticExecutor.m_frame.m_ds_list

namespace
{
    inline std::stringstream& json_indent(std::stringstream& out, size_t indent)
    {
        for (size_t i = 0; i < indent; ++i)
            out << "  ";
        return out;
    }

    void json_encode_value(std::stringstream& out, const Variable& v)
    {
        if (v.is_numeric())
        {
            out << v.castCoerce<real_t>();
        }
        else if (v.is_string())
        {
            std::string_view sv = v.string_view();
            out << "\"";
            for (size_t i = 0; i < sv.size(); ++i)
            {
                if (sv[i] == '\"')
                    out << "\\\"";
                else if (sv[0] == 0) // paranoia
                    break;
                else
                    out << sv[i];
            }
            out << "\"";
        }
        else if (v.is_array())
        {
            // Encode as [ ]
            if (v.array_height() > 1)
            {
                out << "[ ";
            }

            bool first = true;
            for (size_t i = 0; i < v.array_height(); ++i)
            {
                if (!first) out << ", ";
                first = false;
                out << "[ ";

                bool _first = true;
                for (size_t j = 0; j < v.array_length(i); ++j)
                {
                    if (!_first) out << ", ";
                    _first = false;

                    const Variable& sub = const_cast<Variable&>(v).array_get(i, j, false);
                    json_encode_value(out, sub);
                }

                out << " ]";
            }

            if (v.array_height() > 1)
            {
                out << " ]";
            }
        }
        else
        {
            throw UnknownIntendedBehaviourError(
                "Not sure how to encode " + std::string(v.type_string())
            );
        }
    }

    // forward declaration
    void json_encode_map(std::stringstream& out, ds_index_t index, size_t indent = 0);

    void json_encode_list(std::stringstream& out, ds_index_t index, size_t indent)
    {
        if (!dslm.ds_exists(index))
        {
            throw MiscError("Encoding non-existent map datastructure.");
        }
        DSList& m = dslm.ds_get(index);

        out << "{\n";

        size_t i = 0;
        bool first = true;
        for (const Variable& v : m.m_data)
        {
            DSList::EntryFlag flag = m.m_nested_ds[++i];

            if (!first) out << ",\n";
            first = false;
            json_indent(out, indent + 1);

            // write value
            switch(flag)
            {
            case DSList::NONE:
                json_encode_value(out, v);
                break;
            case DSList::LIST:
                json_encode_list(out, v.castCoerce<ds_index_t>(), indent + 1);
                break;
            case DSList::MAP:
                json_encode_map(out, v.castCoerce<ds_index_t>(), indent + 1);
                break;
            }

            out << "\n";
        }

        json_indent(out, indent) << "}";
    }

    void json_encode_map(std::stringstream& out, ds_index_t index, size_t indent)
    {
        if (!dsmm.ds_exists(index))
        {
            throw MiscError("Encoding non-existent map datastructure.");
        }
        DSMap& m = dsmm.ds_get(index);

        out << "{";

        bool first = true;
        for (const std::pair<const Variable, DSMap::MarkedVariable>& v : m.m_data)
        {
            if (!first) out << ",";
            first = false;
            out << "\n";
            json_indent(out, indent + 1);
            const Variable& key = v.first;
            const DSMap::MarkedVariable& value = v.second;

            if (key.is_numeric())
            {
                out << "\"" << key.castCoerce<real_t>() << "\"";
            }
            else if (key.is_string())
            {
                std::string_view sv = key.string_view();
                out << "\"";
                for (size_t i = 0; i < sv.size(); ++i)
                {
                    if (sv[i] == '\"')
                        out << "\\\"";
                    else
                        out << sv[i];
                }
                out << "\"";
            }
            else
            {
                throw UnknownIntendedBehaviourError(
                    "Not sure how to encode " + std::string(key.type_string())
                );
            }
            out << ": ";

            // write value
            switch(value.m_flag)
            {
            case DSMap::MarkedVariable::NONE:
                json_encode_value(out, value.v);
                break;
            case DSMap::MarkedVariable::LIST:
                json_encode_list(out, value.v.castCoerce<ds_index_t>(), indent + 1);
                break;
            case DSMap::MarkedVariable::MAP:
                json_encode_map(out, value.v.castCoerce<ds_index_t>(), indent + 1);
                break;
            }
        }

        out << "\n";
        json_indent(out, indent) << "}";
    }
}

void ogm::interpreter::fn::json_encode(VO out, V vindex)
{
    ds_index_t index = vindex.castCoerce<ds_index_t>();

    std::stringstream ss;

    json_encode_map(ss, index);

    out = ss.str();
}

using json = nlohmann::json;

namespace
{
    ds_index_t json_decode_object(const json& j);

    ds_index_t json_decode_list(const json& j)
    {
        ogm_assert(j.is_array());
        ds_index_t list = dslm.ds_new();
        Variable dummy;

        size_t i = 0;
        for (auto& iter : j.items())
        {
            auto& val = iter.value();
            Variable vval;
            switch (val.type())
            {
            default:
            case json::value_t::null:
                // add undefined value.
                ds_list_add(dummy, list, vval);
                break;
            case json::value_t::boolean:
                vval = vval.get<bool>();
                ds_list_add(dummy, list, vval);
                break;
            case json::value_t::number_integer:
            case json::value_t::number_unsigned:
            case json::value_t::number_float:
                vval = static_cast<real_t>(val);
                ds_list_add(dummy, list, vval);
                break;
            case json::value_t::object:
                ds_list_add(dummy, list, static_cast<real_t>(json_decode_object(val)));
                ds_list_mark_as_map(dummy, list, i);
                break;
            case json::value_t::array:
                ds_list_add(dummy, list, static_cast<real_t>(json_decode_list(val)));
                ds_list_mark_as_list(dummy, list, i);
                break;
            case json::value_t::string:
                vval = val.get<std::string>();
                ds_list_add(dummy, list, vval);
                break;
            }
            vval.cleanup();
            ++i;
        }

        return list;
    }

    ds_index_t json_decode_object(const json& j)
    {
        ogm_assert(j.is_object());
        Variable dummy;
        ds_index_t map = dsmm.ds_new();
        for (auto& [key, val] : j.items())
        {
            Variable vkey = key;
            Variable vval;
            switch (val.type())
            {
            default:
            case json::value_t::null:
                // add undefined value.
                ds_map_add(dummy, map, key, vval);
                break;
            case json::value_t::boolean:
                vval = vval.get<bool>();
                ds_map_add(dummy, map, key, vval);
                break;
            case json::value_t::number_integer:
            case json::value_t::number_unsigned:
            case json::value_t::number_float:
                vval = static_cast<real_t>(val);
                ds_map_add(dummy, map, key, vval);
                break;
            case json::value_t::object:
                ds_map_add_map(dummy, map, key, static_cast<real_t>(json_decode_object(val)));
                break;
            case json::value_t::array:
                ds_map_add_list(dummy, map, key, static_cast<real_t>(json_decode_list(val)));
                break;
            case json::value_t::string:
                vval = val.get<std::string>();
                ds_map_add(dummy, map, key, vval);
                break;
            }
            vval.cleanup();
            vkey.cleanup();
        }

        return map;
    }
}

void ogm::interpreter::fn::json_decode(VO out, V str)
{
    auto j = json::parse(str.string_view());

    if (j.is_array())
    {
        // wrap array in map.
        ds_index_t map = dsmm.ds_new();
        ds_index_t list = json_decode_list(j);
        Variable v = "default";
        ds_map_add_list(out, map, v, list);
        v.cleanup();
        out = static_cast<real_t>(map);
    }
    else
    {
        out = static_cast<real_t>(json_decode_object(j));
    }
}
