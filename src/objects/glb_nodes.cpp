#include "glb_nodes.h"
#include "raylib.h"
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace {

struct Json {
    enum Kind { Null, Number, String, Array, Object } kind = Null;
    double number = 0.0;
    std::string str;
    std::vector<Json> items;
    std::vector<std::string> keys;

    const Json *Get(const char *key) const {
        for (size_t i = 0; i < keys.size(); i++) {
            if (keys[i] == key) return &items[i];
        }
        return nullptr;
    }
};

struct Parser {
    const char *p;
    const char *end;
    bool ok = true;

    void Skip() {
        while (p < end && isspace((unsigned char)*p)) p++;
    }

    bool Eat(char c) {
        Skip();
        if (p < end && *p == c) {
            p++;
            return true;
        }
        return false;
    }

    std::string ParseString() {
        std::string out;
        p++;
        while (p < end && *p != '"') {
            if (*p == '\\' && p + 1 < end) {
                p++;
                out += (*p == 'n') ? '\n' : (*p == 't') ? '\t' : *p;
            } else {
                out += *p;
            }
            p++;
        }
        if (p >= end) ok = false;
        p++;
        return out;
    }

    Json ParseValue() {
        Json v;
        Skip();
        if (p >= end) {
            ok = false;
            return v;
        }
        if (*p == '{') {
            v.kind = Json::Object;
            p++;
            if (Eat('}')) return v;
            do {
                Skip();
                if (p >= end || *p != '"') {
                    ok = false;
                    return v;
                }
                v.keys.push_back(ParseString());
                if (!Eat(':')) {
                    ok = false;
                    return v;
                }
                v.items.push_back(ParseValue());
            } while (ok && Eat(','));
            if (!Eat('}')) ok = false;
        } else if (*p == '[') {
            v.kind = Json::Array;
            p++;
            if (Eat(']')) return v;
            do {
                v.items.push_back(ParseValue());
            } while (ok && Eat(','));
            if (!Eat(']')) ok = false;
        } else if (*p == '"') {
            v.kind = Json::String;
            v.str = ParseString();
        } else if (*p == '-' || isdigit((unsigned char)*p)) {
            v.kind = Json::Number;
            char *numEnd = nullptr;
            v.number = strtod(p, &numEnd);
            p = numEnd;
        } else {
            while (p < end && isalpha((unsigned char)*p)) p++;
        }
        return v;
    }
};

}  // namespace

bool LoadGlbNodes(const char *path, std::vector<GlbNode> &nodes) {
    int size = 0;
    unsigned char *data = LoadFileData(path, &size);
    if (!data) return false;

    bool ok = false;
    Json root;
    // GLB layout: 12-byte header, then a 8-byte chunk header whose first chunk is the JSON.
    if (size > 20 && memcmp(data, "glTF", 4) == 0 && memcmp(data + 16, "JSON", 4) == 0) {
        unsigned int jsonLength;
        memcpy(&jsonLength, data + 12, 4);
        if (20 + (size_t)jsonLength <= (size_t)size) {
            Parser parser{(const char *)data + 20, (const char *)data + 20 + jsonLength};
            root = parser.ParseValue();
            ok = parser.ok && root.kind == Json::Object;
        }
    }
    UnloadFileData(data);
    if (!ok) return false;

    const Json *jsonNodes = root.Get("nodes");
    const Json *jsonMeshes = root.Get("meshes");
    if (!jsonNodes || !jsonMeshes) return false;

    nodes.assign(jsonNodes->items.size(), GlbNode());
    for (size_t i = 0; i < jsonNodes->items.size(); i++) {
        const Json &jn = jsonNodes->items[i];
        GlbNode &node = nodes[i];
        if (const Json *name = jn.Get("name")) node.name = name->str;

        if (jn.Get("translation") || jn.Get("rotation") || jn.Get("scale")) return false;

        if (const Json *matrix = jn.Get("matrix")) {
            if (matrix->items.size() != 16) return false;
            for (int k = 0; k < 16; k++) node.local.m[k] = (float)matrix->items[k].number;
        }

        if (const Json *children = jn.Get("children")) {
            for (const Json &child : children->items) {
                int c = (int)child.number;
                if (c < 0 || c >= (int)nodes.size()) return false;
                nodes[c].parent = (int)i;
            }
        }

        if (const Json *mesh = jn.Get("mesh")) {
            int m = (int)mesh->number;
            if (m < 0 || m >= (int)jsonMeshes->items.size()) return false;
            const Json *prims = jsonMeshes->items[m].Get("primitives");
            if (!prims) return false;
            for (const Json &prim : prims->items) {
                const Json *mode = prim.Get("mode");
                if (!mode || (int)mode->number == 4) node.trianglePrimitives++;
            }
        }
    }
    return true;
}
