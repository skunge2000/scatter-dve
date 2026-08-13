// scatter-dve — minimal test harness
//
// Deliberately not assert(): Release builds define NDEBUG, which compiles
// assert() to nothing, so an assert-based suite passes silently in exactly the
// configuration that ships. CHECK always evaluates its condition.
//
// Deliberately not an external framework: nothing here needs one yet, and a
// dependency is a thing that breaks between sessions.

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace scatter::test {

inline int g_checks = 0;
inline int g_failures = 0;

inline void report(bool ok, const char* expr, const char* file, int line) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        const char* base = std::strrchr(file, '/');
        std::fprintf(stderr, "FAIL %s:%d  %s\n", base ? base + 1 : file, line, expr);
    }
}

// Call at the end of main(); its return value is the process exit code.
inline int summary(const char* name) {
    if (g_failures == 0) {
        std::printf("PASS %s (%d checks)\n", name, g_checks);
        return 0;
    }
    std::fprintf(stderr, "FAIL %s (%d of %d checks failed)\n",
                 name, g_failures, g_checks);
    return 1;
}

}  // namespace scatter::test

#define CHECK(expr) ::scatter::test::report((expr), #expr, __FILE__, __LINE__)

// For loops: reports only the first failure so a broken invariant over 1024
// code points does not produce 1024 lines of output.
#define CHECK_ONCE(expr)                                    \
    do {                                                    \
        static bool reported_ = false;                      \
        const bool ok_ = (expr);                            \
        if (!ok_ && reported_) { ++::scatter::test::g_checks; break; } \
        if (!ok_) reported_ = true;                         \
        ::scatter::test::report(ok_, #expr, __FILE__, __LINE__); \
    } while (0)
