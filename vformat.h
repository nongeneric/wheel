#pragma once

#include <format>

template<typename... Args>
inline auto vformat(std::string_view format, Args&&... args)
{
    return std::vformat(format, std::make_format_args(args...));
}
