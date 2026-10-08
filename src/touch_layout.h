#pragma once

struct TouchBox {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

struct SettingsButtons {
    TouchBox gfx;
    TouchBox volDown;
    TouchBox volUp;
};

// The web Fullscreen button sits at top 8 / right 8 and is about 35 px tall, so these start 48 px below the safe-area top.
SettingsButtons LayoutSettingsButtons(float screenWidth, float safeRight, float safeTop);

bool TouchBoxContains(const TouchBox &box, float px, float py);
