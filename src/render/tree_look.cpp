#include "render/tree_look.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

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
    VS_IN " vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    VS_OUT " vec2 fragTexCoord;\n"
    VS_OUT " vec4 fragColor;\n"
    "void main() {\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    fragColor = vertexColor;\n"
    "    gl_Position = mvp*vec4(vertexPosition, 1.0);\n"
    "}\n";

// Hard alpha cutout: cards write depth, so soft edges would punch holes in what is drawn after them.
const char *kFragment = GLSL_HEAD
    FS_IN " vec2 fragTexCoord;\n"
    FS_IN " vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    FS_OUT_DECL
    "void main() {\n"
    "    vec4 texel = " TEX "(texture0, fragTexCoord);\n"
    "    if (texel.a < 0.5) discard;\n"
    "    " FS_OUT " = vec4(texel.rgb*fragColor.rgb, 1.0);\n"
    "}\n";

const int kTexW = 128, kTexH = 160;
const float kCardWidth = 5.6f, kCardHeight = 7.0f;

Shader gShader{};
Texture2D gTexture{};
bool gLoaded = false;

struct Blob { float x, y, r; };

Color Px(int r, int g, int b) { return (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, 255}; }

Image MakeTreeImage() {
    Image img = GenImageColor(kTexW, kTexH, BLANK);
    Color *px = (Color *)img.data;
    const Blob blobs[] = {{64, 60, 38}, {36, 80, 27}, {92, 82, 29}, {66, 34, 25}};
    const Color outline = Px(20, 52, 34), lit = Px(124, 190, 74), mid = Px(66, 156, 62), shade = Px(36, 102, 58);
    const Color trunkMid = Px(132, 88, 50), trunkShade = Px(92, 58, 34), trunkLine = Px(44, 28, 18);
    const float line = 4.0f;

    for (int y = 0; y < kTexH; y++) {
        for (int x = 0; x < kTexW; x++) {
            float best = 1e9f, bestLight = 0.0f;
            for (const Blob &b : blobs) {
                float dx = x - b.x, dy = y - b.y;
                float d = sqrtf(dx * dx + dy * dy) - b.r;
                if (d < best) {
                    best = d;
                    bestLight = (dx * 0.55f - dy * 0.83f) / b.r;
                }
            }

            Color c = BLANK;
            if (best < 0.0f) {
                c = bestLight > 0.35f ? lit : (bestLight < -0.3f ? shade : mid);
            } else if (best < line) {
                c = outline;
            } else {
                float tx = fabsf(x - 64.0f) - 8.0f;
                bool trunkBody = y >= 90 && y < kTexH && tx < 0.0f;
                bool trunkEdge = y >= 90 && y < kTexH && tx < line && !trunkBody;
                if (trunkBody) c = (x > 66 ? trunkMid : trunkShade);
                else if (trunkEdge) c = trunkLine;
            }
            px[y * kTexW + x] = c;
        }
    }
    return img;
}

}  // namespace

void LoadTreeLook() {
    if (gLoaded) return;
    gShader = LoadShaderFromMemory(kVertex, kFragment);
    if (gShader.id == 0 || gShader.id == rlGetShaderIdDefault()) return;
    Image img = MakeTreeImage();
    gTexture = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(gTexture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(gTexture, TEXTURE_WRAP_CLAMP);
    gLoaded = gTexture.id > 0;
    if (!gLoaded) UnloadShader(gShader);
}

void UnloadTreeLook() {
    if (!gLoaded) return;
    UnloadTexture(gTexture);
    UnloadShader(gShader);
    gLoaded = false;
}

bool TreeCardsAvailable() { return gLoaded; }

void DrawTreeCards(const ScatterState &scatter, float density, Vector3 viewPosition, float drawDistance) {
    if (!gLoaded) return;
    size_t count = (size_t)(Clamp(density, 0.0f, 1.0f) * (float)scatter.props.size());
    if (count == 0) return;
    float maxDistSq = drawDistance * drawDistance;

    rlDisableBackfaceCulling();
    BeginShaderMode(gShader);
    rlSetTexture(gTexture.id);
    for (size_t i = 0; i < count; i++) {
        const ScatterProp &p = scatter.props[i];
        if (p.kind != ScatterKind::Tree) continue;
        float dx = p.position.x - viewPosition.x, dz = p.position.z - viewPosition.z;
        if (dx * dx + dz * dz > maxDistSq) continue;

        rlCheckRenderBatchLimit(8);
        rlBegin(RL_QUADS);
        float k = 0.88f + p.tint / 850.0f;
        unsigned char v = (unsigned char)fminf(255.0f * k, 255.0f);
        rlColor4ub(v, v, v, 255);
        float halfW = kCardWidth * 0.5f * p.scale, h = kCardHeight * p.scale;
        for (int card = 0; card < 2; card++) {
            float a = p.yaw + card * PI * 0.5f;
            float ax = cosf(a) * halfW, az = sinf(a) * halfW;
            Vector3 o = p.position;
            rlTexCoord2f(0.0f, 1.0f); rlVertex3f(o.x - ax, o.y, o.z - az);
            rlTexCoord2f(1.0f, 1.0f); rlVertex3f(o.x + ax, o.y, o.z + az);
            rlTexCoord2f(1.0f, 0.0f); rlVertex3f(o.x + ax, o.y + h, o.z + az);
            rlTexCoord2f(0.0f, 0.0f); rlVertex3f(o.x - ax, o.y + h, o.z - az);
        }
        rlEnd();
    }
    rlSetTexture(0);
    EndShaderMode();
    rlEnableBackfaceCulling();
}
