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

#ifdef NDEBUG
#define inline_if_ndebug inline
#else
#define inline_if_ndebug
#endif

namespace ogm::interpreter
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

#ifdef OGM_2DARRAY
#define OGM_2DARRAY_DEFAULT_ROW 0,
#else
#define OGM_2DARRAY_DEFAULT_ROW
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
    #ifdef OGM_STRUCT_SUPPORT
        VT_STRUCT, // lightweight object
        #ifdef OGM_GARBAGE_COLLECTOR
            VT_STRUCT_ROOT, // lightweight object (GC root -- for example, stored directly in an instance or global.)
        #endif
    #endif
    #ifdef OGM_FUNCTION_SUPPORT
    VT_FUNCTION,
    #endif
    VT_PTR, // other data
};

extern const char* const variable_type_string[];

template <typename Data>
class VariableComponentHandle
{
private:
    // mutable because we need empty and null to look similar
    // externally.
    mutable Data* m_data;

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
    inline_if_ndebug const Data& constructData() const;

public:

    inline_if_ndebug void initialize();
    inline_if_ndebug void initialize(const VariableComponentHandle<Data>&);
    inline_if_ndebug void initialize(Data*);
    inline_if_ndebug void decrement(); // used when cleaned up

    #ifdef OGM_GARBAGE_COLLECTOR
    inline_if_ndebug void decrement_gc();
    inline_if_ndebug void increment_gc();
    #endif

    template<bool gc_root>
    inline_if_ndebug const Data& getReadable() const;

    #ifdef OGM_GARBAGE_COLLECTOR
    template<bool gc_root>
    inline_if_ndebug Data& getWriteable(GCNode* owner);

    void gc_integrity_check() const;
    #else

    // the gc_root template is here just for the sake of
    // not having to write different code in some cases
    // depending on whether the garbage collector is enabled.
    // (It should have no effect.)
    template<bool gc_root=false>
    inline_if_ndebug Data& getWriteable();
    #endif

    template<bool gc_root=false>
    inline_if_ndebug Data& getWriteableNoCopy();
};

// base class for variable component data such as array data or struct data.
class VariableComponentData
{
    template<typename>
    friend class VariableComponentHandle;
private:
    // this is a naive refcount.
    int32_t m_reference_count = 0;

    #ifdef OGM_GARBAGE_COLLECTOR
    // this is the refcount marking this as a GC root.
    // (essentially, this should be 1 for each direct reference from
    // an instance or global reference.)
    int32_t m_gc_reference_count = 0;

public:
    GCNode* m_gc_node{ g_gc.construct_node(
        [this]() -> void
        {
            this->cleanup();
        },
        [this]() -> void
        {
            delete this;
        },
        this
    ) };
    #endif

public:
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

    // cleanup phase occurs before delete phase.
    // (this allows safely unlinking from other variables before
    // all unused variables are deleted.)
    virtual void cleanup()=0;

    virtual ~VariableComponentData()=default;
};

class VariableArrayData;
typedef VariableComponentHandle<VariableArrayData> VariableArrayHandle;

#ifdef OGM_STRUCT_SUPPORT
class VariableStructData;
typedef VariableComponentHandle<VariableStructData> VariableStructHandle;

class Instance;
class VariableStructData : public VariableComponentData
{
    // nota bene: full namespace here is required due to GCC bug #52625.
    friend class ogm::interpreter::VariableComponentHandle<VariableStructData>;
public:
    Instance* m_instance;

private:
    VariableStructData();

    VariableStructData(const VariableStructData& other)
        : m_instance{ nullptr }
    {
        // it should not be possible to copy a struct yet.
        assert(false);
    }

    void cleanup();

    ~VariableStructData();
};
#endif

class Variable
{
    // 1 byte
    byte m_tag = (byte)VT_UNDEFINED;

    // 3 bytes
    union
    {
        uint32_t m_24 :24;
        #ifdef OGM_FUNCTION_SUPPORT
        uint32_t m_bytecode_index : 24;
        #endif
    };

