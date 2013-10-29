#include "MathTools.h"

bool epseq(float a, float b) {
    return std::abs(a - b) < 1e-4f;
}

bool epseq(glm::vec2 a, glm::vec2 b) {
    return epseq(a.x, b.x) && epseq(a.y, b.y);
}
