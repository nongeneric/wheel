#pragma once

#include <vector>
#include "Config.h"

int updateHighscores(HighscoreRecord newRecord, std::vector<HighscoreRecord>& known, bool lines);
