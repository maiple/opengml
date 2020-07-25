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

#define DETAIL_SPECIALIZATION(C)                    \
template<>                                          \
Error& Error::detail<C>(const typename DetailTraits<C>::Type& v)    \
{                                                   \
    if (m_detail_##C.set) return *this;             \
    DETAIL_FILTER_SPECIALIZATION_##C                \
    m_detail_##C.value = v;                         \
    m_detail_##C.set = true;                        \
    return *this;                                   \
}

#define DETAIL_FILTER_SPECIALIZATION_location_at if (m_detail_location_start.set || m_detail_location_end.set) return *this;
#define DETAIL_FILTER_SPECIALIZATION_location_start if (m_detail_location_at.set) return *this;
#define DETAIL_FILTER_SPECIALIZATION_location_end if (m_detail_location_at.set) return *this;
#define DETAIL_FILTER_SPECIALIZATION_resource_type
#define DETAIL_FILTER_SPECIALIZATION_resource_name
#define DETAIL_FILTER_SPECIALIZATION_resource_event
#define DETAIL_FILTER_SPECIALIZATION_resource_section

DETAIL_SPECIALIZATION(location_at)
DETAIL_SPECIALIZATION(location_start)
DETAIL_SPECIALIZATION(location_end)
DETAIL_SPECIALIZATION(resource_type)
DETAIL_SPECIALIZATION(resource_name)
DETAIL_SPECIALIZATION(resource_event)
DETAIL_SPECIALIZATION(resource_section)

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

    // location
    if (m_detail_location_at.set)
    {
        indent = true;
        const ogm_location_t& location = m_detail_location_at.value;
        ss << "At " << COLOUR(1);
        if (location.m_source) ss << location.m_source << ":";
        ss << location.m_source_line + 1 << ":" << location.m_source_column + 1;
        ss << COLOUR(0) << ":\n";
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
        ss << COLOUR(0) << ":\n";
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

    m_what = ss.str();
}

}