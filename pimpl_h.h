#ifndef PIMPL_H_H
#define PIMPL_H_H

#include <memory>

template<typename T>
class pimpl {
private:
    std::unique_ptr<T> m;
public:
    pimpl();
    pimpl(pimpl<T>&& other) : m(std::move(other.m)) { }
    template<typename ...Args> pimpl( Args&& ... );
    ~pimpl();
    T* operator->();
    T& operator*();
};

#endif
