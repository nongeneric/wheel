#pragma once

#include "ITetris.h"

#include <memory>
#include <functional>

std::unique_ptr<ITetris> makeTetris(int hor, int vert, std::function<PieceType::t()> generator);
