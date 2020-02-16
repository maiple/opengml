#pragma once

#include "ogm/bytecode/Namespace.hpp"

#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/serialize.hpp"

#ifdef OPTIMIZE_STRING_APPEND
#include "COWGOAString.hpp"
#endif

#ifdef OGM_GARBAGE_COLLECTOR
#include "Garbage.hpp"
#endif

#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <type_traits>


namespace ogm { namespace interpreter
{
using namespace ogm;
typedef bool bool_t;
typedef unsigned char byte;
typedef char char_t;
typedef std::string string_t;
typedef std::string_view string_view_t;

#ifndef OPTIMIZE_STRING_APPEND
typedef std::string string_data_t;
#else
typedef COWGOAString string_data_t;
#endif

enum VariableType {
    VT_UNDEFINED, // not defined
    VT_BOOL, // boolean
    VT_INT, // int32_t
    VT_UINT64, // uint64_t
    VT_REAL, // real number
    VT_STRING, // string
    VT_ARRAY, // array
    #ifdef OGM_GARBAGE_COLLECTOR
    VT_ARRAY_ROOT, // array (GC root -- for example, stored directly in an instance or global.)
    #endif
    VT_PTR // other data
};

extern const char* const variable_type_string[];

class VariableArrayData;

// a copy-on-write handler for variable array data
// Variable (with type VT_ARRAY) has a VariableArrayHandle,
// which points to a VariableArrayData, which
// contains a reference count and a vector.
class VariableArrayHandle
{
private:
    // mutable because we need empty and null to look similar
    // externally.
    mutable VariableArrayData* m_data;

private:

    inline bool is_null() const
    {
        return !m_data;
    }

    // creates a new array empty data that this handle points to.
    // asserts that this was previously null.
    // (this is considered const because empty and null are not seen
    // as different externally)
    template<bool gc_root>
    inline const VariableArrayData& constructData() const;

public:

    inline void initialize();
    inline void initialize(const VariableArrayHandle&);
    inline void initialize_as_empty_array();
    inline void decrement(); // used when cleaned up

    #ifdef OGM_GARBAGE_COLLECTOR
    inline void decrement_gc();
    inline void increment_gc();
    #endif

    template<bool gc_root>
    inline const VariableArrayData& getReadable() const;

    #ifdef OGM_GARBAGE_COLLECTOR
    template<bool gc_root>
    inline VariableArrayData& getWriteable(GCNode* owner);
    
    void gc_integrity_check() const;
    #else

    // the gc_root template is here just for the sake of
    // not having to write different code in some cases
    // depending on whether the garbage collector is enabled.
    // (It should have no effect.)
    template<bool gc_root=false>
    inline VariableArrayData& getWriteable();
    #endif

    template<bool gc_root=false>
    inline VariableArrayData& getWriteableNoCopy();
};

class Variable
{
    // 1 byte
    byte m_tag = (byte)VT_REAL;

    // 8 bytes
    union
    {
      real_t m_real;
      int32_t m_int;
      uint64_t m_uint64;
      string_data_t* m_string;
      VariableArrayHandle m_array;
      void* m_ptr;
    };

public:
    Variable()
        : m_tag( VT_UNDEFINED )
    { }
    Variable(bool_t v)
        : m_tag( VT_BOOL )
        , m_int( v )
    { }
    Variable(real_t v)
        : m_tag( VT_REAL )
        , m_real( v )
    { }
    Variable(int32_t v)
        : m_tag( VT_INT )
        , m_int( v )
    { }
    Variable(uint32_t v)
        : m_tag( VT_UINT64 )
        , m_uint64( v )
    { }
    Variable(int64_t v)     { set(v); }
    Variable(uint64_t v)
        : m_tag( VT_UINT64 )
        , m_uint64( v )
    { }
    Variable(const char* v)
        : m_tag( VT_STRING )
        , m_string( new string_data_t(v) )
    { }
    Variable(const string_t& v)
        : m_tag( VT_STRING )
        , m_string( new string_data_t(v) )
    { }
    Variable(const string_view_t& v)
        : m_tag( VT_STRING )
        , m_string( new string_data_t(v) )
    { }
    #ifdef OPTIMIZE_STRING_APPEND
    Variable(const COWGOAString& v)
        : m_tag( VT_STRING )
        , m_string( new COWGOAString(v) )
    { }
    #endif
    Variable(void* v)
        : m_tag( VT_PTR )
        , m_ptr( v )
    { }
    Variable(Variable&& v)
        : m_tag( v.m_tag )
        // uint64 is large enough to copy any data value from v.
        , m_uint64( v.m_uint64 )
    { }
    template<typename A>
    Variable(std::vector<A> vec);

    // under emscripten, size_t is 4 bytes but not uint32_t
    // so this header is used in that circumstance.
    template<typename T, class =
    typename std::enable_if<
               std::is_same<T, size_t>::value
            && !std::is_same<size_t, uint32_t>::value
            && !std::is_same<size_t, uint64_t>::value
        >::type
    >
    Variable(T v)
        : m_tag( VT_UINT64 )
        , m_uint64( v )
    { }