    // 8 bytes
    union
    {
      real_t m_real;
      int32_t m_int;
      uint64_t m_uint64;
      string_data_t* m_string;
      VariableArrayHandle m_array;
      #ifdef OGM_STRUCT_SUPPORT
      VariableStructHandle m_struct;
      #endif
      #ifdef OGM_FUNCTION_SUPPORT
      Variable* m_binding;
      #endif
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
            m_int = static_cast<int32_t>(v);
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
            #ifdef OGM_STRUCT_SUPPORT
                case VT_STRUCT:
                case_VT_STRUCT:
                    m_struct.initialize(v.m_struct);
                    break;
                #ifdef OGM_GARBAGE_COLLECTOR
                    case VT_STRUCT_ROOT:
                        // shed ROOT quality.
                        m_tag = VT_STRUCT;
                        goto case_VT_STRUCT;
                #endif
            #endif
            #ifdef OGM_FUNCTION_SUPPORT
                case VT_FUNCTION:
                    m_bytecode_index = v.m_bytecode_index;
                    m_binding = new Variable;
                    m_binding->copy(*v.m_binding);
                    break;
            #endif
            case VT_UNDEFINED:
                break;
            default:
                ogm_assert(false);
                throw MiscError("(internal) Unknown variable type for Variable::set()");
        }

        return *this;
    }

    #ifdef OGM_FUNCTION_SUPPORT
    inline Variable& set_function_binding(Variable&& binding, bytecode_index_t index)
    {
        cleanup();
        m_tag = VT_FUNCTION;
        assert(index <= 0xffffff);
        m_bytecode_index = index;
        m_binding = new Variable(std::move(binding));
        return *this;
    }

    inline bytecode_index_t get_bytecode_index() const
    {
        ogm_assert(is_function());
        return m_bytecode_index;
    }

    inline const Variable& get_binding() const
    {
        ogm_assert(is_function());
        return *m_binding;
    }
    #endif

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
        m_24 = v.m_24;
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
        #ifdef OGM_STRUCT_SUPPORT
        case VT_STRUCT:
            m_tag = VT_STRUCT_ROOT;
            m_struct.increment_gc();
            break;
        #endif
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
        #ifdef OGM_STRUCT_SUPPORT
        case VT_STRUCT_ROOT:
            m_tag = VT_STRUCT;
            m_struct.decrement_gc();
            break;
        #endif
        default:
            break;
        }
    }
    #endif

    inline VariableType get_type() { return static_cast<VariableType>(m_tag); };

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
        case VT_PTR:
            throw UnknownIntendedBehaviourError("cond(): ptr");
            break;
        default:
            return false;
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

    inline_if_ndebug const char* type_string() const;

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
        return get_type() == VT_ARRAY_ROOT
        #ifdef OGM_STRUCT_SUPPORT
            || get_type() == VT_STRUCT_ROOT
        #endif
            ;
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

    #ifdef OGM_STRUCT_SUPPORT
    inline bool is_struct() const
    {
        return get_type() == VT_STRUCT
        #ifdef OGM_GARBAGE_COLLECTOR
            || get_type() == VT_STRUCT_ROOT
        #endif
        ;
    }
    #endif

    #ifdef OGM_FUNCTION_SUPPORT
    inline bool is_function() const
    {
        return get_type() == VT_FUNCTION;
    }
    #endif

    // returns a direct reference to the variable of the given type.
    // this is unsafe because it doesn't check the variable's type!
    // only use this if you are statically certain of the variable's type.
    // cannot be used with strings.
    template<typename T>
    inline_if_ndebug T& get();

    // returns a direct reference to the variable of the given type.
    // this is unsafe because it doesn't check the variable's type!
    // only use this if you are statically certain of the variable's type.
    // cannot be used with strings.
    template<typename T>
    inline_if_ndebug const T& get() const;

    // returns the variable, casting it to the desired type if necessary,
    // or throwing a type error if it is the wrong type.
    // does not round or alter the value.
    template<typename T>
    inline_if_ndebug T castExact() const;

    // returns value converted to the given type
    // possibly adjusts the value if necessary.
    // converting to bool evaluates cond().
    // can throw an error if the types are not reconcilable (e.g. ptr and string)
    template<typename T>
    inline_if_ndebug T castCoerce() const;

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
        #ifdef OGM_STRUCT_SUPPORT
            case VT_STRUCT:
                m_struct.decrement();

                // paranoia
                m_tag = VT_UNDEFINED;
                break;
            #ifdef OGM_GARBAGE_COLLECTOR
                case VT_STRUCT_ROOT:
                    m_struct.decrement_gc();
                    m_struct.decrement();

                    // paranoia
                    m_tag = VT_UNDEFINED;
                    break;
            #endif
        #endif
        #ifdef OGM_FUNCTION_SUPPORT
            case VT_FUNCTION:
                delete m_binding;
                m_binding = nullptr;

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
    inline_if_ndebug const Variable& array_at(size_t i
        #ifdef OGM_2DARRAY
        , size_t j
        #endif
    ) const;

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
    inline_if_ndebug Variable& array_get(size_t i
        #ifdef OGM_2DARRAY
        , size_t j
        #endif
        , bool copy=true, GCNode* owner=nullptr);
    #else
    inline_if_ndebug Variable& array_get(size_t i
        #ifdef OGM_2DARRAY
        , size_t j
        #endif
        , bool copy=true);
    #endif

    inline_if_ndebug size_t array_height() const;

    #ifdef OGM_2DARRAY
    inline_if_ndebug size_t array_length(size_t row = 0) const;
    #endif

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

    #ifdef OGM_STRUCT_SUPPORT
    void make_struct(
        #ifdef OGM_GARBAGE_COLLECTOR
        GCNode* owner
        #endif
    )
    {
        m_tag = VT_STRUCT;
        m_struct.initialize();
        m_struct.getWriteable<false>(
            #ifdef OGM_GARBAGE_COLLECTOR
            owner
            #endif
        );
    }

    Instance* get_struct()
    {
        switch(m_tag)
        {
            case VT_STRUCT:
                return m_struct.getWriteableNoCopy<false>().m_instance;
            #ifdef OGM_GARBAGE_COLLECTOR
            case VT_STRUCT_ROOT:
                return m_struct.getWriteableNoCopy<true>().m_instance;
            #endif
            default:
            ogm_assert(false);
            throw MiscError("(internal error) Cannot get_struct on non-struct variable.");
        }
    }

    void set_from_struct(
        VariableStructData* data
    )
    {
        m_tag = VT_STRUCT;
        m_struct.initialize(data);
    }
    #endif

    #ifdef OGM_GARBAGE_COLLECTOR
    inline_if_ndebug GCNode* get_gc_node() const;

    template<bool write>
    void serialize(typename state_stream<write>::state_stream_t& s, GCNode* owner=nullptr);

    #else

    template<bool write>
    void serialize(typename state_stream<write>::state_stream_t& s);
    #endif

    // e.g. for `std::stringstream ss; ss << some_variable;`.
    std::ostream& write_to_stream(std::ostream&, size_t depth=0) const;

