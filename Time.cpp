#include "Time.h"

void spinSleep(chrono::steady_clock::duration duration)
{
    auto past = chrono::steady_clock::now();
    while (chrono::steady_clock::now() - past < duration) ;
}
