#ifndef WRAPIDJSON_VALUE_REF_H
#define WRAPIDJSON_VALUE_REF_H

#include <limits>

#include <rapidjson/document.h>

#include "string_view.hpp"
#include "optional.hpp"

#include "type_traits.h"

namespace wrapidjson {

template<typename T>
using optional = nonstd::optional<T>;
using string_view = nonstd::string_view;

/////////////////////////////////////////////////////////////////////////////////////////////
/// Wrapper rapidjson::GenericIterators.
/////////////////////////////////////////////////////////////////////////////////////////////
template <typename IteratorType, typename ReferenceType, typename AllocatorType>
class Iterator : public std::iterator<std::forward_iterator_tag, ReferenceType, ReferenceType, const ReferenceType*, ReferenceType> {
    friend class ArrayRef;
    friend class ObjectRef;
public:
    Iterator(IteratorType ptr, AllocatorType& allocator)
        : ptr_(ptr), alloc_(&allocator) {}
    Iterator(const Iterator& rfs)
        : ptr_(rfs.ptr_), alloc_(rfs.alloc_) {}
    ~Iterator() = default;

    Iterator& operator=(const Iterator& other){ptr_ = other.ptr_; alloc_ = other.alloc_; return *this;}

    Iterator& operator++(){ ++ptr_; return *this; }                         // ++itr
    Iterator& operator--(){ --ptr_; return *this; }                         // --itr
    Iterator  operator++(int){ Iterator old(*this); ++ptr_; return old; }   // itr++
    Iterator  operator--(int){ Iterator old(*this); --ptr_; return old; }   // itr--

    Iterator operator+(int n) const { return Iterator(ptr_+n, *alloc_); }   // itr +
    Iterator operator-(int n) const { return Iterator(ptr_-n, *alloc_); }   // itr -

    Iterator& operator+=(int n) { ptr_+=n; return *this; }                  // itr +=
    Iterator& operator-=(int n) { ptr_-=n; return *this; }                  // itr -=

    bool operator==(const Iterator& rfs) const { return ptr_ == rfs.ptr_; }
    bool operator!=(const Iterator& rfs) const { return ptr_ != rfs.ptr_; }
    bool operator<=(const Iterator& rfs) const { return ptr_ <= rfs.ptr_; }
    bool operator>=(const Iterator& rfs) const { return ptr_ >= rfs.ptr_; }
    bool operator< (const Iterator& rfs) const { return ptr_ < rfs.ptr_; }
    bool operator> (const Iterator& rfs) const { return ptr_ > rfs.ptr_; }
    int  operator- (const Iterator& rfs) const { return ptr_ - rfs.ptr_; }

    ReferenceType operator*() const { return ReferenceType(*ptr_, *alloc_); }
    ReferenceType operator->() const { return ReferenceType(*ptr_, *alloc_); }
    ReferenceType operator[](size_t n) const { return ReferenceType(ptr_[n], *alloc_); }

private:
    IteratorType    ptr_;
    AllocatorType*  alloc_;
};

/////////////////////////////////////////////////////////////////////////////////////////////
/// Predefine class 
/////////////////////////////////////////////////////////////////////////////////////////////
class ValueRef;
class Document;

using ValueIterator = Iterator<rapidjson::Value::ValueIterator, ValueRef, rapidjson::Document::AllocatorType>;
class ArrayRef;

struct MemberRef;
using MemberIterator = Iterator<rapidjson::Value::MemberIterator, MemberRef, rapidjson::Document::AllocatorType>;
class ObjectRef;

/////////////////////////////////////////////////////////////////////////////////////////////
/// ValueRef for rapidjson::value
/////////////////////////////////////////////////////////////////////////////////////////////
class ValueRef {
    friend class ArrayRef;
    friend class ObjectRef;

public:
    /// constructors:
    ValueRef(rapidjson::Value&, rapidjson::Document::AllocatorType&);
    ValueRef(const ValueRef&);
    ValueRef(const ArrayRef&);
    ValueRef(const ObjectRef&);
    ValueRef(const Document&);

    /// destructor
    virtual ~ValueRef() = default;

    /// copy assignment
    ValueRef& operator=(const ValueRef&);
    ValueRef& operator=(const Document&);

