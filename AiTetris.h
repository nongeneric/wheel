#pragma once

#include "ITetris.h"

#include <memory>
#include <functional>

std::unique_ptr<ITetris> makeAiTetris();
