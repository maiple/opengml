#pragma once

#ifdef NDEBUG
#define inline_if_ndebug inline
#else
#define inline_if_ndebug
#endif

namespace ogm::interpreter
{

template<typename Data>
inline_if_ndebug void VariableComponentHandle<Data>::initialize()
{
    m_data = nullptr;
}

template<typename Data>
inline_if_ndebug void VariableComponentHandle<Data>::initialize(const VariableComponentHandle<Data>& other)
{
    m_data = other.m_data;

    m_data->increment();
}

template<typename Data>
inline_if_ndebug void VariableComponentHandle<Data>::decrement()
{
    if (!is_null())
    {
        m_data->decrement();
    }
}

#ifdef OGM_GARBAGE_COLLECTOR
template<typename Data>
inline_if_ndebug void VariableComponentHandle<Data>::decrement_gc()
{
    if (!is_null())
    {
        m_data->decrement_gc();
    }
}

template<typename Data>
inline_if_ndebug void VariableComponentHandle<Data>::increment_gc()
{
    if (!is_null())
    {
        m_data->increment_gc();
    }
}
#endif

template<typename Data> template<bool gc_root>
inline_if_ndebug const Data& VariableComponentHandle<Data>::constructData() const
{
    ogm_assert(is_null());

    m_data = new Data();
    
    // we increment because this handle is pointing to the data.
    m_data->increment();

    #ifdef OGM_GARBAGE_COLLECTOR
    if (gc_root)
    {
        m_data->increment_gc();
    }
    #endif

    return *m_data;
}


template<typename Data> template<bool gc_root>
inline_if_ndebug const Data& VariableComponentHandle<Data>::getReadable() const
{
    if (is_null())
    {
        constructData<gc_root>();
    }
    return *m_data;
}

template<typename Data> template<bool gc_root>
inline_if_ndebug Data& VariableComponentHandle<Data>::getWriteable(
    #ifdef OGM_GARBAGE_COLLECTOR
    GCNode* owner
    #endif
)
{
    if (is_null())
    {
        constructData<gc_root>();
        
        #ifdef OGM_GARBAGE_COLLECTOR
        if (owner) owner->add_reference(m_data->m_gc_node);
        #endif
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
            m_data = new Data(*m_data);

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

template<typename Data> template<bool gc_root>
inline_if_ndebug Data& VariableComponentHandle<Data>::getWriteableNoCopy()
{
    if (!m_data)
    {
        // FIXME: ensure this is the intended behaviour!
        // FIXME: no root is supplied here.
        constructData<gc_root>();
    }

    return *m_data;
}

template<typename A>
Variable::Variable(std::vector<A> vec)
    : m_tag (VT_ARRAY)
{
    m_array.initialize();
    auto& data = m_array.getWriteableNoCopy<false>();
    ogm_assert(data.m_vector.size() == 0);
    #ifdef OGM_2DARRAY
    data.m_vector.emplace_back();
    #endif
    std::copy(vec.begin(), vec.end(), data.m_vector
        #ifdef OGM_2DARRAY
        .front()
        #endif
        .begin()
    );
}

template<>
inline_if_ndebug bool_t& Variable::get()
{
    ogm_assert(m_tag == VT_BOOL);
    bool* b = static_cast<bool*>(static_cast<void*>(&m_int))+(sizeof(int) - 1) * IS_BIG_ENDIAN;
    return *b;
}

template<>
inline_if_ndebug const bool_t& Variable::get() const
{
    ogm_assert(m_tag == VT_BOOL);
    const bool* b = static_cast<const bool*>(static_cast<void*>(const_cast<int32_t*>(&m_int)))+(sizeof(int) - 1) * IS_BIG_ENDIAN;
    return *b;
}

template<>
inline_if_ndebug bool_t Variable::castExact() const
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
inline_if_ndebug bool_t Variable::castCoerce() const
{
    return this->cond();
}

template<>
inline_if_ndebug const bool_t& Variable::coerce()
{
    m_int = this->cond();
    m_tag = VT_BOOL;
    return get<bool_t>();
}

template<>
inline_if_ndebug int32_t& Variable::get()
{
    ogm_assert(m_tag == VT_INT);
    return m_int;
}

template<>
inline_if_ndebug const int32_t& Variable::get() const
{
    ogm_assert(m_tag == VT_INT);
    return m_int;
}

template<>
inline_if_ndebug uint32_t& Variable::get()
{
    ogm_assert(m_tag == VT_INT);
	// this is fine. :|
    return *static_cast<uint32_t*>(static_cast<void*>(&m_int));
}

template<>
inline_if_ndebug const uint32_t& Variable::get() const
{
    ogm_assert(m_tag == VT_INT);
	// also fine.
    return *static_cast<uint32_t*>(static_cast<void*>(const_cast<int32_t*>(&m_int)));
}

template<>
inline_if_ndebug int32_t Variable::castExact() const
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
inline_if_ndebug uint32_t Variable::castExact() const
{
	return castExact<int32_t>();
}

template<>
inline_if_ndebug int32_t Variable::castCoerce() const
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
inline_if_ndebug const int32_t& Variable::coerce()
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
inline_if_ndebug uint64_t& Variable::get()
{
    ogm_assert(m_tag == VT_UINT64);
    return m_uint64;
}

template<>
inline_if_ndebug const uint64_t& Variable::get() const
{
    ogm_assert(m_tag == VT_UINT64);
    return m_uint64;
}

template<>
inline_if_ndebug uint64_t Variable::castExact() const
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
inline_if_ndebug typename std::enable_if<
        !std::is_same<size_t, uint32_t>::value
        && !std::is_same<size_t, uint64_t>::value,
    size_t>::type Variable::castExact() const
{
    return castExact<uint64_t>();
}
#endif

template<>
inline_if_ndebug uint64_t Variable::castCoerce() const
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
inline_if_ndebug typename std::enable_if<
        !std::is_same<size_t, uint32_t>::value
        && !std::is_same<size_t, uint64_t>::value,
    size_t>::type Variable::castCoerce() const
{
    return castCoerce<uint64_t>();
}
#endif

template<>
inline_if_ndebug int64_t Variable::castCoerce() const
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
inline_if_ndebug uint32_t Variable::castCoerce() const
{
    return castCoerce<uint64_t>();
}

template<>
inline_if_ndebug const uint64_t& Variable::coerce()
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
inline_if_ndebug real_t& Variable::get()
{
    ogm_assert(m_tag == VT_REAL);
    return m_real;
}

template<>
inline_if_ndebug const real_t& Variable::get() const
{
    ogm_assert(m_tag == VT_REAL);
    return m_real;
}

template<>
inline_if_ndebug real_t Variable::castExact() const
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
inline_if_ndebug real_t Variable::castCoerce() const
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
inline_if_ndebug const real_t& Variable::coerce()
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

inline_if_ndebug string_view_t Variable::string_view() const
{
    ogm_assert(m_tag == VT_STRING);
    #ifndef OPTIMIZE_STRING_APPEND
    return *m_string;
    #else
    return m_string->view();
    #endif
}

template<>
inline_if_ndebug string_t Variable::castCoerce() const
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
inline_if_ndebug string_view_t Variable::castCoerce() const
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
inline_if_ndebug void*& Variable::get()
{
    ogm_assert(m_tag == VT_PTR);
    return m_ptr;
}

template<>
inline_if_ndebug void* const& Variable::get() const
{
    ogm_assert(m_tag == VT_PTR);
    return m_ptr;
}

template<>
inline_if_ndebug void* Variable::castExact() const
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
inline_if_ndebug void* const& Variable::castCoerce() const
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
inline_if_ndebug void* const& Variable::coerce()
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
inline_if_ndebug VariableArrayHandle& Variable::get()
{
    ogm_assert(m_tag == VT_ARRAY);
    return m_array;
}

template<>
inline_if_ndebug const VariableArrayHandle& Variable::get() const
{
    ogm_assert(m_tag == VT_ARRAY);
    return m_array;
}

inline_if_ndebug std::ostream& Variable::write_to_stream(std::ostream& out, size_t depth) const
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
                #ifdef OGM_2DARRAY
                for (size_t j = 0; j < v.array_length(i); ++j)
                {
                    if (!_first)
                    {
                        out << ", ";
                    }
                    _first = false;

                    v.array_at(i, j).write_to_stream(out, depth);
                }
                #else
                v.array_at(i).write_to_stream(out, depth);
                #endif
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

inline_if_ndebug size_t Variable::array_height() const
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

#ifdef OGM_2DARRAY
inline_if_ndebug size_t Variable::array_length(size_t row) const
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
#endif

inline_if_ndebug const Variable& Variable::array_at(size_t row
    #ifdef OGM_2DARRAY
    , size_t column
    #endif
) const
{
    if (!is_array())
    {
        throw MiscError("Indexing variable which is not an array.");
    }

    if (row >= array_height())
    {
        throw MiscError("Array index out of bounds: " + std::to_string(row)
         #ifdef OGM_2DARRAY
         + "," + std::to_string(column)
         #endif
         + " not in bounds " + std::to_string(array_height()) + ", 0");
    }
    else
    {
        const auto& rv = (is_gc_root())
            ? m_array.getReadable<false>().m_vector.at(row)
            : m_array.getReadable<true>().m_vector.at(row);
        #ifdef OGM_2DARRAY
        if (column >= rv.size())
        {
            throw MiscError("Array index out of bounds: " + std::to_string(row) + "," + std::to_string(column) + " not in bounds " + std::to_string(array_height()) + ", " + std::to_string(row_vec.size()));
        }
        else
        {
            return rv.at(column);
        }
        #else
        return rv;
        #endif
    }
}

inline_if_ndebug Variable& Variable::array_get(
    size_t row
    #ifdef OGM_2DARRAY
    , size_t column
    #endif
    , bool copy
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
        #ifndef OGM_2DARRAY
        size_t prev_size = vec.size();
        #endif
        
        vec.resize(row + 1);
        
        #ifndef OGM_2DARRAY
        for (size_t i = prev_size; i < row; ++i)
        {
            vec.at(i).set(0.0);
        }
        #endif
    }

    #ifdef OGM_2DARRAY
    auto& row_vec = vec[row];

    if (column >= row_vec.size())
    {
        row_vec.reserve(column + 1);

        // fill with zeros up to column inclusive.
        for (size_t i = row_vec.size(); i <= column; i++)
        {
            row_vec.emplace_back(0.0);
        }
    }
    return row_vec[column];
    #else
    return vec[row];
    #endif
}

#ifdef OGM_GARBAGE_COLLECTOR
inline_if_ndebug GCNode* Variable::get_gc_node() const
{
    if (is_array())
    {
        // this constructs an empty array if necessary.
        return getReadableArray().m_gc_node;
    }
    #ifdef OGM_STRUCT_SUPPORT
    else if (is_struct())
    {
        return m_struct.getReadable<false>().m_gc_node;
    }
    #endif
    else
    {
        return nullptr;
    }
}
#endif


inline_if_ndebug const char* Variable::type_string() const
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
                        #ifdef OGM_2DARRAY
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
                        #else
                            // this const cast is fine, because serialize<false> ought to be const.
                            #ifdef OGM_GARBAGE_COLLECTOR
                            const_cast<Variable&>(array_at(i)).template serialize<write>(s, nullptr);
                            #else
                            const_cast<Variable&>(array_at(i)).template serialize<write>(s);
                            #endif
                        #endif
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
                        #ifdef OGM_2DARRAY
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
                        #else
                            // TODO: arrays should be serialized by-reference (i.e., unswizzled).

                            // the const version of this function never gets here,
                            // so the const cast is okay.
                            #ifdef OGM_GARBAGE_COLLECTOR
                            const_cast<Variable&>(array_get(i, false, owner))
                                .template serialize<write>(s, get_gc_node());
                            #else
                            const_cast<Variable&>(array_get(i, false)).template serialize<write>(s);
                            #endif
                        #endif
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

}

#undef inline_if_ndebug