    // WARNING: `set` does not clean-up previous value!
    // call cleanup() first if it is possibly needed.
    inline Variable& set(bool_t v)   { m_tag = VT_BOOL;   m_int    = v; return *this; }
    inline Variable& set(uint64_t v) { m_tag = VT_UINT64; m_uint64 = v; return *this; }
    inline Variable& set(int32_t v)  { m_tag = VT_INT;    m_int    = v; return *this; }
    inline Variable& set(int64_t v)  {
        if (v < 0 && static_cast<int32_t>(v) == v)
        {
            m_tag = VT_INT;
            m_int = v;
        }
        else
        {
            m_tag = VT_UINT64;
            m_uint64 = static_cast<uint64_t>(v);
        }
        return *this;
    }
    inline Variable& set(real_t v)   { m_tag = VT_REAL;   m_real   = v; return *this; }
    inline Variable& set(const char* v)
    {
        m_tag = VT_STRING;
        m_string = new string_data_t( v );
        return *this;
    }
    inline Variable& set(const string_t& v)
    {
        m_tag = VT_STRING;
        m_string = new string_data_t( v );
        return *this;
    }
    inline Variable& set(const string_view_t& v)
    {
        m_tag = VT_STRING;
        m_string = new string_data_t( v );
        return *this;
    }
    #ifdef OPTIMIZE_STRING_APPEND
    inline Variable& set(const COWGOAString& v)
    {
        m_tag = VT_STRING;
        m_string = new string_data_t( v );
        return *this;
    }
    #endif
    inline Variable& set(void* v)    { m_tag = VT_PTR; m_ptr = v; return *this; };
    inline Variable& set(const Variable& v)
    {
        m_tag = v.m_tag;
        switch(m_tag)
        {
            case VT_BOOL:
            case VT_INT:
            case VT_UINT64:
            case VT_REAL:
            case VT_PTR:
                // large enough to hold any data value
                m_uint64 = v.m_uint64;
                break;
            case VT_STRING:
                m_string = new string_data_t(*v.m_string);
                break;
            case VT_ARRAY:
            case_VT_ARRAY:
                m_array.initialize(v.m_array);
                break;
            #ifdef OGM_GARBAGE_COLLECTOR
            case VT_ARRAY_ROOT:
                // shed ROOT quality.
                m_tag = VT_ARRAY;
                goto case_VT_ARRAY;
            #endif
            case VT_UNDEFINED:
                break;
            default:
                ogm_assert(false);
                throw MiscError("(internal) Unknown variable type for Variable::set()");
        }

        return *this;
    }

    // WARNING: `operator=` does not cleanup previous value!
    // call cleanup() first if it is possibly needed.
    inline Variable& operator=(bool_t v)          { return set(v); }
    inline Variable& operator=(int32_t v)         { return set(v); }
    inline Variable& operator=(int64_t v)         { return set(v); }
    inline Variable& operator=(uint32_t v)        { return set(static_cast<uint64_t>(v)); }
    inline Variable& operator=(uint64_t v)        { return set(v); }

    template<class A = typename std::enable_if<true>::type>
    inline Variable& operator=(unsigned long v)
    {
        return set(static_cast<uint64_t>(v));
    }
    inline Variable& operator=(real_t v)          { return set(v); }
    inline Variable& operator=(const string_t& v) { return set(v); }
    inline Variable& operator=(const string_view_t& v)  { return set(v); }
    #ifdef OPTIMIZE_STRING_APPEND
    inline Variable& operator=(const COWGOAString& v)   { return set(v); }
    #endif
    inline Variable& operator=(void* v)           { return set(v); }
    inline Variable& operator=(const char* v)     { return set(v); }
    inline Variable& operator=(Variable&& v)
    {
        m_tag = v.m_tag;
        // uint64 is large enough to copy any data in v.
        m_uint64 = v.m_uint64;
        return *this;
    };

    inline Variable& copy(const Variable& v)      { return set(v); };

    // copies without reference counting
    // don't use this unless you're certain exactly one of the source
    // and dest variables will be cleaned up.
    // use std::move() instead if you
    // won't need the source anymore.
    inline Variable& copy_raw(const Variable& v)
    {
        m_tag = v.m_tag;
        // uint64 is large enough to copy any data in v.
        m_uint64 = v.m_uint64;
        return *this;
    }

    #ifdef OGM_GARBAGE_COLLECTOR
    // marks as garbage collection root.
    // (applies to arrays only.)
    void make_root()
    {
        switch (m_tag)
        {
        case VT_ARRAY:
            m_tag = VT_ARRAY_ROOT;
            m_array.increment_gc();
            break;
        default:
            break;
        }
    }

    void make_not_root()
    {
        switch (m_tag)
        {
        case VT_ARRAY_ROOT:
            m_tag = VT_ARRAY;
            m_array.decrement_gc();
            break;
        default:
            break;
        }
    }
    #endif

    inline VariableType get_type()                { return static_cast<VariableType>(m_tag); };

