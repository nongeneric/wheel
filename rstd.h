#pragma once

#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>

namespace rstd {
    template<typename Cont>
    typename Cont::value_type accumulate(Cont& cont, typename Cont::value_type initial) {
        return std::accumulate(begin(cont), end(cont), initial);
    }
    template<typename T, typename R>
    std::vector<R> fmap(std::vector<T> const& a, std::function<R(const T&)> f) {
        std::vector<R> res;
        for (const T& t : a) {
            res.push_back(f(t));
        }
        return res;
    }
}
