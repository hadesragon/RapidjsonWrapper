namespace Json {

/////////////////////////////////////////////////////////////////////////////////////////////
/// Wrapper rapidjson::Document
/////////////////////////////////////////////////////////////////////////////////////////////

inline ValueRef Document::get_root() const
{
    return ValueRef(*document_, document_->GetAllocator());
}

/////////////////////////////////////////////////////////////////////////////////////////////
/// ValueRef for rapidjson::value
/////////////////////////////////////////////////////////////////////////////////////////////
inline ValueRef::ValueRef(const ArrayRef& array)
    : value_(array.get_valueref().value_), alloc_(array.get_valueref().alloc_)
{}

inline ValueRef::ValueRef(const ObjectRef& obj)
    : value_(obj.get_valueref().value_), alloc_(obj.get_valueref().alloc_)
{}

inline ValueRef& ValueRef::operator=(const Value& other)
{
    value_.CopyFrom(other.value_, alloc_); // copy value explicitly
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
        throw std::runtime_error("ValueRef::push_back allow ArrayType");
    }
    ArrayRef(*this).push_back(value);
}

/// set to empty Array
inline ValueRef ValueRef::operator[](size_t idx) const {
    if ( not value_.IsArray() ) {
        throw std::runtime_error("ValueRef is not array type");
    } else if (idx >= value_.Size() ) {
        throw std::runtime_error("ValueRef[idx] out_of_range");
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
        throw std::runtime_error("ValueRef[key] allow ObjectType");
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

inline std::string ValueRef::to_string(size_t MAX_LENGTH_SIZE) const {
    if (value_.IsNull()) {
        return "Null";
    } else if (value_.IsNumber()) {
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
    } else {
        std::string str;
        Value value(*this);
        value.save_to_buffer(str);
        if ( str.size() > MAX_LENGTH_SIZE ) {
            str = str.substr(0, MAX_LENGTH_SIZE) + "...";
        }
        return str;
    }
    return "";
}
inline rapidjson::Value& ValueRef::get_rvalue() const {
    return value_;
}
inline ArrayRef ValueRef::get_array() const {
    return ArrayRef(*this);
}
inline ObjectRef ValueRef::get_object() const {
    return ObjectRef(*this);
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
/// ValueRef::set_object tempalte impl
/////////////////////////////////////////////////////////////////////////////////////////////
template<typename Value, template <typename...> class Container, typename...Args, typename std::enable_if<
    detail::is_map<Container<std::string, Value, Args...>>::value
    , Container<std::string, Value, Args...>>::type*
>
inline ObjectRef ValueRef::set_object(const Container<std::string, Value, Args...>& map, bool str_copy) {
    set_container(map, str_copy);
    return ObjectRef(*this);
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

/////////////////////////////////////////////////////////////////////////////////////////////
/// ObjectRef::get tempalte impl
/////////////////////////////////////////////////////////////////////////////////////////////

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

} // namespace Json
