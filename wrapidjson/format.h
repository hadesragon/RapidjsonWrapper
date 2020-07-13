#ifndef WRAPIDJSON_FORMAT_H_
#define WRAPIDJSON_FORMAT_H_

#include <cstdio>
#include <cstdlib>
#include <string>

namespace wrapidjson {
namespace detail {

namespace format_helper
{
    template <class T> inline T cast(T v) { return v; } // POD type
    inline const char *cast(const std::string& v) { return v.c_str(); }
};

inline std::string format(const std::string& format) { return format; }

template<typename ... Args>
inline std::string format(const std::string& format, Args&& ... args)
{
    size_t size = std::snprintf(nullptr, 0, format.c_str(), format_helper::cast(std::forward<Args>(args))...) + 1;
    std::unique_ptr<char[]> buffer(new char[size]);
    std::snprintf(buffer.get(), size, format.c_str(), format_helper::cast(std::forward<Args>(args))...);
    return std::string(buffer.get(), buffer.get() + size - 1);
}

} // namespace detail
} // namespace wrapidjson

#endif // WRAPIDJSON_FORMAT_H_
