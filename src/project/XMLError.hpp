#pragma once

#include <pugixml.hpp>

#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"

namespace ogm::project
{
    template<typename Error, typename... A>
    inline void check_xml_result(error_code_t error_code, const pugi::xml_parse_result& result, const char* filepath, std::string preamble, A... args)
    {
        if (!result)
        {
            if (preamble != "") preamble += "\n";
            std::string source = read_file_contents(filepath);
            std::string lineinfo;
            size_t line, col;
            if (get_string_line_column_position(source.c_str(), source.c_str() + result.offset, line, col))
            {
                lineinfo = " (line " + std::to_string(line) + ", column " + std::to_string(col) + ")";
            }

            throw Error(error_code, args..., "{}", preamble + result.description() + lineinfo);
        }
    }

    // FIXME: move this elsewhere
    inline ogm_location_t get_location_from_offset_in_file(size_t offset, const char* contents, const char* source)
    {
        size_t line, col;
        get_string_line_column_position(contents, contents + offset, line, col);

        return ogm_location_t(line, col, line, col, source);
    }
}