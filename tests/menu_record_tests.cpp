#include "progress.h"
#include <cstdio>

namespace {

int g_failures = 0;

void ExpectLine(const char *name, float time, int stars, const char *want) {
    std::string got = FormatRecordLine(time, stars);
    if (got != want) {
        g_failures++;
        printf("FAIL %s: got \"%s\" want \"%s\"\n", name, got.c_str(), want);
    }
}

}  // namespace

int main() {
    ExpectLine("no record", -1.0f, 0, "");
    ExpectLine("minutes and tenths", 83.4f, 2, "Best: 1:23.4   Stars: 2/3");
    ExpectLine("under a minute", 23.5f, 3, "Best: 0:23.5   Stars: 3/3");
    ExpectLine("zero stars", 20.0f, 0, "Best: 0:20.0   Stars: 0/3");
    ExpectLine("rounding carries into minute", 59.96f, 1, "Best: 1:00.0   Stars: 1/3");
    ExpectLine("stars clamped", 10.0f, 9, "Best: 0:10.0   Stars: 3/3");
    if (g_failures == 0) printf("menu record tests passed\n");
    return g_failures == 0 ? 0 : 1;
}