    template<typename T>
    ValueRef& operator=(T value) {
        value_ = value;
        return *this;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Against long != int64_t
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename T = long>
    ValueRef& operator=(long value) {
        value_ = static_cast<int64_t>(value);
        return *this;
    }

    template<typename T = unsigned long>
    ValueRef& operator=(unsigned long value) {
        value_ = static_cast<uint64_t>(value);
        return *this;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Against long long != int64_t
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename T = long long>
    ValueRef& operator=(long long value) {
        value_ = static_cast<int64_t>(value);
        return *this;
    }

    template<typename T = unsigned long long>
    ValueRef& operator=(unsigned long long value) {
        value_ = static_cast<uint64_t>(value);
        return *this;
    }

    /// catches string literals (without copying):
    template<int N>
    ValueRef& operator=(const char(&s)[N]) {
        value_.SetString(s, N-1); // we don't want to store the \0 character
        return *this;
    }

    /// catches std::string (makes a copy):
    ValueRef& operator=(const std::string& s);
    ValueRef& operator=(const char* s);

    /// catches string_view (without copy)
    ValueRef& operator=(const string_view& s);

    /// assign from Array reference
    ValueRef& operator=(const ArrayRef& array);

    /// assign from Object reference
    ValueRef& operator=(const ObjectRef& object);

    /// assign from all continaer ( array, map )
    template<template <typename...> class Container, typename ...Args>
    ValueRef& operator=(const Container<Args...>& container) {
        set_container(container);
        return *this;
    }

    /// set Container ( Container<Number> )
    template<typename T, template <typename...> class Container, typename...Args,
        detail::enable_if_sequence_t<T, Container, Args...>* = nullptr
    >
    void set_container(const Container<T, Args...>& array, bool str_copy = true);

    /// set Container ( Continaer<String> )
    template<template <typename...> class Container, typename...Args,
        detail::enable_if_sequence_t<std::string, Container, Args...>* = nullptr
    >
    void set_container(const Container<std::string, Args...>& array, bool str_copy = true);

    /// assign from map<string, Number>
    template<typename T, template <typename...> class Container, typename...Args,
        detail::enable_if_strmap_t<T, Container, Args...>* = nullptr
    >
    void set_container(const Container<std::string, T, Args...>& map, bool str_copy = true);

    /// assign from map<string, string>
    template<template <typename...> class Container,
        detail::enable_if_strmap_t<std::string, Container>* = nullptr
    >
    void set_container(const Container<std::string, std::string>& map, bool str_copy = true);

    /// set to Null
    ValueRef& set_null() {
        value_.SetNull();
        return *this;
    }

    /// set to empty Array
    ArrayRef set_array();

    /// set to Array
    void push_back(const ValueRef& value);

    /// get array
    ValueRef operator[](size_t idx) const;

    /// set to empty Object
    ObjectRef set_object();

    /// set to Object
    ValueRef operator[](const char* name) const;

    /// set to Object
    ValueRef operator[](const std::string& name) const;

    /// check member
    bool has(const std::string& name) const;

    /// find member
    optional<ValueRef> find(const std::string& name) const;

    /// get type info
    bool is_bool() const { return value_.IsBool(); }
    bool is_number() const { return value_.IsNumber(); }
    bool is_integral() const {
        return value_.IsInt() or value_.IsUint() or value_.IsInt64() or value_.IsUint64();
    }
    bool is_double() const { return value_.IsDouble(); }
    bool is_string() const { return value_.IsString(); }
    bool is_array() const { return value_.IsArray(); }
    bool is_object() const { return value_.IsObject(); }
    bool is_null() const { return value_.IsNull(); }

    /// test if two Values point to the same rapidjson::Value
    bool operator==(const ValueRef& other) { return (&value_ == &other.value_); }
    bool operator!=(const ValueRef& other) { return !(operator==(other)); }

    /// type = as<type>
    template<typename T, detail::enable_if_num_t<T>* = nullptr>
    T as() const;

    template<typename T, detail::enable_if_char_t<T>* = nullptr>
    char as() const;

    template<typename T, detail::enable_if_cptr_t<T>* = nullptr>
    const char* as() const;

    template<typename T, detail::enable_if_str_t<T>* = nullptr>
    std::string as() const;


    /// optional<type> = get<type>
    template<typename T, detail::enable_if_bool_t<T>* = nullptr>
    optional<bool> get() const;

    template<typename T, detail::enable_if_char_t<T>* = nullptr>
    optional<char> get() const;

    // long, long long
    template<typename T, detail::enable_if_long_t<T>* = nullptr>
    optional<T> get() const;

    // int8_t, short, int
    template<typename T, detail::enable_if_int_t<T>* = nullptr>
    optional<T> get() const;

    // unsigned long, unsigned long long
    template<typename T, detail::enable_if_ulong_t<T>* = nullptr>
    optional<T> get() const;

    // uint8_t, unsigned short, unsigned int
    template<typename T, detail::enable_if_uint_t<T>* = nullptr>
    optional<T> get() const;

    // float, double
    template<typename T, detail::enable_if_float_t<T>* = nullptr>
    optional<T> get() const;

    template<typename T, detail::enable_if_cptr_t<T>* = nullptr>
    optional<const char*> get() const;

    template<typename T, detail::enable_if_str_t<T>* = nullptr>
    optional<std::string> get() const;

    ValueRef get_ref() const;
    ArrayRef get_array() const;
    ObjectRef get_object() const;

    rapidjson::Value& get_rvalue() const;

    ValueRef* operator->() { return this; } // for iterator

    bool empty() const;

    size_t size() const;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // IF long != int64_t
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// set Container ( Container<long> )
    template<template <typename...> class Container, typename...Args, typename std::enable_if<
        !std::is_same<int64_t, long>::value &&
        detail::is_iterable<Container<long, Args...>>::value &&
        !detail::is_map<Container<long, Args...>>::value
        , Container<long, Args...>>::type* = nullptr
    >
    void set_container(const Container<long, Args...>& array, bool str_copy = true);

    /// set Container ( Container<unsigned long> )
    template<template <typename...> class Container, typename...Args, typename std::enable_if<
        !std::is_same<int64_t, long>::value &&
        detail::is_iterable<Container<unsigned long, Args...>>::value &&
        !detail::is_map<Container<unsigned long, Args...>>::value
        , Container<unsigned long, Args...>>::type* = nullptr
    >
    void set_container(const Container<unsigned long, Args...>& array, bool str_copy = true);
    /// assing from map<string, long> ( long != int64_t )
    template<template <typename...> class Container, typename...Args, typename std::enable_if<
        !std::is_same<int64_t, long>::value &&
        detail::is_map<Container<std::string, long, Args...>>::value
        , Container<std::string, long, Args...>>::type* = nullptr
    >
    void set_container(const Container<std::string, long, Args...>& map, bool str_copy = true);

    /// assing from map<string, unsinged long> ( unsigned long != uint64_t )
    template<template <typename...> class Container, typename...Args, typename std::enable_if<
        !std::is_same<int64_t, long>::value &&
        detail::is_map<Container<std::string, unsigned long, Args...>>::value
        , Container<std::string, unsigned long, Args...>>::type* = nullptr
    >
    void set_container(const Container<std::string, unsigned long, Args...>& map, bool str_copy = true);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // IF long long != int64_t
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// set Container ( Container<long long> )
    template<template <typename...> class Container, typename...Args, typename std::enable_if<
        !std::is_same<int64_t, long long>::value &&
        detail::is_iterable<Container<long long, Args...>>::value &&
        !detail::is_map<Container<long long, Args...>>::value
        , Container<long long, Args...>>::type* = nullptr
    >
    void set_container(const Container<long long, Args...>& array, bool str_copy = true);

    /// set Container ( Container<unsigned long long> )
    template<template <typename...> class Container, typename...Args, typename std::enable_if<
        !std::is_same<int64_t, long long>::value &&
        detail::is_iterable<Container<unsigned long long, Args...>>::value &&
        !detail::is_map<Container<unsigned long long, Args...>>::value
        , Container<unsigned long long, Args...>>::type* = nullptr
    >
    void set_container(const Container<unsigned long long, Args...>& array, bool str_copy = true);
    /// assing from map<string, long long> ( long long != int64_t )
    template<template <typename...> class Container, typename...Args, typename std::enable_if<
        !std::is_same<int64_t, long long>::value &&
        detail::is_map<Container<std::string, long long, Args...>>::value
        , Container<std::string, long long, Args...>>::type* = nullptr
    >
    void set_container(const Container<std::string, long long, Args...>& map, bool str_copy = true);

    /// assing from map<string, unsinged long long> ( unsigned long long != uint64_t )
    template<template <typename...> class Container, typename...Args, typename std::enable_if<
        !std::is_same<int64_t, long long>::value &&
        detail::is_map<Container<std::string, unsigned long long, Args...>>::value
        , Container<std::string, unsigned long long, Args...>>::type* = nullptr
    >
    void set_container(const Container<std::string, unsigned long long, Args...>& map, bool str_copy = true);

protected:
    rapidjson::Value&                   value_;
    rapidjson::Document::AllocatorType& alloc_;
};

/////////////////////////////////////////////////////////////////////////////////////////////
/// ArrayRef ( Reference Value for array )
/////////////////////////////////////////////////////////////////////////////////////////////
class ArrayRef {
    friend class ValueRef;

public:
    ArrayRef(const ArrayRef&);
    ArrayRef(const ValueRef& value);
    ArrayRef& operator=(const ArrayRef& other) = delete;

    ~ArrayRef() = default;

    template<typename T, template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_iterable<Container<T, Args...>>::value &&
        !detail::is_map<Container<T, Args...>>::value
        , Container<T, Args...>>::type* = nullptr
    >
    ArrayRef& operator=(const Container<T, Args...>& array);

    template<typename T, template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_iterable<Container<T, Args...>>::value &&
        !detail::is_map<Container<T, Args...>>::value
        , Container<T, Args...>>::type* = nullptr
    >
    void set_container(const Container<T, Args...>& array, bool str_copy = true);

    ValueRef operator[](size_t index) const;

    size_t size() const;
    bool empty() const;
    size_t capacity() const;
    void reserve(size_t n);
    void resize(size_t n);

    template <typename T>
    void resize(size_t n, T&& value);

    void clear();

    ValueIterator begin() const;
    ValueIterator end() const;
    ValueRef front() const;
    ValueRef back() const;

    template <typename T>
    optional<std::vector<T>> get_vector();

    template <typename T>
    std::vector<T> as_vector(std::function<bool(const T&)> func = [](const T&){return true;});

    template<typename T>
    void push_back(T&& value);
    ValueRef push_back();
    void pop_back();
    ValueIterator erase(const ValueIterator& pos);
    ValueIterator erase(const ValueIterator& first, const ValueIterator& last);

    ValueRef get_value_ref() const;

protected:
    ValueRef valueRef_;
};

/////////////////////////////////////////////////////////////////////////////////////////////
/// ObjectRef ( Reference rapidjson::value for Object )
/////////////////////////////////////////////////////////////////////////////////////////////
class ObjectRef {
    friend class ValueRef;

public:
    ObjectRef(const ValueRef&);
    ObjectRef(const ObjectRef&);

    ~ObjectRef() = default;

    ObjectRef& operator=(const ObjectRef& other) = delete;

    template<typename Value, template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_map<Container<std::string, Value, Args...>>::value
        , Container<std::string, Value, Args...>>::type* = nullptr
    >
    ObjectRef& operator=(const Container<std::string, Value, Args...>& map);

    /// set_container map<std::string, Value>
    template<typename Value, template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_map<Container<std::string, Value, Args...>>::value
        , Container<std::string, Value, Args...>>::type* = nullptr
    >
    void set_container(const Container<std::string, Value, Args...>& map, bool str_copy = true);

    /// get_value<bool>()
    template<typename T, typename std::enable_if<
        std::is_same<bool, T>::value, T>::type* = nullptr
    >
    optional<bool> get_value(const std::string& name) const;

    /// get_value<String>()
    template<typename T, typename std::enable_if<
        detail::is_string<T>::value, T>::type* = nullptr
    >
    optional<std::string> get_value(const std::string& name) const;

    /// get_value<const char>()
    template<typename T, typename std::enable_if<
        detail::is_const_char<T>::value, T>::type* = nullptr
    >
    optional<const char*> get_value(const std::string& name) const;

    /// get_value<Number>()
    template<typename T, typename std::enable_if<
        std::numeric_limits<T>::is_bounded && !std::is_same<bool, T>::value, T>::type* = nullptr
    >
    optional<T> get_value(const std::string& name) const;

    /// get_value<T>(default_value)
    template<typename T>
    T get_value(const std::string& name, const T& defval) const;

    ValueRef operator[](const std::string& name) const;
    ValueRef operator[](const char* name) const;
    ValueRef operator[](const string_view& name) const;

    optional<ValueRef> find(const std::string& name) const;

    template<template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_iterable<Container<std::string, Args...>>::value &&
        !detail::is_map<Container<std::string, Args...>>::value
        , Container<std::string, Args...> >::type* = nullptr
    >
    MemberIterator find_any(Container<std::string> names) const;

    template<template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_iterable<Container<std::string, Args...>>::value &&
        !detail::is_map<Container<std::string, Args...>>::value
        , Container<std::string, Args...> >::type* = nullptr
    >
    bool find_all(Container<std::string> names) const;

    int count(const std::string& name) const;
    size_t size() const;
    bool empty() const;
    bool has(const std::string& name) const;
    void clear();

    MemberIterator begin() const;
    MemberIterator end() const;

    template<typename T>
    void insert(const char* name, T&& value);

    template<typename T>
    void insert(const std::string& name, T&& value);

    template<typename T>
    void insert(const string_view& name, T&& value);

    ValueRef insert(const char* name);
    ValueRef insert(const std::string& name);
    ValueRef insert(const string_view& name);

    MemberIterator erase(const std::string& name);
    MemberIterator erase(const MemberIterator& pos);
    MemberIterator erase(const MemberIterator& first, const MemberIterator& last);

    ValueRef get_value_ref() const;
protected:
    ValueRef valueRef_;
};

} // namespace wrapidjson

#include "value_ref_impl.h"

#endif // WRAPIDJSON_VALUE_REF_H
