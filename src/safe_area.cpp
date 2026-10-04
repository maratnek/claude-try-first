#include "safe_area.h"
#include "raylib.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IOS
#define FLIGHT_SAFE_AREA_SDL 1
// Not "SDL.h": raylib's SDL platform links SDL2, whose headers are installed under SDL2/ or flat depending on the target.
#include <SDL_video.h>
#endif
#endif

SafeArea GetSafeArea() {
    SafeArea area;
#ifdef FLIGHT_SAFE_AREA_SDL
    SDL_Rect full, usable;
    if (SDL_GetDisplayBounds(0, &full) != 0 || SDL_GetDisplayUsableBounds(0, &usable) != 0) return area;
    if (full.w <= 0 || full.h <= 0) return area;
    // On iOS, SDL reports usable bounds as the display minus the safe-area insets, in points.
    float sx = (float)GetScreenWidth() / (float)full.w;
    float sy = (float)GetScreenHeight() / (float)full.h;
    area.left = (float)(usable.x - full.x) * sx;
    area.top = (float)(usable.y - full.y) * sy;
    area.right = (float)((full.x + full.w) - (usable.x + usable.w)) * sx;
    area.bottom = (float)((full.y + full.h) - (usable.y + usable.h)) * sy;
#endif
    return area;
}
