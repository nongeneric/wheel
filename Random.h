#pragma once

#include <random>
#include <ctime>
#include <functional>

template <typename T>
struct Distrib { static_assert(sizeof(T) && false, "bad type"); };
template <>
struct Distrib<float> {
    typedef typename std::uniform_real_distribution<float> type;
};
template <>
struct Distrib<unsigned> {
    typedef typename std::uniform_int_distribution<unsigned> type;
};
template <>
struct Distrib<int> {
    typedef typename std::uniform_int_distribution<int> type;
};

template <typename T>
class Random {
    typename Distrib<T>::type _distribution;
    std::mt19937 _engine;
    std::function<T()> _rndfunc;
public:
    Random(T from, T to)
        : _distribution(from, to)
    {
        _engine.seed(time(NULL));
        _rndfunc = std::bind(_distribution, _engine);
    }
    T operator()() {
        return _rndfunc();
    }
};
