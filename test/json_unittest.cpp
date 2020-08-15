#include <string>
#include <list>
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>

#include <gtest/gtest.h>

#include "wrapidjson/document.h"

using namespace wrapidjson;

TEST(wrapidjsonTest, ucs4)
{
    std::string json = R"({"content":"\uD83C\uDFAF"})";
    char expected[] = "🎯";
    expected[0] = 0xF0;
    expected[1] = 0x9F;
    expected[2] = 0x8E;
    expected[3] = 0xAF;

    Document value;
    EXPECT_TRUE(value.load_from_buffer(json));
    EXPECT_EQ(expected, value["content"].as<std::string>());

    std::string irf_json = R"([["id1",0.5742],["id2","32.5742"],["id3",5742]])";
    std::map<std::string, double> irf_score_map;

    EXPECT_TRUE(value.load_from_buffer(irf_json));
    for ( const ArrayRef arr2 : value.get_array() ) {
        if (arr2.size() == 2) {
            const char* docid = arr2[0].as<const char*>();
            irf_score_map[docid] = arr2[1].as<double>();
        }
    }

    EXPECT_EQ(0.5742, irf_score_map["id1"]);
    EXPECT_EQ(32.5742, irf_score_map["id2"]);
    EXPECT_EQ(5742, irf_score_map["id3"]);
}

TEST(wrapidjsonTest, assign_basic)
{
    Document value;
    value["1000"] = 1000;
    value["1000s"] = "1000";
    EXPECT_EQ(value["1000"].as<uint64_t>(), 1000u);
    EXPECT_EQ(value["1000"].as<std::string>(), "1000");
    EXPECT_EQ(value["1000s"].as<uint64_t>(), 1000u);
    EXPECT_EQ(value["1000s"].as<std::string>(), "1000");
}

TEST(wrapidjsonTest, long_long_test)
{
    Document value;
    value["long"] = static_cast<long>(1000);
    value["int64_t"] = static_cast<int64_t>(1000);
    value["long_long"] = static_cast<long long>(1000);

    value["ulong"] = static_cast<unsigned long>(1000);
    value["uint64_t"] = static_cast<uint64_t>(1000);
    value["ulong_long"] = static_cast<unsigned long long>(1000);

    value["long_vector"] = std::vector<long>{1,2,3,4,5,6,7,8,9,0};
    value["int64_vector"] = std::vector<int64_t>{1,2,3,4,5,6,7,8,9,0};
    value["long_long_vector"] = std::vector<long long>{1,2,3,4,5,6,7,8,9,0};

    value["long_vector"] = std::vector<unsigned long>{1,2,3,4,5,6,7,8,9,0};
    value["int64_vector"] = std::vector<uint64_t>{1,2,3,4,5,6,7,8,9,0};
    value["long_long_vector"] = std::vector<unsigned long long>{1,2,3,4,5,6,7,8,9,0};
}

TEST(wrapidjsonTest, to_string_test)
{
    Document value;

    value["int"] = 32;
    value["double"] = 12.34;
    value["string"] = "string test";
    value["array"] = std::vector<std::string>{"array", "test"};
    value["object"] = std::map<std::string, int>{{"obj",1}, {"test",2}};

    EXPECT_EQ(value["int"].to_string(), "32");
    EXPECT_EQ(value["double"].to_string(), "12.34");
    EXPECT_EQ(value["string"].to_string(), R"("string test")");
    EXPECT_EQ(value["array"].to_string(), R"(["array","test"])");
    EXPECT_EQ(value["object"].to_string(), R"({"obj":1,"test":2})");
}

