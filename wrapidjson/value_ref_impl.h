#include "format.h"
#include "parse.h"

namespace wrapidjson {

/////////////////////////////////////////////////////////////////////////////////////////////
/// ValueRef for rapidjson::value
/////////////////////////////////////////////////////////////////////////////////////////////
inline ValueRef::ValueRef(rapidjson::Value& value, rapidjson::Document::AllocatorType& alloc) 
    : value_(value), alloc_(alloc)
{}
inline ValueRef::ValueRef(const ValueRef& rfs) 
    : value_(rfs.value_), alloc_(rfs.alloc_)
{}
inline ValueRef::ValueRef(const ArrayRef& array)
    : value_(array.get_value_ref().value_), alloc_(array.get_value_ref().alloc_)
{}

inline ValueRef::ValueRef(const ObjectRef& obj)
    : value_(obj.get_value_ref().value_), alloc_(obj.get_value_ref().alloc_)
{}

/// copy assignment
inline ValueRef& ValueRef::operator=(const ValueRef& other) {
    if (this != &other) {
        value_.CopyFrom(other.value_, alloc_); // copy value explicitly
    }
    return *this;
}

/// catches std::string (makes a copy):
inline ValueRef& ValueRef::operator=(const std::string& s) {
    value_.SetString(s.data(), s.length(), alloc_); // make copy via allocator!
    return *this;
}

inline ValueRef& ValueRef::operator=(const char* s) {
    value_.SetString(s, alloc_); // make copy via allocator!
    return *this;
}

inline ValueRef& ValueRef::operator=(const string_view& s) {
    value_.SetString(s.data(), s.length());
    return *this;
}

/// assign from Array reference
inline ValueRef& ValueRef::operator=(const ArrayRef& array)
{
    return operator=(array.valueRef_);
}

inline ValueRef& ValueRef::operator=(const ObjectRef& object)
{
    return operator=(object.valueRef_);
}

/// set to empty Array
inline ArrayRef ValueRef::set_array() {
    value_.SetArray();
    return ArrayRef(*this);
}

/// set to empty Array
inline void ValueRef::push_back(const ValueRef& value) {
    if ( value_.IsNull()) {
        value_.SetArray();
    } else if ( not value_.IsArray() ) {
        throw std::runtime_error("ValueRef::push_back allow only ArrayType");
    }
    ArrayRef(*this).push_back(value);
}

/// set to empty Array
inline ValueRef ValueRef::operator[](size_t idx) const {
    if ( not value_.IsArray() ) {
        throw std::runtime_error(detail::format("ValueRef[%u] allow only ArrayType", idx));
    } else if (idx >= value_.Size() ) {
        throw std::runtime_error(detail::format("ValueRef[%u] out_of_range(%u)", idx, value_.Size()));
    }
    return ArrayRef(*this)[idx];
}

inline ObjectRef ValueRef::set_object() {
    value_.SetObject();
    return ObjectRef(*this);
}

/// set to Object
inline ValueRef ValueRef::operator[](const char* name) const {
    if ( value_.IsNull() ) {
        value_.SetObject();
    } else if (not value_.IsObject()) {
        throw std::runtime_error(detail::format("ValueRef[%s] allow ObjectType", name));
    }
    return ObjectRef(*this)[name];
}
/// set to Object
inline ValueRef ValueRef::operator[](const std::string& name) const {
    return this->operator[](name.c_str());
}
/// check member
inline bool ValueRef::has(const std::string& name) const {
    if (value_.IsObject()) {
        return ObjectRef(*this).has(name);
    }
    return false;
}
/// find member
inline optional<ValueRef> ValueRef::find(const std::string& name) const {
    optional<ValueRef> ret;
    if ( value_.IsObject() ) {
        ret = ObjectRef(*this).find(name);
    }
    return ret;
}

inline rapidjson::Value& ValueRef::get_rvalue() const {
    return value_;
}
inline ValueRef ValueRef::get_ref() const {
    return *this;
}
inline ArrayRef ValueRef::get_array() const {
    return ArrayRef(*this);
}
inline ObjectRef ValueRef::get_object() const {
    return ObjectRef(*this);
}

inline bool ValueRef::empty() const {
    if ( value_.IsObject() ) {
        return value_.ObjectEmpty();
    } else if ( value_.IsArray() ) {
        return value_.Empty();
    } else if ( value_.IsString() ) {
        return value_.GetStringLength() == 0;
    }
    return false;
}

size_t ValueRef::size() const {
    if ( value_.IsObject() ) {
        return value_.MemberCount();
    } else if ( value_.IsArray() ) {
        return value_.Size();
    } else if ( value_.IsString() ) {
        return value_.GetStringLength();
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/// ValueRef::as tempalte impl
/// type = as<type> 인터페이스
/////////////////////////////////////////////////////////////////////////////////////////////
template<typename T, typename std::enable_if<std::is_same<T, char>::value, T>::type*>
inline char ValueRef::as() const {
    if (value_.IsNumber()) {
        if (value_.IsInt()) {
            return static_cast<char>(value_.GetInt());
        } else if (value_.IsUint()) {
            return static_cast<char>(value_.GetUint());
        } else if (value_.IsInt64()) {
            return static_cast<char>(value_.GetInt64());
        } else if (value_.IsUint64()) {
            return static_cast<char>(value_.GetUint64());
        } else if (value_.IsDouble()) {
            return static_cast<char>(value_.GetDouble());
        }
    } else if (value_.IsString() and value_.GetStringLength() > 0) {
        return value_.GetString()[0];
    }
    return ' ';
}

template<typename T, typename std::enable_if<std::is_arithmetic<T>::value && !std::is_same<T, char>::value, T>::type*>
T ValueRef::as() const {
    if (value_.IsNumber()) {
        if (value_.IsInt()) {
            return static_cast<T>(value_.GetInt());
        } else if (value_.IsUint()) {
            return static_cast<T>(value_.GetUint());
        } else if (value_.IsInt64()) {
            return static_cast<T>(value_.GetInt64());
        } else if (value_.IsUint64()) {
            return static_cast<T>(value_.GetUint64());
        } else if (value_.IsDouble()) {
            return static_cast<T>(value_.GetDouble());
        }
    } else if (value_.IsBool()) {
        return static_cast<T>(value_.GetBool());
    } else if (value_.IsString()) {
        return detail::parse<T>(value_.GetString(),0);
    }
    return 0;
}

template<typename T, typename std::enable_if<detail::is_const_char<T>::value, T>::type*>
inline const char* ValueRef::as() const {
    if (value_.IsString()) {
        return value_.GetString();
    }
    return nullptr;
}

template<typename T, typename std::enable_if<detail::is_string<T>::value, T>::type*>
inline std::string ValueRef::as() const {
    if (value_.IsNumber()) {
        if (value_.IsInt()) {
            return std::to_string(value_.GetInt());
        } else if (value_.IsUint()) {
            return std::to_string(value_.GetUint());
        } else if (value_.IsInt64()) {
            return std::to_string(value_.GetInt64());
        } else if (value_.IsUint64()) {
            return std::to_string(value_.GetUint64());
        } else if (value_.IsDouble()) {
            return std::to_string(value_.GetDouble());
        }
    } else if (value_.IsBool()) {
        return (value_.GetBool() ? "true" : "false");
    } else if (value_.IsString()) {
        return value_.GetString();
    }
    return "";
}

/////////////////////////////////////////////////////////////////////////////////////////////
/// ValueRef::get tempalte impl
/// optional<type> = get<type> 인터페이스
/////////////////////////////////////////////////////////////////////////////////////////////
template<typename T, typename std::enable_if<
    std::is_integral<T>::value && std::is_same<T, bool>::value, T>::type*>
inline optional<bool> ValueRef::get() const {
    optional<bool> res;
    if ( value_.IsBool() ) {
        res = value_.GetBool();
    }
    return res;
}

template<typename T, typename std::enable_if<
    std::is_integral<T>::value && std::is_same<T, char>::value, T>::type*>
inline optional<char> ValueRef::get() const {
    optional<char> res;
    if (value_.IsString() && value_.GetStringLength() == 1) {
        res = value_.GetString()[0];
    }
    return res;
}

template<typename T, typename std::enable_if<
    std::is_integral<T>::value && std::is_signed<T>::value && sizeof(T) == 8, T>::type*>
inline optional<T> ValueRef::get() const {
    optional<T> res;
    if ( value_.IsInt64() ) {
        res = static_cast<T>(value_.GetInt64());
    }
    return res;
}

template<typename T, typename std::enable_if<
    std::is_integral<T>::value && !std::is_same<T, char>::value && std::is_signed<T>::value && sizeof(T) < 8, T>::type*>
inline optional<T> ValueRef::get() const {
    optional<T> res;
    if ( value_.IsInt() and value_.GetInt() >= std::numeric_limits<T>::min() and value_.GetInt() <= std::numeric_limits<T>::max() ) {
        res = static_cast<T>(value_.GetInt());
    }
    return res;
}

template<typename T, typename std::enable_if<
    std::is_integral<T>::value && !std::is_same<T, bool>::value && std::is_unsigned<T>::value && sizeof(T) == 8, T>::type*>
inline optional<T> ValueRef::get() const {
    optional<T> res;
    if ( value_.IsUint64() ) {
        res = static_cast<T>(value_.GetUint64());
    }
    return res;
}

template<typename T, typename std::enable_if<
    std::is_integral<T>::value && !std::is_same<T, bool>::value && std::is_unsigned<T>::value && sizeof(T) < 8, T>::type*>
inline optional<T> ValueRef::get() const {
    optional<T> res;
    if ( value_.IsUint() and value_.GetUint() >= std::numeric_limits<T>::min() and value_.GetUint() <= std::numeric_limits<T>::max() ) {
        res = static_cast<T>(value_.GetUint());
    }
    return res;
}

template<typename T, typename std::enable_if<std::is_floating_point<T>::value, T>::type*>
inline optional<T> ValueRef::get() const {
    optional<T> res;
    if (value_.IsNumber()) {
        res = static_cast<T>(value_.GetDouble());
    }
    return res;
}

template<typename T, typename std::enable_if<detail::is_const_char<T>::value, T>::type*>
inline optional<T> ValueRef::get() const {
    optional<const char*> res;
    if (value_.IsString()) {
        res = value_.GetString();
    }
    return res;
}

template<typename T, typename std::enable_if<detail::is_string<T>::value, T>::type*>
inline optional<T> ValueRef::get() const {
    optional<std::string> res;
    if (value_.IsString()) {
        res = std::string(value_.GetString(), value_.GetStringLength());
    }
    return res;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/// ValueRef::set_container tempalte impl
/////////////////////////////////////////////////////////////////////////////////////////////

/// set Container ( Container<Number> )
template<typename T, template <typename...> class Container, typename...Args, typename std::enable_if<
    detail::is_iterable<Container<T, Args...>>::value &&
    !detail::is_map<Container<T, Args...>>::value
    , Container<T, Args...>>::type*
>
inline void ValueRef::set_container(const Container<T, Args...>& array, bool str_copy)
{
    (void)str_copy;
    value_.SetArray();
    value_.Reserve(array.size(), alloc_);
    for (auto& k : array) {
        value_.PushBack(rapidjson::Value(k), alloc_);
    }
}


/// set Container ( Continaer<String> )
template<template <typename...> class Container, typename...Args, typename std::enable_if<
    detail::is_iterable<Container<std::string, Args...>>::value &&
    !detail::is_map<Container<std::string, Args...>>::value
    , Container<std::string, Args...> >::type*
>
inline void ValueRef::set_container(const Container<std::string, Args...>& array, bool str_copy)
{
    value_.SetArray();
    value_.Reserve(array.size(), alloc_);
    for (auto& k : array) {
        if ( str_copy ) {
            value_.PushBack(rapidjson::Value(k.data(), k.length(), alloc_), alloc_);    // string copy
        } else {
            value_.PushBack(rapidjson::Value(k.data(), k.length()), alloc_);            // string not copy
        }
    }
}

/// assing from map<string, Value>
template<typename Value, template <typename...> class Container, typename...Args, typename std::enable_if<
    detail::is_map<Container<std::string, Value, Args...>>::value
    , Container<std::string, Value, Args...>>::type*
>
inline void ValueRef::set_container(const Container<std::string, Value, Args...>& map, bool str_copy)
{
    value_.SetObject();
    for (auto& k : map){
        rapidjson::Value name;
        if ( str_copy ) {
            name.SetString(k.first.data(), k.first.length(), alloc_);
        } else {
            name.SetString(k.first.data(), k.first.length());
        }
        rapidjson::Value value(k.second);
        value_.AddMember(name.Move(), value.Move(), alloc_);
    }
}

/// assign from map<string, string>
template<template <typename...> class Container, typename std::enable_if<
    detail::is_map<Container<std::string, std::string>>::value
    , Container<std::string, std::string>>::type*
>
inline void ValueRef::set_container(const Container<std::string, std::string>& map, bool str_copy)
{
    value_.SetObject();
    for (auto& k : map){
        rapidjson::Value name, value;
        if ( str_copy ) {
            name.SetString(k.first.data(), k.first.length(), alloc_);
            value.SetString(k.second.data(), k.second.length(), alloc_);
        } else {
            name.SetString(k.first.data(), k.first.length());
            value.SetString(k.second.data(), k.second.length());
        }
        value_.AddMember(name.Move(), value.Move(), alloc_);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// long != rapidjson::SizeType(int64_t)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// set Container ( Container<long> )
template<template <typename...> class Container, typename...Args, typename std::enable_if<
    !std::is_same<int64_t, long>::value &&
    detail::is_iterable<Container<long, Args...>>::value &&
    !detail::is_map<Container<long, Args...>>::value
    , Container<long, Args...>>::type*
>
inline void ValueRef::set_container(const Container<long, Args...>& array, bool str_copy)
{
    (void)str_copy;
    value_.SetArray();
    value_.Reserve(array.size(), alloc_);
    for (auto& k : array) {
        value_.PushBack(rapidjson::Value(static_cast<int64_t>(k)), alloc_);
    }
}

/// set Container ( Container<unsigned long> )
template<template <typename...> class Container, typename...Args, typename std::enable_if<
    !std::is_same<uint64_t, unsigned long>::value &&
    detail::is_iterable<Container<unsigned long, Args...>>::value &&
    !detail::is_map<Container<unsigned long, Args...>>::value
    , Container<unsigned long, Args...>>::type*
>
inline void ValueRef::set_container(const Container<unsigned long, Args...>& array, bool str_copy)
{
    (void)str_copy;
    value_.SetArray();
    value_.Reserve(array.size(), alloc_);
    for (auto& k : array) {
        value_.PushBack(rapidjson::Value(static_cast<uint64_t>(k)), alloc_);
    }
}

/// assign from map<string, long>
template<template <typename...> class Container, typename...Args, typename std::enable_if<
    !std::is_same<int64_t, long>::value &&
    detail::is_map<Container<std::string, long, Args...>>::value
    , Container<std::string, long, Args...>>::type*
>
inline void ValueRef::set_container(const Container<std::string, long, Args...>& map, bool str_copy)
{
    value_.SetObject();
    for (auto& k : map){
        rapidjson::Value name;
        if ( str_copy ) {
            name.SetString(k.first.data(), k.first.length(), alloc_);
        } else {
            name.SetString(k.first.data(), k.first.length());
        }
        rapidjson::Value value(static_cast<int64_t>(k.second));
        value_.AddMember(name.Move(), value.Move(), alloc_);
    }
}

/// assign from map<string, unsigned long>
template<template <typename...> class Container, typename...Args, typename std::enable_if<
    !std::is_same<uint64_t, unsigned long>::value &&
    detail::is_map<Container<std::string, unsigned long, Args...>>::value
    , Container<std::string, unsigned long, Args...>>::type*
>
inline void ValueRef::set_container(const Container<std::string, unsigned long, Args...>& map, bool str_copy)
{
    value_.SetObject();
    for (auto& k : map){
        rapidjson::Value name;
        if ( str_copy ) {
            name.SetString(k.first.data(), k.first.length(), alloc_);
        } else {
            name.SetString(k.first.data(), k.first.length());
        }
        rapidjson::Value value(static_cast<uint64_t>(k.second));
        value_.AddMember(name.Move(), value.Move(), alloc_);
    }
}

/*
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// long long != rapidjson::SizeType(int64_t)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// set Container ( Container<long long> )
template<template <typename...> class Container, typename...Args, typename std::enable_if<
    !std::is_same<int64_t, long long>::value &&
    detail::is_iterable<Container<long long, Args...>>::value &&
    !detail::is_map<Container<long long, Args...>>::value
    , Container<long long, Args...>>::type*
>
inline void ValueRef::set_container(const Container<long long, Args...>& array, bool str_copy)
{
    (void)str_copy;
    value_.SetArray();
    value_.Reserve(array.size(), alloc_);
    for (auto& k : array) {
        value_.PushBack(rapidjson::Value(static_cast<int64_t>(k)), alloc_);
    }
}

/// set Container ( Container<unsigned long long> )
template<template <typename...> class Container, typename...Args, typename std::enable_if<
    !std::is_same<uint64_t, unsigned long long>::value &&
    detail::is_iterable<Container<unsigned long long, Args...>>::value &&
    !detail::is_map<Container<unsigned long long, Args...>>::value
    , Container<unsigned long long, Args...>>::type*
>
inline void ValueRef::set_container(const Container<unsigned long long, Args...>& array, bool str_copy)
{
    (void)str_copy;
    value_.SetArray();
    value_.Reserve(array.size(), alloc_);
    for (auto& k : array) {
        value_.PushBack(rapidjson::Value(static_cast<uint64_t>(k)), alloc_);
    }
}

/// assign from map<string, long long>
template<template <typename...> class Container, typename...Args, typename std::enable_if<
    !std::is_same<int64_t, long long>::value &&
    detail::is_map<Container<std::string, long long, Args...>>::value
    , Container<std::string, long long, Args...>>::type*
>
inline void ValueRef::set_container(const Container<std::string, long long, Args...>& map, bool str_copy)
{
    value_.SetObject();
    for (auto& k : map){
        rapidjson::Value name;
        if ( str_copy ) {
            name.SetString(k.first.data(), k.first.length(), alloc_);
        } else {
            name.SetString(k.first.data(), k.first.length());
        }
        rapidjson::Value value(static_cast<int64_t>(k.second));
        value_.AddMember(name.Move(), value.Move(), alloc_);
    }
}

/// assign from map<string, unsigned long long>
template<template <typename...> class Container, typename...Args, typename std::enable_if<
    !std::is_same<uint64_t, unsigned long long>::value &&
    detail::is_map<Container<std::string, unsigned long long, Args...>>::value
    , Container<std::string, unsigned long long, Args...>>::type*
>
inline void ValueRef::set_container(const Container<std::string, unsigned long long, Args...>& map, bool str_copy)
{
    value_.SetObject();
    for (auto& k : map){
        rapidjson::Value name;
        if ( str_copy ) {
            name.SetString(k.first.data(), k.first.length(), alloc_);
        } else {
            name.SetString(k.first.data(), k.first.length());
        }
        rapidjson::Value value(static_cast<uint64_t>(k.second));
        value_.AddMember(name.Move(), value.Move(), alloc_);
    }
}
*/

/////////////////////////////////////////////////////////////////////////////////////////////
/// Member Reference for ObjectIterator
/////////////////////////////////////////////////////////////////////////////////////////////
struct MemberRef {
    MemberRef(rapidjson::Value::Member& ref, rapidjson::Document::AllocatorType& allocator)
        : name(ref.name, allocator), value(ref.value, allocator) {}
    MemberRef(const MemberRef& rfs)
        : name(rfs.name), value(rfs.value) {}
    ~MemberRef() {}

    MemberRef& operator=(const MemberRef& other){
        name = other.name; value = other.value; // copy name and value from other to this
        return *this;
    }
    MemberRef* operator->() { return this; } // needed by MemberIterator

    ValueRef name;
    ValueRef value;
};

/////////////////////////////////////////////////////////////////////////////////////////////
/// ArrayRef ( Reference Value for array )
/////////////////////////////////////////////////////////////////////////////////////////////
inline ArrayRef::ArrayRef(const ArrayRef& rfs)
    : valueRef_(rfs.valueRef_) {}

inline ArrayRef::ArrayRef(const ValueRef& value)
    : valueRef_(value)
{
    if ( valueRef_.value_.IsNull() ) {
        valueRef_.value_.SetArray();
    } else if ( not valueRef_.value_.IsArray() ) {
        throw std::runtime_error("Value is not arrayType, ArrayRef must derived by arrayType");
    }
}

template<typename T, template <typename...> class Container, typename...Args, typename std::enable_if<
    detail::is_iterable<Container<T, Args...>>::value &&
    !detail::is_map<Container<T, Args...>>::value
    , Container<T, Args...>>::type*
>
inline ArrayRef& ArrayRef::operator=(const Container<T, Args...>& array) {
    valueRef_.set_container(array);
    return *this;
}

template<typename T, template <typename...> class Container, typename...Args, typename std::enable_if<
    detail::is_iterable<Container<T, Args...>>::value &&
    !detail::is_map<Container<T, Args...>>::value
    , Container<T, Args...>>::type*
>
inline void ArrayRef::set_container(const Container<T, Args...>& array, bool str_copy) {
    valueRef_.set_container(array, str_copy);
}

inline ValueRef ArrayRef::operator[](size_t index) const {
    if ( index >= valueRef_.value_.Size() ) {
        throw std::runtime_error("Array index out_of_range");
    }
    return ValueRef(valueRef_.value_[index], valueRef_.alloc_);
}

inline size_t ArrayRef::size() const {
    return valueRef_.value_.Size();
}

inline bool ArrayRef::empty() const {
    return valueRef_.value_.Empty();
}

inline size_t ArrayRef::capacity() const {
    return valueRef_.value_.Capacity();
}

inline void ArrayRef::reserve(size_t n) {
    valueRef_.value_.Reserve(n, valueRef_.alloc_);
}

inline void ArrayRef::resize(size_t n) {
    int diff = n - size();
    reserve(n);
    if (diff > 0) {
        for (int i = 0; i < diff; ++i) {
            valueRef_.value_.PushBack(rapidjson::Value(), valueRef_.alloc_);
        }
    } else if (diff < 0) {
        diff *= -1;
        for (int i = 0; i < diff; ++i) {
            valueRef_.value_.PopBack();
        }
    }
}

template <typename T>
inline void ArrayRef::resize(size_t n, T&& value) {
    int diff = n - size();
    reserve(n);
    if (diff > 0) {
        for (int i = 0; i < diff; ++i) {
            rapidjson::Value temp;
            ValueRef dummy(temp, valueRef_.alloc_);
            dummy = std::forward<T>(value);
            valueRef_.value_.PushBack(temp.Move(), valueRef_.alloc_);
        }
    } else if (diff < 0) {
        diff *= -1;
        for (int i = 0; i < diff; ++i) {
            valueRef_.value_.PopBack();
        }
    }
}

inline void ArrayRef::clear() {
    valueRef_.value_.Clear();
}

inline ValueIterator ArrayRef::begin() const {
    return ValueIterator(valueRef_.value_.Begin(), valueRef_.alloc_);
}

inline ValueIterator ArrayRef::end() const {
    return ValueIterator(valueRef_.value_.End(), valueRef_.alloc_);
}

inline ValueRef ArrayRef::front() const {
    if ( valueRef_.value_.Empty() ) {
        throw std::runtime_error("Empty Array front() is null");
    }
    return ValueRef(valueRef_.value_[0], valueRef_.alloc_);
}

inline ValueRef ArrayRef::back() const {
    if ( valueRef_.value_.Empty() ) {
        throw std::runtime_error("Empty Array back() is null");
    }
    return ValueRef(valueRef_.value_[size()-1], valueRef_.alloc_);
}

template <typename T>
inline optional<std::vector<T>> ArrayRef::get_vector()
{
    optional<std::vector<T>> result;
    std::vector<T> res;
    res.reserve(size());
    for (const auto& value : *this)
    {
        auto value_ = value.get<T>();
        if ( value_ ) {
            res.emplace_back(std::move(*value_));
        } else {
            return result;
        }
    }
    result = res;
    return result;
}

template <typename T>
inline std::vector<T> ArrayRef::as_vector(std::function<bool(const T&)> func)
{
    std::vector<T> result;
    result.reserve(size());
    for (const auto& value : *this)
    {
        T value_ = value.as<T>();
        if ( func(value_) ) {
            result.emplace_back(std::move(value_));
        }
    }
    return result;
}

template<typename T>
inline void ArrayRef::push_back(T&& value) {
    rapidjson::Value temp;
    ValueRef dummy(temp, valueRef_.alloc_);
    dummy = std::forward<T>(value);
    valueRef_.value_.PushBack(temp.Move(), valueRef_.alloc_);
}

inline ValueRef ArrayRef::push_back() {
    valueRef_.value_.PushBack(rapidjson::Value(), valueRef_.alloc_);
    return ValueRef(valueRef_.value_[size()-1], valueRef_.alloc_);
}

inline void ArrayRef::pop_back() {
    if ( not valueRef_.value_.Empty() ) {
        valueRef_.value_.PopBack();
    }
}

inline ValueIterator ArrayRef::erase(const ValueIterator& pos) {
    return ValueIterator(valueRef_.value_.Erase(pos.ptr_), valueRef_.alloc_);
}

inline ValueIterator ArrayRef::erase(const ValueIterator& first, const ValueIterator& last) {
    return ValueIterator(valueRef_.value_.Erase(first.ptr_, last.ptr_), valueRef_.alloc_);
}

inline ValueRef ArrayRef::get_value_ref() const {
    return valueRef_;
}

inline ObjectRef::ObjectRef(const ValueRef& value)
    : valueRef_(value)
{
    static const size_t STRING_MAX_SIZE = 15;
    if ( valueRef_.value_.IsNull() ) {
        valueRef_.value_.SetObject();
    } else if ( not valueRef_.value_.IsObject() ) {
        throw std::runtime_error("Value is not ObjectType, ObjectRef must derived by ObjectType");
    }
}

inline ObjectRef::ObjectRef(const ObjectRef& rfs)
    : valueRef_(rfs.valueRef_)
{}

template<typename Value, template <typename...> class Container, typename...Args, typename std::enable_if<
    detail::is_map<Container<std::string, Value, Args...>>::value
    , Container<std::string, Value, Args...>>::type*
>
inline ObjectRef& ObjectRef::operator=(const Container<std::string, Value, Args...>& map) {
    valueRef_.set_container(map);
    return *this;
}

/// set_container map<std::string, Value>
template<typename Value, template <typename...> class Container, typename...Args, typename std::enable_if<
    detail::is_map<Container<std::string, Value, Args...>>::value
    , Container<std::string, Value, Args...>>::type*
>
inline void ObjectRef::set_container(const Container<std::string, Value, Args...>& map, bool str_copy) {
    valueRef_.set_container(map, str_copy);
}

inline ValueRef ObjectRef::operator[](const std::string& name) const {
    rapidjson::Value key(name.data(), name.length());
    auto it = valueRef_.value_.FindMember(key);
    if (it == valueRef_.value_.MemberEnd()){
        valueRef_.value_.AddMember(rapidjson::Value(name.data(), name.length(), valueRef_.alloc_), rapidjson::Value(), valueRef_.alloc_);
        it = valueRef_.value_.MemberEnd()-1;
    }
    return ValueRef(it->value, valueRef_.alloc_);
}

inline ValueRef ObjectRef::operator[](const char* name) const {
    rapidjson::Value key(rapidjson::StringRef(name));
    auto it = valueRef_.value_.FindMember(key);
    if (it == valueRef_.value_.MemberEnd()) {
        valueRef_.value_.AddMember(rapidjson::Value(rapidjson::StringRef(name), valueRef_.alloc_), rapidjson::Value(), valueRef_.alloc_);
        it = valueRef_.value_.MemberEnd()-1;
    }
    return ValueRef(it->value, valueRef_.alloc_);
}

inline ValueRef ObjectRef::operator[](const string_view& name) const {
    rapidjson::Value key(rapidjson::StringRef(name.data(), name.length()));
    auto it = valueRef_.value_.FindMember(key);
    if (it == valueRef_.value_.MemberEnd()) {
        valueRef_.value_.AddMember(rapidjson::Value(rapidjson::StringRef(name.data(), name.length())), rapidjson::Value(), valueRef_.alloc_);
        it = valueRef_.value_.MemberEnd()-1;
    }
    return ValueRef(it->value, valueRef_.alloc_);
}

inline optional<ValueRef> ObjectRef::find(const std::string& name) const {
    optional<ValueRef> ret;
    rapidjson::Value key(name.data(), name.length());
    auto it = valueRef_.value_.FindMember(key);
    if ( it != valueRef_.value_.MemberEnd() ) {
        ret = ValueRef(it->value, valueRef_.alloc_);
    }
    return ret;
}

inline int ObjectRef::count(const std::string& name) const {
    rapidjson::Value key(name.data(), name.length());
    auto it = valueRef_.value_.FindMember(key);
    return (it != valueRef_.value_.MemberEnd());
}

inline size_t ObjectRef::size() const {
    return valueRef_.value_.MemberCount();
}

inline bool ObjectRef::empty() const {
    return valueRef_.value_.ObjectEmpty();
}

inline bool ObjectRef::has(const std::string& name) const {
    return valueRef_.value_.HasMember(name.c_str());
}

inline void ObjectRef::clear() {
    return valueRef_.value_.RemoveAllMembers();
}

inline MemberIterator ObjectRef::begin() const {
    return MemberIterator(valueRef_.value_.MemberBegin(), valueRef_.alloc_);
}

inline MemberIterator ObjectRef::end() const {
    return MemberIterator(valueRef_.value_.MemberEnd(), valueRef_.alloc_);
}

template<typename T>
inline void ObjectRef::insert(const char* name, T&& value) {          // key copy
    rapidjson::Value temp;
    ValueRef dummy(temp, valueRef_.alloc_);
    dummy = std::forward<T>(value);
    valueRef_.value_.AddMember(rapidjson::Value(name, strlen(name), valueRef_.alloc_), temp.Move(), valueRef_.alloc_);
}

template<typename T>
inline void ObjectRef::insert(const std::string& name, T&& value) {
    rapidjson::Value temp;
    ValueRef dummy(temp, valueRef_.alloc_);
    dummy = std::forward<T>(value);
    valueRef_.value_.AddMember(rapidjson::Value(name.data(), name.length(), valueRef_.alloc_), temp.Move(), valueRef_.alloc_);
}

template<typename T>
inline void ObjectRef::insert(const string_view& name, T&& value) {
    rapidjson::Value temp;
    ValueRef dummy(temp, valueRef_.alloc_);
    dummy = std::forward<T>(value);
    valueRef_.value_.AddMember(rapidjson::Value(name.data(), name.length()), temp.Move(), valueRef_.alloc_);
}

inline ValueRef ObjectRef::insert(const char* name) {
    valueRef_.value_.AddMember(rapidjson::Value(name, strlen(name), valueRef_.alloc_), rapidjson::Value(), valueRef_.alloc_);
    auto it = (valueRef_.value_.MemberEnd() - 1);
    return ValueRef(it->value, valueRef_.alloc_);
}

inline ValueRef ObjectRef::insert(const std::string& name) {
    valueRef_.value_.AddMember(rapidjson::Value(name.data(), name.length(), valueRef_.alloc_), rapidjson::Value(), valueRef_.alloc_);
    auto it = (valueRef_.value_.MemberEnd() - 1);
    return ValueRef(it->value, valueRef_.alloc_);
}

inline ValueRef ObjectRef::insert(const string_view& name) {
    valueRef_.value_.AddMember(rapidjson::Value(name.data(), name.length()), rapidjson::Value(), valueRef_.alloc_);
    auto it = (valueRef_.value_.MemberEnd() - 1);
    return ValueRef(it->value, valueRef_.alloc_);
}

inline MemberIterator ObjectRef::erase(const std::string& name)  {
    auto it = valueRef_.value_.FindMember(rapidjson::Value(name.data(), name.length()));
    if (it != valueRef_.value_.MemberEnd()) {
        return MemberIterator(valueRef_.value_.EraseMember(it), valueRef_.alloc_);
    } else {
        return MemberIterator(it, valueRef_.alloc_);
    }
}

inline MemberIterator ObjectRef::erase(const MemberIterator& pos) {
    return MemberIterator(valueRef_.value_.EraseMember(pos.ptr_), valueRef_.alloc_);
}

inline MemberIterator ObjectRef::erase(const MemberIterator& first, const MemberIterator& last) {
    return MemberIterator(valueRef_.value_.EraseMember(first.ptr_, last.ptr_), valueRef_.alloc_);
}

inline ValueRef ObjectRef::get_value_ref() const {
    return valueRef_;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/// ObjectRef::get tempalte impl
/////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline T ObjectRef::get_value(const std::string& name, const T& defval) const {
    optional<T> value = get_value<T>(name);
    if ( not value ) {
        value = defval;
    }
    return *value;
}

template<typename T, typename std::enable_if<
    std::is_same<bool, T>::value, T>::type*
>
inline optional<bool> ObjectRef::get_value(const std::string& name) const {
    rapidjson::Value key(name.data(), name.length());
    auto it = valueRef_.value_.FindMember(key);
    if ( it != valueRef_.value_.MemberEnd() )  {
        return ValueRef(it->value, valueRef_.alloc_).get<bool>();
    }
    return optional<bool>();
}

template<typename T, typename std::enable_if<
    detail::is_string<T>::value, T>::type*
>
inline optional<std::string> ObjectRef::get_value(const std::string& name) const {
    optional<std::string> ret;
    rapidjson::Value key(name.data(), name.length());
    auto it = valueRef_.value_.FindMember(key);
    if ( it != valueRef_.value_.MemberEnd() and it->value.IsString()) {
        ret = std::string(it->value.GetString(), it->value.GetStringLength());
    }
    return ret;
}

template<typename T, typename std::enable_if<
    detail::is_const_char<T>::value, T>::type*
>
inline optional<const char*> ObjectRef::get_value(const std::string& name) const {
    optional<const char*> ret;
    rapidjson::Value key(name.data(), name.length());
    auto it = valueRef_.value_.FindMember(key);
    if ( it != valueRef_.value_.MemberEnd() and it->value.IsString()) {
        ret = it->value.GetString();
    }
    return ret;
}

template<typename T, typename std::enable_if<
    std::numeric_limits<T>::is_bounded && !std::is_same<bool, T>::value, T>::type*
>
inline optional<T> ObjectRef::get_value(const std::string& name) const {
    rapidjson::Value key(name.data(), name.length());
    auto it = valueRef_.value_.FindMember(key);
    if ( it != valueRef_.value_.MemberEnd() ) {
        return ValueRef(it->value, valueRef_.alloc_).get<T>();
    }
    return optional<T>();
}


/////////////////////////////////////////////////////////////////////////////////////////////
/// ValueRef::find_xxx tempalte impl
/////////////////////////////////////////////////////////////////////////////////////////////
template<template <typename...> class Container, typename...Args, typename std::enable_if<
    detail::is_iterable<Container<std::string, Args...>>::value &&
    !detail::is_map<Container<std::string, Args...>>::value
    , Container<std::string, Args...> >::type*
>
inline MemberIterator ObjectRef::find_any(Container<std::string> names) const
{
    for ( const auto& name : names ) {
        auto it = valueRef_.value_.FindMember(rapidjson::Value(name.data(), name.length()));
        if (it != valueRef_.value_.MemberEnd()) {
            return MemberIterator(it, valueRef_.alloc_);
        }
    }
    return MemberIterator(valueRef_.value_.MemberEnd(), valueRef_.alloc_);
}

template<template <typename...> class Container, typename...Args, typename std::enable_if<
    detail::is_iterable<Container<std::string, Args...>>::value &&
    !detail::is_map<Container<std::string, Args...>>::value
    , Container<std::string, Args...> >::type*
>
inline bool ObjectRef::find_all(Container<std::string> names) const
{
    for ( const auto& name : names ) {
        auto it = valueRef_.value_.FindMember(rapidjson::Value(name.data(), name.length()));
        if (it == valueRef_.value_.MemberEnd()) {
            return false;
        }
    }
    return true;
}

} // namespace wrapidjson
