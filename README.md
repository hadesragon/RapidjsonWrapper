## Wrapidjson is RapidJSON wrapper

* [RapidJSON GitHub](https://github.com/Tencent/rapidjson/)
* [RapidJSON Documentation](http://rapidjson.org/)

### Introduction

* Wrapidjson is RapidJSON Wrapper.
* Wrapidjson want to hide rapidjson boilerplate code.
* Wrapidjson want to simple usage.
* Wrapidjson use [optional-lite](https://github.com/martinmoene/optional-lite), [string-view-lite](https://github.com/martinmoene/string-view-lite) header.

### Installation

WrapidJson is a header-only C++ library. Just copy the `wrapidjson` folder to project's include path.

### Usage

#### RapidJSON
~~~~~~~~~~cpp
// rapidjson/example/simpledom/simpledom.cpp`
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>

using namespace rapidjson;

int main() {
    // 1. Parse a JSON string into DOM.
    const char* json = "{\"project\":\"rapidjson\",\"stars\":10}";
    Document d;
    d.Parse(json);

    // 2. Modify it by DOM.
    Value& s = d["stars"];
    s.SetInt(s.GetInt() + 1);

    // 3. Stringify the DOM
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    d.Accept(writer);

    // Output {"project":"rapidjson","stars":11}
    std::cout << buffer.GetString() << std::endl;
    return 0;
}
~~~~~~~~~~
#### Wrapidjson
~~~~~~~~~~cpp
#include "wrapidjson/document.h"
#include <iostream>
#include <string>

using namespace wrapidjson;

int main() {
    // 1. Parse a JSON.
    const char* json = "{\"project\":\"rapidjson\",\"stars\":10}";
    Document doc(json);
    
    // 2. Get and Set.
    auto s = obj["stars"];
    s = (s.as<int>() + 1);
    
    // 3. Stringify
    std::string buffer;
    doc.save_to_buffer(buffer);
    
    // Output {"project":"rapidjson","stars":11}
    std::cout << buffer << std::endl;
    return 0;
}
~~~~~~~~~~
### Sample Code
#### Document
* Document has rapidjson::Document 
* Wrapping read & write code
~~~~~~~~~~cpp
#include "wrapidjson/document.h"

using namespace wrapidjson;

int main() {
    const  char* json =  "{\"project\":\"rapidjson\",\"stars\":10}";
    Document doc;
    // String 
    bool success = doc.load_from_buffer(json);
    std::string buffer;
    doc.save_to_buffer(buffer);
    doc.set_null();
    
    // File
    success = doc.load_from_file("/home/wrapidjson/json_file");
    success = doc.save_to_file("/home/wrapidjson/new_file");
    doc.set_null();
    
    // Stream
    std::stringstream ss(json), out;
    success = doc.load_from_stream(ss);
    success = doc.save_to_stream(out);
    return 0;
}
~~~~~~~~~~
#### ValueRef
* ValueRef has **reference** of rapidjson::Value and rapidjson::Document::Allocator
* Wrapping Set or Get function
~~~~~~~~~~cpp
#include "wrapidjson/document.h"

using namespace wrapidjson;

int main() {
    Document doc;
    ValueRef intval = doc["intval"];
    intval = 123;
    auto strval = doc["strval"] = "123.456";

    // get<> function check type
    optional<const char*> str = intval.get<const char*>();
    // str has nullptr
    auto integer = strval.get<int>();
    // integer has nullptr

    // as<> function get or convert type
    auto ival = intval.as<int>();
    // ival is 123
    auto sval = intval.as<std::string>();
    // sval is "123"
    auto dval = strval.as<double>();
    // dval is 123.456
    
    std::string buffer;
    doc.save_to_string(buffer);
    std::cout << buffer << std::endl; // {"intval":123, "doubleval":123.456}
    return 0;
} 
~~~~~~~~~~
#### ArrayRef
* ArrayRef has **ValueRef**
* Wrapping Array Function
~~~~~~~~~~cpp
#include "wrapidjson/document.h"

using namespace wrapidjson;

int main() {
    Document doc;
    // change to array type
    ArrayRef array = doc;
    // rapidjson::Value::Reserve
    array.reserve(10);
    // push_back call rapidjson::Value::PushBack
    array.push_back(1);
    array.push_back(2.0);
    array.push_back("3");

    // make sub array
    ArrayRef sub_array = array.push_back();
    
    // avaiable set STL container
    sub_array = std::vector<std::string>{"1","2","3","4"};
    
    // auto convert array type
    auto set_array = array.push_back();
    set_array = std::set<double>{1.0, 2.0, 3.0};

    // make object array
    ArrayRef map_array = array.push_back();
    map_array.resize(5);
    // range base loop
    for (ObjectRef obj : map_array) {
        obj["key"] = "value";
    }

    // get_vector check type
    auto get_vec = array.get_vector<int>();
    // get_vec is nullptr

    // as_vector convert type
    auto as_vec = array.as_vector<int>();
    // as_vec is [1,2,3,0,0,0]  array or object convert to 0
    
    return 0;
}
~~~~~~~~~~
#### ObjectRef
* ObjectRef has ValueRef
* Wrapping Map Function
~~~~~~~~~~cpp
#include "wrapidjson/document.h"

using namespace wrapidjson;

int main() {
    Document doc;

    ObjectRef first = doc["first"];

    // avaiable set std::map or std::unordered_map
    first = std::map<std::string, double>{{"1",1.0}, {"2",2.0}};
    
    // convert to object type
    doc["second"] = std::unordered_map<std::string, uint64_t>{{"1", 1}, {"2",2}};

    first.insert("3", 3.0);
    ObjectRef third = first.insert("4");
    
    return 0;    
}
~~~~~~~~~~
#### StringView
* ValueRef string function always copy string
* Using string_view prevent copy string
* More Speed and More careful dangling
~~~~~~~~~~cpp
#include "wrapidjson/document.h"

using namespace wrapidjson;

int main() {
    Document doc;

    // string_view is not copy string
    auto first = doc["first"];
    first["chr*"] = "const char* is copy string";
    first["string] = std::string("std::string is copy string");
    first["string_view"] = string_view("wrapidjson::string_view is not copy string");


    auto vec = std::vector<std::string>{"1","2","3","4","5"};

    // set_container function allow string_view
    auto second = doc["second"];
    // copy string
    second = vec;
    
    // not copy string
    bool str_copy = false;
    auto third = doc["third"];
    third.set_container(vec, str_copy);
    return 0;    
}
~~~~~~~~~~
