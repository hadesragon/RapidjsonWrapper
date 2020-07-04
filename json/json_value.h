#ifndef JSON_VALUE_H_
#define JSON_VALUE_H_

#include <limits>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <fstream>
#include <cstdio>

#include <nonstd/optional.hpp>
#include <nonstd/string_view.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/error/en.h>
#include <rapidjson/error/error.h>

#include "format.h"
#include "parse.h"
#include "type_traits.h"

namespace Json {

/////////////////////////////////////////////////////////////////////////////////////////////
/// Predefine
/////////////////////////////////////////////////////////////////////////////////////////////
class ValueRef;
class ArrayRef;
class ObjectRef;
class Value;
class Document;
struct MemberRef;

template<typename T>
using optional = nonstd::optional<T>;
using string_view = nonstd::string_view;

/////////////////////////////////////////////////////////////////////////////////////////////
/// std::istream wrapper
/////////////////////////////////////////////////////////////////////////////////////////////
class IStream {
public:
    using Ch = char;
    IStream(std::istream& is) : is_(is) { }
    IStream(const IStream&) = delete;
    IStream& operator=(const IStream&) = delete;

    Ch Peek() const {
        int c = is_.peek();
        return c == std::char_traits<Ch>::eof() ? '\0' : static_cast<Ch>(c);
    }
    Ch Take() {
        int c = is_.get();
        return c == std::char_traits<Ch>::eof() ? '\0' : static_cast<Ch>(c);
    }
    size_t Tell() const { return static_cast<size_t>(is_.tellg()); }

    Ch* PutBegin() { throw std::runtime_error("IStream::PutBegin not implement"); }
    void Put(Ch) { throw std::runtime_error("IStream::Put not implement"); }
    void Flush() { throw std::runtime_error("IStream::Flush not implement"); }
    size_t PutEnd(Ch*) { throw std::runtime_error("IStream::PutEnd not implement"); }
private:
    std::istream& is_;
};

/////////////////////////////////////////////////////////////////////////////////////////////
/// std::ostream wrapper
/////////////////////////////////////////////////////////////////////////////////////////////
class OStream {
public:
    using Ch = char;
    OStream(std::ostream& os) : os_(os) {}
    OStream(const OStream&) = delete;
    OStream& operator=(const OStream&) = delete;

    Ch Peek() const { throw std::runtime_error("OStream::Peek not implement"); }
    Ch Take() { throw std::runtime_error("OStream::Take not implement"); }
    size_t Tell() const { throw std::runtime_error("OStream::Tell not implement"); }
    Ch* PutBegin() { throw std::runtime_error("OStream::PutBegin not implement"); }
    void Put(Ch c) { os_.put(c); }
    void Flush() { os_.flush(); }
    size_t PutEnd(Ch*) { throw std::runtime_error("OStream::PutEnd not implement"); }

private:
    std::ostream& os_;
};

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
/// Typedef Iterators.
/////////////////////////////////////////////////////////////////////////////////////////////
using ValueIterator = Iterator<rapidjson::Value::ValueIterator, ValueRef, rapidjson::Document::AllocatorType>;
using MemberIterator = Iterator<rapidjson::Value::MemberIterator, MemberRef, rapidjson::Document::AllocatorType>;

/////////////////////////////////////////////////////////////////////////////////////////////
/// Wrapper rapidjson::Document
/////////////////////////////////////////////////////////////////////////////////////////////
class Document {
    static const size_t BUFFER_SIZE = 65536;
public:
    Document() : document_(std::make_shared<rapidjson::Document>())
    {}

    virtual ~Document() = default;

    /// load JSON data
    inline bool load_from_file(const std::string& path) {
        FILE* fp = fopen(path.c_str(), "r");
        if (fp == nullptr) {
            return false;
        }

        char    readBuffer[BUFFER_SIZE];
        rapidjson::FileReadStream is(fp, readBuffer, BUFFER_SIZE);
        document_->ParseStream(is);
        fclose(fp);
        return not document_->HasParseError();
    }

