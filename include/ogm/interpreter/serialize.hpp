#pragma once

#include "Buffer.hpp"

#include <iostream>
#include <type_traits>
#include <vector>
#include <map>
#include <string>
#include <string_view>
#include <cassert>

namespace ogm::interpreter
{
    typedef Buffer ostate_stream_t;
    typedef Buffer istate_stream_t;

    template<bool write>
    struct state_stream;

    template<>
    struct state_stream<true>
    {
        typedef ostate_stream_t state_stream_t;
    };

    template<>
    struct state_stream<false>
    {
        typedef istate_stream_t state_stream_t;
    };

    template<typename> struct int_
    {
        typedef int type;
    };

    // https://stackoverflow.com/a/9154394
    namespace detail
    {
        template<class>
        struct sfinae_true : std::true_type
        {};

        template<class T, class A0>
        static auto test_stream(int)
          -> sfinae_true<decltype(std::declval<T>().template serialize<false>(std::declval<A0&>()))>;

        template<class, class A0>
        static auto test_stream(long) -> std::false_type;
    }

    template<class T>
    struct has_serialize :
        decltype(detail::test_stream<T, state_stream<false>::state_stream_t>(0))
    { };

    /*template<typename T>
    struct has_serialize : std::enable_if<std::is_void<
        decltype(
            std::declval<T>().serialize<false>(
                *std::declval<state_stream<false>::state_stream_t*>()
            )
        )
    >::value, std::true_type>::type
    { };*/

    static_assert(!has_serialize<int>::value, "ints do not have serialize method.");

    template<bool write, typename T, typename std::enable_if<has_serialize<T>::value, T>::type* = nullptr >
    void _serialize(typename state_stream<write>::state_stream_t& s, T& t)
    {
        // use class' own serialize method.
        t.template serialize<write>(s);
    }

    template<bool write, typename T, typename std::enable_if<
        !has_serialize<T>::value && write && !std::is_same<std::string, T>::value && !std::is_same<std::string_view, T>::value,
        T
    >::type* = nullptr >
    void _serialize(typename state_stream<write>::state_stream_t& s, const T& t)
    {
        static_assert(
            !std::is_pointer<T>::value,
            "Pointers should be (un)swizzled, not serialized."
        );

        static_assert(! has_serialize<T>::value, "should use bespoke serialize function.");

        s.write(reinterpret_cast<const char*>(&t), sizeof(t));
    }

    template<bool write, typename T, typename std::enable_if<
        !has_serialize<T>::value && !write && !std::is_same<std::string, T>::value && !std::is_same<std::string_view, T>::value,
        T
        >::type* = nullptr >
    void _serialize(typename state_stream<write>::state_stream_t& s, T& t)
    {
        static_assert(
            !std::is_pointer<T>::value,
            "Pointers should be (un)swizzled, not serialized."
        );

        static_assert(! has_serialize<T>::value, "should use bespoke serialize function.");

        ogm_assert(s.good());
        ogm_assert(!s.eof());
        s.read(reinterpret_cast<char*>(&t), sizeof(t));
    }

    // string
    template<bool write, typename T, typename std::enable_if<std::is_same<T, std::string>::value && write, T>::type* = nullptr >
    void _serialize(typename state_stream<write>::state_stream_t& s, const T& t)
    {
        size_t len = t.length();
        _serialize<write>(s, len);
        s.write(t.c_str(), len);
    }

    // string
    template<bool write, typename T, typename std::enable_if<std::is_same<T, std::string>::value && !write, T>::type* = nullptr >
    void _serialize(typename state_stream<write>::state_stream_t& s, T& t)
    {
        size_t len = t.length();
        _serialize<write>(s, len);
        t.resize(len);
        if (len > 0)
        {
            s.read(&t[0], len);
        }
    }

    // string view
    template<bool write, typename T, typename std::enable_if<std::is_same<T, std::string_view>::value && write, T>::type* = nullptr >
    void _serialize(typename state_stream<write>::state_stream_t& s, const T& t)
    {
        size_t len = t.length();
        _serialize<write>(s, len);
        if (len > 0)
        {
            s.write(&t[0], len);
        }
    }

    // serializes vector by resizing and serializing elements
    template<bool write, typename T>
    void _serialize_vector_replace(typename state_stream<write>::state_stream_t& s, std::vector<T>& vec)
    {
        size_t size;
        if (write)
        {
            size = vec.size();
        }
        _serialize<write>(s, size);
        if (!write)
        {
            vec.resize(size);
        }

        for (size_t i = 0; i < size; ++i)
        {
            _serialize<write>(s, vec.at(i));
        }
    }

    // serializes vector by mapping/unmapping elements.
    // T -- element type of vector
    // MapW(const T& element, const S& out_s) -> void
    // MapR(const S& s, T& out_element) -> void
    template<bool write, typename S, typename T, typename MapW, typename MapR>
    void _serialize_vector_map(typename state_stream<write>::state_stream_t& s, std::vector<T>& vec, MapW mapw, MapR mapr)
    {
        size_t size;
        if (write)
        {
            size = vec.size();
        }
        _serialize<write>(s, size);
        if (!write)
        {
            vec.resize(size);
        }

        for (size_t i = 0; i < size; ++i)
        {
            if (write)
            {
                S v;
                mapw(vec.at(i), v);
                _serialize<write>(s, v);
            }
            else
            {
                S v;
                _serialize<write>(s, v);
                const T& t = vec.at(i);
                mapr(v, vec.at(i));
            }
        }
    }

    template<bool write, typename K, typename T>
    void _serialize_map(typename state_stream<write>::state_stream_t& s, std::map<K, T>& map)
    {
        int32_t len;
        if (write)
        {
            len = map.size();
        }
        _serialize<write>(s, len);
        if (write)
        {
            for (auto& iter : map)
            {
                auto key = iter.first;
                _serialize<write>(s, key);
                _serialize<write>(s, iter.second);
            }
        }
        if (!write)
        {
            map.clear();
            for (size_t i = 0; i < len; ++i)
            {
                K k;
                _serialize<write>(s, k);
                _serialize<write>(s, map[k]);
            }
        }
    }

    // throws error if reading frame offset.
    template<bool write>
    void _serialize_canary(typename state_stream<write>::state_stream_t& s)
    {
        const uint64_t K_CANARY = 0xDEADBEEF4B1D1337;
        auto canary = K_CANARY;
        _serialize<write>(s, canary);
        assert(canary == K_CANARY);
    }
}
