/*******************************************************************************
* djinterp [tools]                                                 test_daudit.c
*
* Exercises `daudit.h`.
*
*
* path:      /tests/djinterp/tools/dawk/test_daudit.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
/* Replays repairs from this tool's own history against the audit.  Each case
   states the verdict the gate must reach; a case that passes when it should
   fail means the gate cannot be trusted. */
#include "../../../../inc/djinterp/tools/dawk/daudit.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;

static void
check(const char* name, const char* before, const char* after,
      struct d_audit_claim claim, int want_ok)
{
    const char* why = NULL;
    const int   ok  = d_audit_verify(before, after, &claim, &why);
    const int   hit = (ok == want_ok);
    if (!hit) ++failures;
    printf("  %s  %-44s %s%s%s\n", hit ? "ok  " : "FAIL", name,
           ok ? "accepted" : "refused", ok ? "" : ": ", ok ? "" : why);
}

int
main(void)
{
    struct d_audit_claim c;

    printf("-- repairs that were right --\n");
    memset(&c, 0, sizeof c); c.kind = D_AUDIT_GEOMETRY;
    check("date unit moved to column 80",
          "* author(s): X          created: 2023.03.12",
          "* author(s): X                created: 2023.03.12", c, 1);
    memset(&c, 0, sizeof c); c.kind = D_AUDIT_FILL;
    check("closing rule widened by one star", "***/", "****/", c, 1);
    memset(&c, 0, sizeof c); c.kind = D_AUDIT_SWAP;
    c.head = 12; c.run = 2; c.expect = "db"; c.head2 = 16; c.run2 = 6; c.expect2 = "config";
    check("tag list swapped", "* djinterp [db][config]  x.h",
          "* djinterp [config][db]  x.h", c, 1);
    memset(&c, 0, sizeof c); c.kind = D_AUDIT_INSERT; c.head = 6; c.value = "  // G_H";
    check("bare #endif given a comment", "#endif", "#endif  // G_H", c, 1);
    c.replaces = 1;
    check("stale block comment converted", "#endif  /*  OLD_ */", "#endif  // G_H", c, 1);

    printf("-- corruptions this tool actually produced --\n");
    memset(&c, 0, sizeof c); c.kind = D_AUDIT_GEOMETRY;
    check("value glued to its label", "* x created:   2023.03.12",
          "* x created:2023.03.12", c, 0);
    memset(&c, 0, sizeof c); c.kind = D_AUDIT_SUBSTITUTE;
    c.head = 65; c.run = 13; c.expect = "cfg_archive.h"; c.value = "cfg_archive.h";
    check("title rebuilt from the node's text, not the line",
          "cfg_archive.h", "cfg_archive.h", c, 0);
    memset(&c, 0, sizeof c); c.kind = D_AUDIT_SUBSTITUTE;
    c.head = 19; c.run = 6; c.expect = "fn_bui"; c.value = "fn_builder.h";
    check("stale offsets spliced a name into itself",
          "* djinterp [c]     fn_builder.h", "* djinterp [c]     fn_builder.huilder.h", c, 0);
    memset(&c, 0, sizeof c); c.kind = D_AUDIT_FILL;
    check("fill repair that changed other text", "***/", "**x*/", c, 0);
    memset(&c, 0, sizeof c); c.kind = D_AUDIT_INSERT; c.head = 6; c.value = "  // G_H"; c.replaces = 1;
    check("block comment holding prose discarded",
          "#endif  /* keep this note */", "#endif  // G_H", c, 0);

    printf("-- corruptions the gate CANNOT see (a wrong node, faithfully repaired) --\n");
    memset(&c, 0, sizeof c); c.kind = D_AUDIT_SUBSTITUTE;
    c.head = 24; c.run = 4; c.expect = "date"; c.value = "created";
    check("'date:' inside 'update:' relabelled",
          "*     rcu - read-copy-update: x", "*     rcu - read-copy-upcreated: x", c, 1);
    memset(&c, 0, sizeof c); c.kind = D_AUDIT_INSERT; c.head = 17; c.value = "  // G_H";
    check("comment appended beside a block comment",
          "#endif  /* OLD */", "#endif  /* OLD */  // G_H", c, 1);

    printf("\n%d case(s) reached the wrong verdict\n", failures);
    return failures ? 1 : 0;
}
