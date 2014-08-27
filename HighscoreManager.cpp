#include "HighscoreManager.h"

#include "rstd.h"
#include <functional>

namespace {
using namespace std::placeholders;

bool isLeftWorse(std::function<int(HighscoreRecord)> getter, HighscoreRecord left, HighscoreRecord right) {
    bool rightHasBetterLevel = left.initialLevel < right.initialLevel;
    return getter(left) < getter(right) ||
           ((getter(left) == getter(right)) && rightHasBetterLevel);
}

void sort(std::vector<HighscoreRecord>& known, std::function<int(HighscoreRecord)> getter) {
    std::sort(begin(known), end(known), std::bind(isLeftWorse, getter, _1, _2));
    rstd::reverse(known);
}

}

int updateHighscores(HighscoreRecord newRecord, std::vector<HighscoreRecord>& known, bool lines) {
    auto getter = [lines](HighscoreRecord r) {
        return lines ? r.lines : r.score;
    };
    sort(known, getter);
    int pos = -1;
    if (known.size() < 6 || (!known.empty() && isLeftWorse(getter, known.back(), newRecord))) {
        known.push_back(newRecord);
        sort(known, getter);
        pos = std::distance(begin(known), std::find(begin(known), end(known), newRecord));
    }
    if (known.size() > 6) {
        known.resize(6);
    }
    return pos;
}