    bool operator==(const Variable& v) const;
    bool operator!=(const Variable& v) const;
    bool operator>=(const Variable&) const;
    bool operator> (const Variable&) const;
    bool operator<=(const Variable&) const;
    bool operator< (const Variable&) const;
    inline bool cond() const
    {
        switch(m_tag)
        {
        case VT_UNDEFINED:
            throw MiscError("Condition on undefined variable.");
        case VT_BOOL:
            return m_int;
        case VT_INT:
            return m_int > 0;
        case VT_UINT64:
            return !!m_uint64;
        case VT_REAL:
            // [sic]
            return m_real >= 0.5;
        #ifdef OGM_GARBAGE_COLLECTOR
        case VT_ARRAY_ROOT:
        #endif
        case VT_ARRAY:
            throw UnknownIntendedBehaviourError("cond(): array");
        case VT_STRING:
            throw UnknownIntendedBehaviourError("cond(): string");
        case VT_PTR:
            throw UnknownIntendedBehaviourError("cond(): ptr");
            break;
        default:
            throw NotImplementedError("cond for unknown type");
        }
    }

    void operator+= (const Variable&);
    void operator-= (const Variable&);
    void operator*= (const Variable&);
    void operator/= (const Variable&);

    void operator%= (const Variable&);
    void operator<<=(const Variable&);
    void operator>>=(const Variable&);
    void operator&= (const Variable&);
    void operator|= (const Variable&);
    void operator^= (const Variable&);
    void idiv(const Variable&);
    void invert();

#if 0
    const Variable& operator[](int i) const;
    const Variable& operator[](const Variable&) const;
    Variable& operator[](int i);
    Variable& operator[](const Variable&);
#endif

    inline VariableType get_type() const { return (VariableType) m_tag; }

    inline const char* type_string() const;

    inline bool is_numeric() const
    {
        return get_type() == VT_BOOL || get_type() == VT_INT || get_type() == VT_UINT64 || get_type() == VT_REAL;
    }

    // checks if type is integral (not if an integer is represented)
    inline bool is_integral() const
    {
        return is_numeric() && get_type() != VT_REAL;
    }

    inline bool is_string() const
    {
        return get_type() == VT_STRING;
    }

    inline bool is_array() const
    {
        return get_type() == VT_ARRAY

        #ifdef OGM_GARBAGE_COLLECTOR
            || get_type() == VT_ARRAY_ROOT
        #endif
        ;
    }

    inline bool is_gc_root() const
    {
        #ifdef OGM_GARBAGE_COLLECTOR
        return get_type() == VT_ARRAY_ROOT;
        #else
        return false;
        #endif
    }

    inline bool is_undefined() const
    {
        return get_type() == VT_UNDEFINED;
    }
    
    inline bool is_pointer() const
    {
        return get_type() == VT_PTR;
    }

    // returns a direct reference to the variable of the given type.
    // this is unsafe because it doesn't check the variable's type!
    // only use this if you are statically certain of the variable's type.
    // cannot be used with strings.
    template<typename T>
    inline T& get();

    // returns a direct reference to the variable of the given type.
    // this is unsafe because it doesn't check the variable's type!
    // only use this if you are statically certain of the variable's type.
    // cannot be used with strings.
    template<typename T>
    inline const T& get() const;

    // returns the variable, casting it to the desired type if necessary,
    // or throwing a type error if it is the wrong type.
    // does not round or alter the value.
    template<typename T>
    inline T castExact() const;

    // returns value converted to the given type
    // possibly adjusts the value if necessary.
    // converting to bool evaluates cond().
    // can throw an error if the types are not reconcilable (e.g. ptr and string)
    template<typename T>
    inline T castCoerce() const;

    // converts the variable to hold the given type
    // possibly adjusts the value if necessary.
    // can throw an error if the types are not reconcilable (e.g. ptr and string)
    // converting to bool evaluates cond().
    // cannot be used with string.
    template<typename T>
    const T& coerce();

    // obtains a string view for the given string
    // an error is thrown if not a string.
    // resultant string view could be broken if ANY string variable is edited,
    // due to the way copy-on-write, grow-on-append works.
    std::string_view string_view() const;

    size_t string_length() const
    {
        ogm_assert(is_string());
        return m_string->length();
    }

    inline void shrink_string_to_range(size_t begin, size_t end)
    {
        ogm_assert(is_string());
        ogm_assert(begin <= end);
        ogm_assert(end <= m_string->length());

        #ifndef OPTIMIZE_STRING_APPEND
        if (begin == 0)
        {
            m_string->resize(end);
        }
        else
        {
            *m_string = m_string->substr(begin, end - begin);
        }
        #else
        m_string->shrink({ begin, end });
        #endif
    }

    inline void shrink_string_to_range(size_t begin)
    {
        ogm_assert(is_string());
        shrink_string_to_range(begin, m_string->length());
    }
    
    void gc_integrity_check() const;

    inline void cleanup()
    {
        switch (get_type())
        {
        case VT_STRING:
            delete m_string;

            // paranoia
            m_tag = VT_UNDEFINED;
            break;
        case VT_ARRAY:
            m_array.decrement();

            // paranoia
            m_tag = VT_UNDEFINED;
            break;
        #ifdef OGM_GARBAGE_COLLECTOR
        case VT_ARRAY_ROOT:
            m_array.decrement_gc();
            m_array.decrement();

            // paranoia
            m_tag = VT_UNDEFINED;
            break;
        #endif
        default:
            break;
        }
        return;
    }

    // if not already an array, switches the variable to be
    // array type, cleaning up its previous value.
    // if 'generate' is true, then additionally generate
    // an empty array rather than a null array data reference.
    // (two array variables can share an empty array this way.)
    inline void array_ensure(bool generate=false)
    {
        if (!is_array())
        {
            cleanup();
            m_tag = VT_ARRAY;
            m_array.initialize();
            if (generate)
            {
                m_array.getReadable<false>();
            }
        }
    }

