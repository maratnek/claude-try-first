#pragma once
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(__EMSCRIPTEN__) || (defined(__APPLE__) && TARGET_OS_IPHONE)
#define SETTINGS_MOBILE_OR_WEB 1
#endif

enum class GraphicsPreset { Low, Medium, High };

struct GraphicsSettings {
    GraphicsPreset preset = GraphicsPreset::High;
    bool distanceMarkers = true;
    bool obstacleWires = true;
    bool characters = true;
    bool propBlur = true;
    bool blobShadow = true;
    bool terrainColors = true;
    float scatterDensity = 1.0f;
    int smokePuffs = 64;
    int debrisPieces = 7;
};

void ApplyGraphicsPreset(GraphicsSettings &settings, GraphicsPreset preset);
void CycleGraphicsPreset(GraphicsSettings &settings);
const char *GraphicsPresetName(const GraphicsSettings &settings);

// Low on Web/mobile, High on desktop; on desktop a saved choice overrides that.
void InitGraphicsSettings(GraphicsSettings &settings);
void SaveGraphicsSettings(const GraphicsSettings &settings);
