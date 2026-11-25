/*******************************************************************************
* djinterp [c][parse]                                                parse_log.h
*
* Parse's diagnostics, written to the log as its records.
*   Parse's two tables of codes - parse.expr's (expr/parse.h) and
* parse.core's (diagnostic.h) - are declared here and defined in
* parse_log.c, so that only a program built with the log links them, and
* nothing else of parse names the log. The unit is a streaming hook
* (diagnostic.h's d_parse_diag_sink_stream): attached to a sink, it writes
* each diagnostic the sink accepts as one record of its code, at its grade's
* level, with its facts, and its notes as a level "note" (one) or "notes"
* (several, keyed "0", "1", ...), each with its words and its span.
*   It asks for facts alone, since the log writes a record's text from its
* code's template - the template parse writes the sink's text from - so a
* run that only logs writes no text. A diagnostic whose emitter gave words of
* its own rather than its code's template (the machine's, the program's) is
* written with those words as the fact `message`.
*
* HOW TO USE
* ----------
*     d_parse_log_add();                // once, before settings are read
*     ...
*     d_parse_log_attach(&sink);        // each sink whose diagnostics to log
*
* OPTIONS
* -------
*   parse.log (settings.h): off, d_parse_log_attach attaches nothing, and no
* facts are gathered for the log. A record is written only where parse.log
* is on and the log wants its code: log.level, log.parse_expr.level (or
* log.parse_core.level) and its grade's level decide the rest.
*
* CONFIGURATION
* -------------
*   D_CFG_PARSE_LOG (cfg_parse.h) compiles the unit, by default where the log
* is built (D_CFG_LOG_ENABLED). D_CFG_PARSE_LOG_CODES, by default the log's
* D_CFG_LOG_CODES, builds the tables' names, templates and facts into the
* records; without them a code is written "parse.expr:4".
*   On the log's 64-bit floor, as log.h is.
*
*
* path:      /inc/djinterp/c/modules/parse/parse_log.h
* link(s):   /docs/djinterp/log/log_design.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE TABLES
    ----------
    1.  Declared
    2.  Registering
    3.  Finding a diagnostic's code
2.  THE SUBSCRIBER
    --------------
    1.  Attaching
    2.  The hook
*/

#ifndef DJINTERP_C_PARSE_LOG_H
#define DJINTERP_C_PARSE_LOG_H 1

// djinterp
#include "../../djinterp.h"                              // framework root
#include "../../../config/c/modules/parse/cfg_parse.h"   // D_INTERNAL_PARSE_LOG
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // RE_STD_HAS_INT64, uint16_t

#if ( (D_INTERNAL_PARSE_LOG == 1) && RE_STD_HAS_INT64 )

// djinterp
#include "../../log/log.h"   // the tables' kit, records
#include "./diagnostic.h"    // D_PARSE_CORE_CODES, sinks, events
#include "./expr/parse.h"    // D_PARSE_EXPR_CODES


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  THE TABLES
//==============================================================================


// 1.1    Declared
//------------------------------------------------------------------------------
// d_log_module_parse_expr, "parse.expr", and a code apiece:
// d_log_code_parse_expr_unexpected, ...; d_log_module_parse_core,
// "parse.core": d_log_code_parse_core_truncated, ...
D_LOG_CODES_DECLARE(parse_expr, D_PARSE_EXPR_CODES)
D_LOG_CODES_DECLARE(parse_core, D_PARSE_CORE_CODES)

// 1.2    Registering
//------------------------------------------------------------------------------
/**
 * @brief Registers both tables with the log, as d_log_module_add does: once,
 *        before settings are read and threads start, so that
 *        log.parse_expr.level may be set and d_log_code_find finds parse's
 *        codes.
 *
 * @return 0 when both are registered, or already were; -1 when the log
 *         refuses either.
 */
int d_parse_log_add(void);

// 1.3    Finding a diagnostic's code
//------------------------------------------------------------------------------
/**
 * @brief The log's code for a diagnostic of parse's: parse.expr's for the
 *        expression domain, parse.core's for the core, machine and program
 *        domains, which report the core codes.
 *
 * @param[in] _domain  the diagnostic's domain.
 * @param[in] _code    its code.
 * @return the code; NULL for a domain or a code with no table's row.
 */
const struct dj_log_code* d_parse_log_code(uint16_t _domain,
                                           uint16_t _code);


//==============================================================================
// 2.  THE SUBSCRIBER
//==============================================================================


// 2.1    Attaching
//------------------------------------------------------------------------------
/**
 * @brief Attaches the subscriber to a sink, as its streaming hook, asking
 *        for facts alone - where parse.log is on. Registers the tables
 *        first, if d_parse_log_add has not.
 *
 * @param[in,out] _sink  the sink; its streaming hook, if any, is replaced.
 * @return 1 when attached; 0 when parse.log is off, the sink then
 *         unchanged; -1 for a NULL sink, or tables the log refuses.
 */
int d_parse_log_attach(struct dj_parse_diag_sink* _sink);

/**
 * @brief Takes the subscriber off a sink, if it is the sink's streaming
 *        hook.
 *
 * @param[in,out] _sink  the sink; ignored if NULL.
 */
void d_parse_log_detach(struct dj_parse_diag_sink* _sink);

// 2.2    The hook
//------------------------------------------------------------------------------
/**
 * @brief The subscriber itself, a dj_parse_diag_stream_fn: writes one
 *        diagnostic, with its notes, as one record. A program whose sink
 *        streams to a hook of its own calls it from there.
 *
 *   A note given alone, apart from the diagnostic it continues, is not a
 * record of its own, and a diagnostic of a domain with no table is not
 * written.
 *
 * @param[in] _ctx    unused; NULL.
 * @param[in] _sink   the sink; unused.
 * @param[in] _event  the diagnostic, its facts and its notes.
 */
void d_parse_log_event(void*                             _ctx,
                       const struct dj_parse_diag_sink*  _sink,
                       const struct dj_parse_diag_event* _event);


D_EXTERN_C_END


#endif  // D_INTERNAL_PARSE_LOG && RE_STD_HAS_INT64

#endif  // DJINTERP_C_PARSE_LOG_H
