#pragma once

#include "IWidget.h"
#include "../Text.h"
#include <memory>

class CrispBitmap : public IWidget {
    struct impl;
    std::unique_ptr<impl> m;
public:
    CrispBitmap();
    CrispBitmap(CrispBitmap&&);
    ~CrispBitmap();
    void setBitmap(Bitmap bitmap);
    void setColor(glm::vec3 color);
    void animate(fseconds) override;
    void draw() override;
    void measure(glm::vec2, glm::vec2 framebuffer) override;
    void arrange(glm::vec2 pos, glm::vec2) override;
    glm::vec2 desired() override;
    void setTransform(glm::mat4 transform) override;
};
