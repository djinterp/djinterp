/*******************************************************************************
* djinterp [c][parse]                                       stress_tests_bytes.c
*
* Expression core v2's stress suite, block "bytes": seeded random bytes
* through the default scanner, alone and into the parser. Whatever the bytes,
* a scan ends, each token lies inside the text after the one before, and a
* parse ends with a status, a term that verifies and diagnostics placed in
* the text, in steps linear in its length. The libFuzzer harness and its
* corpus replay run the same path, d_tests_stress_fuzz_one.
*
*
* path:      /tests/djinterp/c/modules/parse/stress/stress_tests_bytes.c
* link(s):   /docs/djinterp/c/modules/parse/expr/expression_core_v2.html
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "./stress_tests.h"  // corresponding header

#if ( (D_INTERNAL_PARSE_EXPR_SCAN == 1) && \
      (D_INTERNAL_PARSE_EXPR_HEAP == 1) )

// the longest random text, in bytes
#define D_INTERNAL_STRESS_TEXT_MAX  256u

// a class no role reads: a byte the scanner has no class for is a token of
// it, so a scan always reaches the end and a parse reads past every stray
#define D_INTERNAL_STRESS_STRAY     200u

// the texts a test names before it stops naming them
#define D_INTERNAL_STRESS_SHOWN     5u

// dj_tests_stress_counted
//   struct: a source that counts its reads, over the scanner's.
struct dj_tests_stress_counted
{
    struct dj_parse_expr_source inner;
    uint32_t                    reads;
};

/*
d_tests_stress_counted_next
  The scanner's next token, counted.
*/
static int
d_tests_stress_counted_next(
    void*                       _ctx,
    uint8_t                     _position,
    uint32_t                    _mode,
    struct dj_parse_expr_token* _out
)
{
    struct dj_tests_stress_counted* counted =
        (struct dj_tests_stress_counted*)_ctx;

    ++counted->reads;

    return counted->inner.next(counted->inner.ctx, _position, _mode, _out);
}

/*
d_tests_stress_text
  A random text of up to 255 bytes: three in four from the characters the
languages' tokens are made of, the fourth any byte at all, NUL included.
*/
static size_t
d_tests_stress_text(
    uint32_t* _rng,
    char*     _out
)
{
    static const char ALPHABET[] = "abcfxy019 +-*/^!'()[]|,;<>=&~$\"\\\n\t.?:";
    const size_t      length     =
        d_tests_expr_random(_rng, D_INTERNAL_STRESS_TEXT_MAX);

    for (size_t i = 0u; i < length; ++i)
    {
        _out[i] = (d_tests_expr_random(_rng, 4u) == 0u)
                  ? (char)d_tests_expr_random(_rng, 256u)
                  : ALPHABET[d_tests_expr_random(_rng,
                                                 sizeof(ALPHABET) - 1u)];
    }

    return length;
}

/*
d_tests_stress_config
  The scanner's configuration: names, numbers and strings, a stray class,
and dawk's two further words where they are wanted.
*/
static void
d_tests_stress_config(
    struct dj_parse_expr_scan_config* _config,
    bool                              _dawk
)
{
    static const struct dj_parse_expr_scan_word MORE[] =
    {
        { "getline", C_GETLINE }, { "|", C_BAR }
    };

    memset(_config, 0, sizeof(*_config));
    _config->name_class   = C_NAME;
    _config->number_class = C_NUM;
    _config->string_class = C_STRING;
    _config->stray_class  = D_INTERNAL_STRESS_STRAY;

    if (_dawk)
    {
        _config->words      = MORE;
        _config->word_count = 2u;
    }

    return;
}

/*
d_tests_stress_scan_one
  Scans one text to its end: true where every read gave a token, each
inside the text, not before the last one's end and not empty, and the reads
were at most one more than the bytes.
*/
static bool
d_tests_stress_scan_one(
    const struct dj_parse_expr_signature*   _s,
    const struct dj_parse_expr_scan_config* _config,
    const char*                             _text,
    size_t                                  _length,
    uint32_t*                               _tokens
)
{
    struct dj_parse_expr_source  source;
    struct dj_parse_expr_scanner scanner;
    struct dj_parse_expr_token   token;
    uint32_t                     end   = 0u;
    uint32_t                     reads = 0u;

    if (!d_parse_expr_scan(&source, &scanner, _s, _config, _text, _length))
    {
        return false;
    }

    for (;;)
    {
        const int got = source.next(source.ctx, D_PARSE_EXPR_OPERAND, 1u,
                                    &token);

        ++reads;

        // linear: one read a byte, and one for the end
        if ( (got < 0) ||
             (reads > (uint32_t)_length + 1u) )
        {
            return false;
        }

        if (got == 0)
        {
            break;
        }

        if ( (token.length == 0u)                              ||
             (token.offset < end)                              ||
             ((size_t)token.offset + token.length > _length) )
        {
            return false;
        }

        end = token.offset + token.length;
        ++*_tokens;
    }

    return true;
}