    // retrieves the item at the given array position,
    // throwing an error if there is no such position.
    inline const Variable& array_at(size_t i, size_t j) const;

    // retrives a not-necessarily-initialized reference to
    // the item at the given array position,
    // initializing all values up to the column inclusive to zero.
    // invokes array_ensure() by default to make this an array.
    // if 'copy' is true, will copy the array if others have a reference to it.
    // TODO: const (non-copy) version of this.
    #ifdef OGM_GARBAGE_COLLECTOR
    // GCNode refers to the GCNode that should gain a reference to this array's data
    // if it is created via this get call.
    // For example, it might be the node of the array that this
    // array is contained in, if applicable.
    // It should be nullptr if there is no such node (e.g. if this is an instance,
    // global, or local variable.)
    inline Variable& array_get(size_t i, size_t j, bool copy=true, GCNode* owner=nullptr);
    #else
    inline Variable& array_get(size_t i, size_t j, bool copy=true);
    #endif

    inline size_t array_height() const;

    inline size_t array_length(size_t row = 0) const;

    const VariableArrayData& getReadableArray() const
    {
        switch(m_tag)
        {
        case VT_ARRAY:
            return m_array.getReadable<false>();
        #ifdef OGM_GARBAGE_COLLECTOR
        case VT_ARRAY_ROOT:
            return m_array.getReadable<true>();
        #endif
        default:
            ogm_assert(false);
            throw MiscError("(internal error) Cannot getReadableArray on non-array variable.");
        }
    }

    #ifdef OGM_GARBAGE_COLLECTOR
    inline GCNode* get_gc_node() const;

    template<bool write>
    void serialize(typename state_stream<write>::state_stream_t& s, GCNode* owner=nullptr);

    #else

    template<bool write>
    void serialize(typename state_stream<write>::state_stream_t& s);
    #endif



    // e.g. for `std::stringstream ss; ss << some_variable;`.
    std::ostream& write_to_stream(std::ostream&, size_t depth=0) const;

private:
    void check_type(VariableType) const;
    class TypeCastError : public std::exception
    {
    public:
        TypeCastError(VariableType src, VariableType dst)
            : m_src(src)
            , m_dst(dst)
            , m_message("Error casting type ")
        { combine(); }
        TypeCastError(std::string s, VariableType src, VariableType dst)
            : m_src(src)
            , m_dst(dst)
            , m_message(s)
        { combine(); }
        void combine();
        virtual const char* what() const noexcept override
        {
            return m_message.c_str();
        }
    protected:
        VariableType m_src;
        VariableType m_dst;
        std::string m_message;
    };
};

// if a variable is an array, its data field will be a copy-on-write pointer
// to one of these.
class VariableArrayData
{
    friend class VariableArrayHandle;

public:
    std::vector<std::vector<Variable>> m_vector;

private:
    // this is a naive refcount.
    int32_t m_reference_count;

    // this is the refcount marking this as a GC root.
    // (essentially, this should be 1 for each direct reference from
    // an instance or global reference.)
    int32_t m_gc_reference_count = 0;

public:
    #ifdef OGM_GARBAGE_COLLECTOR
    GCNode* m_gc_node{ g_gc.construct_node(
        [this]() -> void
        {
            delete this;
        }
    ) };
    #endif

private:
    VariableArrayData()
        : m_vector()
        , m_reference_count(0)
    { }

    VariableArrayData(const VariableArrayData& other)
        : m_vector()
        , m_reference_count(0)
    {
        // copy data
        m_vector.reserve(other.m_vector.size());
        for (const auto& row : other.m_vector)
        {
            m_vector.emplace_back();
            m_vector.back().reserve(row.size());
            for (const Variable& v : row)
            {
                m_vector.back().emplace_back();
                m_vector.back().back().copy(v);
            }
        }
    }

    inline void increment()
    {
        ++m_reference_count;
    }

    inline void decrement()
    {
        ogm_assert(m_reference_count > 0);
        if (--m_reference_count == 0)
        {
            // if GC is enabled, GC will delete this later anyway.
            #ifndef OGM_GARBAGE_COLLECTOR
            delete this;
            #else
            ogm_assert(m_gc_reference_count == 0);
            #endif
        }
    }

    #ifdef OGM_GARBAGE_COLLECTOR
    inline void increment_gc()
    {
        ++m_gc_reference_count;
        m_gc_node->m_root = true;
    }

    inline void decrement_gc()
    {
        ogm_assert(m_gc_reference_count > 0);
        if (--m_gc_reference_count == 0)
        {
            m_gc_node->m_root = false;
        }
    }
    