    inline bool load_from_buffer(const std::string& buffer) {
        document_->Parse<0>(buffer.c_str());
        return not document_->HasParseError();
    }
    inline bool load_from_stream(std::istream& is) {
        IStream is_wrapper(is);
        document_->ParseStream(is_wrapper);
        return not document_->HasParseError();
    }

    inline std::string get_load_error() {
        return detail::format("Error offset[%u]: %s", (unsigned)document_->GetErrorOffset(), rapidjson::GetParseError_En(document_->GetParseError()));
    }

    /// save JSON data
    inline bool save_to_file(const std::string& path, bool pretty = false) {
        FILE* fp = fopen(path.c_str(), "w");
        if (fp == nullptr) {
            return false;
        }

        char writeBuffer[BUFFER_SIZE];
        rapidjson::FileWriteStream os(fp, writeBuffer, BUFFER_SIZE);

        bool ret = false;
        if (pretty) {
            rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
            ret = document_->Accept(writer);
        } else{
            rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
            ret = document_->Accept(writer);
        }
        fclose(fp);
        return ret;
    }

    inline bool save_to_buffer(std::string& buffer, bool pretty = false) {
        rapidjson::StringBuffer stringBuf;
        bool ret = false;
        if (pretty){
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(stringBuf);
            ret = document_->Accept(writer);
        } else {
            rapidjson::Writer<rapidjson::StringBuffer> writer(stringBuf);
            ret = document_->Accept(writer);
        }
        if ( ret ) {
            buffer.assign(stringBuf.GetString(), stringBuf.GetSize());
        }
        return ret;
    }

    inline bool save_to_stream(std::ostream& os, bool pretty = false) {
        OStream os_wrapper(os);
        if (pretty) {
            rapidjson::PrettyWriter<OStream> writer(os_wrapper);
            return document_->Accept(writer);
        } else {
            rapidjson::Writer<OStream> writer(os_wrapper);
            return document_->Accept(writer);
        }
    }

    /// getRoot
    ValueRef get_root() const;

    /// get the actual rapidjson::Document by reference
    inline rapidjson::Document& get_document() const {
        return *document_;
    }

protected:
    std::shared_ptr<rapidjson::Document> document_;
};

/////////////////////////////////////////////////////////////////////////////////////////////
/// ValueRef for rapidjson::value
/////////////////////////////////////////////////////////////////////////////////////////////
class ValueRef {
    friend class ArrayRef;
    friend class ObjectRef;

public:
    /// constructors:
    ValueRef(rapidjson::Value& value, rapidjson::Document::AllocatorType& alloc) 
        : value_(value), alloc_(alloc) {}
    ValueRef(const ValueRef& rfs) 
        : value_(rfs.value_), alloc_(rfs.alloc_) {}
    ValueRef(const ArrayRef& array);
    ValueRef(const ObjectRef& obj);

    virtual ~ValueRef() = default;

    /// copy assignment
    ValueRef& operator=(const Value&);

    /// copy assignment
    ValueRef& operator=(const ValueRef& other) {
        if (this != &other) {
            value_.CopyFrom(other.value_, alloc_); // copy value explicitly
        }
        return *this;
    }

    template<typename T>
    ValueRef& operator=(T value) {
        value_ = value;
        return *this;
    }

    /// catches string literals (without copying):
    template<int N>
    ValueRef& operator=(const char(&s)[N]) {
        value_.SetString(s, N-1); // we don't want to store the \0 character
        return *this;
    }
    /// catches std::string (makes a copy):
    ValueRef& operator=(const std::string& s) {
        value_.SetString(s.data(), s.length(), alloc_); // make copy via allocator!
        return *this;
    }

    ValueRef& operator=(const char* s) {
        value_.SetString(s, alloc_); // make copy via allocator!
        return *this;
    }

    /// catches string_view (without copy)
    ValueRef& operator=(const string_view& s) {
        value_.SetString(s.data(), s.length());
        return *this;
    }

    /// assign from Array reference
    ValueRef& operator=(const ArrayRef& array);

