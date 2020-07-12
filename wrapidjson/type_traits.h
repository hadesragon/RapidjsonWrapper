#ifndef WRAPIDJSON_TYPE_TRAITS_H_
#define WRAPIDJSON_TYPE_TRAITS_H_

#include <string>
#include <map>
#include <unordered_map>
#include <type_traits>

namespace wrapidjson {
namespace detail {

////////////////////////////////////////////////////////////////////////////////
// void_t
////////////////////////////////////////////////////////////////////////////////
template<typename... Ts>
struct make_void { typedef void type;};

template<typename... Ts>
using void_t = typename make_void<Ts...>::type;

template< bool B, class T = void >
using enable_if_t = typename std::enable_if<B,T>::type;

////////////////////////////////////////////////////////////////////////////////
// is_string
////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct is_string : std::false_type {};

template <typename ...TArgs>
struct is_string<std::basic_string<TArgs...> > : std::true_type {};

////////////////////////////////////////////////////////////////////////////////
// is_const_char
////////////////////////////////////////////////////////////////////////////////
template < typename T >
struct is_const_char : std::false_type {};
template < >
struct is_const_char < const char * > : std::true_type {};

////////////////////////////////////////////////////////////////////////////////
// is_map
////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct is_map : std::false_type {};

template<typename ...TArgs>
struct is_map<std::map<TArgs...>> : std::true_type {};

template<typename ...TArgs>
struct is_map<const std::map<TArgs...>> : std::true_type {};

template<typename ...TArgs>
struct is_map<std::unordered_map<TArgs...>> : std::true_type {};

template<typename ...TArgs>
struct is_map<const std::unordered_map<TArgs...>> : std::true_type {};

////////////////////////////////////////////////////////////////////////////////
// is_iterable
////////////////////////////////////////////////////////////////////////////////
template <typename T, typename = void>
struct is_iterable : std::false_type {};

template <typename T>
struct is_iterable<T, void_t<decltype(std::declval<T>().begin()),
                             decltype(std::declval<T>().end())>>
    : std::true_type {};

////////////////////////////////////////////////////////////////////////////////
// enable_if_*_t
////////////////////////////////////////////////////////////////////////////////
template<typename T>
using enable_if_char_t  = enable_if_t<std::is_same<T, char>::value, T>;
template<typename T>
using enable_if_bool_t  = enable_if_t<std::is_same<T, bool>::value, T>;
template<typename T>
using enable_if_cptr_t  = enable_if_t<is_const_char<T>::value, T>;
template<typename T>
using enable_if_str_t   = enable_if_t<is_string<T>::value, T>;
template<typename T>
using enable_if_num_t   = enable_if_t<std::is_arithmetic<T>::value &&
                            !std::is_same<T, char>::value, T>;
template<typename T>
using enable_if_int_t   = enable_if_t<std::is_integral<T>::value &&
                            !std::is_same<T, char>::value &&
                            std::is_signed<T>::value && sizeof(T) < 8, T>;
template<typename T>
using enable_if_int64_t = enable_if_t<std::is_integral<T>::value &&
                            std::is_signed<T>::value && sizeof(T) == 8, T>;
template<typename T>
using enable_if_signed_t = enable_if_t<std::is_integral<T>::value &&
                            !std::is_same<T, char>::value &&
                            std::is_signed<T>::value, T>;
template<typename T>
using enable_if_uint_t  = enable_if_t<std::is_integral<T>::value &&
                            !std::is_same<T, bool>::value &&
                            std::is_unsigned<T>::value && sizeof(T) < 8, T>;
template<typename T>
using enable_if_uint64_t = enable_if_t<std::is_integral<T>::value &&
                            std::is_unsigned<T>::value && sizeof(T) == 8, T>;
template<typename T>
using enable_if_unsigned_t = enable_if_t<std::is_integral<T>::value &&
                            !std::is_same<T, bool>::value &&
                            std::is_unsigned<T>::value, T>;

template<typename T>
using enable_if_float_t = enable_if_t<std::is_floating_point<T>::value, T>;

////////////////////////////////////////////////////////////////////////////////
// enable_if_sequence
////////////////////////////////////////////////////////////////////////////////
template<typename T, template <typename...> class Container, typename...Args>
using enable_if_sequence_t = enable_if_t<
    is_iterable<Container<T, Args...>>::value &&
    !is_map<Container<T, Args...>>::value, Container<T, Args...>>;

////////////////////////////////////////////////////////////////////////////////
// enable_if_l_sequence  ( long != int64_t )
////////////////////////////////////////////////////////////////////////////////
template<template <typename...> class Container, typename...Args>
using enable_if_l_sequence_t = enable_if_t<
    !std::is_same<int64_t, long>::value &&
    is_iterable<Container<long, Args...>>::value &&
    !is_map<Container<long, Args...>>::value, Container<long, Args...>>;

////////////////////////////////////////////////////////////////////////////////
// enable_if_ul_sequence  ( unsigned long != uint64_t )
////////////////////////////////////////////////////////////////////////////////
template<template <typename...> class Container, typename...Args>
using enable_if_ul_sequence_t = enable_if_t<
    !std::is_same<uint64_t, unsigned long>::value &&
    is_iterable<Container<unsigned long, Args...>>::value &&
    !is_map<Container<unsigned long, Args...>>::value, Container<unsigned long, Args...>>;

////////////////////////////////////////////////////////////////////////////////
// enable_if_ll_sequence ( long long != int64_t )
////////////////////////////////////////////////////////////////////////////////
template<template <typename...> class Container, typename...Args>
using enable_if_ll_sequence_t = enable_if_t<
    !std::is_same<int64_t, long long>::value &&
    is_iterable<Container<long long, Args...>>::value &&
    !is_map<Container<long long, Args...>>::value, Container<long long, Args...>>;

////////////////////////////////////////////////////////////////////////////////
// enable_if_ll_sequence ( unsigned long long != uint64_t )
////////////////////////////////////////////////////////////////////////////////
template<template <typename...> class Container, typename...Args>
using enable_if_ull_sequence_t = enable_if_t<
    !std::is_same<uint64_t, unsigned long long>::value &&
    is_iterable<Container<unsigned long long, Args...>>::value &&
    !is_map<Container<unsigned long long, Args...>>::value, Container<unsigned long long, Args...>>;

////////////////////////////////////////////////////////////////////////////////
// enable_if_str_map
////////////////////////////////////////////////////////////////////////////////
template<typename T, template <typename...> class Container, typename...Args>
using enable_if_strmap_t = enable_if_t<
    is_map<Container<std::string, T, Args...>>::value
    , Container<std::string, T, Args...>>;

////////////////////////////////////////////////////////////////////////////////
// enable_if_str_l_map ( long != int64_t )
////////////////////////////////////////////////////////////////////////////////
template<template <typename...> class Container, typename...Args>
using enable_if_str_l_map_t = enable_if_t<
    !std::is_same<int64_t, long>::value &&
    is_map<Container<std::string, long, Args...>>::value
    , Container<std::string, long, Args...>>;

////////////////////////////////////////////////////////////////////////////////
// enable_if_str_ul_map ( unsigned long != uint64_t )
////////////////////////////////////////////////////////////////////////////////
template<template <typename...> class Container, typename...Args>
using enable_if_str_ul_map_t = enable_if_t<
    !std::is_same<uint64_t, unsigned long>::value &&
    is_map<Container<std::string, unsigned long, Args...>>::value
    , Container<std::string, unsigned long, Args...>>;

////////////////////////////////////////////////////////////////////////////////
// enable_if_str_ll_map ( long long != int64_t )
////////////////////////////////////////////////////////////////////////////////
template<template <typename...> class Container, typename...Args>
using enable_if_str_ll_map_t = enable_if_t<
    !std::is_same<int64_t, long long>::value &&
    is_map<Container<std::string, long long, Args...>>::value
    , Container<std::string, long long, Args...>>;

////////////////////////////////////////////////////////////////////////////////
// enable_if_str_ull_map ( long long != int64_t )
////////////////////////////////////////////////////////////////////////////////
template<template <typename...> class Container, typename...Args>
using enable_if_str_ull_map_t = enable_if_t<
    !std::is_same<uint64_t, unsigned long long>::value &&
    is_map<Container<std::string, unsigned long long, Args...>>::value
    , Container<std::string, unsigned long long, Args...>>;

} // namespace detail
} // namespace wrapidjson

#endif // WRAPIDJSON_TYPE_TRAITS_H_
