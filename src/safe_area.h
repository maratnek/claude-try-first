#pragma once

struct SafeArea {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

SafeArea GetSafeArea();