TEST(wrapidjsonTest, document_copy)
{
    std::string result;
    {
        Document value, array, object; 

        array.set_container(std::vector<std::string>{"a","b","c","d","e"});
        object.set_container(std::unordered_map<std::string, int64_t>{ {"a",1}, {"b",1}, {"c",3} });

        value["array"] = array;         // copy
        value["object"] = object;       // copy
        value["array1"] = array;        // copy
        value["object1"] = object;      // copy
        value["array2"] = array;        // copy
        value["object2"] = object;      // copy
        value["array3"] = array;        // copy
        value["object3"] = object;      // copy

        value.save_to_buffer(result);

        std::cout << result << std::endl;
    }

    {
        Document value;
        value.load_from_buffer(result);
        EXPECT_EQ(value.size(), 8u);
        EXPECT_TRUE(value.is_object());

        EXPECT_TRUE(value["array"].is_array());
        EXPECT_EQ(value["array"].size(), 5u);
        EXPECT_TRUE(value["object"].is_object());
        EXPECT_EQ(value["object"].size(), 3u);
        EXPECT_TRUE(value["array1"].is_array());
        EXPECT_EQ(value["array1"].size(), 5u);
        EXPECT_TRUE(value["object1"].is_object());
        EXPECT_EQ(value["object1"].size(), 3u);
        EXPECT_TRUE(value["array2"].is_array());
        EXPECT_EQ(value["array2"].size(), 5u);
        EXPECT_TRUE(value["object2"].is_object());
        EXPECT_EQ(value["object2"].size(), 3u);
        EXPECT_TRUE(value["array3"].is_array());
        EXPECT_EQ(value["array3"].size(), 5u);
        EXPECT_TRUE(value["object3"].is_object());
        EXPECT_EQ(value["object3"].size(), 3u);
    }
}

TEST(wrapidjsonTest, json_format_error)
{
    Document doc;
    EXPECT_TRUE(doc.load_from_buffer(R"("")"));
    EXPECT_EQ(doc.get_load_error(), "Error offset[0]: No error.");

    EXPECT_TRUE(doc.load_from_buffer(R"(true)"));
    EXPECT_EQ(doc.get_load_error(), "Error offset[0]: No error.");

    EXPECT_TRUE(doc.load_from_buffer(R"(123124)"));
    EXPECT_EQ(doc.get_load_error(), "Error offset[0]: No error.");

    EXPECT_FALSE(doc.load_from_buffer(R"({"a1":["a","b","c","d","e"])"));

#if RAPIDJSON_MAJOR_VERSION >= 1 && RAPIDJSON_MINOR_VERSION > 0
    EXPECT_EQ(doc.get_load_error(), "Error offset[27]: Missing a comma or '}' after an object member.");
#else
    EXPECT_EQ(doc.get_load_error(), "Error offset[28]: Missing a comma or '}' after an object member.");
#endif

    EXPECT_FALSE(doc.load_from_buffer(R"({"e":{"a1":["a","b","c","d","e"]})"));
#if RAPIDJSON_MAJOR_VERSION >= 1 && RAPIDJSON_MINOR_VERSION > 0
    EXPECT_EQ(doc.get_load_error(), "Error offset[33]: Missing a comma or '}' after an object member.");
#else
    EXPECT_EQ(doc.get_load_error(), "Error offset[34]: Missing a comma or '}' after an object member.");
#endif

    EXPECT_FALSE(doc.load_from_buffer(R"(["a","b""c","d","e"])"));
#if RAPIDJSON_MAJOR_VERSION >= 1 && RAPIDJSON_MINOR_VERSION > 0
    EXPECT_EQ(doc.get_load_error(), "Error offset[8]: Missing a comma or ']' after an array element.");
#else
    EXPECT_EQ(doc.get_load_error(), "Error offset[9]: Missing a comma or ']' after an array element.");
#endif

    EXPECT_FALSE(doc.load_from_buffer(R"(["a","b","c","d","e")"));
#if RAPIDJSON_MAJOR_VERSION >= 1 && RAPIDJSON_MINOR_VERSION > 0
    EXPECT_EQ(doc.get_load_error(), "Error offset[20]: Missing a comma or ']' after an array element.");
#else
    EXPECT_EQ(doc.get_load_error(), "Error offset[21]: Missing a comma or ']' after an array element.");
#endif
}

