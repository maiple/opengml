#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include <sstream>

namespace ogm
{

const char* Error::what() const noexcept
{
    try
    {
        assemble_message<true>();
    }
    catch (...)
    {
        try
        {
            // last resort
            m_what = "(opengml internal error while assembling error details: " + std::string(1, m_category) + std::to_string(m_code) + ")";
        }
        catch (...)
        {
            // panic.
            return "(opengml internal error while assembling error details.)";
        }
    }
    return m_what.c_str();
}

#define DETAIL_SPECIALIZATION(C, filter)            \
template<>                                          \
Error& Error::detail<C>(const typename DetailTraits<C>::Type& v)    \
{                                                   \
    if (m_detail_##C.set) return *this;             \
    if (filter) return *this;                       \
    m_detail_##C.value = v;                         \
    m_detail_##C.set = true;                        \
    return *this;                                   \
}

DETAIL_SPECIALIZATION(location_at, m_detail_location_start.set || m_detail_location_end.set)
DETAIL_SPECIALIZATION(location_start, m_detail_location_at.set)
DETAIL_SPECIALIZATION(location_end, m_detail_location_at.set)
DETAIL_SPECIALIZATION(source_buffer, false)
DETAIL_SPECIALIZATION(resource_type, false)
DETAIL_SPECIALIZATION(resource_name, false)
DETAIL_SPECIALIZATION(resource_event, false)
DETAIL_SPECIALIZATION(resource_section, false)

template<bool colour>
void Error::assemble_message() const
{
    if (colour)
    {
        if (!is_terminal())
        {
            assemble_message<false>();
            return;
        }

        // TODO: terminfo(5) to check if ANSI colour codes are supported.
    }

    // ANSI colour code.
    #define COLOUR(cmd) ((colour) ? ("\033[" #cmd "m") : "")

    std::stringstream ss;

    bool indent = false;

    // resource
    if (m_detail_resource_name.set && m_detail_resource_type.set)
    {
        indent = true;
        ss  << "For " << COLOUR(1) << m_detail_resource_type.value << COLOUR(0)
            << " resource " << COLOUR(1;36) << m_detail_resource_name.value
            << COLOUR(0);
        
        if (m_detail_resource_event.set && m_detail_resource_event.value.length())
            ss << " in event " << COLOUR(1) << m_detail_resource_event.value << COLOUR(0);

        if (m_detail_resource_section.set && m_detail_resource_section.value.length())
            ss << " in section " << COLOUR(1;36) << m_detail_resource_section.value << COLOUR(0);

        ss << ":\n";
    }

    std::string line_preview = "";

    // for loc preview
    const char* loc_source = nullptr;
    const char* loc_start = nullptr;
    const char* loc_end = nullptr;
    size_t loc_line_number;

    // location
    if (m_detail_location_at.set)
    {
        indent = true;
        const ogm_location_t& location = m_detail_location_at.value;
        ss << "At " << COLOUR(1);
        if (location.m_source) ss << location.m_source << ":";
        ss << location.m_source_line + 1 << ":" << location.m_source_column + 1;
        ss << COLOUR(0) << ":";

        if (m_detail_source_buffer.set)
        {
            loc_source = m_detail_source_buffer.value.c_str();
            loc_start = get_string_position_line_column(
                loc_source, location.m_line, location.m_column
            );
            loc_line_number = location.m_source_line;
        }
        else
        {
            ss << " (source unavailable)";
        }
        
        ss << "\n";
    }
    else if (m_detail_location_start.set && m_detail_location_end.set)
    {
        indent = true;
        const ogm_location_t& location = m_detail_location_start.value;
        const ogm_location_t& end = m_detail_location_end.value;
        ss << "Within " << COLOUR(1);
        if (location.m_source) ss << location.m_source << ":";
        ss << location.m_source_line + 1 << ":" << location.m_source_column + 1;
        ss << "-" << end.m_source_line + 1 << ":" << end.m_source_column + 1;
        ss << COLOUR(0) << ":";

        if (m_detail_source_buffer.set)
        {
            loc_source = m_detail_source_buffer.value.c_str();
            loc_start = get_string_position_line_column(
                loc_source, location.m_line, location.m_column
            );
            loc_end = get_string_position_line_column(
                loc_source, end.m_line, end.m_column
            );
            loc_line_number = location.m_source_line;
        }
        else
        {
            ss << " (source unavailable)";
        }

        ss << "\n";
    }
    
    // error colour
    const char* errcol = "";
    if (colour)
    {
        switch(m_code)
        {
        // red
        case 'F':
        case 'P':
        case 'C': errcol = COLOUR(31); break;

        // purple
        case 'R': errcol = COLOUR(35); break;

        // black-on-purple
        case 'A': errcol = COLOUR(30;45); break;

        // (red)
        default: break;
        }
    }

    if (indent) ss << "\n    ";

    // main payload
    ss << fmt::format(
        "{3}Error {4}{0}{1:0>4d}{5}: {2}\n",
        m_category,
        m_code,
        m_message,
        COLOUR(1;31), // bold red
        errcol, // bold red
        COLOUR(0) // reset
    );

    if (indent) ss << "\n";

    // loc preview
    if (loc_start)
    {
        loc_preview<colour>(ss, loc_source, loc_start, loc_end, loc_line_number);
    }

    ss << COLOUR(0);
    m_what = ss.str();
}

struct line_preview
{
    const char* m_start=0;
    const char* m_end=0;
    const char* m_highligh_start=0;
    const char* m_highlight_end=0;
    size_t m_line_number=0;
};

template<bool colour>
void Error::loc_preview(
    std::stringstream& ss,
    const char* source,
    const char* start,
    const char* end,
    size_t line_number
) const
{
    line_preview previews[3] = {{}, {}, {}};

    if (!end) end = start + 1;

    // find the start and end of the line in question.
    const char* line_start = get_line_start(source, start);
    const char* line_end = get_line_end(line_start);
    previews[1] = { line_start, line_end, start, end, line_number };

    // find the previous line
    {
        const char* prev_line_start = get_line_start(source, line_start - 1);
        const char* prev_line_end = get_line_end(prev_line_start);
        if (prev_line_end < line_start && prev_line_start < prev_line_end)
        {
            previews[0] = { prev_line_start, prev_line_end, start, end, line_number - 1 };
        }
    }

    // find the next line
    {
        const char* next_line_start = line_end;
        if (*next_line_start == '\r') ++next_line_start;
        if (*next_line_start == '\n') ++next_line_start;
        const char* next_line_end = get_line_end(next_line_start);
        if (next_line_start > line_end && next_line_end > next_line_start)
        {
            previews[2] = { next_line_start, next_line_end, start, end, line_number + 1 };
        }
    }

    // TODO: uniformly trim all previews

    const size_t maxlen = std::max<size_t>(3, fmt::format("{}", line_number + 1).length());

    for (size_t i = 0; i < 3; ++i)
    {
        const line_preview& preview = previews[i];
        if (!preview.m_start) continue;
        ss << COLOUR(2); // dim
        ss << fmt::format("{1:>{0}}| ", maxlen, preview.m_line_number+1);
        ss << COLOUR(0);

        // split up line into highlighted and non-highlighted sections.

        const char* sec[4];
        sec[0] = preview.m_start;
        sec[1] = clamp(start, preview.m_start, preview.m_end);
        sec[2] = clamp(end, preview.m_start, preview.m_end);
        sec[3] = preview.m_end;

        ss.write(sec[0], sec[1] - sec[0]);
        ss << COLOUR(1;4;31); // bold, red, underline
        ss.write(sec[1], sec[2] - sec[1]);
        ss << COLOUR(0;24); // reset and underline off
        ss.write(sec[2], sec[3] - sec[2]);

        ss << "\n";
    }
    ss << "\n";
}

}