    void gc_integrity_check() const;
    #endif

};

const Variable k_undefined_variable;

inline void VariableArrayHandle::initialize()
{
    m_data = nullptr;
}

inline void VariableArrayHandle::initialize(const VariableArrayHandle& other)
{
    m_data = other.m_data;

    m_data->increment();
}

inline void VariableArrayHandle::decrement()
{
    if (!is_null())
    {
        m_data->decrement();
    }
}

#ifdef OGM_GARBAGE_COLLECTOR
inline void VariableArrayHandle::decrement_gc()
{
    if (!is_null())
    {
        m_data->decrement_gc();
    }
}

inline void VariableArrayHandle::increment_gc()
{
    if (!is_null())
    {
        m_data->increment_gc();
    }
}
#endif

template<bool gc_root>
inline const VariableArrayData& VariableArrayHandle::constructData() const
{
    ogm_assert(is_null());

    m_data = new VariableArrayData();
    m_data->increment();

    #ifdef OGM_GARBAGE_COLLECTOR
    if (gc_root)
    {
        m_data->increment_gc();
    }
    #endif

    return *m_data;
}

template<bool gc_root>
inline const VariableArrayData& VariableArrayHandle::getReadable() const
{
    if (is_null())
    {
        constructData<gc_root>();
    }
    return *m_data;
}

template<bool gc_root>
inline VariableArrayData& VariableArrayHandle::getWriteable(
    #ifdef OGM_GARBAGE_COLLECTOR
    GCNode* owner
    #endif
)
{
    if (is_null())
    {
        constructData<gc_root>();
    }
    else
    {
        if (m_data->m_reference_count > 1)
        // copy the data
        {
            // unlink with previous data
            #ifdef OGM_GARBAGE_COLLECTOR
            if (gc_root) m_data->decrement_gc();
            if (owner) owner->remove_reference(m_data->m_gc_node);
            #endif
            m_data->decrement();

            // create new data
            m_data = new VariableArrayData(*m_data);

            // link with it
            m_data->increment();
            #ifdef OGM_GARBAGE_COLLECTOR
            if (owner) owner->add_reference(m_data->m_gc_node);
            if (gc_root) m_data->increment_gc();
            #endif
        }
    }
    return *m_data;
}

template<bool gc_root>
inline VariableArrayData& VariableArrayHandle::getWriteableNoCopy()
{
    if (!m_data)
    {
        // FIXME: ensure this is the intended behaviour!
        constructData<gc_root>();
    }

    return *m_data;
}

typedef Variable var;

template<typename A>
Variable::Variable(std::vector<A> vec)
    : m_tag (VT_ARRAY)
{
    m_array.initialize();
    auto& data = m_array.getWriteableNoCopy<false>();
    ogm_assert(data.m_vector.size() == 0);
    data.m_vector.emplace_back();
    std::copy(vec.begin(), vec.end(), data.m_vector.front().begin());
}

template<>
inline bool_t& Variable::get()
{
    ogm_assert(m_tag == VT_BOOL);
    bool* b = static_cast<bool*>(static_cast<void*>(&m_int))+(sizeof(int) - 1) * IS_BIG_ENDIAN;
    return *b;
}

template<>
inline const bool_t& Variable::get() const
{
    ogm_assert(m_tag == VT_BOOL);
    const bool* b = static_cast<const bool*>(static_cast<void*>(const_cast<int32_t*>(&m_int)))+(sizeof(int) - 1) * IS_BIG_ENDIAN;
    return *b;
}

template<>
inline bool_t Variable::castExact() const
{
    switch(m_tag)
    {
        case VT_BOOL:
            return !!m_int;
        case VT_INT:
            if (m_int != 0 && m_int != 1)
            {
                throw TypeCastError("Range error converting ", static_cast<VariableType>(m_tag), VT_BOOL);
            }
            return m_int;
        case VT_REAL:
            if (m_real != 0 && m_real != 1)
            {
                throw TypeCastError("Range error converting ", static_cast<VariableType>(m_tag), VT_BOOL);
            }
            return m_real != 0;
        case VT_UINT64:
            if (m_uint64 != 0 && m_uint64 != 1)
            {
                throw TypeCastError("Range error converting ", static_cast<VariableType>(m_tag), VT_BOOL);
            }
            return m_uint64 != 0;
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_INT);
    }
}

template<>
inline bool_t Variable::castCoerce() const
{
    return this->cond();
}

template<>
inline const bool_t& Variable::coerce()
{
    m_int = this->cond();
    m_tag = VT_BOOL;
    return get<bool_t>();
}

template<>
inline int32_t& Variable::get()
{
    ogm_assert(m_tag == VT_INT);
    return m_int;
}

template<>
inline const int32_t& Variable::get() const
{
    ogm_assert(m_tag == VT_INT);
    return m_int;
}

template<>
inline uint32_t& Variable::get()
{
    ogm_assert(m_tag == VT_INT);
	// this is fine. :|
    return *static_cast<uint32_t*>(static_cast<void*>(&m_int));
}

template<>
inline const uint32_t& Variable::get() const
{
    ogm_assert(m_tag == VT_INT);
	// also fine.
    return *static_cast<uint32_t*>(static_cast<void*>(const_cast<int32_t*>(&m_int)));
}

template<>
inline int32_t Variable::castExact() const
{
    switch(m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            return m_int;
        case VT_REAL:
            if (m_real == (int32_t) m_real)
            {
                return (int32_t) m_real;
            }
            else
            {
                throw TypeCastError("Range error converting ",static_cast<VariableType>(m_tag), VT_INT);
            }
		case VT_UINT64:
            if (m_uint64 < 0x100000000)
            {
                return (int32_t) m_uint64;
            }
            else
            {
                throw TypeCastError("Range error converting ", static_cast<VariableType>(m_tag), VT_INT);
            }
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_INT);
    }
}

