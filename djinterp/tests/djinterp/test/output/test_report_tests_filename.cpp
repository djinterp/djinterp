/*******************************************************************************
* djinterp [test]                                 test_report_tests_filename.cpp
*
*   Section VII of the test_report suite: FILE-NAME EXPANSION.
*
*   expand_report_file_name lowers a pattern into a path by folding in the run's
* facts - {module}, {index}, {title}, {date}, {time}, {ext} - sanitizing the two
* components that come from user data so a module name is safe on disk.  It is the
* mechanism behind the file_name_pattern option and the document_per_module layout,
* which means the strings it produces are CREATED AS FILES.  That raises the stakes
* on everything below: a bug here is not a wrong label in a report, it is a file
* written somewhere unintended.
*
*   AND THERE IS ONE.  The expansion is six sequential replace_all passes over the
* pattern, and a value substituted by an EARLY pass is re-scanned by every LATER
* one.  Sanitization maps hostile characters to '_' but leaves BRACES alone - so a
* module named "{title}" survives sanitization intact, is substituted into the
* pattern by the {module} pass, and is then read as a placeholder by the {title}
* pass that follows it.  The module name pulls in the report title.  Same for
* {date}, {time}, and {ext}.
*
*   tests_report_expand_is_vulnerable_to_placeholder_injection demonstrates it.
*
*
* path:      /tests/djinterp/test/output/test_report_tests_filename.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstddef>
#include <string>
// djinterp
#include "test_report_tests.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                sanitize_file_component_helper                            ///
///////////////////////////////////////////////////////////////////////////////

// tests_report_sanitize_replaces_every_hostile_character
//   the documented hostile set: path separators, wildcards, redirection and quote
// characters, and whitespace.  Every one becomes '_', so a module name can never
// escape its directory or open a wildcard.
bool
tests_report_sanitize_replaces_every_hostile_character()
{
    //   each hostile character, alone, becomes exactly one underscore
    const char _hostile[] = { '/', '\\', ':', '*', '?', '"', '<', '>', '|', ' ', '\t' };

    std::size_t _i = 0;

    for (_i = 0; _i < sizeof(_hostile); ++_i)
    {
        const std::string _one(1, _hostile[_i]);

        D_RT_CHECK(dt::internal::sanitize_file_component_helper(_one) == "_");
    }

    //   ...and all of them together, in a plausibly nasty name
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("a/b\\c:d*e?f\"g<h>i|j k\tl") ==
               "a_b_c_d_e_f_g_h_i_j_k_l");

    //   the traversal attack the sanitizer exists to stop
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("../../etc/passwd") ==
               ".._.._etc_passwd");

    //   ...note the DOTS survive (they are not hostile), so the result is not a
    // traversal but it does still contain "..".  It cannot separate components,
    // which is what matters: ".._.." is one harmless filename, not two directories.
    D_RT_CHECK(!contains(dt::internal::sanitize_file_component_helper("../../etc/passwd"), "/"));

    //   the length is preserved exactly - substitution, never deletion
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("a/b").size() == 3u);

    return true;
}


// tests_report_sanitize_replaces_control_characters
//   everything below 0x20 - newline, carriage return, NUL-adjacent control codes -
// is hostile.  A module name carrying an embedded newline cannot smuggle it into a
// filename.
bool
tests_report_sanitize_replaces_control_characters()
{
    std::string _raw = "a";

    _raw += '\n';
    _raw += 'b';
    _raw += '\r';
    _raw += 'c';
    _raw += '\x01';
    _raw += 'd';

    D_RT_CHECK(dt::internal::sanitize_file_component_helper(_raw) == "a_b_c_d");

    //   the boundary: 0x1F is hostile, 0x20 (space) is hostile for its own reason,
    // and 0x21 ('!') is not
    D_RT_CHECK(dt::internal::sanitize_file_component_helper(std::string(1, '\x1f')) == "_");
    D_RT_CHECK(dt::internal::sanitize_file_component_helper(std::string(1, '\x20')) == "_");
    D_RT_CHECK(dt::internal::sanitize_file_component_helper(std::string(1, '\x21')) == "!");

    return true;
}


// tests_report_sanitize_lets_del_through
//   A GAP, pinned as fact.
//
//   The control-character test is `(unsigned char)c < 0x20`, which catches
// everything below space and nothing above it.  DEL (0x7F) is a control character
// that sits ABOVE the printable range, so it sails through untouched.
//
//   Minor - a filename containing DEL is legal on POSIX and merely unpleasant - but
// it is a hole in a check whose stated purpose is "control characters", and the fix
// is one clause.  Pinned so it is a known gap rather than an unknown one.
bool
tests_report_sanitize_lets_del_through()
{
    const std::string _del(1, '\x7f');

    //   DEL is NOT sanitized - it comes back unchanged
    D_RT_CHECK(dt::internal::sanitize_file_component_helper(_del) == _del);
    D_RT_CHECK(dt::internal::sanitize_file_component_helper(_del) != "_");

    //   ...while every control character below space IS caught
    D_RT_CHECK(dt::internal::sanitize_file_component_helper(std::string(1, '\x1f')) == "_");

    //   so a module name can carry a DEL into a real filename
    std::string _name = "core";

    _name += '\x7f';

    D_RT_CHECK(contains(dt::internal::sanitize_file_component_helper(_name), "\x7f"));
    D_RT_CHECK(contains(dt::expand_report_file_name("{module}.{ext}", _name, 0, "", "pdf"),
                        "\x7f"));

    return true;
}


// tests_report_sanitize_passes_utf8_through
//   the hostile test is on the UNSIGNED value, so bytes >= 0x80 are not treated as
// control codes and a UTF-8 module name survives intact.  This is deliberate and
// correct: mangling every non-ASCII byte to '_' would make a Japanese or accented
// module name unreadable rather than merely safe.
bool
tests_report_sanitize_passes_utf8_through()
{
    //   "café" in UTF-8 - the é is two bytes, both >= 0x80
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("caf\xc3\xa9") == "caf\xc3\xa9");

    //   a high byte on its own is not mistaken for a control character.  (Had the
    // test been on a signed char, 0xC3 would read as negative and slip past a naive
    // `< 0x20` - which is exactly the bug the cast to unsigned char avoids.)
    D_RT_CHECK(dt::internal::sanitize_file_component_helper(std::string(1, '\xc3')) ==
               std::string(1, '\xc3'));

    return true;
}


// tests_report_sanitize_does_not_trim_despite_the_comment
//   A STALE COMMENT, pinned as fact.
//
//   The helper's own documentation says:
//
//       "A leading/trailing run is left in place save for TRIMMING SURROUNDING
//        SPACES, so the result is always a usable single path component."
//
//   It does no such thing.  There is no trimming anywhere in the function - a
// leading or trailing space is simply replaced with '_' like any other hostile
// character, so the result carries a leading and trailing underscore instead.
//
//   The behaviour is defensible (the name is still a usable single component, which
// is what the comment was reaching for) but it is not what the comment describes,
// and a caller trusting the comment will be surprised by the underscores.  Either
// the comment or the code is wrong; this test pins which.
bool
tests_report_sanitize_does_not_trim_despite_the_comment()
{
    //   the comment promises "a b"; the code delivers "__a_b__"
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("  a b  ") == "__a_b__");

    //   not trimmed - the length is preserved exactly, as everywhere else
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("  a b  ").size() == 7u);

    //   leading and trailing whitespace both become underscores
    D_RT_CHECK(dt::internal::sanitize_file_component_helper(" x")  == "_x");
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("x ")  == "x_");
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("\tx") == "_x");

    //   and a name of nothing but spaces becomes nothing but underscores, rather
    // than the empty string a trimming implementation would have produced
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("   ") == "___");
    D_RT_CHECK(!dt::internal::sanitize_file_component_helper("   ").empty());

    return true;
}


// tests_report_sanitize_keeps_benign_punctuation
//   dots, dashes, underscores, and the alphanumerics are all left alone - the
// sanitizer is a denylist, not an allowlist, so an ordinary module name passes
// through completely unchanged.
bool
tests_report_sanitize_keeps_benign_punctuation()
{
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("core") == "core");
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("a.b-c_d") == "a.b-c_d");
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("v1.2.3") == "v1.2.3");
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("MiXeD_CaSe") == "MiXeD_CaSe");
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("(paren)[brack]") ==
               "(paren)[brack]");

    return true;
}


// tests_report_sanitize_of_empty_is_empty
//   the degenerate input.  Note the consequence: an unnamed module expands to an
// EMPTY filename component, so the default pattern yields ".pdf" - a dotfile with
// no stem.  The sanitizer is not the place to fix that, but it is worth knowing
// that nothing downstream fixes it either.
bool
tests_report_sanitize_of_empty_is_empty()
{
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("").empty());

    //   ...and the filename an unnamed module actually gets
    D_RT_CHECK(dt::expand_report_file_name("", "", 0, "", "pdf") == ".pdf");

    return true;
}


// tests_report_sanitize_does_not_escape_braces
//   THE INJECTION VECTOR, isolated.
//
//   '{' and '}' are not in the hostile set - reasonably enough, since they are legal
// in a filename.  But the sanitized value is then substituted into a pattern that is
// still being scanned for {placeholders} by five subsequent passes, so surviving
// braces are not inert: they are live syntax.
//
//   See tests_report_expand_is_vulnerable_to_placeholder_injection for what that
// buys an attacker (or a merely unlucky module name).
bool
tests_report_sanitize_does_not_escape_braces()
{
    //   braces pass straight through, unescaped and unreplaced
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("{ext}") == "{ext}");
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("{title}") == "{title}");
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("{}") == "{}");

    //   the sanitizer neither doubles them (as the PDF layer's brace_escape does)
    // nor strips them
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("{ext}") != "{{ext}}");
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("{ext}") != "ext");

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///                replace_all_helper                                        ///
///////////////////////////////////////////////////////////////////////////////

// tests_report_replace_all_replaces_every_occurrence
bool
tests_report_replace_all_replaces_every_occurrence()
{
    D_RT_CHECK(dt::internal::replace_all_helper("{m}-{m}", "{m}", "v") == "v-v");
    D_RT_CHECK(dt::internal::replace_all_helper("{m}{m}{m}", "{m}", "x") == "xxx");
    D_RT_CHECK(dt::internal::replace_all_helper("a{m}b", "{m}", "") == "ab");  // empty value
    D_RT_CHECK(dt::internal::replace_all_helper("", "{m}", "v").empty());      // empty subject

    // adjacent and overlapping-looking cases
    D_RT_CHECK(dt::internal::replace_all_helper("aaa", "aa", "b") == "ba");    // left to right

    return true;
}


// tests_report_replace_all_of_an_empty_token_is_the_subject
//   the guard against an infinite loop: an empty token would match at every
// position forever, so it is short-circuited to return the subject untouched.
bool
tests_report_replace_all_of_an_empty_token_is_the_subject()
{
    D_RT_CHECK(dt::internal::replace_all_helper("a{b}c", "", "X") == "a{b}c");
    D_RT_CHECK(dt::internal::replace_all_helper("", "", "X").empty());

    return true;
}


// tests_report_replace_all_is_not_recursive_within_a_call
//   the documented guarantee - "a value containing _token is not re-scanned" - and
// it holds, WITHIN ONE CALL: substituting "{x}" for "{x}" yields "{x}{x}" and stops,
// rather than looping forever.
//
//   Read the guarantee carefully, though.  It is per-CALL, not per-EXPANSION.
// expand_report_file_name makes SIX of these calls in sequence, and each one
// re-scans everything the previous ones substituted.  The non-recursion here is
// exactly what makes the injection below possible to reason about - and exactly what
// fails to prevent it.
bool
tests_report_replace_all_is_not_recursive_within_a_call()
{
    //   the value contains the token, and is not re-scanned - the call terminates
    D_RT_CHECK(dt::internal::replace_all_helper("{x}", "{x}", "{x}{x}") == "{x}{x}");

    //   ...nor is a value that would form a NEW token when spliced against its
    // neighbours
    D_RT_CHECK(dt::internal::replace_all_helper("{a}{b}", "{a}", "{") == "{{b}");

    //   one pass, left to right, no second look
    D_RT_CHECK(dt::internal::replace_all_helper("{m}", "{m}", "{m}") == "{m}");

    return true;
}


// tests_report_replace_all_leaves_an_absent_token_alone
bool
tests_report_replace_all_leaves_an_absent_token_alone()
{
    D_RT_CHECK(dt::internal::replace_all_helper("abc", "{m}", "v") == "abc");
    D_RT_CHECK(dt::internal::replace_all_helper("{other}", "{m}", "v") == "{other}");

    //   a partial match is not a match
    D_RT_CHECK(dt::internal::replace_all_helper("{module_name}", "{module}", "v") ==
               "{module_name}");

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///                expand_report_file_name                                   ///
///////////////////////////////////////////////////////////////////////////////

// tests_report_expand_empty_pattern_falls_back
//   an unset pattern is not an error - it becomes "{module}.{ext}", so a caller who
// configures nothing still gets a sensible per-module filename.
bool
tests_report_expand_empty_pattern_falls_back()
{
    D_RT_CHECK(dt::expand_report_file_name("", "core", 0, "T", "pdf") == "core.pdf");
    D_RT_CHECK(dt::expand_report_file_name("", "core", 7, "T", "html") == "core.html");

    //   the fallback is exactly the documented one - the index and title are not in
    // it, so they do not appear
    D_RT_CHECK(!contains(dt::expand_report_file_name("", "core", 7, "TITLE", "pdf"), "7"));
    D_RT_CHECK(!contains(dt::expand_report_file_name("", "core", 7, "TITLE", "pdf"), "TITLE"));

    return true;
}


// tests_report_expand_folds_every_placeholder
//   the whole vocabulary, in one pattern.  Each of the six is substituted; none is
// left behind.
bool
tests_report_expand_folds_every_placeholder()
{
    const std::string _out = dt::expand_report_file_name(
        "{title}_{module}_{index}.{ext}", "core", 3, "The Run", "pdf");

    D_RT_CHECK(_out == "The_Run_core_3.pdf");        // note the title's space -> '_'

    //   no placeholder survives
    D_RT_CHECK(!contains(_out, "{"));
    D_RT_CHECK(!contains(_out, "}"));

    //   {index} is emitted verbatim, as a decimal - the model does not renumber it,
    // so whatever the caller passes is what lands in the name
    D_RT_CHECK(contains(dt::expand_report_file_name("{index}", "m", 0,  "", ""), "0"));
    D_RT_CHECK(dt::expand_report_file_name("{index}", "m", 42, "", "") == "42");

    //   {date} and {time} are covered separately - they read a live clock

    return true;
}


// tests_report_expand_preserves_literals_and_directories
//   the LITERAL portions of the pattern are emitted untouched - including any
// directory prefix the caller supplies.  Only the substituted VALUES are sanitized,
// which is what lets a caller write a path and still get safe components inside it.
bool
tests_report_expand_preserves_literals_and_directories()
{
    D_RT_CHECK(dt::expand_report_file_name("out/reports/{module}.{ext}",
                                           "core", 1, "T", "pdf") ==
               "out/reports/core.pdf");

    //   the separators in the PATTERN survive (they are literal) while separators in
    // the VALUE do not (they are sanitized) - the whole point of the split
    D_RT_CHECK(dt::expand_report_file_name("out/{module}.{ext}",
                                           "a/b", 0, "T", "pdf") ==
               "out/a_b.pdf");

    //   a pattern with no placeholders at all is returned as-is
    D_RT_CHECK(dt::expand_report_file_name("fixed.txt", "core", 0, "T", "pdf") ==
               "fixed.txt");

    //   ...and the extension is a literal dot plus a substituted value, so a caller
    // may put the dot wherever they like
    D_RT_CHECK(dt::expand_report_file_name("{module}-report.{ext}",
                                           "core", 0, "T", "pdf") ==
               "core-report.pdf");

    return true;
}


// tests_report_expand_leaves_an_unknown_placeholder_alone
//   a token outside the vocabulary is not an error and is not blanked - it is left
// in the output verbatim.  So a typo'd pattern produces a filename containing a
// literal "{moduel}", which is at least VISIBLE (the same failure mode the render
// layer chose, and the right one - a silently blanked component would collide).
bool
tests_report_expand_leaves_an_unknown_placeholder_alone()
{
    D_RT_CHECK(dt::expand_report_file_name("{nope}-{module}.{ext}",
                                           "core", 1, "T", "pdf") ==
               "{nope}-core.pdf");

    //   the classic typo
    D_RT_CHECK(contains(dt::expand_report_file_name("{moduel}.{ext}", "core", 0, "", "pdf"),
                        "{moduel}"));

    //   an unclosed brace is likewise inert
    D_RT_CHECK(dt::expand_report_file_name("{module.{ext}", "core", 0, "", "pdf") ==
               "{module.pdf");

    return true;
}


// tests_report_expand_sanitizes_module_and_title_but_not_ext
//   the two components that come from USER DATA are sanitized; the extension is not.
// That is a reasonable division (the extension is chosen by the framework, not the
// user) but it does mean an ext carrying a separator would build a path:
//
//       expand("{module}.{ext}", "core", 0, "", "p/df")  ->  "core.p/df"
//
//   which is a directory, not a file.  Not reachable through the framework's own
// callers, but the asymmetry is worth knowing.
bool
tests_report_expand_sanitizes_module_and_title_but_not_ext()
{
    //   the module is sanitized
    D_RT_CHECK(dt::expand_report_file_name("{module}.{ext}", "a b/c", 0, "", "pdf") ==
               "a_b_c.pdf");

    //   the title is sanitized
    D_RT_CHECK(dt::expand_report_file_name("{title}.{ext}", "m", 0, "My T:tle", "pdf") ==
               "My_T_tle.pdf");

    //   the extension is NOT
    D_RT_CHECK(dt::expand_report_file_name("{module}.{ext}", "core", 0, "", "p/df") ==
               "core.p/df");
    D_RT_CHECK(contains(dt::expand_report_file_name("{module}.{ext}", "core", 0, "", "p df"),
                        " "));

    return true;
}


// tests_report_expand_dates_from_the_live_clock
//   {date} and {time} read the LOCAL WALL CLOCK, so their values cannot be asserted
// without the suite failing at midnight.  What IS a contract is their SHAPE:
// YYYYMMDD (8 digits) and HHMMSS (6 digits), both bare decimal.  Asserted
// structurally, and asserted to be STABLE within one expansion.
bool
tests_report_expand_dates_from_the_live_clock()
{
    const std::string _date = dt::expand_report_file_name("{date}", "m", 0, "", "");
    const std::string _time = dt::expand_report_file_name("{time}", "m", 0, "", "");

    D_RT_CHECK(_date.size() == 8u);        // YYYYMMDD
    D_RT_CHECK(is_digits(_date));

    D_RT_CHECK(_time.size() == 6u);        // HHMMSS
    D_RT_CHECK(is_digits(_time));

    //   the year is plausible - this pins the FORMAT (year first, not day first),
    // which a %d%m%Y typo would break and a digit-count check alone would not
    D_RT_CHECK(_date.compare(0, 2, "20") == 0);

    //   the two placeholders are independent, and both survive in one pattern
    const std::string _both =
        dt::expand_report_file_name("{date}-{time}.{ext}", "m", 0, "", "pdf");

    D_RT_CHECK(_both.size() == (8u + 1u + 6u + 4u));    // 8 + '-' + 6 + ".pdf"
    D_RT_CHECK(contains(_both, "-"));
    D_RT_CHECK(contains(_both, ".pdf"));

    //   the helper itself fills both, or neither
    std::string _d;
    std::string _t;

    dt::internal::report_timestamp_helper(_d, _t);

    D_RT_CHECK(_d.size() == 8u);
    D_RT_CHECK(_t.size() == 6u);
    D_RT_CHECK(is_digits(_d));
    D_RT_CHECK(is_digits(_t));

    return true;
}


// tests_report_expand_repeats_a_placeholder
//   a placeholder used twice is substituted twice - replace_all, not replace_first.
bool
tests_report_expand_repeats_a_placeholder()
{
    D_RT_CHECK(dt::expand_report_file_name("{module}/{module}.{ext}",
                                           "core", 0, "", "pdf") ==
               "core/core.pdf");

    D_RT_CHECK(dt::expand_report_file_name("{index}_{index}_{index}",
                                           "m", 5, "", "") ==
               "5_5_5");

    return true;
}


// tests_report_expand_is_vulnerable_to_placeholder_injection
//   THE BUG.
//
//   expand_report_file_name is six sequential replace_all passes, in this order:
//
//       1. {module}    <- value comes from user data, sanitized
//       2. {title}     <- value comes from user data, sanitized
//       3. {index}
//       4. {date}
//       5. {time}
//       6. {ext}
//
//   Each pass rewrites the WHOLE string, including the text the earlier passes just
// substituted into it.  replace_all_helper is non-recursive WITHIN a call, so no
// single pass loops - but nothing stops pass 2 from reading what pass 1 wrote.
//
//   Sanitization does not save us, because '{' and '}' are not hostile characters:
// a module named "{title}" is sanitized to "{title}" (unchanged) and substituted
// verbatim into the pattern, where pass 2 promptly expands it.
//
//   So the MODULE NAME can inject any placeholder that is substituted after it:
//
//       module "{title}"  ->  the filename contains the report TITLE
//       module "{date}"   ->  the filename contains the run DATE
//       module "{ext}"    ->  the filename contains the EXTENSION
//
//   How bad is it?  Not a path traversal - the injected VALUES are themselves either
// sanitized ({title}) or framework-controlled ({index}, {date}, {time}, {ext}), so an
// attacker cannot smuggle a '/' through this door.  But it is a real
// confused-deputy: a module name silently becomes something else, two modules can
// COLLIDE onto one filename (overwriting each other's report), and the mechanism is
// entirely invisible at the call site.
//
//   THE FIX is either to sanitize braces out of the substituted components (one
// clause in the hostile set), or to expand the pattern in a SINGLE pass with a real
// scanner - the same design the render layer already uses, and for exactly this
// reason.
bool
tests_report_expand_is_vulnerable_to_placeholder_injection()
{
    // -- a module named "{title}" pulls in the report title ---------------------
    D_RT_CHECK(dt::expand_report_file_name("{module}.{ext}",
                                           "{title}",      // the module's NAME
                                           0,
                                           "SECRET",       // the report's title
                                           "pdf") ==
               "SECRET.pdf");                              // <- the title, not the module

    // -- a module named "{ext}" pulls in the extension --------------------------
    D_RT_CHECK(dt::expand_report_file_name("{module}.{ext}",
                                           "{ext}",
                                           0,
                                           "T",
                                           "pdf") ==
               "pdf.pdf");

    // -- a module named "{index}" pulls in the index ----------------------------
    D_RT_CHECK(dt::expand_report_file_name("{module}.{ext}",
                                           "{index}",
                                           42,
                                           "T",
                                           "pdf") ==
               "42.pdf");

    // -- a module named "{date}" pulls in the live clock ------------------------
    const std::string _dated =
        dt::expand_report_file_name("{module}.{ext}", "{date}", 0, "T", "pdf");

    D_RT_CHECK(_dated.size() == 12u);                      // 8 digits + ".pdf"
    D_RT_CHECK(is_digits(_dated.substr(0, 8)));
    D_RT_CHECK(_dated.compare(0, 2, "20") == 0);           // ...it really is the date

    // -- the injection survives sanitization, which is why it works -------------
    D_RT_CHECK(dt::internal::sanitize_file_component_helper("{title}") == "{title}");

    // -- TWO MODULES CAN COLLIDE ONTO ONE FILENAME -----------------------------
    //     the practical consequence: a document_per_module run writes both of these
    //   modules to the SAME path, and the second silently overwrites the first
    const std::string _honest =
        dt::expand_report_file_name("{module}.{ext}", "pdf",    0, "T", "pdf");
    const std::string _injected =
        dt::expand_report_file_name("{module}.{ext}", "{ext}",  1, "T", "pdf");

    D_RT_CHECK(_honest   == "pdf.pdf");
    D_RT_CHECK(_injected == "pdf.pdf");
    D_RT_CHECK(_honest == _injected);                      // <- a collision

    // -- the ORDER of the passes is what decides which way the injection runs ---
    //     {module} is substituted FIRST, so it can reach every other placeholder.
    //   {ext} is substituted LAST, so it can reach none of them - an extension
    //   named "{module}" stays literal.
    D_RT_CHECK(dt::expand_report_file_name("x.{ext}", "core", 0, "T", "{module}") ==
               "x.{module}");                              // <- inert, not expanded

    //   and {title} (pass 2) can reach {date}/{time}/{ext} but NOT {module} (pass 1)
    D_RT_CHECK(dt::expand_report_file_name("{title}", "core", 0, "{module}", "pdf") ==
               "{module}");                                // <- inert: {module} already ran

    return true;
}


NS_END   // testing
NS_END   // djinterp