    /// assign from Object reference
    ValueRef& operator=(const ObjectRef& object);

    /// set Value (Number, String, Null)
    template<typename T>
    ValueRef& setValue(T&& value) {
        return operator=(std::forward<T>(value));
    }

    /// assign from all continaer ( array, map )
    template<template <typename...> class Container, typename ...Args>
    ValueRef& operator=(const Container<Args...>& container) {
        set_container(container);
        return *this;
    }

    /// set Container ( Container<Number> )
    template<typename T, template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_iterable<Container<T, Args...>>::value &&
        !detail::is_map<Container<T, Args...>>::value
        , Container<T, Args...>>::type* = nullptr
    >
    void set_container(const Container<T, Args...>& array, bool str_copy = true);

    /// set Container ( Continaer<String> )
    template<template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_iterable<Container<std::string, Args...>>::value &&
        !detail::is_map<Container<std::string, Args...>>::value
        , Container<std::string, Args...> >::type* = nullptr
    >
    void set_container(const Container<std::string, Args...>& array, bool str_copy = true);

    /// assign from map<string, Number>
    template<typename Value, template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_map<Container<std::string, Value, Args...>>::value
        , Container<std::string, Value, Args...>>::type* = nullptr
    >
    void set_container(const Container<std::string, Value, Args...>& map, bool str_copy = true);

    /// assign from map<string, string>
    template<template <typename...> class Container, typename std::enable_if<
        detail::is_map<Container<std::string, std::string>>::value
        , Container<std::string, std::string>>::type* = nullptr
    >
    void set_container(const Container<std::string, std::string>& map, bool str_copy = true);

    /// set to Null
    ValueRef& set_null() {
        value_.SetNull();
        return *this;
    }

    /// set to empty Array
    ArrayRef set_array();

    /// set to Array from array
    template<typename T, template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_iterable<Container<T, Args...>>::value &&
        !detail::is_map<Container<T, Args...>>::value
        , Container<T, Args...>>::type* = nullptr
    >
    ArrayRef set_array(const Container<T, Args...>& array, bool str_copy = true);

    /// set to Array
    void push_back(const ValueRef& value);

    /// get array
    ValueRef operator[](size_t idx) const;

    /// set to empty Object
    ObjectRef set_object();

    /// set to Object from a map
    template<typename Value, template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_map<Container<std::string, Value, Args...>>::value
        , Container<std::string, Value, Args...>>::type* = nullptr
    >
    ObjectRef set_object(const Container<std::string, Value, Args...>& map, bool str_copy = true);

    /// set to Object
    ValueRef operator[](const char* name) const;

    /// set to Object
    ValueRef operator[](const std::string& name) const;

    /// check member
    bool has(const std::string& name) const;

    /// find member
    optional<ValueRef> find(const std::string& name) const;

    /// get type info
    inline bool is_bool() const { return value_.IsBool(); }
    inline bool is_number() const { return value_.IsNumber(); }
    inline bool is_integral() const { return value_.IsInt() or value_.IsUint() or value_.IsInt64() or value_.IsUint64(); }
    inline bool is_double() const { return value_.IsDouble(); }
    inline bool is_string() const { return value_.IsString(); }
    inline bool is_array() const { return value_.IsArray(); }
    inline bool is_object() const { return value_.IsObject(); }
    inline bool is_null() const { return value_.IsNull(); }

    /// test if two Values point to the same rapidjson::Value
    bool operator==(const ValueRef& other) { return (&value_ == &other.value_); }
    bool operator!=(const ValueRef& other) { return !(operator==(other)); }

    /// type = as<type>
    template<typename T, typename std::enable_if<std::is_same<T, char>::value, T>::type* = nullptr>
    char as() const;

    template<typename T, typename std::enable_if<
        std::is_arithmetic<T>::value && !std::is_same<T, char>::value, T>::type* = nullptr>
    T as() const;

    template<typename T, typename std::enable_if<detail::is_const_char<T>::value, T>::type* = nullptr>
    const char* as() const;

