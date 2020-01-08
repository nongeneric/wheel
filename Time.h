#pragma once

#include <boost/chrono.hpp>

namespace chrono = boost::chrono;
using fseconds = chrono::duration<float>;

void spinSleep(chrono::steady_clock::duration duration);