TEST(wrapidjsonTest, rapidjson_format)
{
    std::string result;
    {
        // SET
        Document root;

        std::vector<std::string> a = {"a","b","c","d","e"};
        std::map<std::string, int64_t> b = { {"a",1}, {"b",2}, {"c",3} };

        root["array"] = a;
        root["object"] = b;

        ArrayRef array = root["array"];

        array.push_back("f");                   // str copy
        array.push_back("g");                   // str copy
        auto item_h = array.push_back();
        item_h = "h";                           // str copy
        auto item_i = array.push_back();
        item_i = string_view("i"); // str no copy

        ObjectRef object = root["object"];
        object["d"] = 4;                    // copy
        object[std::string("e")] = 5;       // copy
        object[string_view("f")] = 6; // no copy

        auto g = object.insert("g");
        g = "g";
        auto h = object.insert(std::string("h"));
        std::string h_str = "h";
        h = string_view(h_str.data(), h_str.length());
        std::string i = "itest";
        object.insert(string_view("i"), i);

        std::set<int> c = {1,2,3,4,5,6};
        std::map<std::string, std::string> d = { {"a","1"}, {"b","1"}, {"c","3"} };

        ArrayRef array1 = root["array1"];
        array1 = c;
        ObjectRef object1 = root["object1"];
        object1 = d;

        root.save_to_buffer(result);
        std::cout << result << std::endl;
        root.set_null();
    }

    {
        // GET
        Document root;
        root.load_from_buffer(result);

        EXPECT_TRUE(root.is_object());
        EXPECT_TRUE(root["object"].is_object());
        EXPECT_TRUE(root["array"].is_array());

        EXPECT_THROW(ArrayRef array = root["object"], std::exception);
        ObjectRef object = root["object"];
        EXPECT_EQ(object.size(), 9u);
        EXPECT_EQ(object["a"].as<int>(), 1);
        EXPECT_EQ(object["b"].as<int>(), 2);
        EXPECT_EQ(object["c"].as<int>(), 3);
        EXPECT_EQ(object["d"].as<int>(), 4);
        EXPECT_EQ(object["e"].as<int>(), 5);
        EXPECT_EQ(object["f"].as<int>(), 6);

        EXPECT_THROW(ObjectRef object = root["array"], std::exception);
        ArrayRef array = root["array"];
        EXPECT_EQ(array.size(), 9u);
        EXPECT_EQ(array[0].as<std::string>(), "a");
        EXPECT_EQ(array[1].as<std::string>(), "b");
        EXPECT_EQ(array[2].as<std::string>(), "c");
        EXPECT_EQ(array[3].as<std::string>(), "d");
        EXPECT_EQ(array[4].as<std::string>(), "e");
        EXPECT_EQ(array[5].as<std::string>(), "f");
        EXPECT_EQ(array[6].as<std::string>(), "g");
        EXPECT_EQ(array[7].as<std::string>(), "h");
        EXPECT_EQ(array[8].as<std::string>(), "i");
        root.set_null();
    }
}