/*
d_tests_stress_parse_one
  Scans and parses one text: true where the parse ended with a status, read
at most two more tokens than the text has bytes, left a term that verifies,
and placed every diagnostic inside the text.
*/
static bool
d_tests_stress_parse_one(
    const struct dj_parse_expr_signature*   _s,
    const struct dj_parse_expr_scan_config* _config,
    const char*                             _text,
    size_t                                  _length,
    int*                                    _status
)
{
    static struct dj_parse_diagnostic   items[32];
    static char                         messages[4096];
    struct dj_parse_diag_sink           sink;
    struct dj_parse_expr_term           term;
    struct dj_parse_expr_result         result;
    struct dj_parse_expr_options        options;
    struct dj_parse_expr_scanner        scanner;
    struct dj_tests_stress_counted      counted;
    struct dj_parse_expr_source         source;
    bool                                held;

    memset(&options, 0, sizeof(options));
    memset(&result, 0, sizeof(result));
    memset(&counted, 0, sizeof(counted));
    d_parse_diag_sink_init(&sink, items, 32u, messages,
                           (uint32_t)sizeof(messages));
    d_parse_expr_term_init_heap(&term);

    if (!d_parse_expr_scan(&counted.inner, &scanner, _s, _config, _text,
                           _length))
    {
        d_parse_expr_term_release(&term);

        return false;
    }

    source.next         = d_tests_stress_counted_next;
    source.ctx          = &counted;
    options.text        = _text;
    options.text_length = _length;
    *_status            = d_parse_expr_parse(_s, &source, &options, &term,
                                             &sink, &result);
    held                = ( ( (*_status == D_PARSE_EXPR_OK)    ||
                              (*_status == D_PARSE_EXPR_ERROR) ||
                              (*_status == D_PARSE_EXPR_FATAL) )          &&
                            (counted.reads <= (uint32_t)_length + 2u)     &&
                            (d_parse_expr_term_verify(&term)) );

    for (uint32_t i = 0u; ( (held) && (i < sink.count) && (i < 32u) ); ++i)
    {
        held = ( (size_t)items[i].span.offset + items[i].span.length <=
                 _length );
    }

    d_parse_expr_term_release(&term);

    return held;
}

/*
d_tests_stress_scan_bytes
  Random bytes through the default scanner.
  Tests the following:
  - each of D_TESTS_PARSE_STRESS_TEXTS texts, drawn from a fixed seed, scans
    to its end, by math's signature and dawk's in turn
  - each token lies inside the text, after the last, and is not empty
  - a text of n bytes takes at most n + 1 reads
*/
bool
d_tests_stress_scan_bytes(void)
{
    struct dj_parse_expr_signature   sigs[2];
    struct dj_parse_expr_scan_config configs[2];
    static char                      text[D_INTERNAL_STRESS_TEXT_MAX];
    uint32_t                         rng    = 20261117u;
    uint32_t                         lost   = 0u;
    uint32_t                         tokens = 0u;
    bool                             ok     = ( (d_tests_expr_math(&sigs[0],
                                                                   true)) &&
                                                (d_tests_expr_dawk(&sigs[1])) );

    d_tests_stress_config(&configs[0], false);
    d_tests_stress_config(&configs[1], true);

    for (uint32_t i = 0u; ( (ok) && (i < D_TESTS_PARSE_STRESS_TEXTS) ); ++i)
    {
        const size_t   length = d_tests_stress_text(&rng, text);
        const uint32_t which  = i % 2u;

        if (!d_tests_stress_scan_one(&sigs[which], &configs[which], text,
                                     length, &tokens))
        {
            if (lost < D_INTERNAL_STRESS_SHOWN)
            {
                D_TEST_NOTE("text %lu (%lu bytes) did not scan cleanly",
                            (unsigned long)i, (unsigned long)length);
            }

            ++lost;
        }
    }

    D_TEST_NOTE("%lu texts scanned into %lu tokens; %lu did not scan "
                "cleanly", (unsigned long)D_TESTS_PARSE_STRESS_TEXTS,
                (unsigned long)tokens, (unsigned long)lost);

    d_parse_expr_signature_release(&sigs[0]);
    d_parse_expr_signature_release(&sigs[1]);

    return d_tests_expr_expect( (ok) && (lost == 0u),
                                "every random text scans to its end in "
                                "linear steps", __LINE__);
}

