#pragma once

#include <algorithm>

namespace rstd {
    template<typename T, typename Cont>
    void fill(Cont& cont, T val) {
        std::fill(std::begin(cont), std::end(cont), val);
    }
    template<typename Pred, typename Cont>
    bool any_of(Cont& cont, Pred pred) {
        return std::any_of(std::begin(cont), std::end(cont), pred);
    }
    template<typename Pred, typename Cont>
    bool all_of(Cont& cont, Pred pred) {
        return std::all_of(std::begin(cont), std::end(cont), pred);
    }
    template<typename Pred, typename Cont>
    typename Cont::iterator stable_partition(Cont& cont, Pred pred) {
        return std::stable_partition(std::begin(cont), std::end(cont), pred);
    }
    template<typename Cont, typename Iter>
    void copy(Cont& cont, Iter out) {
        std::copy(std::begin(cont), std::end(cont), out);
    }
    template<typename Cont>
    void reverse(Cont& cont) {
        std::reverse(std::begin(cont), std::end(cont));
    }
}