TEST(wrapidjsonTest, json_get)
{
    std::string result;
    {
        // SET
        Document obj;

        obj["char_i"] = 44;
        obj["char_c"] = "A";
        obj["char_s"] = "AAS";

        obj["int8-"] = -128;
        obj["int8+"] = 127;
        obj["int8e"] = 128;
        obj["int8s"] = "127";

        obj["uint8-"] = -1;
        obj["uint8+"] = 255;
        obj["uint8e"] = 256;
        obj["uint8s"] = "255";

        obj["int32-"] = -2147483648L;
        obj["int32+"] = 2147483647;
        obj["int32e"] = 2147483648L;
        obj["int32s"] = "2147483647";

        obj["uint32-"] = -1;
        obj["uint32+"] = 4294967295u;
        obj["uint32e"] = 4294967296uL;
        obj["uint32s"] = "4294967295";

        obj["int64-"] = (-9223372036854775807L-1);
        obj["int64+"] = 9223372036854775807L;
        obj["int64s"] = "9223372036854775807";

        obj["uint64-"] = -1;
        obj["uint64+"] = 18446744073709551614uL;
        obj["uint64s"] = "18446744073709551614";

        obj["double-"] = -92233720368547.75806;
        obj["double+"] = 922337203685477.5806;
        obj["doubles"] = "922337203685477.5806";

        obj.save_to_buffer(result);
        std::cout << result << std::endl;
        obj.set_null();
    }

    {
        // GET
        Document doc;
        doc.load_from_buffer(result);
        EXPECT_TRUE(doc.is_object());
        EXPECT_FALSE(doc.is_null());
        EXPECT_FALSE(doc.is_array());
        EXPECT_FALSE(doc.empty());

        ObjectRef object = doc;
        EXPECT_EQ(object.size(), 28u);

        EXPECT_FALSE(object["char_i"].get<char>());
        EXPECT_TRUE(static_cast<bool>(object["char_c"].get<char>()));
        EXPECT_FALSE(object["char_s"].get<char>());

        EXPECT_EQ(object["char_i"].as<char>(), ',');
        EXPECT_EQ(object["char_c"].as<char>(), 'A');
        EXPECT_EQ(object["char_s"].as<char>(), 'A');

        EXPECT_EQ(*object["int8-"].get<int8_t>(), -128);
        EXPECT_EQ(object["int8-"].as<int>(), -128);
        EXPECT_TRUE(object["int8-"].is_integral());
        EXPECT_EQ(*object["int8+"].get<int8_t>(), 127);
        EXPECT_EQ(object["int8+"].as<int>(), 127);
        EXPECT_TRUE(object["int8+"].is_integral());
        EXPECT_FALSE(object["int8e"].get<int8_t>());
        EXPECT_EQ(object["int8e"].as<int>(), 128);
        EXPECT_TRUE(object["int8e"].is_integral());
        EXPECT_FALSE(object["int8s"].get<int8_t>());
        EXPECT_EQ(object["int8s"].as<int>(), 127);
        EXPECT_FALSE(object["int8s"].is_integral());

        EXPECT_FALSE(object["uint8-"].get<uint8_t>());
        EXPECT_EQ(object["uint8-"].as<uint32_t>(), -1);
        EXPECT_TRUE(object["uint8-"].is_integral());
        EXPECT_EQ(*object["uint8+"].get<uint8_t>(), 255);
        EXPECT_EQ(object["uint8+"].as<uint32_t>(), 255);
        EXPECT_TRUE(object["uint8+"].is_integral());
        EXPECT_FALSE(object["uint8e"].get<uint8_t>());
        EXPECT_EQ(object["uint8e"].as<uint32_t>(), 256);
        EXPECT_TRUE(object["uint8e"].is_integral());
        EXPECT_FALSE(object["uint8s"].get<uint8_t>());
        EXPECT_EQ(object["uint8s"].as<uint32_t>(), 255);
        EXPECT_FALSE(object["uint8s"].is_integral());

        EXPECT_EQ(*object["int32-"].get<int32_t>(), -2147483648);
        EXPECT_EQ(object["int32-"].as<int>(), -2147483648);
        EXPECT_TRUE(object["int32-"].is_integral());
        EXPECT_EQ(*object["int32+"].get<int32_t>(), 2147483647);
        EXPECT_EQ(object["int32+"].as<int>(), 2147483647);
        EXPECT_TRUE(object["int32+"].is_integral());
        EXPECT_FALSE(object["int32e"].get<int32_t>());
        EXPECT_EQ(object["int32e"].as<int>(), -2147483648); // overflow
        EXPECT_TRUE(object["int32e"].is_integral());
        EXPECT_FALSE(object["int32s"].get<int32_t>());
        EXPECT_EQ(object["int32s"].as<int>(), 2147483647);
        EXPECT_FALSE(object["int32s"].is_integral());

        EXPECT_FALSE(object["uint32-"].get<uint32_t>());
        EXPECT_EQ(object["uint32-"].as<uint32_t>(), -1);
        EXPECT_TRUE(object["uint32-"].is_integral());
        EXPECT_EQ(*object["uint32+"].get<uint32_t>(), 4294967295);
        EXPECT_EQ(object["uint32+"].as<uint32_t>(), 4294967295);
        EXPECT_TRUE(object["uint32+"].is_integral());
        EXPECT_FALSE(object["uint32e"].get<uint32_t>());
        EXPECT_EQ(object["uint32e"].as<uint32_t>(), 0);         // overflow
        EXPECT_TRUE(object["uint32e"].is_integral());
        EXPECT_FALSE(object["uint32s"].get<uint32_t>());
        EXPECT_EQ(object["uint32s"].as<uint32_t>(), 4294967295);
        EXPECT_FALSE(object["uint32s"].is_integral());

        EXPECT_EQ(*object["int64-"].get<int64_t>(),(-9223372036854775807-1));
        EXPECT_EQ(object["int64-"].as<int64_t>(),(-9223372036854775807-1));
        EXPECT_TRUE(object["int64-"].is_integral());
        EXPECT_EQ(*object["int64+"].get<int64_t>(), 9223372036854775807L);
        EXPECT_EQ(object["int64+"].as<int64_t>(), 9223372036854775807L);
        EXPECT_TRUE(object["int64+"].is_integral());
        EXPECT_FALSE(object["int64s"].get<int64_t>());
        EXPECT_EQ(object["int64s"].as<int64_t>(), 9223372036854775807L);
        EXPECT_FALSE(object["int64s"].is_integral());

        EXPECT_FALSE(object["uint64-"].get<uint64_t>());
        EXPECT_EQ(object["uint64-"].as<uint64_t>(), -1);
        EXPECT_TRUE(object["uint64-"].is_integral());
        EXPECT_EQ(*object["uint64+"].get<uint64_t>(), 18446744073709551614u);
        EXPECT_EQ(object["uint64+"].as<uint64_t>(), 18446744073709551614u);
        EXPECT_TRUE(object["uint64+"].is_integral());
        EXPECT_FALSE(object["uint64s"].get<uint64_t>());
        EXPECT_EQ(object["uint64s"].as<uint64_t>(), 18446744073709551614u);
        EXPECT_FALSE(object["uint64s"].is_integral());

        EXPECT_EQ(*object["double-"].get<double>(), -92233720368547.75806);
        EXPECT_FALSE(object["double-"].is_integral());
        EXPECT_EQ(*object["double+"].get<double>(), 922337203685477.5806);
        EXPECT_FALSE(object["double+"].is_integral());
        EXPECT_FALSE(object["doubles"].get<double>());
        EXPECT_FALSE(object["doubles"].is_integral());
        EXPECT_EQ(object["doubles"].as<double>(), 922337203685477.5806);

    }
}

