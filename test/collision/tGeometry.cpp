#include "catch/catch.hpp"
#include "ogm/common/types.hpp"
#include "ogm/geometry/Ellipse.hpp"

#include <map>
#include <iostream>

using namespace ogm;
using namespace ogm::geometry;

typedef Vector<real_t> vector_t;

TEST_CASE( "ellipses", "[geometry]" )
{
    vector_t c { 10, 50 };
    vector_t mat { 2, 0.75 };
    {
        Ellipse<real_t> e{ c, mat, 0 };

        REQUIRE(e.contains(c));
        REQUIRE(!e.contains({ 0, 0 }));

        REQUIRE(!e.contains({ 10, 51 }));
        REQUIRE(e.contains({ 11, 50 }));
        REQUIRE(!e.contains({ 10, 49 }));
        REQUIRE(e.contains({ 9, 50 }));
    }

    {
        INFO("Rotated ellipse.")
        Ellipse<real_t> e{ c, mat, TAU/4 };

        REQUIRE(e.contains(c));
        REQUIRE(!e.contains({ 0, 0 }));

        REQUIRE(e.contains({ 10, 51 }));
        REQUIRE(!e.contains({ 11, 50 }));
        REQUIRE(e.contains({ 10, 49 }));
        REQUIRE(!e.contains({ 9, 50 }));
    }

    {
        INFO("Half-rotated ellipse.")
        Ellipse<real_t> e{ c, mat, TAU/8 };

        REQUIRE(e.contains(c));
        REQUIRE(!e.contains({ 0, 0 }));

        REQUIRE(e.contains({ 11, 51 }));
        REQUIRE(!e.contains({ 10, 51 }));
        REQUIRE(!e.contains({ 11, 50 }));
        REQUIRE(e.contains({ 9, 49 }));
    }
}