/*
d_tests_stress_parse_bytes
  Random bytes scanned and parsed.
  Tests the following:
  - each of D_TESTS_PARSE_STRESS_TEXTS texts, drawn from a fixed seed, by
    math's signature and dawk's in turn, ends with a status
  - a text of n bytes takes at most n + 2 reads of its source
  - the term left verifies, and every diagnostic lies inside the text
*/
bool
d_tests_stress_parse_bytes(void)
{
    struct dj_parse_expr_signature   sigs[2];
    struct dj_parse_expr_scan_config configs[2];
    static char                      text[D_INTERNAL_STRESS_TEXT_MAX];
    unsigned long                    by_status[3] = { 0u, 0u, 0u };
    uint32_t                         rng          = 20261118u;
    uint32_t                         lost         = 0u;
    bool                             ok           =
        ( (d_tests_expr_math(&sigs[0], true)) &&
          (d_tests_expr_dawk(&sigs[1])) );

    d_tests_stress_config(&configs[0], false);
    d_tests_stress_config(&configs[1], true);

    for (uint32_t i = 0u; ( (ok) && (i < D_TESTS_PARSE_STRESS_TEXTS) ); ++i)
    {
        const size_t   length = d_tests_stress_text(&rng, text);
        const uint32_t which  = i % 2u;
        int            status = -1;

        if (!d_tests_stress_parse_one(&sigs[which], &configs[which], text,
                                      length, &status))
        {
            if (lost < D_INTERNAL_STRESS_SHOWN)
            {
                D_TEST_NOTE("text %lu (%lu bytes) parsed out of bounds "
                            "(status %d)", (unsigned long)i,
                            (unsigned long)length, status);
            }

            ++lost;
        }
        else
        {
            ++by_status[ (status == D_PARSE_EXPR_OK)    ? 0
                       : (status == D_PARSE_EXPR_ERROR) ? 1
                                                        : 2 ];
        }
    }

    D_TEST_NOTE("%lu texts parsed: %lu read, %lu with errors, %lu fatal; "
                "%lu out of bounds", (unsigned long)D_TESTS_PARSE_STRESS_TEXTS,
                by_status[0], by_status[1], by_status[2],
                (unsigned long)lost);

    d_parse_expr_signature_release(&sigs[0]);
    d_parse_expr_signature_release(&sigs[1]);

    return d_tests_expr_expect( (ok) && (lost == 0u),
                                "every random text parses to a status, in "
                                "bounds and in linear steps", __LINE__);
}

/*
d_tests_stress_fuzz_one
  One input, by dawk's signature, the richest of the four: the libFuzzer
harness's body and the corpus replay's. The signature is built on the first
call and kept for the process.
*/
bool
d_tests_stress_fuzz_one(
    const uint8_t* _data,
    size_t         _size
)
{
    static struct dj_parse_expr_signature   s;
    static struct dj_parse_expr_scan_config config;
    static bool                             built = false;
    int                                     status;

    // the scanner takes a text below 4 GiB; a fuzzer's inputs are far less
    if (_size > (size_t)0xFFFFu)
    {
        return true;
    }

    if (!built)
    {
        if (!d_tests_expr_dawk(&s))
        {
            return false;
        }

        d_tests_stress_config(&config, true);
        built = true;
    }

    return d_tests_stress_parse_one(&s, &config, (const char*)_data, _size,
                                    &status);
}

#else

/*
d_tests_stress_scan_bytes
  The scanner or the heap compiled out: skipped.
*/
bool
d_tests_stress_scan_bytes(void)
{
    D_TEST_SKIP("the scanner or the term's heap is compiled out "
                "(D_CFG_PARSE_EXPR_SCAN or _HEAP is 0)");
}

/*
d_tests_stress_parse_bytes
  The scanner or the heap compiled out: skipped.
*/
bool
d_tests_stress_parse_bytes(void)
{
    D_TEST_SKIP("the scanner or the term's heap is compiled out "
                "(D_CFG_PARSE_EXPR_SCAN or _HEAP is 0)");
}

/*
d_tests_stress_fuzz_one
  The scanner or the heap compiled out: every input holds.
*/
bool
d_tests_stress_fuzz_one(
    const uint8_t* _data,
    size_t         _size
)
{
    (void)_data;
    (void)_size;

    return true;
}

#endif  // D_INTERNAL_PARSE_EXPR_SCAN, D_INTERNAL_PARSE_EXPR_HEAP
