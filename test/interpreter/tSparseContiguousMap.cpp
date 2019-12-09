#include "catch/catch.hpp"
#include "ogm/interpreter/SparseContiguousMap.hpp"

#include <map>
#include <iostream>

using namespace ogm::interpreter;

TEST_CASE( "scm basic insert and removal", "[sparse contiguous map]" )
{
    SparseContiguousMap<int16_t, int32_t> scm;

    // check that checking containment does not insert
    REQUIRE(!scm.contains(8));
    REQUIRE(!scm.contains(8));
    scm[8] = 4;
    REQUIRE(scm.contains(8));
    REQUIRE(!scm.contains(7));
    REQUIRE(scm.at(8) == 4);
    REQUIRE(scm[8] == 4);
    const int32_t v = const_cast<const SparseContiguousMap<int16_t, int32_t>*>(&scm)->at(8);
    REQUIRE(v == 4);
}

TEST_CASE( "deterministic scm-map comparison regression", "[sparse contiguous map]" )
{
    SparseContiguousMap<int8_t, int32_t> scm;
    std::map<uint8_t, int32_t> map;

    const uint8_t regval[] = {
        103, 105,115, 255,74, 41,205, 171,242, 227,70, 194,84, 27,
        232, 141,118, 46,99, 159,201, 102,50, 183,49, 163,90, 93,
        5, 88,233
    };

    for (size_t i = 0; i < sizeof(regval); i++)
    {
        INFO("no. " + std::to_string(i));

        if (i % 2 == 0)
        {
            if (map.find(i) == map.end() && i % 8 == 2)
            {
                REQUIRE(!scm.contains(i));

                // default value is zero.
                REQUIRE(scm[i] == 0);
            }
            uint32_t v = i % 30;
            scm[i] = v;
            map[i] = v;
            REQUIRE(scm.at(i) == v);
            REQUIRE(map[i] == v);
            REQUIRE(scm[i] == v);
        }
        else
        {
            if (map.find(i) == map.end())
            {
                REQUIRE(!scm.contains(i));
            }
            else
            {
                REQUIRE(scm.contains(i));
                REQUIRE(scm.at(i) == map[i]);
            }
        }
    }
}

TEST_CASE( "scm-map comparison", "[sparse contiguous map]" )
{
    for (size_t j = 27; j < 32; j++)
    {
        SparseContiguousMap<int8_t, int32_t> scm;
        std::map<uint8_t, int32_t> map;

        INFO("trial no. " + std::to_string(j));
        for (size_t i = 0; i < 6; i++)
        {
            // randomly insert / check contains, ensure scm agrees with map.
            uint8_t key = std::rand();

            if (i % 2 == 0)
            {
                if (map.find(i) == map.end() && i % 8 == 2)
                {
                    REQUIRE(!scm.contains(i));

                    // default value is zero.
                    REQUIRE(scm[i] == 0);
                }
                uint32_t v = std::rand();
                scm[i] = v;
                map[i] = v;
                REQUIRE(scm.at(i) == v);
                REQUIRE(scm[i] == v);
            }
            else
            {
                if (map.find(i) == map.end())
                {
                    REQUIRE(!scm.contains(i));
                }
                else
                {
                    REQUIRE(scm.contains(i));
                    REQUIRE(scm.at(i) == map[i]);
                }
            }
        }
    }
}
