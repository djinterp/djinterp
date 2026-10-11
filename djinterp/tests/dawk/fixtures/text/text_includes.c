/* text_includes.c: which long #include lines the limit does not measure */
#include "../../../../../../inc/djinterp/a/path/that/is/long/text_includes.h"  // corresponding header
// negative above: the guide's words on the file's own header
    #include "../../../../../../../../../../inc/djinterp/a/long/path/djinterp.h"  // framework root
// positive above: words that were the guide's and are not: measured as any
#include "../../../../../../inc/djinterp/a/path/that/is/long/another_header.h"  // what it gives
// positive above: a summary in the author's words can be shortened
#include "../../../../../../../../../../inc/djinterp/a/path/that/is/very/long/indeed/header.h"
// negative above: the path alone passes the limit, and a path is indivisible
#include "../../../../../../../../../../inc/djinterp/a/path/that/is/very/long/indeed/header.h"  // words
// negative above: so it is with the author's words after it
#  include "../../../../../../inc/djinterp/a/path/that/is/long/text_includes.h"  // corresponding header
// negative above: blanks after the `#`, as inside a conditional
static const char* const d_text_words = "a string that runs to the line's end";  // corresponding header
// positive above: the words at the end of a line that is no #include
// a comment that names #include and is long enough to pass the limit  // framework root
// positive above: a comment that names the directive is not one
#include "../../../../../../inc/djinterp/a/path/that/is/long/text_includes.h"  // corresponding header.
// positive above: the summary is not the guide's, by one period
#include <a/system/header/whose/path/in/angle/brackets/passes/the/limit/by/a/few/characters.h>
// negative above: the same in angle brackets
#include "./short.h"  // corresponding header
// negative above: within the limit, as any line
    #include "../../../../../../../../inc/djinterp/a/long/path/text_includes.h"  // corresponding header
// negative above: the guide's words still, and the line indented
