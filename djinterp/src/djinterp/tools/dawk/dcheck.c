/*******************************************************************************
* djinterp [dawk]                                                       dcheck.c
*
* Check driver:
*   Walks a tree, asks an extension to build nodes for each file, matches a
* stylesheet against them, and reports what the registry says is wrong.  The
* driver knows nothing about banners, properties or file formats; it knows
* only how to run the loop.
*   Under --fix it applies at most one repair per file per pass.  A node's
* columns are measured once, from the file as it was read, so a second repair
* in the same pass would splice at offsets the first has already moved.  The
* next pass measures what is actually on disk, and the run repeats until no
* repair is produced.
*
* path:      /src/djinterp/tools/dawk/dcheck.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dmatch.h"  // corresponding header
// std
#include <stdio.h>   // FILE, fopen, printf
#include <stdlib.h>  // malloc, calloc, free
#include <string.h>  // strcmp, strrchr
// djinterp
#include "../../../../inc/djinterp/tools/dawk/dbanner.h"    // d_banner_build
#include "../../../../inc/djinterp/tools/dawk/dedit.h"      // d_edit_rewrite_line
#include "../../../../inc/djinterp/tools/dawk/dnode.h"      // d_node_tree
#include "../../../../inc/djinterp/tools/dawk/dregistry.h"  // d_registry_evaluate
#include "../../../../inc/djinterp/tools/dawk/dsource.h"    // d_awk_source
#include "../../../../inc/djinterp/tools/dawk/dss.h"        // d_dss_sheet
#include "../../../../inc/djinterp/tools/dawk/dtree.h"      // d_awk_source_tree_init


// d_check_tally
//   struct: one row of the report.  Counting per property per rule is what
// distinguishes a rule that matched nothing from one that matched and held.
struct d_check_tally
{
    const char*  property;
    uint32_t     line;
    size_t       matched;
    size_t       failed;
};


/*
main
  Walks a tree, builds a banner per header, runs the sheet, and prints one
row per declaration.  First match wins is not yet exercised because no two
rules in the sheet carry the same property for the same node; when they do,
this is the loop that has to stop at the first.
*/
int
main(
    int    argc,
    char** argv
)
{
    if (argc < 3)
    {
        (void)fprintf(stderr, "usage: dcheck [--fix] <sheet.dss> <root>\n");
        return 2;
    }

    int        first  = 1;
    bool       fixing = false;
    size_t     repairs = 0;

    if (strcmp(argv[1], "--fix") == 0)
    {
        fixing = true;
        first  = 2;
    }

    if ((first + 1) >= argc)
    {
        (void)fprintf(stderr, "usage: dcheck [--fix] <sheet.dss> <root>\n");
        return 2;
    }

    FILE* const sheet_file = fopen(argv[first], "rb");

    if (!sheet_file)
    {
        (void)fprintf(stderr, "dcheck: cannot open %s\n", argv[first]);
        return 2;
    }

    (void)fseek(sheet_file, 0, SEEK_END);

    const long size = ftell(sheet_file);

    (void)fseek(sheet_file, 0, SEEK_SET);

    char* const source = malloc((size_t)size + 1u);

    if (!source)
    {
        (void)fclose(sheet_file);
        return 2;
    }

    const size_t read = fread(source, 1u, (size_t)size, sheet_file);

    source[read] = '\0';

    (void)fclose(sheet_file);

    struct d_dss_error  error;
    struct d_dss_sheet* const sheet = d_dss_parse(source, read, &error);

    if (!sheet)
    {
        (void)fprintf(stderr, "%s:%u:%u: %s\n", argv[first], error.line,
                      error.column,
                      error.message ? error.message : "parse failed");
        free(source);
        return 1;
    }

    struct d_awk_source walk;

    if (!d_awk_source_tree_init(&walk, argv[first + 1]))
    {
        (void)fprintf(stderr, "dcheck: cannot walk %s\n", argv[first + 1]);
        d_dss_free(sheet);
        free(source);
        return 2;
    }

    const size_t rule_count = d_dss_rule_count(sheet);

    struct d_check_tally* const tally =
        calloc(d_dss_rule_count(sheet) * 8u + 8u,
               sizeof(struct d_check_tally));

    if (!tally)
    {
        d_dss_free(sheet);
        free(source);
        return 2;
    }

    size_t tally_count = 0;

    for (size_t at = 0; at < rule_count; ++at)
    {
        const struct d_dss_rule* const rule = d_dss_rule_at(sheet, at);

        for (uint32_t which = 0; which < rule->declaration_count; ++which)
        {
            const struct d_dss_declaration* const declaration =
                d_dss_declaration_at(sheet,
                                     rule->first_declaration + which);

            tally[tally_count].property = d_dss_text(sheet,
                                                     declaration->property);
            tally[tally_count].line     = declaration->line;

            ++tally_count;
        }
    }

    struct d_node_tree* const tree = d_node_tree_new();

    size_t      files    = 0;
    size_t      banners  = 0;
    const char* record   = NULL;
    size_t      length   = 0;

    while (walk.next_record(walk.user, &record, &length))
    {
        const char* const dot = strrchr(record, '.');

        // headers only
        if ( (!dot) ||
             ((strcmp(dot, ".h") != 0) && (strcmp(dot, ".hpp") != 0)) )
        {
            continue;
        }

        ++files;

        d_node_tree_clear(tree);

        if (d_banner_build(tree, record, argv[first + 1])
            == D_DSS_NO_INDEX)
        {
            continue;
        }

        ++banners;

        size_t slot = 0;

        // A node's columns are measured once, from the file as it was read.
        // Applying a second repair to the same file in the same pass would
        // splice at offsets that the first repair has already moved, which
        // corrupts the line rather than fixing it.  One repair per file per
        // pass; the next pass measures what is actually on disk.
        bool repaired_this_file = false;

        for (size_t at = 0; at < rule_count; ++at)
        {
            const struct d_dss_rule* const rule = d_dss_rule_at(sheet, at);

            for (uint32_t node = 0;
                 node < (uint32_t)d_node_count(tree);
                 ++node)
            {
                if (!d_match_rule(sheet, tree, node, rule))
                {
                    continue;
                }

                for (uint32_t which = 0;
                     which < rule->declaration_count;
                     ++which)
                {
                    const struct d_dss_declaration* const declaration =
                        d_dss_declaration_at(sheet,
                                             rule->first_declaration + which);

                    bool evaluated = false;

                    const bool held = d_registry_evaluate(sheet, tree, node,
                                                          declaration,
                                                          &evaluated);

                    if (!evaluated)
                    {
                        continue;
                    }

                    ++tally[slot + which].matched;

                    if (!held)
                    {
                        ++tally[slot + which].failed;

                        if ((!fixing) || (repaired_this_file))
                        {
                            continue;
                        }

                        struct d_node* const target = d_node_at(tree, node);

                        if ((!target) || (target->line == 0))
                        {
                            continue;
                        }

                        char line_text[D_BANNER_LINE_MAX];

                        // the line on disk, not the node's own text
                        if (!d_edit_read_line(record, target->line,
                                                  line_text,
                                                  sizeof(line_text)))
                        {
                            continue;
                        }

                        char repaired[D_BANNER_LINE_MAX];

                        char        derived[D_BANNER_LINE_MAX];
                        const char* derived_or_null = NULL;

                        if (strcmp(d_dss_text(sheet, declaration->property),
                                   "content") == 0)
                        {
                            if (!d_registry_derive(sheet, tree, node,
                                                   declaration, derived,
                                                   sizeof(derived)))
                            {
                                continue;
                            }

                            derived_or_null = derived;
                        }

                        if (!d_registry_repair(tree, node,
                                               d_dss_text(sheet,
                                                   declaration->property),
                                               d_registry_number(sheet,
                                                   declaration),
                                               line_text,
                                               derived_or_null,
                                               repaired,
                                               sizeof(repaired)))
                        {
                            continue;
                        }

                        if (d_edit_rewrite_line(record, target->line,
                                                    repaired))
                        {
                            ++repairs;
                            repaired_this_file = true;
                            break;
                        }
                    }
                }

                if (repaired_this_file)
                {
                    break;
                }
            }

            slot += rule->declaration_count;

            if (repaired_this_file)
            {
                break;
            }
        }
    }

    (void)printf("\n%zu headers, %zu with a banner\n\n", files, banners);
    (void)printf("  %-16s %-8s %8s %8s  %s\n", "PROPERTY", "SHEET",
                 "MATCHED", "FAILED", "RATE");

    for (size_t at = 0; at < tally_count; ++at)
    {
        if (tally[at].matched == 0)
        {
            continue;
        }

        (void)printf("  %-16s %-8u %8zu %8zu  %5.1f%%\n",
                     tally[at].property,
                     tally[at].line,
                     tally[at].matched,
                     tally[at].failed,
                     100.0 * (double)tally[at].failed
                           / (double)tally[at].matched);
    }

    if (fixing)
    {
        (void)printf("\n  %zu lines repaired\n", repairs);
    }

    if (walk.release)
    {
        walk.release(walk.user);
    }

    d_node_tree_free(tree);
    free(tally);
    d_dss_free(sheet);
    free(source);

    return 0;
}