TEST(wrapidjsonTest, json_merge)
{
    Document root;

    std::vector<std::string> a = {"a","b","c","d","e"};
    std::set<int> b = {1,2,3,4,5,6};
    std::map<std::string, int64_t> c = { {"a",1}, {"b",2}, {"c",3} };
    std::map<std::string, std::string> d = { {"a","1"}, {"b","2"}, {"c","3"} };

    root["a"] = a;
    root["b"] = b;
    root["c"] = c;
    root["d"] = d;
    root["e"] = Document(R"({"a1":["a","b","c","d","e"],"b2":[1,2,3,4,5,6],"c2":{"a":1,"b":2,"c":3},"d2":{"a":"1","b":"2","c":"3"}})");

    std::string res;
    root.save_to_buffer(res);
    std::cout << res << std::endl;
    std::string exp = R"({"a":["a","b","c","d","e"],"b":[1,2,3,4,5,6],"c":{"a":1,"b":2,"c":3},"d":{"a":"1","b":"2","c":"3"},"e":{"a1":["a","b","c","d","e"],"b2":[1,2,3,4,5,6],"c2":{"a":1,"b":2,"c":3},"d2":{"a":"1","b":"2","c":"3"}}})";
    EXPECT_EQ(res, exp);
}

TEST(wrapidjsonTest, object_test)
{
    std::string res;
    {
        Document root;

        std::map<std::string, int64_t> a = { {"a",-1}, {"b",-2}, {"c",-3} };
        std::map<std::string, uint32_t> b = { {"a",1}, {"b",2}, {"c",3} };
        std::map<std::string, std::string> c = { {"a","1"}, {"b","2"}, {"c","3"} };
        std::map<std::string, double> d = { {"a", 1.0}, {"b", 2.0}, {"c", 3.0}, {"d", 4.0},{"e", 5.0} };
        std::map<std::string, bool> e = { {"a",false}, {"b",true}, {"c",false} };

        root["a"] = a;
        root["b"] = b;
        root["c"].set_container(c, false);
        root["d"] = d;
        root["e"] = e;

        root.save_to_buffer(res);
        std::cout << res << std::endl;
        root.set_null();
    }

    {
        Document root;
        root.load_from_buffer(res);

        // object get function
        EXPECT_FALSE(static_cast<bool>(root["a"].get_object().get_value<int64_t>("d")));
        EXPECT_TRUE(static_cast<bool>(root["a"].get_object().get_value<int64_t>("a")));
        EXPECT_FALSE(static_cast<bool>(root["b"].get_object().get_value<uint32_t>("d")));
        EXPECT_TRUE(static_cast<bool>(root["b"].get_object().get_value<uint32_t>("a")));
        EXPECT_FALSE(static_cast<bool>(root["c"].get_object().get_value<const char*>("d")));
        EXPECT_TRUE(static_cast<bool>(root["c"].get_object().get_value<const char*>("a")));
        auto re = root["d"].get_object().get_value<double>("d");
        EXPECT_TRUE(static_cast<bool>(re));
        EXPECT_EQ(*re, 4.0);
        auto bl = root["e"].get_object().get_value<bool>("b");
        EXPECT_TRUE(static_cast<bool>(bl));
        EXPECT_TRUE(static_cast<bool>(*bl));

        // object get_with_default function
        EXPECT_EQ(root["a"].get_object().get_value<int64_t>("d", -10), -10);
        EXPECT_EQ(root["b"].get_object().get_value<uint32_t>("d", 10), 10);
        EXPECT_EQ(root["c"].get_object().get_value<std::string>("d", "a"), "a");
        EXPECT_EQ(root["d"].get_object().get_value<double>("d", 10.0), 4.0);
        EXPECT_FALSE(root["e"].get_object().get_value<bool>("a", true));
    }
}


