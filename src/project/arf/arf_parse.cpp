#include "ogm/project/arf/arf_parse.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"

#include <string_view>

namespace ogm::project
{

namespace
{
inline bool is_ws(char c)
{
    return (std::isspace(c) || c == '-' || c == ':');
}

void skip_ws(const char*& c)
{
    while (*c)
    {
        if (is_ws(*c) && *c != '\n' && *c != '\r')
        {
            c++;
        }
        else
        {
            break;
        }
    }
}

void skip_ws_ln(const char*& c)
{
    while (*c)
    {
        if (is_ws(*c))
        {
            c++;
        }
        else
        {
            break;
        }
    }
}

void err_eof(const char*& c)
{
    if (!*c)
    {
        throw MiscError("Unexpected EOF while parsing ARF");
    }
}

std::string_view read_token(const char*& c)
{
    const char* p = c;
    while (*c)
    {
         if (is_ws(*c))
         {
             break;
         }
         else
         {
             ++c;
         }
    }
    return { p, static_cast<size_t>(c - p) };
}

std::string_view read_line(const char*& c)
{
    const char* p = c;
    while (*c)
    {
         if (*c == '\n' || *c == '\r')
         {
             break;
         }
         else
         {
             ++c;
         }
    }
    return { p, static_cast<size_t>(c - p) };
}

void read_dict(const char*& c, ARFSection& out)
{
    while (true)
    {
        skip_ws_ln(c);
        if (!*c) break;
        if (*c == '#') break;
        std::string_view key = read_token(c);
        skip_ws(c);

        // OPTIMIZE (trim / use string_view)
        std::string value { read_line(c) };
        trim(value);
        out.m_dict[std::string{ key }] = std::string{ value };
    }
}

void read_text(const std::vector<std::string>& schema_names, const char*& c, ARFSection& out)
{
    const char* p = c;
    while (true)
    {
        if (!*c) break;
        if (*c == '#' && *(c - 1) == '\n')
        {
            // look ahead, check if this is a section.
            const char* test = c;
            while (*test && *test == '#')
            {
                ++test;
            }

            skip_ws(test);
            std::string_view name = read_token(test);
            if (std::find(schema_names.begin(), schema_names.end(), name) != schema_names.end())
            {
                // this is a section; we're done parsing.
                break;
            }
        }

        ++c;
    }

    // remove trailing newlines.
    while (p <= c && *c && (*c == '#' || *c == '\n' || *c == '\r'))
    {
        --c;
    }
    if (*c) ++c;

    // string resolved.
    out.m_text = std::string(p, c - p);
}

bool read_section(const std::vector<std::string>& schema_names, const ARFSchema* schema, const char*& c, ARFSection& out)
{
    const char* s = c;

    if (!*c)
    {
        return false;
    }

    if (*c != '#')
    {
        return false;
    }

    while (*c && *c == '#')
    {
        ++c;
    }

    skip_ws(c);
    std::string_view sec_name = read_token(c);

    if (sec_name != schema->m_name)
    {
        // do not parse.
        c = s;
        return false;
    }

    // read name
    out.m_name = std::string { sec_name };

    // read details
    while (*c)
    {
        skip_ws(c);
        if (!*c)
        {
            break;
        }
        if (*c == '\n' || *c == '\r')
        {
            // read eof.
            ++c;
            break;
        }
        out.m_details.emplace_back(read_token(c));
    }

    skip_ws_ln(c);

    // read contents
    if (schema->m_content_type == ARFSchema::DICT)
    {
        read_dict(c, out);
    }
    else
    {
        read_text(schema_names, c, out);
    }

    // read subsections
    skip_ws_ln(c);
    if (schema->m_component_schema)
    {
        while (true)
        {
            ARFSection* section = new ARFSection();
            if (!read_section(schema_names, schema->m_component_schema, c, *section))
            {
                // no section read
                delete section;
                break;
            }
            else
            {
                out.m_sections.push_back(section);
            }
        }
    }

    return true;
}

void get_schema_names(const ARFSchema* schema, std::vector<std::string>& out_names)
{
    out_names.emplace_back(schema->m_name);
    if (schema->m_component_schema)
    {
        get_schema_names(schema->m_component_schema, out_names);
    }
}
}

void arf_parse(const ARFSchema* schema, const char* source, ARFSection& out)
{
    skip_ws_ln(source);
    err_eof(source);

    std::vector<std::string> schema_names;
    get_schema_names(schema, schema_names);

    if (!read_section(schema_names, schema, source, out))
    {
        throw MiscError("No matching header in ARF.");
    }
    skip_ws_ln(source);
    if (*source)
    {
        throw MiscError("Failed to reach EOF while parsing ARF.");
    }
}
}