    template<typename T, typename std::enable_if<detail::is_string<T>::value, T>::type* = nullptr>
    std::string as() const;


    /// optional<type> = get<type>
    template<typename T, typename std::enable_if<
        std::is_integral<T>::value && std::is_same<T, bool>::value, T>::type* = nullptr>
    optional<bool> get() const;

    template<typename T, typename std::enable_if<
        std::is_integral<T>::value && std::is_same<T, char>::value, T>::type* = nullptr>
    optional<char> get() const;

    template<typename T, typename std::enable_if<
        std::is_integral<T>::value && std::is_signed<T>::value && sizeof(T) == 8, T>::type* = nullptr>
    optional<T> get() const;

    template<typename T, typename std::enable_if<
        std::is_integral<T>::value && !std::is_same<T, char>::value
        && std::is_signed<T>::value && sizeof(T) < 8, T>::type* = nullptr>
    optional<T> get() const;

    template<typename T, typename std::enable_if<
        std::is_integral<T>::value && !std::is_same<T, bool>::value
        && std::is_unsigned<T>::value && sizeof(T) == 8, T>::type* = nullptr>
    optional<T> get() const;

    template<typename T, typename std::enable_if<
        std::is_integral<T>::value && !std::is_same<T, bool>::value
        && std::is_unsigned<T>::value && sizeof(T) < 8, T>::type* = nullptr>
    optional<T> get() const;

    template<typename T, typename std::enable_if<std::is_floating_point<T>::value, T>::type* = nullptr>
    optional<T> get() const;

    template<typename T, typename std::enable_if<detail::is_const_char<T>::value, T>::type* = nullptr>
    optional<T> get() const;

    template<typename T, typename std::enable_if<detail::is_string<T>::value, T>::type* = nullptr>
    optional<T> get() const;

    std::string to_string(size_t MAX_LENGTH_SIZE=std::numeric_limits<size_t>::max()) const;
    rapidjson::Value& get_rvalue() const;
    ArrayRef get_array() const;
    ObjectRef get_object() const;

    inline ValueRef* operator->() {
        return this;
    }

    inline  bool empty() const {
        if ( value_.IsObject() ) {
            return value_.ObjectEmpty();
        } else if ( value_.IsArray() ) {
            return value_.Empty();
        } else if ( value_.IsString() ) {
            return value_.GetStringLength() == 0;
        }
        return false;
    }

    inline size_t size() const {
        if ( value_.IsObject() ) {
            return value_.MemberCount();
        } else if ( value_.IsArray() ) {
            return value_.Size();
        } else if ( value_.IsString() ) {
            return value_.GetStringLength();
        }
        return 0;
    }

    /// long != int64_t
    ValueRef& operator=(std::enable_if<!std::is_same<int64_t, long>::value, long>::type value) {
        value_ = static_cast<int64_t>(value);
        return *this;
    }
    /// unsigned long != uint64_t
    ValueRef& operator=(std::enable_if<!std::is_same<uint64_t, unsigned long>::value, unsigned long>::type value) {
        value_ = static_cast<uint64_t>(value);
        return *this;
    }
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
    ~ArrayRef() = default;

    ArrayRef(const ArrayRef& rfs)
        : valueRef_(rfs.valueRef_) {}

    ArrayRef(const ValueRef& value) 
        : valueRef_(value)
    {
        static const size_t STRING_MAX_SIZE = 15;
        if ( valueRef_.value_.IsNull() ) {
            valueRef_.value_.SetArray();
        } else if ( not valueRef_.value_.IsArray() ) {
            throw std::runtime_error(detail::format("Value(%s) is not arrayType, ArrayRef must derived by arrayType", value.to_string(STRING_MAX_SIZE)));
        }
    }
    ArrayRef& operator=(const ArrayRef& other) = delete;

    template<typename T, template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_iterable<Container<T, Args...>>::value &&
        !detail::is_map<Container<T, Args...>>::value
        , Container<T, Args...>>::type* = nullptr
    >
    ArrayRef& operator=(const Container<T, Args...>& array) {
        valueRef_.set_container(array);
        return *this;
    }

