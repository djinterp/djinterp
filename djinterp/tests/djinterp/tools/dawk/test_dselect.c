/*******************************************************************************
* djinterp [tools]                                                test_dselect.c
*
* Test_dselect.c -- selector engine: the Selectors Level 4 pseudo-classes.
*
*
* path:      /tests/djinterp/tools/dawk/test_dselect.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
// test_dselect.c -- selector engine: the Selectors Level 4 pseudo-classes
// (:not :is :has :first-child :last-child :only-child :nth-child
// :nth-last-child), relative selectors inside :has, contiguity of nested
// selector lists, and the parser's `a[x] [y]` warning.
#include "../../../../inc/djinterp/tools/dawk/dmatch.h"
#include "../../../../inc/djinterp/tools/dawk/dnode.h"
#include "../../../../inc/djinterp/tools/dawk/dss.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;

static void check(int ok, const char* what)
{
    if (!ok) { printf("  FAIL %s\n", what); ++failures; }
}

// the fixture:  file
//                 fn(name=a)  > param(n=1) param(n=2) param(n=3) body > ret
//                 fn(name=b)  > param(n=1) body
//                 fn(name=c)  > body
//                 decl(name=d)
static struct d_node_tree* tree;
static uint32_t file_n, fa, fb, fc, dd, pa[3], pb, ba, bb, bc, ret;

static void build(void)
{
    tree   = d_node_tree_new();
    file_n = d_node_add(tree, D_DSS_NO_INDEX, "file");
    fa     = d_node_add(tree, file_n, "fn");
    (void)d_node_set_attribute(tree, fa, "name", "a");
    for (int k = 0; k < 3; ++k) {
        char n[2] = { (char)('1' + k), 0 };
        pa[k] = d_node_add(tree, fa, "param");
        (void)d_node_set_attribute(tree, pa[k], "n", n);
    }
    ba  = d_node_add(tree, fa, "body");
    ret = d_node_add(tree, ba, "ret");
    fb  = d_node_add(tree, file_n, "fn");
    (void)d_node_set_attribute(tree, fb, "name", "b");
    pb  = d_node_add(tree, fb, "param");
    (void)d_node_set_attribute(tree, pb, "n", "1");
    bb  = d_node_add(tree, fb, "body");
    fc  = d_node_add(tree, file_n, "fn");
    (void)d_node_set_attribute(tree, fc, "name", "c");
    bc  = d_node_add(tree, fc, "body");
    dd  = d_node_add(tree, file_n, "decl");
    (void)d_node_set_attribute(tree, dd, "name", "d");
}

// matches: the nodes, in index order, that the sheet's first rule selects
static void matches(const char* sheet_text, char* out, size_t size)
{
    struct d_dss_error error;
    struct d_dss_sheet* sheet = d_dss_parse(sheet_text, strlen(sheet_text), &error);
    out[0] = 0;
    if (!sheet) { snprintf(out, size, "PARSE: %s", error.message ? error.message : "?"); return; }
    const struct d_dss_rule* rule = d_dss_rule_at(sheet, 0);
    for (uint32_t n = 0; n < (uint32_t)d_node_count(tree); ++n) {
        if (!d_match_rule(sheet, tree, n, rule)) continue;
        char item[16];
        snprintf(item, sizeof item, "%s%u", out[0] ? " " : "", n);
        strncat(out, item, size - strlen(out) - 1);
    }
    d_dss_free(sheet);
}

static void expect(const char* sheet_text, const char* want)
{
    char got[256];
    matches(sheet_text, got, sizeof got);
    if (strcmp(got, want) != 0) {
        printf("  FAIL %-44s got [%s] want [%s]\n", sheet_text, got, want);
        ++failures;
    }
}

static const char* ids(uint32_t a, uint32_t b, uint32_t c)
{
    static char buf[64];
    if (c != 999) snprintf(buf, sizeof buf, "%u %u %u", a, b, c);
    else if (b != 999) snprintf(buf, sizeof buf, "%u %u", a, b);
    else snprintf(buf, sizeof buf, "%u", a);
    return buf;
}

int main(void)
{
    build();
    char want[64];

    // structural
    strcpy(want, ids(pa[0], pb, 999));
    expect("param:first-child { x: 1; }", want);
    expect("param:last-child { x: 1; }", "");
    strcpy(want, ids(bc, ret, 999));
    { char w2[64]; snprintf(w2, sizeof w2, "%u %u", ret, bc); expect("*:only-child { x: 1; }", w2); }
    strcpy(want, ids(pa[1], 999, 999));
    expect("param:nth-child(2) { x: 1; }", want);
    { char w2[64]; snprintf(w2, sizeof w2, "%u %u %u", pa[0], pa[2], pb); expect("param:nth-child(odd) { x: 1; }", w2); }
    { char w2[64]; snprintf(w2, sizeof w2, "%u", pa[1]); expect("param:nth-child(2n) { x: 1; }", w2); }
    { char w2[64]; snprintf(w2, sizeof w2, "%u %u", pa[1], pa[2]); expect("param:nth-child(n + 2) { x: 1; }", w2); }
    // fn b's own param is first of two children, so it is in both of these
    { char w2[64]; snprintf(w2, sizeof w2, "%u %u %u", pa[0], pa[1], pb); expect("param:nth-child(-n+2) { x: 1; }", w2); }
    { char w2[64]; snprintf(w2, sizeof w2, "%u %u", pa[2], pb); expect("param:nth-last-child(2) { x: 1; }", w2); }
    { char w2[64]; snprintf(w2, sizeof w2, "%u %u", fb, fc); expect("fn:not([name=a]) { x: 1; }", w2); }
    { char w2[64]; snprintf(w2, sizeof w2, "%u %u", fa, dd); expect(":is(fn[name=a], decl) { x: 1; }", w2); }

    // :has, relative forms
    { char w2[64]; snprintf(w2, sizeof w2, "%u %u", fa, fb); expect("fn:has(> param) { x: 1; }", w2); }
    { char w2[64]; snprintf(w2, sizeof w2, "%u", fa); expect("fn:has(ret) { x: 1; }", w2); }
    { char w2[64]; snprintf(w2, sizeof w2, "%u", fa); expect("fn:has(> body > ret) { x: 1; }", w2); }
    expect("fn:has(> ret) { x: 1; }", "");
    { char w2[64]; snprintf(w2, sizeof w2, "%u %u", fa, fb); expect("fn:has(+ fn) { x: 1; }", w2); }
    { char w2[64]; snprintf(w2, sizeof w2, "%u %u %u", fa, fb, fc); expect("fn:has(... decl) { x: 1; }", w2); }
    { char w2[64]; snprintf(w2, sizeof w2, "%u %u", fb, fc); expect("fn:not(:has(ret)) { x: 1; }", w2); }
    { char w2[64]; snprintf(w2, sizeof w2, "%u", file_n); expect("file:has(fn[name=c]) { x: 1; }", w2); }
    { char w2[64]; snprintf(w2, sizeof w2, "%u", fc); expect("fn:not(:has(> param)) { x: 1; }", w2); }

    // nested lists stay contiguous: a rule list whose members nest
    { char w2[64]; snprintf(w2, sizeof w2, "%u %u", fc, dd);
      expect("fn:not(:has(param), :is([name=a])), decl:not(:has(x)) { x: 1; }", w2); }

    // unknown pseudo-classes are refused, nested ones too
    { struct d_dss_error e; const char* t = "fn:not(:frist-child) { x: 1; }";
      struct d_dss_sheet* sh = d_dss_parse(t, strlen(t), &e);
      uint32_t line = 0; const char* name = NULL;
      check(sh && d_match_unsupported(sh, &line, &name) && name && !strcmp(name, "frist-child"),
            "a misspelt pseudo-class inside :not is reported");
      d_dss_free(sh); }
    { struct d_dss_error e; const char* t = "fn:first-child(2) { x: 1; }";
      check(d_dss_parse(t, strlen(t), &e) == NULL, "a structural pseudo-class takes no argument"); }
    { struct d_dss_error e; const char* t = "fn:nth-child(x) { x: 1; }";
      check(d_dss_parse(t, strlen(t), &e) == NULL, "An+B is checked"); }

    // C2: whitespace before `[` warns; `*[` silences it
    { struct d_dss_error e; const char* t = "fn[name=a]\n    [n=1] { x: 1; }";
      struct d_dss_sheet* sh = d_dss_parse(t, strlen(t), &e);
      check(sh && d_dss_warning_count(sh) == 1 && d_dss_warning_at(sh, 0)->line == 2,
            "a compound broken across a line warns, on the line of the `[`");
      d_dss_free(sh); }
    { struct d_dss_error e; const char* t = "fn[name=a] *[n=1] { x: 1; }";
      struct d_dss_sheet* sh = d_dss_parse(t, strlen(t), &e);
      check(sh && d_dss_warning_count(sh) == 0, "an explicit `*[` does not warn");
      d_dss_free(sh); }

    d_node_tree_free(tree);
    printf(failures ? "%d failure(s)\n" : "all checks passed\n", failures);
    return failures ? 1 : 0;
}
