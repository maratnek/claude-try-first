#pragma once

enum class GraphicsPreset { Low, Medium, High };

struct GraphicsSettings {
    GraphicsPreset preset = GraphicsPreset::High;
    bool distanceMarkers = true;
    bool obstacleWires = true;
    bool characters = true;
    bool propBlur = true;
};

void ApplyGraphicsPreset(GraphicsSettings &settings, GraphicsPreset preset);
void CycleGraphicsPreset(GraphicsSettings &settings);
const char *GraphicsPresetName(const GraphicsSettings &settings);

// Low on Web/mobile, High on desktop; on desktop a saved choice overrides that.
void InitGraphicsSettings(GraphicsSettings &settings);
void SaveGraphicsSettings(const GraphicsSettings &settings);
