/*******************************************************************************
* djinterp [c/parse]                                     syntax_tests_fixtures.c
*
* The syntax items' suite, its fixtures: a text lexed, grouped and
* recognized, a token found by its spelling, and expectations held to what
* was recognized, each that fails noted on the test.
*
*
* path:      /tests/djinterp/c/modules/parse/syntax/syntax_tests_fixtures.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "./syntax_tests.h"  // corresponding header


/*
d_tests_syntax_find
  The index of the `_nth` token (from 0) spelled `_spelling`, or the count
where there is none.
*/
size_t
d_tests_syntax_find(
    const struct dj_tests_syntax_run* _run,
    const char*                       _spelling,
    uint32_t                          _nth
)
{
    const size_t length = strlen(_spelling);
    uint32_t     seen   = 0u;

    for (size_t i = 0u; i < _run->count; ++i)
    {
        if ( (_run->tokens[i].span.length == length) &&
             (strncmp(_run->text + _run->tokens[i].span.offset, _spelling,
                      length) == 0) )
        {
            if (seen == _nth)
            {
                return i;
            }

            ++seen;
        }
    }

    return _run->count;
}

/*
d_tests_syntax_read
  Lexes a text as C++ or C with the lexer's options given, then groups and
recognizes it, with the syntax options given or, for NULL, by
d_syntax_items_build, as a caller that names none does.
*/
bool
d_tests_syntax_read(
    struct dj_tests_syntax_run*     _run,
    const char*                     _text,
    bool                            _cpp,
    unsigned                        _lex,
    const struct dj_syntax_options* _options
)
{
    struct dj_source source;

    memset(_run, 0, sizeof(*_run));
    _run->text = _text;
    d_source_init(&source, _text, strlen(_text), (_cpp) ? "t.hpp" : "t.h");

    struct dj_lexer* lexer = (_cpp) ? d_lex_cpp_create(&source, 0u, _lex)
                                    : d_lex_c_create(&source, 0u, _lex);

    _run->count = (lexer) ? d_lex_tokenize(lexer, _run->tokens,
                                           D_TESTS_SYNTAX_TOKENS)
                          : 0u;
    d_lex_destroy(lexer);

    if ( (_run->count == 0u) || (_run->count >= D_TESTS_SYNTAX_TOKENS) ||
         (!d_syntax_group(_run->tokens, _run->count, &_run->groups)) )
    {
        D_TEST_NOTE("the text did not lex and group (%lu tokens)",
                    (unsigned long)_run->count);

        return false;
    }

    if ( (_options)
         ? (!d_syntax_items_build_with(_run->tokens, _run->count,
                                       &_run->groups, _text, _options,
                                       &_run->items))
         : (!d_syntax_items_build(_run->tokens, _run->count, &_run->groups,
                                  _text, &_run->items)) )
    {
        D_TEST_NOTE("the items were not built");

        return false;
    }

    return true;
}

/*
d_tests_syntax_release
  Releases a run's groups and items.
*/
void
d_tests_syntax_release(
    struct dj_tests_syntax_run* _run
)
{
    d_syntax_items_free(&_run->items);
    d_syntax_groups_free(&_run->groups);

    return;
}

/*
d_tests_syntax_spelled
  Whether a token, or D_SYNTAX_NONE, is spelled as expected, NULL meaning
none.
*/
static bool
d_tests_syntax_spelled(
    const struct dj_tests_syntax_run* _run,
    uint32_t                          _at,
    const char*                       _spelling
)
{
    if ( (_at == D_SYNTAX_NONE) || (!_spelling) )
    {
        return ( (_at == D_SYNTAX_NONE) && (!_spelling) );
    }

    return ( (_run->tokens[_at].span.length == strlen(_spelling)) &&
             (strncmp(_run->text + _run->tokens[_at].span.offset,
                      _spelling, strlen(_spelling)) == 0) );
}

/*
d_tests_syntax_params
  How many parameters an item's group holds: the items whose scope is it.
*/
static int
d_tests_syntax_params(
    const struct dj_tests_syntax_run* _run,
    const struct dj_syntax_item*      _item
)
{
    int count = 0;

    for (size_t k = 0u;
         ( (_item->params != D_SYNTAX_NONE) && (k < _run->items.count) );
         ++k)
    {
        if ( (_run->items.items[k].scope == _item->params) &&
             (_run->items.items[k].kind == D_SYNTAX_ITEM_PARAM) )
        {
            ++count;
        }
    }

    return count;
}

