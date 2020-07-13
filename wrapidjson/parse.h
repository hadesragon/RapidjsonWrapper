#ifndef WRAPIDJSON_PARSE_H_
#define WRAPIDJSON_PARSE_H_

#include <string>
#include <sstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cerrno>

#include "optional.hpp"
#include "type_traits.h"

namespace wrapidjson {
namespace detail {

template<typename T>
using optional = nonstd::optional<T>;

template<typename T, enable_if_bool_t<T>* = nullptr>
inline optional<bool> parse(const std::string& value) {
    optional<bool> res;
    if( not value.empty() ) {
        std::string lower_value_string = value;
        std::transform(lower_value_string.begin(),
            lower_value_string.end(),
            lower_value_string.begin(),
            ::tolower);
        if (lower_value_string == "true" or lower_value_string == "false") {
            res = ("true" == lower_value_string);
        }
    }
    return res;
}

template<typename T, enable_if_signed_t<T>* = nullptr>
inline optional<T> parse(const std::string& value) {
    optional<T> res;
    if ( not value.empty() ) {
        const char* const value_c_str = value.c_str();
        char* end;
        errno = 0;
        const long lvalue = strtol(value_c_str, &end, 10);
        if ( end != value_c_str and '\0' == *end
                and lvalue <= std::numeric_limits<T>::max()
                and lvalue >= std::numeric_limits<T>::min()
                and errno != ERANGE)    // overflow check
        {
            res = static_cast<T>(lvalue);
        }
    }
    return res;
}

template<typename T, enable_if_unsigned_t<T>* = nullptr>
inline optional<T> parse(const std::string& value) {
    optional<T> res;
    if ( not value.empty() and value.find("-") == std::string::npos ) {
        const char* const value_c_str = value.c_str();
        char* end;
        errno = 0;
        const unsigned long lvalue = strtoul(value_c_str, &end, 10);
        if ( end != value_c_str and '\0' == *end
                and lvalue <= std::numeric_limits<T>::max()
                and lvalue >= std::numeric_limits<T>::min()
                and errno != ERANGE)    // overflow check
        {
            res = static_cast<T>(lvalue);
        }
    }
    return res;
}

template<typename T, enable_if_float_t<T>* = nullptr>
inline optional<T> parse(const std::string& value) {
    optional<T> res;
    if ( not value.empty() ) {
        const char* const value_c_str = value.c_str();
        char* end;
        errno = 0;
        const double dvalue = strtod(value_c_str, &end);
        if (end != value_c_str and '\0' == *end and errno != ERANGE) {
            res = static_cast<T>(dvalue);
        }
    }
    return res;
}

template <typename T>
inline T parse(const std::string& value, const T& default_value)
{
    optional<T> res = parse<T>(value);
    if ( not res ) {
        res = default_value;
    }
    return *res;
}

} // namespace detail
} // namespace wrapidjson

#endif // WRAPIDJSON_PARSE_H_
