#include "progress.h"
#include "settings.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <cstdlib>

ProgressData LoadProgress() {
    char *text = static_cast<char *>(EM_ASM_PTR({
        var s = null;
        try { s = window.localStorage.getItem('flightgame.progress'); } catch (e) {}
        if (s === null) return 0;
        var n = lengthBytesUTF8(s) + 1;
        var p = _malloc(n);
        stringToUTF8(s, p, n);
        return p;
    }));
    if (!text) return ProgressData{};
    ProgressData data = ParseProgress(text);
    free(text);
    return data;
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