/*
d_tests_syntax_check
  Holds the item beginning at an expectation's token to it, and notes what
differs.
*/
static bool
d_tests_syntax_check(
    const struct dj_tests_syntax_run*    _run,
    const struct dj_tests_syntax_expect* _expect
)
{
    const size_t at = d_tests_syntax_find(_run, _expect->first, _expect->nth);
    const struct dj_syntax_item* item = NULL;

    for (size_t k = 0u; ( (at < _run->count) && (k < _run->items.count) ); ++k)
    {
        if (_run->items.items[k].first == (uint32_t)at)
        {
            item = &_run->items.items[k];
            break;
        }
    }

    if ( (item)                                                     &&
         (item->kind == _expect->kind)                              &&
         (item->statement == _expect->statement)                    &&
         (d_tests_syntax_spelled(_run, item->name, _expect->name))  &&
         ( (_expect->params == D_TESTS_SYNTAX_ANY) ||
           (d_tests_syntax_params(_run, item) == _expect->params) ) &&
         ((item->flags & _expect->set) == _expect->set)             &&
         ((item->flags & _expect->clear) == 0u)                     &&
         (item->level == _expect->level)                            &&
         ( (!_expect->reaches) ||
           (item->last >= (uint32_t)d_tests_syntax_find(_run,
                                                        _expect->reaches,
                                                        0u)) ) )
    {
        return true;
    }

    if (!item)
    {
        D_TEST_NOTE("`%s` (%lu): no item begins there", _expect->first,
                    (unsigned long)_expect->nth);
    }
    else
    {
        D_TEST_NOTE("`%s` (%lu): %s %s, %d parameter(s), flags 0x%04x, "
                    "level %u",
                    _expect->first, (unsigned long)_expect->nth,
                    d_syntax_item_kind_name(item->kind),
                    d_syntax_statement_name(item->statement),
                    d_tests_syntax_params(_run, item),
                    (unsigned)item->flags, (unsigned)item->level);
    }

    return false;
}

/*
d_tests_syntax_text
  Reads a text as C++ (or C) with the head options, and the syntax options
given, then holds each expectation to it; every one that fails is noted.
*/
bool
d_tests_syntax_text(
    const char*                          _text,
    bool                                 _cpp,
    const struct dj_syntax_options*      _options,
    const struct dj_tests_syntax_expect* _expect,
    size_t                               _expected
)
{
    static struct dj_tests_syntax_run run;

    const bool built = d_tests_syntax_read(&run, _text, _cpp,
                                           D_TESTS_SYNTAX_LEX, _options);
    bool       ok    = built;

    for (size_t i = 0u; ( (built) && (i < _expected) ); ++i)
    {
        if (!d_tests_syntax_check(&run, &_expect[i]))
        {
            ok = false;
        }
    }

    d_tests_syntax_release(&run);

    return ok;
}

/*
d_tests_syntax_pp_text
  Reads a text as C, the lexer's options none, and holds each expected
token to its count of conditionals, and its item to its `pp_total`; every
one that fails is noted.
*/
bool
d_tests_syntax_pp_text(
    const char*                             _text,
    const struct dj_tests_syntax_pp_expect* _expect,
    size_t                                  _expected
)
{
    static struct dj_tests_syntax_run run;

    const bool built = ( (d_tests_syntax_read(&run, _text, false, 0u,
                                               NULL)) &&
                         (run.items.pp_of) );
    bool       ok    = built;

    for (size_t i = 0u; ( (built) && (i < _expected) ); ++i)
    {
        const size_t   at   = d_tests_syntax_find(&run, _expect[i].spelling,
                                                  _expect[i].nth);
        const uint32_t item = (at < run.count) ? run.items.item_of[at]
                                               : D_SYNTAX_NONE;

        if ( (at == run.count)                                       ||
             (run.items.pp_of[at] != _expect[i].pp)                  ||
             ( (_expect[i].total != D_TESTS_SYNTAX_PP_ANY) &&
               ( (item == D_SYNTAX_NONE) ||
                 (run.items.items[item].pp_total != _expect[i].total) ) ) )
        {
            D_TEST_NOTE("`%s` (%lu): pp %d, its item's total %d; not %d, %d",
                        _expect[i].spelling, (unsigned long)_expect[i].nth,
                        (at == run.count) ? -1 : (int)run.items.pp_of[at],
                        (item == D_SYNTAX_NONE)
                            ? -1
                            : (int)run.items.items[item].pp_total,
                        (int)_expect[i].pp, (int)_expect[i].total);
            ok = false;
        }
    }

    d_tests_syntax_release(&run);

    return ok;
}
