#include "progress.h"
#include "settings.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

ProgressData LoadProgress() {
    const char *text = emscripten_run_script_string(
        "(function(){try{return window.localStorage.getItem('flightgame.progress')||'';}catch(e){return '';}})()");
    return text ? ParseProgress(text) : ProgressData{};
}

void SaveProgress(const ProgressData &data) {
    std::string text = SerializeProgress(data);
    EM_ASM({
        try { window.localStorage.setItem('flightgame.progress', UTF8ToString($0)); } catch (e) {}
    }, text.c_str());
}

#elif defined(SETTINGS_MOBILE_OR_WEB)

ProgressData LoadProgress() { return ProgressData{}; }
void SaveProgress(const ProgressData &) {}

#else
#include "raylib.h"

namespace {
const char *ProgressPath() { return TextFormat("%sprogress.cfg", GetApplicationDirectory()); }
}  // namespace

ProgressData LoadProgress() {
    if (!FileExists(ProgressPath())) return ProgressData{};
    char *text = LoadFileText(ProgressPath());
    if (!text) return ProgressData{};
    ProgressData data = ParseProgress(text);
    UnloadFileText(text);
    return data;
}

void SaveProgress(const ProgressData &data) {
    std::string text = SerializeProgress(data);
    SaveFileText(ProgressPath(), const_cast<char *>(text.c_str()));
}
#endif
