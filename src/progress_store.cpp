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
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace {
// The app sandbox's $HOME/Documents is writable and survives app restarts; the .app bundle itself is read-only.
std::string ProgressPath() {
    const char *home = std::getenv("HOME");
    return home ? std::string(home) + "/Documents/progress.cfg" : std::string();
}
}  // namespace

ProgressData LoadProgress() {
    std::string path = ProgressPath();
    if (path.empty()) return ProgressData{};
    std::ifstream in(path);
    if (!in) return ProgressData{};
    std::stringstream text;
    text << in.rdbuf();
    return ParseProgress(text.str());
}

void SaveProgress(const ProgressData &data) {
    std::string path = ProgressPath();
    if (path.empty()) return;
    std::ofstream out(path, std::ios::trunc);
    if (out) out << SerializeProgress(data);
}

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
