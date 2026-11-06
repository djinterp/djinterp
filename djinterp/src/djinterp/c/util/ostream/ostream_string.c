/*******************************************************************************
* djinterp [c]                                                  ostream_string.c
*
*   The dj_string destination: a storage destination whose window is the
* string's spare capacity, so a byte is written once, into its final place.
* It is kept out of ostream.c so that a program streaming only to files,
* consoles and fixed arrays never links dstring.c.
*
*   THE WINDOW IS [text + size, text + capacity - 1). The last byte of the
* capacity is kept for the terminator, which every commit writes, so the
* string is a valid C string after each d_ostream_flush. Growth goes through
* d_string_reserve, which doubles, so a run of writes is amortised O(1) and
* the string's own policy decides how much it holds.
*
*
* path:      /src/djinterp/c/util/ostream/ostream_string.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/ostream/ostream.h"
// std
#include <errno.h>   // errno, ENOMEM
#include <stddef.h>  // size_t
// djinterp
#include "../../../../../inc/djinterp/c/dckdint.h"  // d_ckd_add
#include "../../../../../inc/djinterp/c/dstring.h"  // struct dj_string,
                                                    // d_string_reserve
#include "../../../../../inc/djinterp/c/util/sink_common.h"
                                                    // struct dj_pack_sink
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int32_t


/**
 * @brief The dj_string destination's grow: commits the window into the
 *        string's size, then reserves room for _need more.
 *
 * @param[in,out] _os    the stream.
 * @param[in]     _need  the room wanted; 0 asks only for the commit.
 * @return 1, or 0 with errno ENOMEM when the string cannot grow.
 *
 * The window always starts at text + size, so committing is adding
 * [base, cur) to the size. The capacity asked for leaves the terminator's
 * byte outside the window; a request that would overflow size_t is refused
 * before it reaches the allocator.
 */
static int32_t
d_internal_ostream_string_grow(
    struct dj_ostream* _os,
    size_t             _need
)
{
    struct dj_string* const string = (struct dj_string*)_os->sink.context;

    string->size += (size_t)(_os->cur - _os->base);

    if (string->text != NULL)
    {
        string->text[string->size] = '\0';
    }

    // room for _need more, the terminator's byte included
    if (_need != 0u)
    {
        size_t required = 0u;

        // what the string must hold, checked before it is asked for
        if ( (d_ckd_add(&required, string->size, _need)) ||
             (d_ckd_add(&required, required, 1u)) )
        {
            errno = ENOMEM;

            return 0;
        }

        if ( (string->capacity < required) &&
             (!d_string_reserve(string, required)) )
        {
            errno = ENOMEM;

            return 0;
        }
    }

    // the spare capacity, less the terminator's byte, is the window
    if ( (string->text != NULL) &&
         (string->capacity > (string->size + 1u)) )
    {
        d_ostream_window_set(_os,
                             string->text + string->size,
                             string->capacity - string->size - 1u);
    }
    else
    {
        d_ostream_window_clear(_os);
    }

    return 1;
}

/**
 * @brief The dj_string destination's description: bytes arrive unaltered.
 *
 * @param[in]  _context  the string.
 * @param[out] _out      the description; arrives zeroed.
 * @return 1.
 */
static int32_t
d_internal_ostream_string_query(
    void*                   _context,
    struct dj_ostream_info* _out
)
{
    (void)_context;

    _out->is_binary_safe = 1;

    return 1;
}

// d_internal_ostream_string_ops
//   table: the dj_string destination's operations. No close slot: the string
// is borrowed, and the commit that close sends is all it needs.
static const struct dj_ostream_ops d_internal_ostream_string_ops =
{
    sizeof(struct dj_ostream_ops),
    &d_internal_ostream_string_grow,
    NULL,
    NULL,
    &d_internal_ostream_string_query
};

/*
d_ostream_init_string
  A null string reaches d_ostream_init as neither a write nor a table, which
refuses it; otherwise d_ostream_init's first grow takes the spare capacity
the string already has as the first window.
*/
int32_t
d_ostream_init_string(
    struct dj_ostream* _os,
    struct dj_string*  _string
)
{
    struct dj_pack_sink sink;

    sink.write   = NULL;
    sink.context = _string;

    return d_ostream_init(_os,
                          sink,
                          (_string != NULL) ? &d_internal_ostream_string_ops
                                            : NULL,
                          0);
}
