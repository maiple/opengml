#include "Variable.hpp"

namespace ogm
{
// wrapper for variable, which automatically cleans-up on deletion.
class SafeVariable
{
private:
    Variable v;
    
public:
    template<typename... Args>
    SafeVariable(Args... a) : v(a...) {}
    SafeVariable(Variable&& other) : v(other)
    {
        other.cleanup();
        other.m_tag = VT_UNDEFINED;
    }
    SafeVariable(SafeVariable&& other) : SafeVariable(std::move(other.v)) { }
    
    operator const Variable&() const { return v; }
    
    #ifdef OGM_FUNCTION_SUPPORT
    inline Variable& set_function_binding(Variable&& binding, bytecode_index_t index)
    {
        return v.set_function_binding(binding, index);
    }
    
    inline bytecode_index_t get_bytecode_index() const
    {
        return v.get_bytecode_index()
    }
    
    inline bytecode_index_t get_binding() const
    {
        return v.get_binding()
    }
    #endif
    
    template<typename T>
    inline SafeVariable& operator=(T& x)
    {
        v.cleanup();
        v = x;
        v.make_root();
        return *this;
    }
    
    template<typename T>
    inline SafeVariable& operator=(T&& x)
    {
        v.cleanup();
        v = std::move(x);
        v.make_root();
        return *this;
    }
    
    friend Variable& operator=(Variable& lhs, SafeVariable&& rhs)
    {
        lhs = std::move(rhs.v);
        return lhs;
    }
    
    inline VariableType get_type() { return v.get_type(); }
    
    bool operator==(const Variable& o) const { return v == o; }
    bool operator!=(const Variable& o) const { return v != o; }
    bool operator>=(const Variable& o) const { return v >= o; }
    bool operator> (const Variable& o) const { return v > o; }
    bool operator<=(const Variable& o) const { return v <= o; }
    bool operator< (const Variable& o) const { return v < o; }
    
    inline bool cond() const
    { return v.cond(); }
    
    void operator+= (const Variable& o) { v += o; }
    void operator-= (const Variable& o) { v -= o; }
    void operator*= (const Variable& o) { v *= o; }
    void operator/= (const Variable& o) { v /= o; }

    void operator%= (const Variable& o) { v %= o; }
    void operator<<=(const Variable& o) { v <<= o; }
    void operator>>=(const Variable& o) { v >>= o; }
    void operator&= (const Variable& o) { v &= o; }
    void operator|= (const Variable& o) { v |= o; }
    void operator^= (const Variable& o) { v ^= o; }
    void idiv(const Variable& o) { v.idiv(o); }
    void invert() { v.invert(); };
    
    const char* type_string() const { return v.type_string(); }
    inline bool is_numeric() const { return v.is_numeric(); }
    inline bool is_integral() const { return v.is_integral(); }
    inline bool is_string() const { return v.is_string(); }
    inline bool is_array() const { return v.is_array(); }
    inline bool is_undefined() const { return v.is_undefined(); }
    inline bool is_pointer() const { return v.is_pointer(); }
    
    #ifdef OGM_STRUCT_SUPPORT
    inline bool is_struct() const { return v.is_struct(); }
    #endif
    
    #ifdef OGM_FUNCTION_SUPPORT
    inline bool is_function() const { return v.is_function(); }
    #endif
    
    template<typename T>
    inline T& get() { return v.get<T>(); }
    
    template<typename T>
    inline const T& get() const { return v.get<T>(); }
    
    template<typename T>
    inline T castExact() const { return v.castExact<T>(); }
    
    template<typename T>
    inline T castCoerce() const { return v.castCoerce<T>(); }
    
    template<typename T>
    const T& coerce() { return v.coerce<T>(); }
    
    std::string_view string_view() const { return v.string_view(); }
    
    size_t string_length() const { return v.string_length(); }
    
    inline void shrink_string_to_range(size_t begin, size_t end)
    {
        return v.shrink_string_to_range(begin, end);
    }
    
    inline void shrink_string_to_range(size_t begin)
    {
        v.shrink_string_to_range(begin);
    }
    
    void gc_integrity_check() const
    {
        v.gc_integrity_check();
    }
    
    inline SafeVariable& copy(const Variable& o)
    {
        v.cleanup();
        v.set(o);
        v.make_root();
        return *this;
    }
    
    inline void array_ensure(bool generate=false)
    {
        v.array_ensure(generate);
        v.make_root();
    }
    
    inline const Variable& array_at(size_t i
        #ifdef OGM_2DARRAY
            , size_t j
        #endif
    ) { return v.array_at(i, j); }
    
    inline Variable& array_get(size_t i
        #ifdef OGM_2DARRAY
        , size_t j
        #endif
        , bool copy=true)
    { return v.array_get(i, j, copy); }
    
    inline size_t array_height() const
    { return v.array_height(); }
    
    const VariableArrayData& getReadableArray() const
    {
        return v.getReadableArray();
    }
    
    #ifdef OGM_STRUCT_SUPPORT
    void make_struct()
    {
        v.cleanup();
        v.make_struct();
        v.make_root();
    }
    
    Instance* get_struct()
    { return v.get_struct(); }
    
    void set_from_struct(
        VariableStructData* data
    )
    {
        v.cleanup();
        v.set_from_struct(data);
        v.make_root();
    }
    #endif
    
    ~SafeVariable()
    {
        v.cleanup();
    }
}
}