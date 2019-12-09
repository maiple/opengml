#pragma once

#include "Asset.hpp"
#include "ogm/geometry/Vector.hpp"

#include <map>
#include <string>

namespace ogm
{
namespace asset
{

struct Glyph
{
    // position inside of image
    ogm::geometry::Vector<int32_t> m_position;
    ogm::geometry::Vector<int32_t> m_dimensions;

    // how far in pixels to advance cursor after typing this letter
    int32_t m_shift;

    // maybe this is the x offset applied to the location of the glyph?
    int32_t m_offset;
};

class AssetFont : public Asset
{
public:
    // truetype
    bool m_ttf;
    std::string m_path; // not needed if dynamically allocated
    std::map<uint32_t, Glyph> m_glyphs; // utf8-glyph lookup
    int32_t m_vshift = 0; // height of one line
};

}
}
