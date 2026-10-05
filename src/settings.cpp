#include "settings.h"
#include "raylib.h"
#include <cstdlib>

namespace {

#ifndef SETTINGS_MOBILE_OR_WEB
const char *SettingsPath() {
    return TextFormat("%sgraphics.cfg", GetApplicationDirectory());
}
#endif

}  // namespace

void ApplyGraphicsPreset(GraphicsSettings &settings, GraphicsPreset preset) {
    settings.preset = preset;
    bool low = preset == GraphicsPreset::Low;
    bool high = preset == GraphicsPreset::High;
    settings.distanceMarkers = !low;
    settings.characters = !low;
    settings.propBlur = !low;
    settings.pilotHead = !low;
    settings.damageVisuals = !low;
    settings.blobShadow = true;
    settings.terrainColors = !low;
    settings.obstacleWires = high;
    settings.scatterDensity = low ? 0.0f : (high ? 1.0f : 0.35f);
    settings.cloudCount = low ? 0 : (high ? 30 : 12);
    settings.smokePuffs = low ? 0 : (high ? 64 : 24);
    settings.debrisPieces = low ? 4 : (high ? 12 : 6);
    settings.speedStreaks = low ? 0 : (high ? 40 : 16);
    settings.rainDrops = low ? 0 : (high ? 200 : 80);
    settings.snowFlakes = high ? 200 : 0;
}

void CycleGraphicsPreset(GraphicsSettings &settings) {
    int next = (static_cast<int>(settings.preset) + 1) % 3;
    ApplyGraphicsPreset(settings, static_cast<GraphicsPreset>(next));
}

const char *GraphicsPresetName(const GraphicsSettings &settings) {
    switch (settings.preset) {
        case GraphicsPreset::Low: return "Low";
        case GraphicsPreset::Medium: return "Medium";
        default: return "High";
    }
}

void InitGraphicsSettings(GraphicsSettings &settings) {
#ifdef SETTINGS_MOBILE_OR_WEB
    ApplyGraphicsPreset(settings, GraphicsPreset::Low);
#else
    ApplyGraphicsPreset(settings, GraphicsPreset::High);
    if (FileExists(SettingsPath())) {
        char *text = LoadFileText(SettingsPath());
        if (text) {
            int value = atoi(text);
            if (value >= 0 && value <= 2) ApplyGraphicsPreset(settings, static_cast<GraphicsPreset>(value));
            UnloadFileText(text);
        }
    }
#endif
}

void SaveGraphicsSettings(const GraphicsSettings &settings) {
#ifndef SETTINGS_MOBILE_OR_WEB
    SaveFileText(SettingsPath(), const_cast<char *>(TextFormat("%d", static_cast<int>(settings.preset))));
#else
    (void)settings;
#endif
}
