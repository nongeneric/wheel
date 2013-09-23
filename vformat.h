#include <boost/format.hpp>

template <typename FormatType>
void appendFormat(FormatType&) { }

template <typename FormatType, typename T, typename... Ts>
void appendFormat(FormatType& fmt, T arg, Ts... args) {
    appendFormat(fmt % arg, args...);
}

template <typename StrType, typename... Ts>
std::string vformat(StrType formatString, Ts... ts) {
    boost::format fmt(formatString);
    appendFormat(fmt, ts...);
    return str(fmt);
}
