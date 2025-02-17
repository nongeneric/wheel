#pragma once

#include <ctime>
#include <functional>
#include <random>

template <typename T> struct Distrib {
    typedef typename std::uniform_int_distribution<unsigned> type;
};

template <> struct Distrib<float> {
    typedef typename std::uniform_real_distribution<float> type;
};

template <typename T> class Random {
    typename Distrib<T>::type _distribution;
    std::mt19937 _engine;

public:
    Random(T from, T to) : _distribution(from, to) {
        _engine.seed(time(NULL));
    }
    T operator()() {
        return static_cast<T>(_distribution(_engine));
    }
};
