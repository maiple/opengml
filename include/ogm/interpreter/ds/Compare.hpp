#pragma once
#include "ogm/common/types.hpp"
#include "ogm/interpreter/Variable.hpp"

#include <vector>
#include <cstdlib>

namespace ogm { namespace interpreter
{
    // epsilon (for floating point comparisons)
    // can be set during runtime, but that may invalidate existing datastructures.
    // defined in ogm/interpreter/StandardLibrary.cpp
    extern real_t ds_epsilon;

    // comparator for some datastructures
    // determines how to sort variables by key
    struct DSComparator
    {
        int type_order(const Variable& a) const
        {
            // order is: undefined < string < undefined
            if (a.is_array())
            {
                throw UnknownIntendedBehaviourError("array as key");
            }
            if (a.get_type() == VT_UNDEFINED)
            {
                return 0;
            }
            if (a.get_type() == VT_STRING)
            {
                return 1;
            }
            if (a.is_numeric())
            {
                return 2;
            }

            ogm_assert(false);
            return 3;
        }

        bool operator()(const Variable& a, const Variable& b) const
        {
            int ta = type_order(a);
            int tb = type_order(b);
            if (ta == tb)
            {
                if (a.is_numeric())
                {
                    ogm_assert(b.is_numeric());
                    if (a.is_integral() && b.is_integral())
                    {
                        return a < b;
                    }
                    else if (a.is_integral() && !b.is_integral())
                    {
                        return a < b.get<real_t>() - ds_epsilon;
                    }
                    else if (!a.is_integral() && b.is_integral())
                    {
                        return b > a.get<real_t>() + ds_epsilon;
                    }
                    else
                    {
                        return a.get<real_t>() < b.get<real_t>() - ds_epsilon;
                    }
                }
                else switch(a.get_type())
                {
                case VT_UNDEFINED:
                    // undefined == undefined
                    return false;
                case VT_STRING:
                    return a.string_view().compare(b.string_view()) < 0;
                default:
                    ogm_assert(false);
                    return false;
                }
            }
            else
            {
                return ta < tb;
            }
        }
    };
}}
