#include "render/plane_look.h"
#include "render/sun.h"
#include "raymath.h"
#include "rlgl.h"

namespace {

#if defined(PLATFORM_WEB) || defined(__EMSCRIPTEN__) || (defined(__APPLE__) && TARGET_OS_IPHONE)
#define GLSL_HEAD "#version 100\nprecision mediump float;\n"
#define VS_IN "attribute"
#define VS_OUT "varying"
#define FS_IN "varying"
#define FS_OUT_DECL ""
#define FS_OUT "gl_FragColor"
#define TEX "texture2D"
#else
#define GLSL_HEAD "#version 330\n"
#define VS_IN "in"
#define VS_OUT "out"
#define FS_IN "in"
#define FS_OUT_DECL "out vec4 finalColor;\n"
#define FS_OUT "finalColor"
#define TEX "texture"
#endif

const char *kVertex = GLSL_HEAD
    VS_IN " vec3 vertexPosition;\n"
    VS_IN " vec2 vertexTexCoord;\n"
    VS_IN " vec3 vertexNormal;\n"
    "uniform mat4 mvp;\n"
    VS_OUT " vec2 fragTexCoord;\n"
    VS_OUT " vec3 fragNormal;\n"
    "void main() {\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    fragNormal = vertexNormal;\n"
    "    gl_Position = mvp*vec4(vertexPosition, 1.0);\n"
    "}\n";

// Three flat bands (lit / shade / core shadow), warm in the light and cool in the shade; no smooth gradient on purpose.
const char *kFragment = GLSL_HEAD
    FS_IN " vec2 fragTexCoord;\n"
    FS_IN " vec3 fragNormal;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "uniform vec3 lightDir;\n"
    FS_OUT_DECL
    "void main() {\n"
    "    vec4 base = " TEX "(texture0, fragTexCoord)*colDiffuse;\n"
    "    float d = dot(normalize(fragNormal), lightDir);\n"
    "    vec3 tint = vec3(1.08, 1.0, 0.88);\n"
    "    if (d < 0.4) tint = vec3(0.72, 0.72, 0.86);\n"
    "    if (d < -0.3) tint = vec3(0.50, 0.52, 0.70);\n"
    "    " FS_OUT " = vec4(base.rgb*tint, base.a);\n"
    "}\n";

}  // namespace

void LoadPlaneLook(PlaneLook &look) {
    look.shader = LoadShaderFromMemory(kVertex, kFragment);
    look.loaded = look.shader.id > 0 && look.shader.id != rlGetShaderIdDefault();
    if (!look.loaded) return;
    look.lightLoc = GetShaderLocation(look.shader, "lightDir");
    Vector3 sun = Vector3Normalize(kSunDirection);
    SetShaderValue(look.shader, look.lightLoc, &sun, SHADER_UNIFORM_VEC3);
}

void UnloadPlaneLook(PlaneLook &look, Model &model) {
    GraphicsSettings off;
    off.planeShading = false;
    ApplyPlaneLook(look, model, off);
    if (look.loaded) UnloadShader(look.shader);
    look.loaded = false;
}

void ApplyPlaneLook(PlaneLook &look, Model &model, const GraphicsSettings &gfx) {
    Shader chosen = (look.loaded && gfx.planeShading) ? look.shader : Shader{rlGetShaderIdDefault(), rlGetShaderLocsDefault()};
    for (int i = 0; i < model.materialCount; i++) model.materials[i].shader = chosen;
}

void UpdatePlaneLight(PlaneLook &look, float yawDeg, float pitchDeg, float rollDeg) {
    if (!look.loaded) return;
    Vector3 v = Vector3Normalize(kSunDirection);
    v = Vector3RotateByAxisAngle(v, {0.0f, 1.0f, 0.0f}, -yawDeg * DEG2RAD);
    v = Vector3RotateByAxisAngle(v, {1.0f, 0.0f, 0.0f}, pitchDeg * DEG2RAD);
    v = Vector3RotateByAxisAngle(v, {0.0f, 0.0f, 1.0f}, rollDeg * DEG2RAD);
    SetShaderValue(look.shader, look.lightLoc, &v, SHADER_UNIFORM_VEC3);
}
