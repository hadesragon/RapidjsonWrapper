#ifndef TYPE_TRAITS_H_
#define TYPE_TRAITS_H_

#include <string>
#include <map>
#include <unordered_map>
#include <type_traits>

namespace Json {
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

} // namespace detail
} // namespace Json

#endif // TYPE_TRAITS_H_
