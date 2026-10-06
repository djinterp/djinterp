/*******************************************************************************
* djinterp [net]                                               http_fuzz_http1.c
*
* A libFuzzer harness for http1.h's head parsers.
*   An input's first three bytes choose the piece size and the two limits,
* inconsistent pairs included; the rest is parsed as a request and as a
* response, whole and then fed in pieces. It aborts when the two feeds reach
* different verdicts or lengths, when a head's length passes the input, or
* when a view leaves the head; the sanitizers catch the rest. Built only with
* clang's -fsanitize=fuzzer, outside the regular suites.
*
*
* path:      /tests/djinterp/net/http/http_fuzz_http1.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/net/http/http1.h"  // the parsers
// std
#include <stddef.h>  // size_t
#include <stdint.h>  // uint8_t
#include <stdlib.h>  // abort


// d_fuzz_http1_setup
//   struct: what an input's first bytes chose.
struct d_fuzz_http1_setup
{
    size_t             piece;
    size_t             start_line_max;
    size_t             head_max;
    struct d_pack_text message;
};

/*
d_fuzz_http1_parser
  A parser with the setup's limits.
*/
static void
d_fuzz_http1_parser(
    const struct d_fuzz_http1_setup* _setup,
    struct d_http1_parser*           _parser
)
{
    d_http1_parser_init(_parser);
    _parser->start_line_max = _setup->start_line_max;
    _parser->head_max       = _setup->head_max;

    return;
}

/*
d_fuzz_http1_inside
  Whether a view lies within the first _length bytes of a message.
*/
static bool
d_fuzz_http1_inside(
    struct d_pack_text _view,
    struct d_pack_text _message,
    size_t             _length
)
{
    return ( (_view.length == 0u) ||
             ( (_view.data >= _message.data) &&
               (_view.data + _view.length <= _message.data + _length) ) );
}

/*
d_fuzz_http1_parse
  One parse of a prefix, as a request or a response, into fields.
*/
static enum d_http_error
d_fuzz_http1_parse(
    struct d_http1_parser*  _parser,
    struct d_pack_text      _data,
    bool                    _request,
    struct d_http1_request* _out,
    size_t*                 _consumed
)
{
    struct d_http1_response response = { .fields = _out->fields };
    enum d_http_error       result   = D_HTTP_OK;

    // a request, or a response reported through the request's fields
    if (_request)
    {
        return d_http1_parse_request(_parser,
                                     _data,
                                     _out,
                                     _consumed);
    }

    result            = d_http1_parse_response(_parser,
                                               _data,
                                               &response,
                                               _consumed);
    _out->fields      = response.fields;
    _out->target      = response.reason;
    _out->method_name = (struct d_pack_text){ NULL,
                                              0u };

    return result;
}

/*
d_fuzz_http1_views_inside
  Whether a head's length lies within the message, and every view within
the head.
*/
static bool
d_fuzz_http1_views_inside(
    const struct d_fuzz_http1_setup* _setup,
    const struct d_http1_request*    _out,
    size_t                           _whole
)
{
    const struct d_pack_text message = _setup->message;
    bool                     inside  = ( (_whole <= message.length)      &&
                                         (d_fuzz_http1_inside(
                                              _out->method_name,
                                              message,
                                              _whole))                   &&
                                         (d_fuzz_http1_inside(
                                              _out->target,
                                              message,
                                              _whole)) );

    // each field's name and value
    for (size_t i = 0u; i < _out->fields.count; ++i)
    {
        inside = ( (inside)                                    &&
                   (d_fuzz_http1_inside(_out->fields.items[i].name,
                                        message,
                                        _whole))               &&
                   (d_fuzz_http1_inside(_out->fields.items[i].value,
                                        message,
                                        _whole)) );
    }

    return inside;
}

/*
d_fuzz_http1_pieces
  The message fed in the setup's pieces, into a fresh parser, until a
verdict or the end: that verdict, and the head's length.
*/
static enum d_http_error
d_fuzz_http1_pieces(
    const struct d_fuzz_http1_setup* _setup,
    bool                             _request,
    struct d_http1_request*          _out,
    size_t*                          _fed
)
{
    const struct d_pack_text message = _setup->message;
    struct d_http1_parser    parser  = { .line = 0u };
    enum d_http_error        verdict = D_HTTP_ERROR_INCOMPLETE;
    size_t                   length  = 0u;

    d_fuzz_http1_parser(_setup,
                        &parser);

    // one piece more each time, until a verdict or the whole message
    while (verdict == D_HTTP_ERROR_INCOMPLETE)
    {
        const struct d_pack_text prefix = { message.data,
                                            length };

        verdict = d_fuzz_http1_parse(&parser,
                                     prefix,
                                     _request,
                                     _out,
                                     _fed);

        // the whole message has been fed
        if (length == message.length)
        {
            break;
        }

        length = (message.length - length > _setup->piece)
                     ? length + _setup->piece
                     : message.length;
    }

    return verdict;
}

/*
d_fuzz_http1_check
  Parses the message whole and in pieces, and aborts on any disagreement,
or on a length or view outside the head.
*/
static void
d_fuzz_http1_check(
    const struct d_fuzz_http1_setup* _setup,
    bool                             _request
)
{
    struct d_http1_field   items[16];
    struct d_http1_request out    = { .fields = { items,
                                                  16u,
                                                  0u } };
    struct d_http1_parser  parser = { .line = 0u };
    size_t                 whole  = 0u;
    size_t                 fed    = 0u;

    d_fuzz_http1_parser(_setup,
                        &parser);

    const enum d_http_error verdict = d_fuzz_http1_parse(&parser,
                                                         _setup->message,
                                                         _request,
                                                         &out,
                                                         &whole);

    // a length or a view out of place
    if ( (verdict == D_HTTP_OK) &&
         (!d_fuzz_http1_views_inside(_setup,
                                     &out,
                                     whole)) )
    {
        abort();
    }

    out.fields.count = 0u;

    const enum d_http_error pieces = d_fuzz_http1_pieces(_setup,
                                                         _request,
                                                         &out,
                                                         &fed);

    // the two feeds disagree
    if ( (pieces != verdict) ||
         ( (verdict == D_HTTP_OK) &&
           (fed != whole) ) )
    {
        abort();
    }

    return;
}

/*
LLVMFuzzerTestOneInput
  libFuzzer's entry point: three setup bytes, then the message.
*/
int
LLVMFuzzerTestOneInput(
    const uint8_t* _data,
    size_t         _size
)
{
    // too short to choose a setup
    if (_size < 3u)
    {
        return 0;
    }

    const struct d_fuzz_http1_setup setup =
    {
        1u + (size_t)(_data[0] % 16u),
        1u + (size_t)_data[1],
        1u + ((size_t)_data[2] * 2u),
        { (const char*)_data + 3, _size - 3u }
    };

    d_fuzz_http1_check(&setup,
                       true);
    d_fuzz_http1_check(&setup,
                       false);

    return 0;
}