TEST(wrapidjsonTest, array_test)
{
    Document root;

    std::vector<std::string> a = {"a","b","c","d","e"};
    std::vector<int64_t> b = {1,2,3,4,5,6};

    root["a"] = a;
    root["b"] = b;

    auto a_ = root["a"].get_array();
    auto b_ = root["b"].get_array();

    EXPECT_EQ(a, *(a_.get_vector<std::string>()));
    EXPECT_EQ(b, *(b_.get_vector<int64_t>()));

    EXPECT_EQ(a, a_.as_vector<std::string>());
    EXPECT_EQ(b, b_.as_vector<int64_t>());

    a_.push_back(1);
    b_.push_back("7");

    EXPECT_FALSE(a_.get_vector<std::string>());
    EXPECT_FALSE(b_.get_vector<int64_t>());

    a.push_back("1");
    b.push_back(7);

    EXPECT_EQ(a, a_.as_vector<std::string>());
    EXPECT_EQ(b, b_.as_vector<int64_t>());

    a_.push_back("");
    b_.push_back("0");

    EXPECT_FALSE(a_.get_vector<std::string>());
    EXPECT_FALSE(b_.get_vector<int64_t>());

    EXPECT_EQ(a, a_.as_vector<std::string>([](const std::string& v){return not v.empty();}));
    EXPECT_EQ(b, b_.as_vector<int64_t>([](const int64_t& v){return v != 0;}));

    EXPECT_EQ(a_.size(), 7u);
    a.resize(4);
    a_.resize(4);
    EXPECT_EQ(a_.size(), 4u);
    EXPECT_EQ(a, a_.as_vector<std::string>());

    EXPECT_EQ(b_.size(), 8u);
    b.resize(18, 0);
    b_.resize(18, 0);
    EXPECT_EQ(b, b_.as_vector<int64_t>());
    EXPECT_EQ(b_.size(), 18u);
}