    template<typename T, template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_iterable<Container<T, Args...>>::value &&
        !detail::is_map<Container<T, Args...>>::value
        , Container<T, Args...>>::type* = nullptr
    >
    inline void set_container(const Container<T, Args...>& array, bool str_copy = true) {
        valueRef_.set_container(array, true);
    }

    ValueRef operator[](size_t index) const {
        if ( index >= valueRef_.value_.Size() ) {
            throw std::runtime_error("Array index out_of_range");
        }
        return ValueRef(valueRef_.value_[index], valueRef_.alloc_);
    }

    inline size_t size() const {
        return valueRef_.value_.Size();
    }
    inline bool empty() const {
        return valueRef_.value_.Empty();
    }
    inline size_t capacity() const {
        return valueRef_.value_.Capacity();
    }
    inline void reserve(size_t n) {
        valueRef_.value_.Reserve(n, valueRef_.alloc_);
    }
    inline void resize(size_t n) {
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
    inline void resize(size_t n, T&& value) {
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

    inline void clear() {
        valueRef_.value_.Clear();
    }

    inline ValueIterator begin() const {
        return ValueIterator(valueRef_.value_.Begin(), valueRef_.alloc_);
    }
    inline ValueIterator end() const {
        return ValueIterator(valueRef_.value_.End(), valueRef_.alloc_);
    }
    inline ValueRef front() const {
        if ( valueRef_.value_.Empty() ) {
            throw std::runtime_error("Empty Array front() is null");
        }
        return ValueRef(valueRef_.value_[0], valueRef_.alloc_);
    }
    inline ValueRef back() const {
        if ( valueRef_.value_.Empty() ) {
            throw std::runtime_error("Empty Array back() is null");
        }
        return ValueRef(valueRef_.value_[size()-1], valueRef_.alloc_);
    }

    template <typename T>
    inline optional<std::vector<T>> get_vector()
    {
        optional<std::vector<T>> vector;
        {
            std::vector<T> res;
            res.reserve(size());
            for (const auto& value : *this)
            {
                auto value_ = value.get<T>();
                if ( value_ ) {
                    res.emplace_back(std::move(*value_));
                } else {
                    return vector;
                }
            }
            vector = res;
        }
        return vector;
    }

    template <typename T>
    inline std::vector<T> as_vector(std::function<bool(const T&)> func = [](const T&){return true;})
    {
        std::vector<T> vector;
        vector.reserve(size());
        for (const auto& value : *this)
        {
            T value_ = value.as<T>();
            if ( func(value_) ) {
                vector.emplace_back(std::move(value_));
            }
        }
        return vector;
    }

    template<typename T>
    inline void push_back(T&& value) {
        rapidjson::Value temp;
        ValueRef dummy(temp, valueRef_.alloc_);
        dummy = std::forward<T>(value);
        valueRef_.value_.PushBack(temp.Move(), valueRef_.alloc_);
    }

    inline ValueRef push_back() {
        valueRef_.value_.PushBack(rapidjson::Value(), valueRef_.alloc_);
        return ValueRef(valueRef_.value_[size()-1], valueRef_.alloc_);
    }
    inline void pop_back() {
        if ( not valueRef_.value_.Empty() ) {
            valueRef_.value_.PopBack();
        }
    }
    inline ValueIterator erase(const ValueIterator& pos) {
        return ValueIterator(valueRef_.value_.Erase(pos.ptr_), valueRef_.alloc_);
    }
    inline ValueIterator erase(const ValueIterator& first, const ValueIterator& last) {
        return ValueIterator(valueRef_.value_.Erase(first.ptr_, last.ptr_), valueRef_.alloc_);
    }

    inline std::string to_string() const {
        return valueRef_.to_string();
    }
    inline ValueRef get_valueref() const {
        return valueRef_;
    }
protected:
    ValueRef valueRef_;
};

/////////////////////////////////////////////////////////////////////////////////////////////
/// ValueRef::set_array template impl
/////////////////////////////////////////////////////////////////////////////////////////////
template<typename T, template <typename...> class Container, typename...Args, typename std::enable_if<
    detail::is_iterable<Container<T, Args...>>::value &&
    !detail::is_map<Container<T, Args...>>::value
    , Container<T, Args...>>::type*
>
inline ArrayRef ValueRef::set_array(const Container<T, Args...>& array, bool str_copy) {
    set_container(array, str_copy);
    return ArrayRef(*this);
}

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
/// ObjectRef ( Reference rapidjson::value for Object )
/////////////////////////////////////////////////////////////////////////////////////////////
class ObjectRef {
    friend class ValueRef;

public:
    ObjectRef(const ValueRef& value)
        : valueRef_(value)
    {
        static const size_t STRING_MAX_SIZE = 15;
        if ( valueRef_.value_.IsNull() ) {
            valueRef_.value_.SetObject();
        } else if ( not valueRef_.value_.IsObject() ) {
            throw std::runtime_error(detail::format("Value(%s) is not ObjectType, ObjectRef must derived by ObjectType", valueRef_.to_string(STRING_MAX_SIZE)));
        }
    }

    ObjectRef(const ObjectRef& rfs)
        : valueRef_(rfs.valueRef_) {}

    ~ObjectRef() = default;

    ObjectRef& operator=(const ObjectRef& other) = delete;
    template<typename Value, template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_map<Container<std::string, Value, Args...>>::value
        , Container<std::string, Value, Args...>>::type* = nullptr
    >
    ObjectRef& operator=(const Container<std::string, Value, Args...>& map) {
        valueRef_.set_container(map);
        return *this;
    }

    /// set_container map<std::string, Value>
    template<typename Value, template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_map<Container<std::string, Value, Args...>>::value
        , Container<std::string, Value, Args...>>::type* = nullptr
    >
    inline void set_container(const Container<std::string, Value, Args...>& map, bool str_copy = true) {
        valueRef_.set_container(map, true);
    }

    /// get_value<bool>()
    template<typename T, typename std::enable_if<
        std::is_same<bool, T>::value, T>::type* = nullptr
    >
    inline optional<bool> get_value(const std::string& name) const;

    /// get_value<String>()
    template<typename T, typename std::enable_if<
        detail::is_string<T>::value, T>::type* = nullptr
    >
    inline optional<std::string> get_value(const std::string& name) const;

    /// get_value<const char>()
    template<typename T, typename std::enable_if<
        detail::is_const_char<T>::value, T>::type* = nullptr
    >
    inline optional<const char*> get_value(const std::string& name) const;

    /// get_value<Number>()
    template<typename T, typename std::enable_if<
        std::numeric_limits<T>::is_bounded && !std::is_same<bool, T>::value, T>::type* = nullptr
    >
    inline optional<T> get_value(const std::string& name) const;

    /// get_value<T>(default_value)
    template<typename T>
    inline T get_value(const std::string& name, const T& defval) const {
        optional<T> value = get_value<T>(name);
        if ( not value ) {
            value = defval;
        }
        return *value;
    }

    inline ValueRef operator[](const std::string& name) const {     // key copy
        rapidjson::Value key(name.data(), name.length());
        auto it = valueRef_.value_.FindMember(key);
        if (it == valueRef_.value_.MemberEnd()){
            valueRef_.value_.AddMember(rapidjson::Value(name.data(), name.length(), valueRef_.alloc_), rapidjson::Value(), valueRef_.alloc_);
            it = valueRef_.value_.MemberEnd()-1;
        }
        return ValueRef(it->value, valueRef_.alloc_);
    }
    inline ValueRef operator[](const char* name) const {            // key copy
        rapidjson::Value key(rapidjson::StringRef(name));
        auto it = valueRef_.value_.FindMember(key);
        if (it == valueRef_.value_.MemberEnd()) {
            valueRef_.value_.AddMember(rapidjson::Value(rapidjson::StringRef(name), valueRef_.alloc_), rapidjson::Value(), valueRef_.alloc_);
            it = valueRef_.value_.MemberEnd()-1;
        }
        return ValueRef(it->value, valueRef_.alloc_);
    }

    inline ValueRef operator[](const string_view& name) const {     // key not copy
        rapidjson::Value key(rapidjson::StringRef(name.data(), name.length()));
        auto it = valueRef_.value_.FindMember(key);
        if (it == valueRef_.value_.MemberEnd()) {
            valueRef_.value_.AddMember(rapidjson::Value(rapidjson::StringRef(name.data(), name.length())), rapidjson::Value(), valueRef_.alloc_);
            it = valueRef_.value_.MemberEnd()-1;
        }
        return ValueRef(it->value, valueRef_.alloc_);
    }

    inline optional<ValueRef> find(const std::string& name) const {
        optional<ValueRef> ret;
        rapidjson::Value key(name.data(), name.length());
        auto it = valueRef_.value_.FindMember(key);
        if ( it != valueRef_.value_.MemberEnd() ) {
            ret = ValueRef(it->value, valueRef_.alloc_);
        }
        return ret;
    }

    template<template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_iterable<Container<std::string, Args...>>::value &&
        !detail::is_map<Container<std::string, Args...>>::value
        , Container<std::string, Args...> >::type* = nullptr
    >
    inline MemberIterator find_any(Container<std::string> names) const;

    template<template <typename...> class Container, typename...Args, typename std::enable_if<
        detail::is_iterable<Container<std::string, Args...>>::value &&
        !detail::is_map<Container<std::string, Args...>>::value
        , Container<std::string, Args...> >::type* = nullptr
    >
    inline bool find_all(Container<std::string> names) const;

    inline int count(const std::string& name) const {
        rapidjson::Value key(name.data(), name.length());
        auto it = valueRef_.value_.FindMember(key);
        return (it != valueRef_.value_.MemberEnd());
    }

    inline size_t size() const {
        return valueRef_.value_.MemberCount();
    }
    inline bool empty() const {
        return valueRef_.value_.ObjectEmpty();
    }
    inline bool has(const std::string& name) const {
        return valueRef_.value_.HasMember(name.c_str());
    }
    inline void clear() {
        return valueRef_.value_.RemoveAllMembers();
    }

    inline MemberIterator begin() const {
        return MemberIterator(valueRef_.value_.MemberBegin(), valueRef_.alloc_);
    }
    inline MemberIterator end() const {
        return MemberIterator(valueRef_.value_.MemberEnd(), valueRef_.alloc_);
    }

    template<typename T>
    inline void insert(const char* name, T&& value) {          // key copy
        rapidjson::Value temp;
        ValueRef dummy(temp, valueRef_.alloc_);
        dummy = std::forward<T>(value);
        valueRef_.value_.AddMember(rapidjson::Value(name, strlen(name), valueRef_.alloc_), temp.Move(), valueRef_.alloc_);
    }

    template<typename T>
    inline void insert(const std::string& name, T&& value) {   // key copy
        rapidjson::Value temp;
        ValueRef dummy(temp, valueRef_.alloc_);
        dummy = std::forward<T>(value);
        valueRef_.value_.AddMember(rapidjson::Value(name.data(), name.length(), valueRef_.alloc_), temp.Move(), valueRef_.alloc_);
    }

    template<typename T>
    inline void insert(const string_view& name, T&& value) {   // key not copy
        rapidjson::Value temp;
        ValueRef dummy(temp, valueRef_.alloc_);
        dummy = std::forward<T>(value);
        valueRef_.value_.AddMember(rapidjson::Value(name.data(), name.length()), temp.Move(), valueRef_.alloc_);
    }

    inline ValueRef insert(const char* name) {              // key copy
        valueRef_.value_.AddMember(rapidjson::Value(name, strlen(name), valueRef_.alloc_), rapidjson::Value(), valueRef_.alloc_);
        auto it = (valueRef_.value_.MemberEnd() - 1);
        return ValueRef(it->value, valueRef_.alloc_);
    }

    inline ValueRef insert(const std::string& name) {       // key copy
        valueRef_.value_.AddMember(rapidjson::Value(name.data(), name.length(), valueRef_.alloc_), rapidjson::Value(), valueRef_.alloc_);
        auto it = (valueRef_.value_.MemberEnd() - 1);
        return ValueRef(it->value, valueRef_.alloc_);
    }


    inline ValueRef insert(const string_view& name) {       // key not copy
        valueRef_.value_.AddMember(rapidjson::Value(name.data(), name.length()), rapidjson::Value(), valueRef_.alloc_);
        auto it = (valueRef_.value_.MemberEnd() - 1);
        return ValueRef(it->value, valueRef_.alloc_);
    }

    inline MemberIterator erase(const std::string& name)  {
        auto it = valueRef_.value_.FindMember(rapidjson::Value(name.data(), name.length()));
        if (it != valueRef_.value_.MemberEnd()) {
            return MemberIterator(valueRef_.value_.EraseMember(it), valueRef_.alloc_);
        } else {
            return MemberIterator(it, valueRef_.alloc_);
        }
    }
    inline MemberIterator erase(const MemberIterator& pos) {
        return MemberIterator(valueRef_.value_.EraseMember(pos.ptr_), valueRef_.alloc_);
    }
    inline MemberIterator erase(const MemberIterator& first, const MemberIterator& last) {
        return MemberIterator(valueRef_.value_.EraseMember(first.ptr_, last.ptr_), valueRef_.alloc_);
    }

    inline std::string to_string() const {
        return valueRef_.to_string();
    }
    inline ValueRef get_valueref() const {
        return valueRef_;
    }
protected:
    ValueRef valueRef_;
};

/////////////////////////////////////////////////////////////////////////////////////////////
/// Value ( Document With ValueRef )
/////////////////////////////////////////////////////////////////////////////////////////////
class Value : public Document, public ValueRef
{
public:
    Value() : Document(), ValueRef(*document_, document_->GetAllocator()) {}
    Value(const Value& other) : Value()
    {
        if (this != &other){
            value_.CopyFrom(other.value_, alloc_); // copy value explicitly
        }
    }
    
    Value(const ValueRef& other) : Value()
    {
        value_.CopyFrom(other.get_rvalue(), alloc_); // copy value explicitly
    }
    Value(const ArrayRef& other) : Value() 
    {
        value_.CopyFrom(other.get_valueref().get_rvalue(), alloc_); // copy value explicitly
    }
    Value(const ObjectRef& other) : Value()
    {
        value_.CopyFrom(other.get_valueref().get_rvalue(), alloc_); // copy value explicitly
    }

    Value(const std::string& other) : Value() // json load
    {
        if (not load_from_buffer(other) ) {
            throw std::runtime_error("Value(json_str) parsing error : "+other);
        }
    }

    // init Container
    template<template <typename...> class Container, typename...Args>
    Value(const Container<Args...>& container)
        : Value()
    {
        set_container(container);
    }

    ~Value() override = default;

    Value& operator=(const Value& other) {
        if (this != &other){
            value_.CopyFrom(other.value_, alloc_); // copy value explicitly
        }
        return *this;
    }

    using ValueRef::operator=;
};


/////////////////////////////////////////////////////////////////////////////////////////////
/// stream out
/////////////////////////////////////////////////////////////////////////////////////////////
static std::ostream& operator<<(std::ostream& out, const ValueRef& value_) {
    Value value(value_);
    std::string buffer;
    value.save_to_buffer(buffer);
    out << buffer;
    return out;
}

static std::ostream& operator<<(std::ostream& out, Document& doc) {
    std::string buffer;
    doc.save_to_buffer(buffer);
    out << buffer;
    return out;
}

std::ostream& operator<<(std::ostream& out, Value& value) {
    std::string buffer;
    value.save_to_buffer(buffer);
    out << buffer;
    return out;
}

} // namespace Json

#include "json_value_impl.h"

#endif // JSON_VALUE_H_