template<>
inline uint32_t Variable::castExact() const
{
	return castExact<int32_t>();
}

template<>
inline int32_t Variable::castCoerce() const
{
    switch(m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            return m_int;
        case VT_UINT64:
            return (int32_t) m_uint64;
        case VT_REAL:
            return (int32_t) m_real;
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_INT);
    }
}

template<>
inline const int32_t& Variable::coerce()
{
    switch(m_tag)
    {
        case VT_BOOL:
            m_tag = VT_INT;
            break;
        case VT_INT:
            break;
        case VT_UINT64:
            m_tag = VT_INT;
            m_int = (int32_t) m_uint64;
            break;
        case VT_REAL:
            m_tag = VT_INT;
            m_int = (int32_t) m_real;
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_INT);
    }
    return m_int;
}

template<>
inline uint64_t& Variable::get()
{
    ogm_assert(m_tag == VT_UINT64);
    return m_uint64;
}

template<>
inline const uint64_t& Variable::get() const
{
    ogm_assert(m_tag == VT_UINT64);
    return m_uint64;
}

template<>
inline uint64_t Variable::castExact() const
{
    switch(m_tag)
    {
        case VT_BOOL:
            return m_int;
        case VT_INT:
            if (m_int >= 0)
            {
                return m_int;
            }
            else
            {
                throw TypeCastError("Range error converting ", static_cast<VariableType>(m_tag), VT_UINT64);
            }
        case VT_REAL:
            if (m_real == (uint64_t) m_real)
            {
                return (uint64_t) m_real;
            }
            else
            {
                throw TypeCastError("Range error converting ", static_cast<VariableType>(m_tag), VT_UINT64);
            }
        case VT_UINT64:
            return m_uint64;
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_UINT64);
    }
}

#ifdef EMSCRIPTEN
// FIXME: this enable_if isn't actually safe
template<>
inline typename std::enable_if<
        !std::is_same<size_t, uint32_t>::value
        && !std::is_same<size_t, uint64_t>::value,
    size_t>::type Variable::castExact() const
{
    return castExact<uint64_t>();
}
#endif

template<>
inline uint64_t Variable::castCoerce() const
{
    switch(m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            return m_int;
        case VT_UINT64:
            return m_uint64;
        case VT_REAL:
            return (uint64_t) m_real;
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_UINT64);
    }
}

#ifdef EMSCRIPTEN
// FIXME: this enable_if isn't actually safe
template<>
inline typename std::enable_if<
        !std::is_same<size_t, uint32_t>::value
        && !std::is_same<size_t, uint64_t>::value,
    size_t>::type Variable::castCoerce() const
{
    return castCoerce<uint64_t>();
}
#endif

template<>
inline int64_t Variable::castCoerce() const
{
    switch(m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            return m_int;
        case VT_UINT64:
            return m_uint64;
        case VT_REAL:
            return (int64_t) m_real;
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_UINT64);
    }
}

template<>
inline uint32_t Variable::castCoerce() const
{
    return castCoerce<uint64_t>();
}

template<>
inline const uint64_t& Variable::coerce()
{
    switch(m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            m_tag = VT_UINT64;
            m_uint64 = m_int;
            break;
        case VT_UINT64:
            break;
        case VT_REAL:
            m_tag = VT_UINT64;
            m_uint64 = m_real;
            break;
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_UINT64);
    }
    return m_uint64;
}

template<>
inline real_t& Variable::get()
{
    ogm_assert(m_tag == VT_REAL);
    return m_real;
}

template<>
inline const real_t& Variable::get() const
{
    ogm_assert(m_tag == VT_REAL);
    return m_real;
}

template<>
inline real_t Variable::castExact() const
{
    switch(m_tag)
    {
        case VT_BOOL:
            return (real_t) m_int;
        case VT_INT:
            if (m_int <= 0x1000000 && -m_int <= 0x1000000)
            {
                return (real_t) m_int;
            }
            else
            {
                throw TypeCastError("Range error converting ", static_cast<VariableType>(m_tag), VT_REAL);
            }
        case VT_UINT64:
            if (m_uint64 <= 0x1000000)
            {
                return (real_t) m_uint64;
            }
            else
            {
                throw TypeCastError("Range error converting ", static_cast<VariableType>(m_tag), VT_REAL);
            }
        case VT_REAL:
            return m_real;
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_REAL);
    }
}

template<>
inline real_t Variable::castCoerce() const
{
    switch(m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            return m_int;
        case VT_UINT64:
            return m_uint64;
        case VT_REAL:
            return m_real;
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_REAL);
    }
}

template<>
inline const real_t& Variable::coerce()
{
    switch(m_tag)
    {
        case VT_BOOL:
        case VT_INT:
            m_tag = VT_REAL;
            m_real = m_int;
            break;
        case VT_UINT64:
            m_tag = VT_REAL;
            m_real = m_uint64;
            break;
        case VT_REAL:
            break;
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_REAL);
    }

    return m_real;
}

inline string_view_t Variable::string_view() const
{
    ogm_assert(m_tag == VT_STRING);
    #ifndef OPTIMIZE_STRING_APPEND
    return *m_string;
    #else
    return m_string->view();
    #endif
}

