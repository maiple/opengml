#pragma once

#include <pugixml.hpp>

#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"

namespace ogm::project
{
    inline void check_xml_result(const pugi::xml_parse_result& result, const char* filepath, std::string preamble="")
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
            throw MiscError(preamble + result.description() + lineinfo);
        }
    }
}