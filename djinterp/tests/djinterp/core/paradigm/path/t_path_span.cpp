// t_path_span.cpp -- path_span, the C++ face of the generic path, at every
// level from C++98: the same grammars the C test reads by.
#include "../../../../../inc/djinterp/core/paradigm/path/path_span.hpp"
#include <cstdio>
#include <cstring>
#include <string>

static int g_checks = 0, g_failed = 0;
#define CHECK(c, what) do { g_checks++; if (!(c)) { g_failed++; std::printf("FAIL %s (line %d)\n", what, __LINE__); } } while (0)

static const d_path_grammar posix_g = { "/", '/', NULL, NULL, D_PATH_GRAMMAR_DOT_LABELS };
static const d_path_grammar tree_g  = { ".", '.', NULL, NULL, 0u };

int main()
{
    const djinterp::path_span p("/usr//local/./lib/../bin", posix_g);
    std::string labels;

    for (djinterp::path_span::const_iterator it = p.begin(); it != p.end(); ++it)
    {
        labels += std::string(it->text, it->length) + "|";
    }
    CHECK(labels == "usr|local|.|lib|..|bin|", "iterates every label, none empty");
    CHECK(p.level() == 6 && p.anchored(), "level and anchor");
    CHECK(p.str() == "/usr/local/bin", "str: rendered");
    CHECK(p.parent().str() == "/usr/local", "parent");
    CHECK(p.join(djinterp::path_span("../sbin", posix_g)) == "/usr/local/sbin", "join");
    CHECK(p.starts_with(djinterp::path_span("/usr", posix_g)), "starts_with");
    CHECK(p.meet(djinterp::path_span("usr", posix_g)) == D_PATH_SPAN_NO_MEET, "no meet across anchors");
    {
        const djinterp::path_span t("0.3.1", tree_g);
        std::size_t n = 0;
#if __cplusplus >= 201103L
        for (const d_path_label& l : t) { n += l.length; }
#else
        for (djinterp::path_span::const_iterator it = t.begin(); it != t.end(); it++) { n += it->length; }
#endif
        CHECK(n == 3 && t.level() == 3 && t.parent().str() == "0.3", "a tree's path");
    }
    CHECK(djinterp::path_span().level() == 0 && djinterp::path_span().str().empty(), "the empty span");
    std::printf("t_path_span.cpp: %d checks, %d failed\n", g_checks, g_failed);
    return g_failed != 0;
}