template<>
inline string_t Variable::castCoerce() const
{
    switch(m_tag)
    {
        case VT_STRING:
            return std::string{ string_view() };
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_STRING);
    }
}

// WARNING: if *any* string variable is modified, this string view might break.
// only use if it is certain that no other string variables will be modified.
template<>
inline string_view_t Variable::castCoerce() const
{
    switch(m_tag)
    {
        case VT_STRING:
            return string_view();
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_STRING);
    }
}

template<>
inline void*& Variable::get()
{
    ogm_assert(m_tag == VT_PTR);
    return m_ptr;
}

template<>
inline void* const& Variable::get() const
{
    ogm_assert(m_tag == VT_PTR);
    return m_ptr;
}

template<>
inline void* Variable::castExact() const
{
    switch(m_tag)
    {
        case VT_PTR:
            return m_ptr;
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_UINT64);
    }
}

template<>
inline void* const& Variable::castCoerce() const
{
    switch(m_tag)
    {
        case VT_PTR:
            return m_ptr;
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_PTR);
    }
}

template<>
inline void* const& Variable::coerce()
{
    switch(m_tag)
    {
        case VT_PTR:
            break;
        default:
            throw TypeCastError(static_cast<VariableType>(m_tag), VT_PTR);
    }

    return m_ptr;
}

template<>
inline VariableArrayHandle& Variable::get()
{
    ogm_assert(m_tag == VT_ARRAY);
    return m_array;
}

template<>
inline const VariableArrayHandle& Variable::get() const
{
    ogm_assert(m_tag == VT_ARRAY);
    return m_array;
}

inline std::ostream& Variable::write_to_stream(std::ostream& out, size_t depth) const
{
    // legacy laziness
    const auto& v = *this;

    switch (v.get_type())
    {
    case VT_UNDEFINED:
        out << "<undefined>";
        break;
    case VT_BOOL:
        if (v.get<bool>())
        {
            out << "True";
        }
        else
        {
            out << "False";
        }
        break;
    case VT_INT:
        out << v.get<int32_t>();
        break;
    case VT_UINT64:
        out << v.get<uint64_t>() << "L";
        break;
    case VT_REAL:
        {
            char s[0x40];
            snprintf(s, 0x40, "%.2f", v.get<real_t>());
            out << s;
        }
        break;
    case VT_STRING:
        out << "\"" << v.string_view() << "\"";
        break;
    case VT_ARRAY:
    #ifdef OGM_GARBAGE_COLLECTOR
    case VT_ARRAY_ROOT:
    #endif
        {
            bool first = true;
            out << "[";
            for (size_t i = 0; i < v.array_height(); ++i)
            {
                if (!first)
                {
                    out << ", ";
                }
                first = false;
                bool _first = true;
                out << "[";
                for (size_t j = 0; j < v.array_length(i); ++j)
                {
                    if (!_first)
                    {
                        out << ", ";
                    }
                    _first = false;

                    v.array_at(i, j).write_to_stream(out, depth);
                }
                out << "]";
            }
            out << "]";
        }
        break;
    case VT_PTR:
        {
            char s[0x40];
            snprintf(s, 0x40, "<%p>", v.get<void*>());
            out << s;
        }
        break;
    }

    return out;
}


static std::ostream& operator<<(std::ostream& out, const Variable& v)
{
    return v.write_to_stream(out);
}

inline size_t Variable::array_height() const
{
    switch(m_tag)
    {
    case VT_ARRAY:
        return m_array.getReadable<false>().m_vector.size();
        break;
    #ifdef OGM_GARBAGE_COLLECTOR
    case VT_ARRAY_ROOT:
        return m_array.getReadable<true>().m_vector.size();
        break;
    #endif
    default:
        return 0;
    }
}

inline size_t Variable::array_length(size_t row) const
{
    if (is_array())
    {
        const auto& vec = (is_gc_root())
            ? m_array.getReadable<true>().m_vector
            : m_array.getReadable<false>().m_vector;
        if (row < vec.size())
        {
            return vec.at(row).size();
        }
        else
        {
            throw UnknownIntendedBehaviourError("length of non-existent row.");
        }
    }
    else
    {
        return 0;
    }
}

inline const Variable& Variable::array_at(size_t row, size_t column) const
{
    if (!is_array())
    {
        throw MiscError("Indexing variable which is not an array.");
    }

    if (row >= array_height())
    {
        throw MiscError("Array index out of bounds: " + std::to_string(row) + "," + std::to_string(column) + " not in bounds " + std::to_string(array_height()) + ", 0");
    }
    else
    {
        const auto& row_vec = (is_gc_root())
            ? m_array.getReadable<false>().m_vector.at(row)
            : m_array.getReadable<true>().m_vector.at(row);
        if (column >= row_vec.size())
        {
            throw MiscError("Array index out of bounds: " + std::to_string(row) + "," + std::to_string(column) + " not in bounds " + std::to_string(array_height()) + ", " + std::to_string(row_vec.size()));
        }
        else
        {
            return row_vec.at(column);
        }
    }
}