TEST(wrapidjsonTest, set_container)
{
    Document root;

    std::vector<std::string> a = {"a","b","c","d","e"};
    std::set<long> b = {1,2,3,4,5,6};
    std::map<std::string, int64_t> c = { {"a",1}, {"b",2}, {"c",3} };
    std::map<std::string, std::string> d = { {"a","1"}, {"b","2"}, {"c","3"} };
    std::set<int64_t> b2 = {1,2,3,4,5,6};

    root["a"] = a;
    root["b"] = b;
    root["c"] = c;
    root["d"] = d;
    root["b2"] = b2;
    std::string exp1 = R"({"a":["a","b","c","d","e"],"b":[1,2,3,4,5,6],"c":{"a":1,"b":2,"c":3},"d":{"a":"1","b":"2","c":"3"},"b2":[1,2,3,4,5,6]})";
    std::string doc1;
    root.save_to_buffer(doc1);
    EXPECT_EQ(exp1, doc1);

    std::cout << doc1 << std::endl;

    root["a1"].set_container(a);
    root["b1"].set_container(b);
    root["c1"].set_container(c);
    root["d1"].set_container(d);
    root["b12"].set_container(b2);

    std::string exp2 = R"({"a":["a","b","c","d","e"],"b":[1,2,3,4,5,6],"c":{"a":1,"b":2,"c":3},"d":{"a":"1","b":"2","c":"3"},"b2":[1,2,3,4,5,6],"a1":["a","b","c","d","e"],"b1":[1,2,3,4,5,6],"c1":{"a":1,"b":2,"c":3},"d1":{"a":"1","b":"2","c":"3"},"b12":[1,2,3,4,5,6]})";
    std::string doc2;
    root.save_to_buffer(doc2);
    EXPECT_EQ(exp2, doc2);
    std::cout << doc2 << std::endl;

    root["a2"].set_container(a, false);  // string not copy
    root["b2"].set_container(b, false);  // just copy ( number )
    root["c2"].set_container(c, false);  // string not copy
    root["d2"].set_container(d, false);  // string not copy
    root["b22"].set_container(b2, false);  // just copy ( number )

    std::string exp3 = R"({"a":["a","b","c","d","e"],"b":[1,2,3,4,5,6],"c":{"a":1,"b":2,"c":3},"d":{"a":"1","b":"2","c":"3"},"b2":[1,2,3,4,5,6],"a1":["a","b","c","d","e"],"b1":[1,2,3,4,5,6],"c1":{"a":1,"b":2,"c":3},"d1":{"a":"1","b":"2","c":"3"},"b12":[1,2,3,4,5,6],"a2":["a","b","c","d","e"],"c2":{"a":1,"b":2,"c":3},"d2":{"a":"1","b":"2","c":"3"},"b22":[1,2,3,4,5,6]})";
    std::string doc3;
    root.save_to_buffer(doc3);
    EXPECT_EQ(exp3, doc3);
    std::cout << doc3 << std::endl;

    std::list<bool> e = {false, false, false, true, true, true};
    std::unordered_set<unsigned long> f = {1,2,3,4,5,6,7,8,9,10};
    std::unordered_map<std::string, uint64_t> g = { {"a", 1}, {"b", 2}, {"c", 3}, {"d", 4},{"e", 5} };
    std::unordered_set<uint64_t> f2 = {1,2,3,4,5,6,7,8,9,10};

    root["e"] = e;
    root["f"] = f;
    root["g"] = g;
    root["f2"] = f2;

    EXPECT_EQ(root.size(), 18u);

    root["e1"].set_container(e);
    root["f1"].set_container(f);
    root["g1"].set_container(g);
    root["f12"].set_container(f2);

    EXPECT_EQ(root.size(), 22u);

    root["e2"].set_container(e, false);  // just copy
    root["f2"].set_container(f, false);  // just copy
    root["g2"].set_container(g, false);  // string key not copy
    root["f22"].set_container(f2, false);  // just copy

    EXPECT_EQ(root.size(), 25u);
}

TEST(wrapidjsonTest, find_test)
{
    Document doc;
    auto root = doc.get_object();

    root["TEST2"] = "11111";
    root["TEST4"] = "22222";

    auto itr = root.find_any(std::vector<std::string>{"TEST", "TEST1", "TEST2", "TEST3", "TEST4"});
    EXPECT_NE(itr, root.end());
    EXPECT_EQ(itr->name.as<std::string>(), "TEST2");
    EXPECT_EQ(itr->value.as<std::string>(), "11111");
    EXPECT_EQ(itr->value.as<int>(), 11111);

    itr = root.find_any(std::set<std::string>{"TEST", "TEST4", "TEST1", "TEST2", "TEST3"});
    EXPECT_NE(itr, root.end());
    EXPECT_EQ(itr->name.as<std::string>(), "TEST2");
    EXPECT_EQ(itr->value.as<std::string>(), "11111");
    EXPECT_EQ(itr->value.as<int>(), 11111);

    itr = root.find_any(std::vector<std::string>{"TEST", "TEST4", "TEST1", "TEST2", "TEST3"});
    EXPECT_NE(itr, root.end());
    EXPECT_EQ(itr->name.as<std::string>(), "TEST4");
    EXPECT_EQ(itr->value.as<std::string>(), "22222");
    EXPECT_EQ(itr->value.as<int>(), 22222);

    itr = root.find_any(std::set<std::string>{"TEST", "TEST1", "TEST3"});
    EXPECT_EQ(itr, root.end());

    EXPECT_TRUE(root.find_all(std::vector<std::string>{"TEST2"}));
    EXPECT_FALSE(root.find_all(std::vector<std::string>{"TEST2", "TEST3"}));
}
