/*******************************************************************************
* djinterp [test]                                                    test_dext.c
*
* The extensions end to end: tree shape, file attributes, and the sheet's.
*
*
* path:      /tests/dawk/test_dext.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
// the extensions end to end: tree shape, file attributes, and the sheet's
// findings on files whose violations are known in advance
#include "../../inc/djinterp/tools/dawk/ext/dext_c.h"
#include "../../inc/djinterp/tools/dawk/ext/dext_cpp.h"
#include "../../inc/djinterp/tools/dawk/dmatch.h"
#include "../../inc/djinterp/tools/dawk/dss.h"
#include "../../inc/djinterp/tools/dawk/dproperty.h"
#include <stdio.h>
#include <string.h>

// the extensions take a source; this makes one from text, as a host would
static const struct d_source* source_of(const char* _text, size_t _length, const char* _path)
{
    static struct d_source s;
    d_source_init(&s, _text, _length, _path);
    (void)d_source_skip_bom(&s);
    return &s;
}

// and the input a build reads: that source, and no sink
static const struct d_token_node_input* input_of(const char* _text, size_t _length, const char* _path)
{
    static struct d_token_node_input in;
    in.source = source_of(_text, _length, _path);
    in.sink   = NULL;
    return &in;
}

static int failures = 0;
static void check(int ok, const char* what)
{ if (!ok) { printf("  FAIL %s\n", what); ++failures; } }

static const char* attr(struct d_node_tree* t, uint32_t n, const char* name)
{
    struct d_node* nd = d_node_at(t, n);
    for (uint32_t a = 0; a < nd->attribute_count; ++a) {
        const struct d_node_attribute* at = d_node_attribute_at(t, nd->first_attribute + a);
        if (!strcmp(d_node_name(t, at->name), name))
            return at->value == D_DSS_NO_INDEX ? "" : d_node_text(t, at->value);
    }
    return NULL;
}

static size_t findings(const char* sheet_text, const char* src, int cpp)
{
    struct d_dss_error e;
    struct d_dss_sheet* sh = d_dss_parse(sheet_text, strlen(sheet_text), &e);
    struct d_node_tree* t = d_node_tree_new();
    if (cpp) d_ext_cpp_build(t, D_DSS_NO_INDEX, 0, input_of(src, strlen(src), "t.cpp"), NULL);
    else     d_ext_c_build(t, D_DSS_NO_INDEX, 0, input_of(src, strlen(src), "t.c"), NULL);
    size_t hits = 0;
    for (uint32_t n = 0; n < (uint32_t)d_node_count(t); ++n)
        for (size_t r = 0; r < d_dss_rule_count(sh); ++r)
            if (d_match_rule(sh, t, n, d_dss_rule_at(sh, (uint32_t)r))) ++hits;
    d_node_tree_free(t); d_dss_free(sh);
    return hits;
}

// what the lint driver reports, not what merely matches: the engine's
// evaluator decides, as it does for dcheck -- `violation` fails on every
// match, `start-column` only on a node in another column, and a modifier
// such as `severity` is not a claim at all
static size_t violations(const char* sheet_text, const char* src)
{
    struct d_dss_error e;
    struct d_dss_sheet* sh = d_dss_parse(sheet_text, strlen(sheet_text), &e);
    struct d_node_tree* t = d_node_tree_new();
    d_ext_c_build(t, D_DSS_NO_INDEX, 0, input_of(src, strlen(src), "t.c"), NULL);
    size_t hits = 0;
    for (uint32_t n = 0; n < (uint32_t)d_node_count(t); ++n)
        for (size_t r = 0; r < d_dss_rule_count(sh); ++r) {
            const struct d_dss_rule* rule = d_dss_rule_at(sh, (uint32_t)r);
            if (!d_match_rule(sh, t, n, rule)) continue;
            for (uint32_t d = 0; d < rule->declaration_count; ++d) {
                bool evaluated = false;
                const bool holds = d_property_evaluate(sh, t, n,
                    d_dss_declaration_at(sh, rule->first_declaration + d), &evaluated);
                if (evaluated && !holds) ++hits;
            }
        }
    d_node_tree_free(t); d_dss_free(sh);
    return hits;
}

int main(void)
{
    // file-type claims: .h is C by default, and nothing is claimed twice
    check(d_ext_c_accepts("a.c") && d_ext_c_accepts("a.h"), "C claims .c and .h");
    check(d_ext_cpp_accepts("a.cpp") && d_ext_cpp_accepts("a.hpp"), "C++ claims .cpp and .hpp");
    check(!d_ext_cpp_accepts("a.h") && !d_ext_c_accepts("a.cpp"), "no extension claimed twice");

    // the file node carries the language and the standard apart
    { struct d_node_tree* t = d_node_tree_new();
      uint32_t r = d_ext_cpp_build(t, D_DSS_NO_INDEX, 0, input_of("int x;", 6, "a.cpp"), NULL);
      check(r != D_DSS_NO_INDEX, "C++ tree built");
      check(attr(t, r, "lang") && !strcmp(attr(t, r, "lang"), "c++"), "lang is the language");
      check(attr(t, r, "std") && !strcmp(attr(t, r, "std"), "c++23"), "std is the standard");
      d_node_tree_free(t); }

    // a closing bracket is folded into its group; a group holds its contents
    { struct d_node_tree* t = d_node_tree_new();
      uint32_t r = d_ext_c_build(t, D_DSS_NO_INDEX, 0, input_of("f(a, b);", 8, "a.c"), NULL);
      // found by type, not position: the statement is an item holding it
      uint32_t g = D_DSS_NO_INDEX;
      for (uint32_t n = 0; n < (uint32_t)d_node_count(t); ++n)
          if (!strcmp(d_node_name(t, d_node_at(t, n)->type), "group")) { g = n; break; }
      (void)r;
      check(g != D_DSS_NO_INDEX, "the call's arguments are a group");
      if (g == D_DSS_NO_INDEX) { d_node_tree_free(t); goto after_group; }
      check(!strcmp(d_node_name(t, d_node_at(t, g)->type), "group"), "call args are a group");
      size_t kids = 0;
      for (uint32_t c = d_node_at(t, g)->first_child; c != D_DSS_NO_INDEX;
           c = d_node_at(t, c)->next_sibling) ++kids;
      check(kids == 3, "the group holds a , b and no closer");
      d_node_tree_free(t); }
    after_group:

    // a name with no underscore at all is still caught by the negation
    { const char* sh = "identifier:not([text^=\"d_\"]) { violation: \"x\"; severity: warn; }";
      check(findings(sh, "Bad d_ok", 0) == 1, "an empty prefix is still compared"); }

    // the shipped sheet's rules, on violations planted in advance
    { const char* sheet =
        "file[lang=c] directive[name=define] > identifier[macro]:not([text^=\"D_\"])"
        ":not([text^=\"DJINTERP_\"], [text^=\"_\"]) { violation: \"x\"; severity: warn; }\n"
        "file[lang=c] keyword[text=struct] + identifier:not([text^=\"d_\"])"
        " + group[open=\"{\"] { violation: \"x\"; severity: warn; }\n";
      check(findings(sheet, "#define D_OK 1\n#define bad 1\n", 0) == 1, "one bad macro");
      check(findings(sheet, "#define _POSIX_C_SOURCE 1\n", 0) == 0, "reserved names exempt");
      check(findings(sheet, "#define DJINTERP_X_H 1\n", 0) == 0, "guards exempt");
      check(findings(sheet, "#ifdef _WIN32\n#endif\n", 0) == 0, "tested macros exempt");
      check(findings(sheet, "struct stat st;\n", 0) == 0, "a use of another's tag is not a definition");
      check(findings(sheet, "struct Bad\n{\n    int x;\n};\n", 0) == 1, "a bad tag, brace on its own line");
      check(findings(sheet, "struct d_ok { int x; };\n", 0) == 0, "a good tag");
      check(findings(sheet, "struct Bad { int x; };\n", 1) == 0, "the C sheet does not judge C++"); }

    // the item layer, as the shipped sheet's rules read it
    { const char* sheet =
        "file[lang=c] param identifier[declarator]:not([text^=\"_\"]) { violation: \"x\"; severity: warn; }\n"
        "file[lang=c] fn-def[returns=void][ends!=return] { violation: \"x\"; severity: warn; }\n"
        "file[lang=c] typedef[hides] { violation: \"x\"; severity: warn; }\n"
        "file[lang=c] fn-def[lines>3] { severity: info; }\n";
      check(findings(sheet, "void f(int bad) { return; }\n", 0) == 1, "an unprefixed parameter");
      check(findings(sheet, "void f(int _ok) { return; }\n", 0) == 0, "a prefixed parameter");
      check(findings(sheet, "void f(size_t) ;\n", 0) == 0, "a type alone names no parameter");
      check(findings(sheet, "void f(void (*bad)(int)) { return; }\n", 0) == 1,
            "the name inside a function-pointer parameter");
      check(findings(sheet, "void f(int _a) { g(); }\n", 0) == 1, "a void function without return");
      check(findings(sheet, "int f(int _a) { return _a; }\n", 0) == 0, "a value function");
      check(findings(sheet, "void* f(int _a) { g(); }\n", 0) == 0, "a pointer is not void");
      check(findings(sheet, "typedef struct d_x d_x_t;\n", 0) == 1, "a typedef hiding a struct");
      check(findings(sheet, "typedef void (*d_fn)(struct d_x*);\n", 0) == 0,
            "a function-pointer typedef mentioning a struct");
      check(findings(sheet, "typedef unsigned d_off_t;\n", 0) == 0, "a scalar alias");
      check(findings(sheet, "int f(int _a)\n{\n\n\n    return _a;\n}\n", 0) == 1,
            "a numeric comparison on lines"); }

    // the shipped sheet itself, not a fragment of it: one violation of each
    // naming and declaration rule, and nothing else, must give one finding
    // per rule -- a rule that silently matches nothing fails here.  Each
    // snippet is otherwise conforming, documentation comments included
    { FILE* f = fopen("sheets/c_style.dss", "rb");
      check(f != NULL, "the shipped sheet opens (run from the bundle root)");
      if (f) {
          static char sheet[65536]; size_t n = fread(sheet, 1, sizeof sheet - 1, f);
          fclose(f); sheet[n] = 0;
          check(violations(sheet, "// bad_macro\n//   macro: planted.\n#define bad_macro 1\n") == 1,
                "sheet: a bad macro");
          check(violations(sheet, "#ifndef X_OK\n#define X_OK 1\n#endif\n") == 0,
                "sheet: a fallback definition");
          check(violations(sheet, "// WIN32_LEAN_AND_MEAN\n//   macro: external.\n#define WIN32_LEAN_AND_MEAN\n") == 0,
                "sheet: an external configuration macro");
          check(violations(sheet, "struct Bad\n{\n    int _x;\n};\n") == 1, "sheet: a bad tag");
          check(violations(sheet, "typedef struct d_x d_x_t;\n") == 1, "sheet: a hiding typedef");
          check(violations(sheet,
              "/*\nd_f\n  planted.\n*/\nvoid\nd_f(\n    int bad\n)\n{\n    return;\n}\n") == 1,
              "sheet: a bad parameter name");
          check(violations(sheet,
              "/*\nd_f\n  planted.\n*/\nvoid\nd_f(\n    int _a\n)\n{\n    d_g();\n}\n") == 1,
              "sheet: a void function without return");
          check(violations(sheet,
              "/*\nd_f\n  conforming.\n*/\nint\nd_f(\n    int _a\n)\n{\n    return _a;\n}\n") == 0,
              "sheet: a conforming definition"); } }

    // a fallback for another header's name is not djinterp's to spell
    { const char* sheet =
        "file[lang=c] directive[name=define]:not([guarded]) > identifier[macro]"
        ":not([text^=\"D_\"])[text!=WIN32_LEAN_AND_MEAN] { violation: \"x\"; severity: warn; }\n";
      check(findings(sheet, "#ifndef X_OK\n#define X_OK 1\n#endif\n", 0) == 0,
            "a guarded fallback definition");
      check(findings(sheet, "#if !defined(X_OK)\n/* posix */\n#define X_OK 1\n#endif\n", 0) == 0,
            "a fallback tested with defined(), a comment between");
      check(findings(sheet, "#define X_OK 1\n", 0) == 1, "an unguarded definition");
      check(findings(sheet, "#ifndef Y_OK\n#define X_OK 1\n#endif\n", 0) == 1,
            "a guard testing a different name");
      check(findings(sheet, "#define WIN32_LEAN_AND_MEAN\n", 0) == 0, "an allowed external name"); }

    // parameter layout, once per function, and only past one parameter
    { const char* sheet =
        "file[lang=c] fn-def[aligned][params>1] { violation: \"x\"; severity: warn; }\n"
        "file[lang=c] fn-decl[detached] { violation: \"x\"; severity: warn; }\n";
      check(findings(sheet, "int f(int _a, int _b) { return 0; }\n", 0) == 1,
            "a two-parameter definition on one line");
      check(findings(sheet, "int f(int _a) { return 0; }\n", 0) == 0,
            "a one-parameter definition may stay on one line");
      check(findings(sheet, "int main(void) { return 0; }\n", 0) == 0,
            "(void) is no layout at all");
      check(findings(sheet, "int\nf(\n    int _a,\n    int _b\n)\n{\n    return 0;\n}\n", 0) == 0,
            "the guide's definition layout");
      check(findings(sheet, "int f(\n    int _a);\n", 0) == 1,
            "a declaration with a detached parameter");
      check(findings(sheet, "int f(int _a,\n      int _b);\n", 0) == 0,
            "the guide's declaration layout"); }

    // indentation: a statement is measured, an aligned parameter is not
    { const char* sheet =
        "file[lang=c] *[line-start][level=1][pp-depth=0] { severity: info; }\n";
      check(findings(sheet, "int f(int _a)\n{\n    return _a;\n}\n", 0) == 1,
            "a line-starting statement carries level 1");
      check(findings(sheet, "int f(int _a,\n      int _b);\n", 0) == 0,
            "an aligned parameter carries no level"); }

    // the shipped sheet parses without a warning and uses nothing the
    // matcher refuses: a broken compound or a misspelt pseudo-class there
    // would disable a rule silently
    { FILE* f = fopen("sheets/c_style.dss", "rb");
      if (f) {
          static char text[1 << 16];
          size_t n = fread(text, 1, sizeof text - 1, f); fclose(f);
          struct d_dss_error e;
          struct d_dss_sheet* sh = d_dss_parse(text, n, &e);
          uint32_t line = 0; const char* name = NULL;
          check(sh && d_dss_warning_count(sh) == 0, "the shipped sheet parses without a warning");
          check(sh && !d_match_unsupported(sh, &line, &name), "the shipped sheet uses only supported selectors");
          d_dss_free(sh);
      } }

    printf(failures ? "\n%d check(s) failed\n" : "\nall checks passed\n", failures);
    return failures ? 1 : 0;
}