inline Variable& Variable::array_get(
    size_t row, size_t column, bool copy
    #ifdef OGM_GARBAGE_COLLECTOR
    // the GC node that owns this array.
    // This is used when accessing nested arrays.
    , GCNode* owner
    #endif
)
{
    // FIXME: ensure this is the intended behaviour if not copying.
    array_ensure();

    #ifdef OGM_GARBAGE_COLLECTOR
    // cannot be both the gc root and have an owner.
    ogm_assert(!is_gc_root() || !owner);

    // vector data for this array.
    auto& vec = (copy)
        ? (is_gc_root())
            ? m_array.getWriteable<true>(owner).m_vector
            : m_array.getWriteable<false>(owner).m_vector
        : (is_gc_root())
            ? m_array.getWriteableNoCopy<true>().m_vector
            : m_array.getWriteableNoCopy<false>().m_vector;
    #else

    // vector data for this array.
    auto& vec = (copy)
        ? m_array.getWriteable().m_vector
        : m_array.getWriteableNoCopy().m_vector;
    #endif

    if (row >= vec.size())
    // fill rows
    {
        vec.resize(row + 1);
    }

    auto& row_vec = vec[row];

    if (column >= row_vec.size())
    {
        row_vec.reserve(column + 1);

        // fill with zeros up to column inclusive.
        for (size_t i = row_vec.size(); i <= column; i++)
        {
            row_vec.emplace_back(0);
        }
    }
    
    return row_vec[column];
}

#ifdef OGM_GARBAGE_COLLECTOR
inline GCNode* Variable::get_gc_node() const
{
    if (is_array())
    {
        // this constructs an empty array if necessary.
        return getReadableArray().m_gc_node;
    }
    else
    {
        return nullptr;
    }
}
#endif


inline const char* Variable::type_string() const
{
    switch (get_type())
    {
        case VT_UNDEFINED:  return "undefined";
        case VT_BOOL:       return "boolean";
        case VT_INT:        return "int";
        case VT_UINT64:     return "int64";
        case VT_REAL:       return "real";
        case VT_STRING:     return "string";
        case VT_ARRAY:      return "array";
        case VT_PTR:        return "pointer";
        default:            return "unknown";
    }
}

template<bool write>
void Variable::serialize(typename state_stream<write>::state_stream_t& s
#ifdef OGM_GARBAGE_COLLECTOR
    , GCNode* owner
#endif
)
{
    if (!write) cleanup();
    _serialize<write>(s, m_tag);
    switch (m_tag)
    {
        case VT_UNDEFINED:
            // fallthrough to make branch prediction easier
        case VT_BOOL:
        case VT_INT:
        case VT_UINT64:
        case VT_REAL:
        case VT_PTR: // (serializing a pointer doesn't really make sense.)
            // large enough to store all of the above
            _serialize<write>(s, m_uint64);
            break;
        case VT_STRING:
            if (write)
            {
                typename std::conditional<write, std::string_view, std::string>::type sv{ this->string_view() };
                _serialize<write>(s, sv);
            }
            else
            {
                m_tag = VT_UNDEFINED;
                std::string _s;
                _serialize<write>(s, _s);
                this->set(_s);
            }
            break;
        case VT_ARRAY:
        #ifdef OGM_GARBAGE_COLLECTOR
        case VT_ARRAY_ROOT:
        #endif
            {
                // TODO: shared array references must be respected.
                _serialize_canary<write>(s, 0xDEADBEEFAE1D5A5A);
                if (write)
                {
                    uint64_t h = array_height();
                    _serialize<write>(s, h);
                    for (size_t i = 0; i < h; ++i)
                    {
                        uint64_t l = array_length(i);
                        _serialize<write>(s, l);
                        for (size_t j = 0; j < l; ++j)
                        {
                            // this const cast is fine, because serialize<false> ought to be const.
                            #ifdef OGM_GARBAGE_COLLECTOR
                            const_cast<Variable&>(array_at(i, j)).template serialize<write>(s, nullptr);
                            #else
                            const_cast<Variable&>(array_at(i, j)).template serialize<write>(s);
                            #endif
                        }
                    }
                }
                else
                {
                    {
                        // create a new array.
                        // (have to set the tag back to undefined or else array_ensure
                        // will early-out).
                        auto tag = m_tag;
                        m_tag = VT_UNDEFINED;
                        this->array_ensure();
                        m_tag = tag;
                    }

                    // read array data.
                    uint64_t h;
                    _serialize<write>(s, h);
                    for (size_t i = 0; i < h; ++i)
                    {
                        uint64_t l;
                        _serialize<write>(s, l);
                        for (size_t j = 0; j < l; ++j)
                        {
                            // TODO: arrays should be serialized by-reference (i.e., unswizzled).

                            // the const version of this function never gets here,
                            // so the const cast is okay.
                            #ifdef OGM_GARBAGE_COLLECTOR
                            const_cast<Variable&>(array_get(i, j, false, owner))
                                .template serialize<write>(s, get_gc_node());
                            #else
                            const_cast<Variable&>(array_get(i, j, false)).template serialize<write>(s);
                            #endif
                        }
                    }
                }
                _serialize_canary<write>(s, 0xF4DBEED13A1DEAD7);
                break;
            }
            break;
        default:
            throw MiscError("Cannot serialize unknown type");
    }
}

static_assert(
    std::is_void<
        decltype(
            std::declval<Variable>().serialize<false>(
                *std::declval<state_stream<false>::state_stream_t*>()
            )
        )
    >::value
, "Variable::serialize is void");

static_assert(has_serialize<Variable>::value, "Variables must be serializable");

}}