private:
    // sets to a division by zero.
    void divzero();
    
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
}; // class Variable

// variables can be size 16 or 12 depending on alignment.
static_assert(sizeof(Variable) <= 16, "Variables must be at most size 16.");

// if a variable is an array, its data field will be a copy-on-write pointer
// to one of these.
class VariableArrayData : public VariableComponentData
{
    friend class ogm::interpreter::VariableComponentHandle<VariableArrayData>;
public:
    #ifdef OGM_2DARRAY
    std::vector<std::vector<Variable>> m_vector;
    #else
    std::vector<Variable> m_vector;
    #endif

private:
    VariableArrayData()
        : m_vector()
    { }

    VariableArrayData(const VariableArrayData& other)
        : m_vector()
    {
        // copy data
        m_vector.reserve(other.m_vector.size());
        for (const auto& r : other.m_vector)
        {
            m_vector.emplace_back();
            #ifdef OGM_2DARRAY
            m_vector.back().reserve(r.size());
            for (const Variable& v : r)
            {
                m_vector.back().emplace_back();
                m_vector.back().back().copy(v);
            }
            #else
            m_vector.back().copy(r);
            #endif
        }
    }

    void cleanup();

    ~VariableArrayData();
};

static const Variable k_undefined_variable;

static std::ostream& operator<<(std::ostream& out, const Variable& v)
{
    return v.write_to_stream(out);
}

typedef Variable var;
}

#undef inline_if_ndebug

#ifdef NDEBUG
#include "Variable_impl.inc"
#endif
