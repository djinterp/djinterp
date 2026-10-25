/*******************************************************************************
* djinterp [net]                                                     sftp_test.c
*
* SFTP tests: the codec on its own; the client against an in-process server,
* which can answer out of order, read short, and break the protocol on
* purpose, over tcp.c's connection on a socket pair and over a channel of the
* toy SSH engine relaying to one; the client over connections that fail as
* told; and, when SFTP_TEST_PORT is set, the client over the SSH module
* against a real server.
*   Integration variables: SFTP_TEST_PORT (on 127.0.0.1), SFTP_TEST_USER,
* SFTP_TEST_KEY (a private key file), SFTP_TEST_DIR (a scratch directory the
* user may write), and SFTP_TEST_KNOWN_HOSTS (a file to record the host in).
*
*
* path:      /tests/djinterp/net/sftp/sftp_test.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.10.16
*******************************************************************************/
// std
#include <errno.h>    // errno, EINTR
#include <signal.h>   // signal, SIGPIPE
#include <stdbool.h>  // bool, true, false
#include <stdio.h>    // printf
#include <stdlib.h>   // getenv, atoi, malloc, free
#include <string.h>   // memcmp, memcpy, memset, strlen
// POSIX
#include <arpa/inet.h>   // htonl
#include <netinet/in.h>  // struct sockaddr_in
#include <poll.h>        // poll, POLLIN
#include <pthread.h>     // pthread_create, pthread_join
#include <sys/socket.h>  // socketpair
#include <unistd.h>      // read, write, close
// djinterp
#include "../../../../inc/djinterp/net/net_common.h"  // d_net_connection
#include "../../../../inc/djinterp/net/sftp/sftp.h"   // d_sftp_*
#include "../../../../inc/djinterp/net/ssh/ssh.h"     // d_ssh_*
#include "../../../../inc/djinterp/net/tcp/tcp.h"     // d_tcp_connection
#include "../net_tests_ssh_toy.h"                     // the toy SSH engine


static int g_fail   = 0;
static int g_checks = 0;

/*
d_tests_sftp_check
  Counts one check, and names it when it fails.
*/
static void
d_tests_sftp_check(
    bool        _ok,
    const char* _text
)
{
    g_checks++;

    // a failed check says which
    if (!_ok)
    {
        g_fail++;
        printf("FAIL: %s\n",
               _text);
    }

    return;
}

// D_TESTS_SFTP_CHECK
//   macro: one check, its condition's text naming it when it fails.
#define D_TESTS_SFTP_CHECK(c) d_tests_sftp_check((c), #c)

/*
t_codec
  The codec.
  It runs past 75 lines because one fixture's state runs through every
check, and no cut leaves both halves whole.
*/
static void
t_codec(
    void
)
{
    struct d_ssh_writer      w    = { 0 };
    struct d_ssh_reader      r    = { 0 };
    struct d_sftp_attributes a    = { 0 };
    struct d_sftp_attributes b    = { 0 };
    size_t                   mark = 0;
    size_t                   size = 0;
    struct d_sftp_packet     p    = { 0 };
    struct d_sftp_handle     h    = { 0 };
    struct d_pack_text       msg  = { 0 };
    uint32_t                 code = 0;
    uint32_t                 ext  = 0;
    d_ssh_writer_init(&w);
    d_sftp_attributes_init(&a);
    a.flags = D_SFTP_ATTR_SIZE | D_SFTP_ATTR_UIDGID | D_SFTP_ATTR_PERMISSIONS |
              D_SFTP_ATTR_ACMODTIME;
    a.size        = 0x123456789ull;
    a.uid         = 1000;
    a.gid         = 100;
    a.permissions = 0100644;
    a.atime       = 11;
    a.mtime       = 22;
    D_TESTS_SFTP_CHECK(d_sftp_write_attributes(&w,
                                               &a));
    d_ssh_reader_init(&r,
                      w.data,
                      w.size);
    D_TESTS_SFTP_CHECK( (d_sftp_read_attributes(&r,
                                                &b)) &&
                        (d_ssh_reader_done(&r))      &&
                        (!memcmp(&a,
                                 &b,
                                 sizeof a)) );
    D_TESTS_SFTP_CHECK( (d_sftp_attributes_is_regular(&b))    &&
                        (!d_sftp_attributes_is_directory(&b)) &&
                        (!d_sftp_attributes_is_symlink(&b)) );
    b.permissions = 0040755;
    D_TESTS_SFTP_CHECK(d_sftp_attributes_is_directory(&b));
    b.flags = 0;
    D_TESTS_SFTP_CHECK(!d_sftp_attributes_is_directory(&b));
    // undefined flag bits cannot be skipped, so the record is refused
    {
        static const unsigned char bad[] = { 0, 0, 0, 0x40, 1, 2, 3, 4 };
        d_ssh_reader_init(&r,
                          bad,
                          sizeof bad);
        D_TESTS_SFTP_CHECK( (!d_sftp_read_attributes(&r,
                                                     &b)) &&
                            (r.failed)                    &&
                            (b.flags == 0) );
    }
    // extended pairs are skipped, and the flag dropped
    {
        static const unsigned char ex[] = { 0x80, 0, 0,   0x01, 0, 0, 0, 0,  0,
                                            0,    0, 9,   0,    0, 0, 1, 0,  0,
                                            0,    1, 'k', 0,    0, 0, 1, 'v' };
        d_ssh_reader_init(&r,
                          ex,
                          sizeof ex);
        D_TESTS_SFTP_CHECK( (d_sftp_read_attributes(&r,
                                                    &b))  &&
                            (d_ssh_reader_done(&r))       &&
                            (b.flags == D_SFTP_ATTR_SIZE) &&
                            (b.size == 9) );
    }
    a.flags |= D_SFTP_ATTR_EXTENDED;
    w.size   = 0;
    w.failed = false;
    D_TESTS_SFTP_CHECK(!d_sftp_write_attributes(&w,
                                                &a));
    // packets: begin/end, frame, parse
    w.size   = 0;
    w.failed = false;
    D_TESTS_SFTP_CHECK( (d_sftp_packet_begin(&w,
                                             D_SFTP_FXP_STAT,
                                             77,
                                             &mark)) &&
                        (d_ssh_write_cstring(&w,
                                             "/x"))  &&
                        (d_sftp_packet_end(&w,
                                           mark)) );
    D_TESTS_SFTP_CHECK( (d_sftp_packet_frame(w.data,
                                             w.size,
                                             &size) == D_SFTP_FRAME_COMPLETE) &&
                        (size == w.size) );
    D_TESTS_SFTP_CHECK( (d_sftp_packet_frame(w.data,
                                             w.size - 1,
                                             &size) ==
                         D_SFTP_FRAME_INCOMPLETE) &&
                        (d_sftp_packet_frame(w.data,
                                             3,
                                             &size) ==
                         D_SFTP_FRAME_INCOMPLETE) );
    {
        static const unsigned char zero[] = { 0, 0, 0, 0 },
                                   huge[] = { 0, 0x04, 0, 1 };
        D_TESTS_SFTP_CHECK( (d_sftp_packet_frame(zero,
                                                 4,
                                                 &size) ==
                             D_SFTP_FRAME_MALFORMED) &&
                            (d_sftp_packet_frame(huge,
                                                 4,
                                                 &size) ==
                             D_SFTP_FRAME_MALFORMED) );
    }
    D_TESTS_SFTP_CHECK( (d_sftp_packet_parse(w.data + 4,
                                             w.size - 4,
                                             &p) == D_SFTP_OK) &&
                        (p.type == D_SFTP_FXP_STAT)            &&
                        (p.id == 77)                           &&
                        (d_ssh_reader_remaining(&p.body) == 6) );
    D_TESTS_SFTP_CHECK(d_sftp_packet_parse(w.data + 4,
                                           3,
                                           &p) ==
                       D_SFTP_ERROR_PROTOCOL);
    // status with and without its message
    {
        static const unsigned char st[]   = { 0,   0,   0,   2,   0, 0, 0, 4,
                                              'g', 'o', 'n', 'e', 0, 0, 0, 0 },
                                   bare[] = { 0, 0, 0, 1 };
        d_ssh_reader_init(&r,
                          st,
                          sizeof st);
        D_TESTS_SFTP_CHECK( (d_sftp_read_status(&r,
                                                &code,
                                                &msg) == D_SFTP_OK) &&
                            (code == 2)                             &&
                            (msg.length == 4)                       &&
                            (!memcmp(msg.data,
                                     "gone",
                                     4)) );
        d_ssh_reader_init(&r,
                          bare,
                          sizeof bare);
        D_TESTS_SFTP_CHECK( (d_sftp_read_status(&r,
                                                &code,
                                                &msg) == D_SFTP_OK) &&
                            (code == 1)                             &&
                            (msg.length == 0) );
    }
    D_TESTS_SFTP_CHECK( (d_sftp_error_from_status(3) ==
                         D_SFTP_ERROR_PERMISSION_DENIED) &&
                        (d_sftp_error_from_status(11) ==
                         D_SFTP_ERROR_FAILURE) );
    // a handle longer than 256 bytes is malformed
    w.size   = 0;
    w.failed = false;
    {
        unsigned char big[300];  // filled before it is read
        memset(big,
               'h',
               sizeof big);
        D_TESTS_SFTP_CHECK(d_ssh_write_string(&w,
                                              big,
                                              256));
        d_ssh_reader_init(&r,
                          w.data,
                          w.size);
        D_TESTS_SFTP_CHECK( (d_sftp_read_handle(&r,
                                                &h) == D_SFTP_OK) &&
                            (h.length == 256) );
        w.size = 0;
        D_TESTS_SFTP_CHECK(d_ssh_write_string(&w,
                                              big,
                                              257));
        d_ssh_reader_init(&r,
                          w.data,
                          w.size);
        D_TESTS_SFTP_CHECK(d_sftp_read_handle(&r,
                                              &h) == D_SFTP_ERROR_PROTOCOL);
    }
    // extensions: known names set bits, unknown ones are skipped
    w.size   = 0;
    w.failed = false;
    D_TESTS_SFTP_CHECK( (d_ssh_write_cstring(&w,
                                             "posix-rename@openssh.com")) &&
                        (d_ssh_write_cstring(&w,
                                             "1"))                        &&
                        (d_ssh_write_cstring(&w,
                                             "x@y"))                      &&
                        (d_ssh_write_cstring(&w,
                                             ""))                         &&
                        (d_ssh_write_cstring(&w,
                                             "limits@openssh.com"))       &&
                        (d_ssh_write_cstring(&w,
                                             "1")) );
    d_ssh_reader_init(&r,
                      w.data,
                      w.size);
    D_TESTS_SFTP_CHECK( (d_sftp_read_extensions(&r,
                                                &ext) == D_SFTP_OK) &&
                        (ext == (D_SFTP_EXT_POSIX_RENAME |
                                 D_SFTP_EXT_LIMITS)) );
    d_ssh_reader_init(&r,
                      w.data,
                      w.size - 1);
    D_TESTS_SFTP_CHECK(d_sftp_read_extensions(&r,
                                              &ext) ==
                       D_SFTP_ERROR_PROTOCOL);
    for (int e = 0; e <= 8; e++)
    {
        D_TESTS_SFTP_CHECK(d_sftp_error_string((enum d_sftp_error)e)[0] !=
                           '\0');
    }

    D_TESTS_SFTP_CHECK(
        strcmp(d_sftp_error_string(D_SFTP_ERROR_MESSAGE_TOO_LARGE),
               "too large") == 0);
    d_ssh_writer_free(&w);

    return;
}

// the in-process server
struct d_tests_sftp_file
{
    char           name[128];
    unsigned char* data;
    size_t         size;
    size_t         cap;
    uint32_t       perm;
    int            used;
    int            dir;
};

struct d_tests_sftp_fake
{
    int       fd;
    pthread_t thread;

    struct
    {
        uint32_t version;
        uint32_t read_limit;
        uint32_t short_read;
        int      limits;
        int      reorder;
        int      bad_id;
        int      huge;
        int      bad_attrs;
    } cfg;
    struct d_tests_sftp_file files[16];
    int                      listed;
    unsigned char*           queue[64];
    size_t                   queue_size[64];
    size_t                   queued;
    int                      reads;
    int                      writes;
    int                      reordered;
    int                      shortened;
};

/*
be32
  Reads a big-endian 32-bit integer from four octets.
*/
static uint32_t
be32(
    const unsigned char* _p
)
{
    return ((uint32_t)_p[0] << 24) | ((uint32_t)_p[1] << 16) |
           ((uint32_t)_p[2] << 8) | _p[3];
}

/*
read_exact
  Reads exactly the octets asked for from a descriptor, across short reads;
false at the end of the stream or on an error.
*/
static int
read_exact(
    int    _fd,
    void*  _b,
    size_t _n
)
{
    size_t got = 0;
    while (got < _n)
    {
        ssize_t r = read(_fd,
                         (char*)_b + got,
                         _n - got);
        if (r < 0 && errno == EINTR)
        {
            continue;
        }

        if (r <= 0)
        {
            return 0;
        }

        got += (size_t)r;
    }

    return 1;
}

/*
write_all
  Writes every octet to a descriptor, across short writes; false on an error.
*/
static int
write_all(
    int         _fd,
    const void* _b,
    size_t      _n
)
{
    size_t done = 0;
    while (done < _n)
    {
        ssize_t r = write(_fd,
                          (const char*)_b + done,
                          _n - done);
        if (r < 0 && errno == EINTR)
        {
            continue;
        }

        if (r <= 0)
        {
            return 0;
        }

        done += (size_t)r;
    }

    return 1;
}

/*
fs_begin
  Starts a packet in the writer: a length to fill in later, the type, and the
request id.
*/
static void
fs_begin(
    struct d_ssh_writer* _w,
    uint8_t              _type,
    uint32_t             _id
)
{
    _w->size   = 0;
    _w->failed = false;
    (void)d_ssh_write_uint32(_w,
                             0);
    (void)d_ssh_write_byte(_w,
                           _type);
    (void)d_ssh_write_uint32(_w,
                             _id);

    return;
}

/*
fs_flush
  Sends the replies held back, newest first, so the client sees them out of
order.
*/
static void
fs_flush(
    struct d_tests_sftp_fake* _f
)
{
    for (size_t i = _f->queued; i > 0; i--)
    {
        write_all(_f->fd,
                  _f->queue[i - 1],
                  _f->queue_size[i - 1]);
        free(_f->queue[i - 1]);
    }

    _f->reordered += (_f->queued > 1);
    _f->queued = 0;

    return;
}

/*
fs_send
  Fills in the packet's length and sends it; with reordering on, a reply that
may be reordered is held back instead, up to 64 of them.
*/
static void
fs_send(
    struct d_tests_sftp_fake* _f,
    struct d_ssh_writer*      _w,
    int                       _reorderable
)
{
    size_t n   = _w->size - 4;
    _w->data[0] = (unsigned char)(n >> 24);
    _w->data[1] = (unsigned char)(n >> 16);
    _w->data[2] = (unsigned char)(n >> 8);
    _w->data[3] = (unsigned char)n;
    if (_f->cfg.reorder && _reorderable && _f->queued < 64)
    {
        _f->queue[_f->queued] = malloc(_w->size);
        memcpy(_f->queue[_f->queued],
               _w->data,
               _w->size);
        _f->queue_size[_f->queued++] = _w->size;

        return;
    }

    fs_flush(_f);
    write_all(_f->fd,
              _w->data,
              _w->size);

    return;
}

/*
fs_status
  Sends a STATUS reply with its code and message.
*/
static void
fs_status(
    struct d_tests_sftp_fake* _f,
    struct d_ssh_writer*      _w,
    uint32_t                  _id,
    uint32_t                  _code,
    const char*               _text,
    int                       _reorderable
)
{
    fs_begin(_w,
             D_SFTP_FXP_STATUS,
             _id);
    (void)d_ssh_write_uint32(_w,
                             _code);
    (void)d_ssh_write_cstring(_w,
                              _text);
    (void)d_ssh_write_cstring(_w,
                              "");
    fs_send(_f,
            _w,
            _reorderable);

    return;
}

/*
fs_find
  The slot of the open file with a name, or -1; there are 16.
*/
static struct d_tests_sftp_file*
fs_find(
    struct d_tests_sftp_fake* _f,
    const unsigned char*      _name,
    size_t                    _len
)
{
    for (int i = 0; i < 16; i++)
    {
        if (_f->files[i].used && strlen(_f->files[i].name) == _len &&
            !memcmp(_f->files[i].name,
                    _name,
                    _len))
        {
            return &_f->files[i];
        }
    }

    return NULL;
}

/*
fs_create
  A file in the first free slot of 16, or -1 when they are all taken.
*/
static struct d_tests_sftp_file*
fs_create(
    struct d_tests_sftp_fake* _f,
    const unsigned char*      _name,
    size_t                    _len,
    int                       _dir
)
{
    for (int i = 0; i < 16; i++)
    {
        if (!_f->files[i].used && _len < 128)
        {
            struct d_tests_sftp_file* x = &_f->files[i];
            memset(x,
                   0,
                   sizeof *x);
            memcpy(x->name,
                   _name,
                   _len);
            x->used = 1;
            x->dir  = _dir;
            x->perm = _dir ? 0040755 : 0100644;

            return x;
        }
    }

    return NULL;
}

/*
fs_attrs
  Writes a file's attributes as the version spoken lays them out.
*/
static void
fs_attrs(
    struct d_ssh_writer*            _w,
    const struct d_tests_sftp_file* _x,
    int                             _bad
)
{
    (void)d_ssh_write_uint32(
        _w,
        _bad ? 0x40u : (D_SFTP_ATTR_SIZE | D_SFTP_ATTR_PERMISSIONS));
    (void)d_ssh_write_uint64(_w,
                             _x->size);
    (void)d_ssh_write_uint32(_w,
                             _x->perm);

    return;
}

/*
fs_handle
  The file a handle names -- "F" and a slot below 16 that is in use -- or
NULL.
*/
static struct d_tests_sftp_file*
fs_handle(
    struct d_tests_sftp_fake* _f,
    const unsigned char*      _h,
    size_t                    _n
)
{
    return (_h && _n == 2 && _h[0] == 'F' && _h[1] <
            16 && _f->files[_h[1]].used)
               ? &_f->files[_h[1]]
               : NULL;
}

/*
input_pending
  Whether input waits on a descriptor within 30 milliseconds.
*/
static int
input_pending(
    int _fd
)
{
    struct pollfd p = { _fd, POLLIN, 0 };

    return poll(&p,
                1,
                30) > 0;
}

/*
fs_main
  The in-process server, on its own thread: the version exchange, then one
request after another until the client goes, each answered as the
configuration says -- out of order, short, or broken on purpose.
  It runs past 75 lines because it dispatches every kind of request the
client sends, one branch each, as the guides allow a dispatch; the
requests that read and write a file nest past four levels there.
*/
static void*
fs_main(
    void* _arg
)
{
    struct d_tests_sftp_fake* f = _arg;
    struct d_ssh_writer       w = { 0 };
    d_ssh_writer_init(&w);
    for (;;)
    {
        unsigned char head[4];  // filled before it is read
        if (!read_exact(f->fd,
                        head,
                        4))
        {
            break;
        }

        size_t         n   = be32(head);
        unsigned char* pkt = malloc(n);
        if (!pkt || !read_exact(f->fd,
                                pkt,
                                n))
        {
            free(pkt);
            break;
        }

        struct d_ssh_reader r    = { 0 };
        uint8_t             type = 0;
        uint32_t            id   = 0;
        d_ssh_reader_init(&r,
                          pkt,
                          n);
        (void)d_ssh_read_byte(&r,
                              &type);
        (void)d_ssh_read_uint32(&r,
                                &id);
        const unsigned char *     s1 = NULL, *s2 = NULL;
        size_t                    l1 = 0;
        size_t                    l2 = 0;
        struct d_tests_sftp_file* x  = NULL;
        if (type == D_SFTP_FXP_INIT)
        {
            fs_begin(&w,
                     D_SFTP_FXP_VERSION,
                     f->cfg.version);
            (void)d_ssh_write_cstring(&w,
                                      "posix-rename@openssh.com");
            (void)d_ssh_write_cstring(&w,
                                      "1");
            (void)d_ssh_write_cstring(&w,
                                      "fsync@openssh.com");
            (void)d_ssh_write_cstring(&w,
                                      "1");
            if (f->cfg.limits)
            {
                (void)d_ssh_write_cstring(&w,
                                          "limits@openssh.com");
                (void)d_ssh_write_cstring(&w,
                                          "1");
            }

            fs_send(f,
                    &w,
                    0);
        }
        else if (f->cfg.huge)
        {
            static const unsigned char h[4] = { 0x7f, 0xff, 0xff, 0xff };
            write_all(f->fd,
                      h,
                      4);
        }
        else if (type == D_SFTP_FXP_EXTENDED)
        {
            (void)d_ssh_read_string(&r,
                                    &s1,
                                    &l1);
            if (l1 == 18 && !memcmp(s1,
                                    "limits@openssh.com",
                                    18))
            {
                fs_begin(&w,
                         D_SFTP_FXP_EXTENDED_REPLY,
                         id);
                (void)d_ssh_write_uint64(&w,
                                         34000);
                (void)d_ssh_write_uint64(&w,
                                         f->cfg.read_limit);
                (void)d_ssh_write_uint64(&w,
                                         30000);
                (void)d_ssh_write_uint64(&w,
                                         64);
                fs_send(f,
                        &w,
                        0);
            }
            else if (l1 == 24 && !memcmp(s1,
                                         "posix-rename@openssh.com",
                                         24))
            {
                (void)d_ssh_read_string(&r,
                                        &s1,
                                        &l1);
                (void)d_ssh_read_string(&r,
                                        &s2,
                                        &l2);
                struct d_tests_sftp_file* from = fs_find(f,
                                                         s1,
                                                         l1);
                struct d_tests_sftp_file* to   = fs_find(f,
                                                         s2,
                                                         l2);
                if (!from)
                {
                    fs_status(f,
                              &w,
                              id,
                              2,
                              "no such file",
                              0);
                }
                else
                {
                    if (to && to != from)
                    {
                        free(to->data);
                        to->used = 0;
                    }

                    memset(from->name,
                           0,
                           128);
                    memcpy(from->name,
                           s2,
                           l2);
                    fs_status(f,
                              &w,
                              id,
                              0,
                              "ok",
                              0);
                }
            }
            else
            {
                fs_status(f,
                          &w,
                          id,
                          0,
                          "ok",
                          0);  // fsync
            }
        }
        else if (type == D_SFTP_FXP_OPEN)
        {
            uint32_t                 pflags = 0;
            struct d_sftp_attributes a      = { 0 };
            (void)d_ssh_read_string(&r,
                                    &s1,
                                    &l1);
            (void)d_ssh_read_uint32(&r,
                                    &pflags);
            (void)d_sftp_read_attributes(&r,
                                         &a);
            x = fs_find(f,
                        s1,
                        l1);
            if (!x && (pflags & D_SFTP_OPEN_CREATE))
            {
                x = fs_create(f,
                              s1,
                              l1,
                              0);
                if (x && (a.flags & D_SFTP_ATTR_PERMISSIONS))
                {
                    x->perm = 0100000u | (a.permissions & 07777u);
                }
            }

            if (!x)
            {
                fs_status(f,
                          &w,
                          id,
                          2,
                          "no such file",
                          0);
                goto next;
            }

            if (pflags & D_SFTP_OPEN_TRUNCATE)
            {
                x->size = 0;
            }

            fs_begin(&w,
                     D_SFTP_FXP_HANDLE,
                     id);
            {
                unsigned char hd[2] = { 'F', (unsigned char)(x - f->files) };
                (void)d_ssh_write_string(&w,
                                         hd,
                                         2);
            }
            fs_send(f,
                    &w,
                    0);
        }
        else if (type == D_SFTP_FXP_READ)
        {
            uint64_t off = 0;
            uint32_t len = 0;
            (void)d_ssh_read_string(&r,
                                    &s1,
                                    &l1);
            (void)d_ssh_read_uint64(&r,
                                    &off);
            (void)d_ssh_read_uint32(&r,
                                    &len);
            x = fs_handle(f,
                          s1,
                          l1);
            f->reads++;
            if (!x)
            {
                fs_status(f,
                          &w,
                          id,
                          4,
                          "bad handle",
                          0);
                goto next;
            }

            if (off >= x->size)
            {
                fs_status(f,
                          &w,
                          id,
                          1,
                          "eof",
                          1);
                goto next;
            }

            size_t give = x->size - (size_t)off;
            if (give > len)
            {
                give = len;  // off < size, so the cast is exact
            }

            if (f->cfg.short_read && give > f->cfg.short_read)
            {
                give = f->cfg.short_read;  // the round asked for short reads
                f->shortened++;
            }

            fs_begin(&w,
                     D_SFTP_FXP_DATA,
                     id);
            (void)d_ssh_write_string(&w,
                                     x->data + off,
                                     give);
            fs_send(f,
                    &w,
                    1);
        }
        else if (type == D_SFTP_FXP_WRITE)
        {
            uint64_t off = 0;
            (void)d_ssh_read_string(&r,
                                    &s1,
                                    &l1);
            (void)d_ssh_read_uint64(&r,
                                    &off);
            (void)d_ssh_read_string(&r,
                                    &s2,
                                    &l2);
            x = fs_handle(f,
                          s1,
                          l1);
            f->writes++;
            if (!x)
            {
                fs_status(f,
                          &w,
                          id,
                          4,
                          "bad handle",
                          0);
                goto next;
            }

            if (off + l2 > x->cap)
            {
                x->cap  = (size_t)(off + l2) * 2;
                x->data = realloc(x->data,
                                  x->cap);
            }

            memcpy(x->data + off,
                   s2,
                   l2);
            if (off + l2 > x->size)
            {
                x->size = (size_t)(off + l2);
            }

            fs_status(f,
                      &w,
                      id,
                      0,
                      "ok",
                      1);
        }
        else if (type == D_SFTP_FXP_CLOSE || type == D_SFTP_FXP_SETSTAT ||
                 type == D_SFTP_FXP_SYMLINK)
        {
            fs_status(f,
                      &w,
                      id,
                      0,
                      "ok",
                      0);
        }
        else if (type == D_SFTP_FXP_STAT || type == D_SFTP_FXP_LSTAT ||
                 type == D_SFTP_FXP_FSTAT)
        {
            (void)d_ssh_read_string(&r,
                                    &s1,
                                    &l1);
            x = (type == D_SFTP_FXP_FSTAT) ? fs_handle(f,
                                                       s1,
                                                       l1)
                                           : fs_find(f,
                                                     s1,
                                                     l1);
            if (!x)
            {
                fs_status(f,
                          &w,
                          id,
                          2,
                          "No such file",
                          0);
                goto next;
            }

            fs_begin(&w,
                     D_SFTP_FXP_ATTRS,
                     f->cfg.bad_id ? id + 1000 : id);
            fs_attrs(&w,
                     x,
                     f->cfg.bad_attrs);
            fs_send(f,
                    &w,
                    0);
        }
        else if (type == D_SFTP_FXP_OPENDIR)
        {
            f->listed = 0;
            fs_begin(&w,
                     D_SFTP_FXP_HANDLE,
                     id);
            (void)d_ssh_write_cstring(&w,
                                      "D");
            fs_send(f,
                    &w,
                    0);
        }
        else if (type == D_SFTP_FXP_READDIR)
        {
            if (f->listed)
            {
                fs_status(f,
                          &w,
                          id,
                          1,
                          "eof",
                          0);
                goto next;
            }

            uint32_t count = 0;
            for (int i = 0; i < 16; i++)
            {
                count += (uint32_t)f->files[i].used;
            }

            fs_begin(&w,
                     D_SFTP_FXP_NAME,
                     id);
            (void)d_ssh_write_uint32(&w,
                                     count);
            for (int i = 0; i < 16; i++)
            {
                if (f->files[i].used)
                {
                    (void)d_ssh_write_cstring(&w,
                                              f->files[i].name);
                    (void)d_ssh_write_cstring(&w,
                                              "-rw-r--r-- long");
                    fs_attrs(&w,
                             &f->files[i],
                             0);
                }
            }

            f->listed = 1;
            fs_send(f,
                    &w,
                    0);
        }
        else if (type == D_SFTP_FXP_MKDIR)
        {
            (void)d_ssh_read_string(&r,
                                    &s1,
                                    &l1);
            if (fs_find(f,
                        s1,
                        l1))
            {
                fs_status(f,
                          &w,
                          id,
                          4,
                          "exists",
                          0);
            }
            else
            {
                fs_create(f,
                          s1,
                          l1,
                          1);
                fs_status(f,
                          &w,
                          id,
                          0,
                          "ok",
                          0);
            }
        }
        else if (type == D_SFTP_FXP_REMOVE || type == D_SFTP_FXP_RMDIR)
        {
            (void)d_ssh_read_string(&r,
                                    &s1,
                                    &l1);
            x = fs_find(f,
                        s1,
                        l1);
            if (!x)
            {
                fs_status(f,
                          &w,
                          id,
                          2,
                          "no such file",
                          0);
            }
            else
            {
                free(x->data);
                x->used = 0;
                fs_status(f,
                          &w,
                          id,
                          0,
                          "ok",
                          0);
            }
        }
        else if (type == D_SFTP_FXP_RENAME)
        {
            (void)d_ssh_read_string(&r,
                                    &s1,
                                    &l1);
            (void)d_ssh_read_string(&r,
                                    &s2,
                                    &l2);
            x = fs_find(f,
                        s1,
                        l1);
            if (!x)
            {
                fs_status(f,
                          &w,
                          id,
                          2,
                          "no such file",
                          0);
            }
            else if (fs_find(f,
                             s2,
                             l2))
            {
                fs_status(f,
                          &w,
                          id,
                          4,
                          "target exists",
                          0);
            }
            else
            {
                memset(x->name,
                       0,
                       128);
                memcpy(x->name,
                       s2,
                       l2);
                fs_status(f,
                          &w,
                          id,
                          0,
                          "ok",
                          0);
            }
        }
        else if (type == D_SFTP_FXP_REALPATH || type == D_SFTP_FXP_READLINK)
        {
            (void)d_ssh_read_string(&r,
                                    &s1,
                                    &l1);
            fs_begin(&w,
                     D_SFTP_FXP_NAME,
                     id);
            (void)d_ssh_write_uint32(&w,
                                     1);
            (void)d_ssh_write_cstring(
                &w,
                type == D_SFTP_FXP_REALPATH ? "/home/test" : "target.txt");
            (void)d_ssh_write_cstring(&w,
                                      "");
            (void)d_ssh_write_uint32(&w,
                                     0);
            fs_send(f,
                    &w,
                    0);
        }
        else
        {
            fs_status(f,
                      &w,
                      id,
                      8,
                      "unsupported",
                      0);
        }

        next:
        free(pkt);
        if (f->queued && !input_pending(f->fd))
        {
            fs_flush(f);
        }
    }

    fs_flush(f);
    d_ssh_writer_free(&w);

    return NULL;
}

// the client's end of the pair: tcp.c's connection over it, once fake_open
// has adopted it; otherwise a toy SSH channel may relay to it
static struct d_tests_sftp_fake g_fake;
static int                      g_client_fd;
static struct d_tcp_connection  g_conn;
static int                      g_adopted;
static struct d_sftp_client     g_client;

/*
fake_start
  Starts the in-process server on one end of a socket pair, speaking the
version given, with its limits, reordering and short reads as asked.
*/
static int
fake_start(
    uint32_t _version,
    int      _limits,
    int      _reorder,
    uint32_t _short_read
)
{
    int sv[2];  // filled before it is read
    memset(&g_fake,
           0,
           sizeof g_fake);
    g_adopted = 0;
    if (socketpair(AF_UNIX,
                   SOCK_STREAM,
                   0,
                   sv))
    {
        return 0;
    }

    g_fake.fd             = sv[1];
    g_client_fd           = sv[0];
    g_fake.cfg.version    = _version;
    g_fake.cfg.limits     = _limits;
    g_fake.cfg.reorder    = _reorder;
    g_fake.cfg.short_read = _short_read;
    g_fake.cfg.read_limit = 20000;
    if (pthread_create(&g_fake.thread,
                       NULL,
                       fs_main,
                       &g_fake))
    {
        return 0;
    }

    d_sftp_client_init(&g_client);

    return 1;
}

/*
fake_open
  Opens the client over the socket pair's other end, adopted as a TCP
connection.
*/
static enum d_sftp_error
fake_open(
    void
)
{
    g_adopted = 1;
    if (d_tcp_connection_adopt(&g_conn,
                               g_client_fd,
                               D_NET_PROTOCOL_UNIX) !=
        D_NET_OK)
    {
        return D_SFTP_ERROR_TRANSPORT;
    }

    return d_sftp_client_open_connection(&g_client,
                                         &g_conn.base);
}

/*
fake_stop
  Ends the session: the client leaves a caller's connection open, so closing
its end ends the server's loop, and the thread is joined.
*/
static void
fake_stop(
    void
)
{
    // the client leaves a caller's connection open; closing its end ends the
    // server
    d_sftp_client_close(&g_client);
    if (g_adopted)
    {
        D_TESTS_SFTP_CHECK(d_net_connection_is_open(&g_conn.base));
        d_tcp_connection_close(&g_conn);
    }
    else
    {
        close(g_client_fd);
    }

    pthread_join(g_fake.thread,
                 NULL);
    close(g_fake.fd);
    for (int i = 0; i < 16; i++)
    {
        if (g_fake.files[i].used)
        {
            free(g_fake.files[i].data);
        }
    }

    return;
}

// a connection that fails as told
// reads serve `data`, then end (`read_error` D_NET_OK) or fail with it;
// writes fail with `write_error` once `budget` bytes have gone; closes count
struct d_tests_sftp_failing
{
    struct d_net_connection base;
    const unsigned char*    data;
    size_t                  size;
    size_t                  at;
    size_t                  budget;
    size_t                  written;
    enum d_net_error        read_error;
    enum d_net_error        write_error;
    int                     closes;
};

/*
fc_read
  A read of the failing connection: data from its script, then the failure it
was told to give.
*/
static struct d_net_io_result
fc_read(
    struct d_net_connection* _c,
    void*                    _b,
    size_t                   _n
)
{
    struct d_tests_sftp_failing* f    = (struct d_tests_sftp_failing*)_c;
    size_t                       left = f->size - f->at;
    if (left == 0)
    {
        return d_net_io_result_make(0,
                                    f->read_error);
    }

    if (_n > left)
    {
        _n = left;
    }

    memcpy(_b,
           f->data + f->at,
           _n);
    f->at += _n;

    return d_net_io_result_make(_n,
                                D_NET_OK);
}

/*
fc_write
  A write of the failing connection: accepted until the failure it was told to
give.
*/
static struct d_net_io_result
fc_write(
    struct d_net_connection* _c,
    const void*              _d,
    size_t                   _n
)
{
    struct d_tests_sftp_failing* f = (struct d_tests_sftp_failing*)_c;
    (void)_d;
    if (f->written + _n > f->budget)
    {
        return d_net_io_result_make(0,
                                    f->write_error);
    }

    f->written += _n;

    return d_net_io_result_make(_n,
                                D_NET_OK);
}

/*
fc_is_open
  Open until it is closed.
*/
static bool
fc_is_open(
    const struct d_net_connection* _c
)
{
    return ((const struct d_tests_sftp_failing*)_c)->closes == 0;
}

/*
fc_close
  Counts the close.
*/
static void
fc_close(
    struct d_net_connection* _c
)
{
    ((struct d_tests_sftp_failing*)_c)->closes++;

    return;
}
static const struct d_net_connection_vtable FC_TABLE = {
    fc_read, fc_write, fc_is_open, fc_close, NULL, NULL, NULL, NULL, NULL
};
// a server's VERSION 3, with no extensions
static const unsigned char VERSION3[] = { 0, 0, 0, 5, D_SFTP_FXP_VERSION,
                                          0, 0, 0, 3 };

/*
fc_make
  A failing connection: its operations, and how and when they fail.
*/
static void
fc_make(
    struct d_tests_sftp_failing* _f,
    const unsigned char*         _data,
    size_t                       _size,
    enum d_net_error             _read_error,
    size_t                       _budget,
    enum d_net_error             _write_error
)
{
    memset(_f,
           0,
           sizeof *_f);
    D_TESTS_SFTP_CHECK(d_net_connection_init(&_f->base,
                                             &FC_TABLE));
    _f->data        = _data;
    _f->size        = _size;
    _f->read_error  = _read_error;
    _f->budget      = _budget;
    _f->write_error = _write_error;

    return;
}

/*
fc_open_fails
  Opening over a connection that fails: the error, its two causes, and the
connection still the caller's.
*/
static void
fc_open_fails(
    enum d_net_error  _read_error,
    size_t            _budget,
    enum d_net_error  _write_error,
    enum d_sftp_error _expected,
    enum d_net_error  _cause
)
{
    struct d_tests_sftp_failing f = { 0 };
    fc_make(&f,
            NULL,
            0,
            _read_error,
            _budget,
            _write_error);
    d_sftp_client_init(&g_client);
    D_TESTS_SFTP_CHECK(d_sftp_client_open_connection(&g_client,
                                                     &f.base) ==
                       _expected);
    D_TESTS_SFTP_CHECK( (g_client.net_error == _cause)    &&
                        (g_client.ssh_status == D_SSH_OK) &&
                        (d_sftp_error_from_net(_cause) == _expected) );
    D_TESTS_SFTP_CHECK( (!g_client.open)       &&
                        (!g_client.buffer)     &&
                        (!g_client.connection) &&
                        (f.closes == 0)        &&
                        (g_client.extensions == 0) );
    d_sftp_client_close(&g_client);
    D_TESTS_SFTP_CHECK( (f.closes == 0) &&
                        (g_client.net_error == _cause) );

    return;
}

/*
fc_replies
  A reply big enough that the buffer fills and empties many times: VERSION,
then `count` STATUS replies to requests 1 on, each with a long message.
*/
static unsigned char*
fc_replies(
    size_t  _count,
    size_t* _size
)
{
    const size_t   each = 4 + 1 + 4 + 4 + 4 + 2000 + 4;
    unsigned char* data = malloc(sizeof VERSION3 + _count * each);
    size_t         at   = sizeof VERSION3;
    if (!data)
    {
        return NULL;
    }

    memcpy(data,
           VERSION3,
           sizeof VERSION3);
    for (size_t i = 0; i < _count; i++, at += each)
    {
        const uint32_t      n      = (uint32_t)(each - 4);
        const uint32_t      id     = (uint32_t)(i + 1);
        const unsigned char head[] = { (unsigned char)(n >> 24),
                                       (unsigned char)(n >> 16),
                                       (unsigned char)(n >> 8),
                                       (unsigned char)n,
                                       D_SFTP_FXP_STATUS,
                                       (unsigned char)(id >> 24),
                                       (unsigned char)(id >> 16),
                                       (unsigned char)(id >> 8),
                                       (unsigned char)id,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0x07,
                                       0xd0 };
        memcpy(data + at,
               head,
               sizeof head);
        memset(data + at + sizeof head,
               'm',
               2000);
        memset(data + at + sizeof head + 2000,
               0,
               4);
    }

    *_size = at;

    return data;
}

/*
t_failing
  Checks its scenario.
*/
static void
t_failing(
    void
)
{
    fc_open_fails(D_NET_ERROR_TIMED_OUT,
                  1000,
                  D_NET_OK,
                  D_SFTP_ERROR_TRANSPORT,
                  D_NET_ERROR_TIMED_OUT);
    fc_open_fails(D_NET_OK,
                  1000,
                  D_NET_OK,
                  D_SFTP_ERROR_CONNECTION_LOST,
                  D_NET_ERROR_CLOSED);  // ended too soon
    fc_open_fails(D_NET_ERROR_CONNECTION_RESET,
                  1000,
                  D_NET_OK,
                  D_SFTP_ERROR_CONNECTION_LOST,
                  D_NET_ERROR_CONNECTION_RESET);
    fc_open_fails(D_NET_ERROR_OUT_OF_MEMORY,
                  1000,
                  D_NET_OK,
                  D_SFTP_ERROR_OUT_OF_MEMORY,
                  D_NET_ERROR_OUT_OF_MEMORY);
    fc_open_fails(D_NET_ERROR_PROTOCOL,
                  1000,
                  D_NET_OK,
                  D_SFTP_ERROR_TRANSPORT,
                  D_NET_ERROR_PROTOCOL);
    fc_open_fails(D_NET_OK,
                  0,
                  D_NET_ERROR_CONNECTION_ABORTED,
                  D_SFTP_ERROR_CONNECTION_LOST,
                  D_NET_ERROR_CONNECTION_ABORTED);
    fc_open_fails(D_NET_OK,
                  0,
                  D_NET_ERROR_ACCESS_DENIED,
                  D_SFTP_ERROR_TRANSPORT,
                  D_NET_ERROR_ACCESS_DENIED);

    return;
}

/*
t_failing_2
  t_failing, continued: its checks
  It runs past 75 lines because one fixture's state runs through every
check, and no cut leaves both halves whole.
*/
static void
t_failing_2(
    void
)
{
    struct d_tests_sftp_failing f = { 0 };
    struct d_sftp_attributes    a = { 0 };

    // open, then the stream fails beneath a request; a new open forgets the
    // causes
    fc_make(&f,
            VERSION3,
            sizeof VERSION3,
            D_NET_ERROR_TIMED_OUT,
            1000,
            D_NET_OK);
    d_sftp_client_init(&g_client);
    D_TESTS_SFTP_CHECK( (d_sftp_client_open_connection(&g_client,
                                                       &f.base) == D_SFTP_OK) &&
                        (g_client.open)                                       &&
                        (g_client.version == 3) );
    D_TESTS_SFTP_CHECK( (g_client.connection == &f.base) &&
                        (g_client.net_error == D_NET_OK) );
    D_TESTS_SFTP_CHECK( (d_sftp_client_stat(&g_client,
                                            "x",
                                            &a) == D_SFTP_ERROR_TRANSPORT) &&
                        (g_client.net_error == D_NET_ERROR_TIMED_OUT)      &&
                        (g_client.ssh_status == D_SSH_OK) );
    d_sftp_client_close(&g_client);
    D_TESTS_SFTP_CHECK( (f.closes == 0)  &&
                        (!g_client.open) &&
                        (g_client.net_error == D_NET_ERROR_TIMED_OUT) );
    fc_make(&f,
            VERSION3,
            sizeof VERSION3,
            D_NET_OK,
            1000,
            D_NET_OK);
    D_TESTS_SFTP_CHECK( (d_sftp_client_open_connection(&g_client,
                                                       &f.base) == D_SFTP_OK) &&
                        (g_client.net_error == D_NET_OK) );
    D_TESTS_SFTP_CHECK(d_sftp_client_open_connection(&g_client,
                                                     &f.base) ==
                       D_SFTP_ERROR_STATE);
    D_TESTS_SFTP_CHECK( (d_sftp_client_stat(&g_client,
                                            "x",
                                            &a) ==
                         D_SFTP_ERROR_CONNECTION_LOST) &&
                        (g_client.net_error == D_NET_ERROR_CLOSED) );
    d_sftp_client_close(&g_client);
    // a server's last status and its extensions belong to its session: a
    // closed client has neither, and a new session starts without them
    {
        static const unsigned char REPLIES[] = {
            0,   0,   0,   31,  D_SFTP_FXP_VERSION,
            0,   0,   0,   3,   0,
            0,   0,   17,  'f', 's',
            'y', 'n', 'c', '@', 'o',
            'p', 'e', 'n', 's', 's',
            'h', '.', 'c', 'o', 'm',
            0,   0,   0,   1,   '1',
            0,   0,   0,   21,  D_SFTP_FXP_STATUS,
            0,   0,   0,   1,   0,
            0,   0,   2,   0,   0,
            0,   4,   'g', 'o', 'n',
            'e', 0,   0,   0,   0
        };
        fc_make(&f,
                REPLIES,
                sizeof REPLIES,
                D_NET_OK,
                1000,
                D_NET_OK);
        D_TESTS_SFTP_CHECK( (d_sftp_client_open_connection(&g_client,
                                                           &f.base) ==
                             D_SFTP_OK)             &&
                            (g_client.version == 3) &&
                            (g_client.extensions == D_SFTP_EXT_FSYNC) );
        D_TESTS_SFTP_CHECK( (d_sftp_client_stat(&g_client,
                                                "x",
                                                &a) ==
                             D_SFTP_ERROR_NO_SUCH_FILE) &&
                            (g_client.status_code == 2) &&
                            (!strcmp(g_client.status_message,
                                     "gone")) );
        // a client open over a connection opens no channel besides
        {
            struct d_ssh_session never = { 0 };
            memset(&never,
                   0,
                   sizeof never);
            D_TESTS_SFTP_CHECK( (d_sftp_client_open(&g_client,
                                                    &never) ==
                                 D_SFTP_ERROR_STATE) &&
                                (g_client.open)      &&
                                (g_client.connection == &f.base) );
        }
        d_sftp_client_close(&g_client);
        D_TESTS_SFTP_CHECK( (g_client.version == 0)    &&
                            (g_client.extensions == 0) &&
                            (g_client.status_code == 2) );
        fc_make(&f,
                VERSION3,
                sizeof VERSION3,
                D_NET_OK,
                1000,
                D_NET_OK);
        D_TESTS_SFTP_CHECK( (d_sftp_client_open_connection(&g_client,
                                                           &f.base) ==
                             D_SFTP_OK)                 &&
                            (g_client.status_code == 0) &&
                            (g_client.status_message[0] == '\0') );
        d_sftp_client_close(&g_client);
    }
    // a buffer filled and emptied many times over, a partial packet moving to
    // its front each time, each read held to the room left
    {
        size_t         size  = 0;
        const size_t   count = (2 * D_SFTP_CLIENT_BUFFER_SIZE) / 2021 + 8;
        unsigned char* data  = fc_replies(count,
                                          &size);
        size_t         i     = 0;
        D_TESTS_SFTP_CHECK(data != NULL);
        if (data)
        {
            fc_make(&f,
                    data,
                    size,
                    D_NET_OK,
                    (size_t)-1,
                    D_NET_OK);
            D_TESTS_SFTP_CHECK(
                d_sftp_client_open_connection(&g_client,
                                              &f.base) == D_SFTP_OK);
            for (i = 0; i < count &&
                 d_sftp_client_mkdir(&g_client,
                                     "d",
                                     NULL) == D_SFTP_OK;
                 i++)
            {
            }
            D_TESTS_SFTP_CHECK( (i == count)   &&
                                (f.at == size) &&
                                (strlen(g_client.status_message) ==
                                 D_SFTP_MESSAGE_SIZE - 1) );
            d_sftp_client_close(&g_client);
            free(data);
        }
    }
    // initialization makes any client a closed one, its diagnostics clear
    {
        struct d_sftp_client dirty = { 0 };
        memset(&dirty,
               0xA5,
               sizeof dirty);
        d_sftp_client_init(&dirty);
        D_TESTS_SFTP_CHECK( (!dirty.open)           &&
                            (!dirty.owns_channel)   &&
                            (!dirty.buffer)         &&
                            (!dirty.connection)     &&
                            (dirty.extensions == 0) &&
                            (dirty.version == 0) );
        D_TESTS_SFTP_CHECK( (dirty.read_size == D_SFTP_CHUNK_SIZE)  &&
                            (dirty.write_size == D_SFTP_CHUNK_SIZE) &&
                            (dirty.net_error == D_NET_OK)           &&
                            (dirty.ssh_status == D_SSH_OK) );
        D_TESTS_SFTP_CHECK( (dirty.status_code == 0) &&
                            (dirty.status_message[0] == '\0') );
        d_sftp_client_close(&dirty);
        d_sftp_client_init(NULL);
    }

    return;
}

// sinks and sources
static struct
{
    unsigned char* data;
    size_t         len;
    size_t         cap;
    size_t         fail_after;
} g_sink;

/*
sink_mem
  Collects a download in g_sink, failing once it holds as much as told to.
*/
static enum d_sftp_error
sink_mem(
    void*       _c,
    const void* _d,
    size_t      _n
)
{
    (void)_c;
    if (g_sink.fail_after && g_sink.len >= g_sink.fail_after)
    {
        return D_SFTP_ERROR_FAILURE + 0x200;  // a caller's own error code
    }

    if (g_sink.len + _n > g_sink.cap)
    {
        g_sink.cap  = (g_sink.len + _n) * 2;
        g_sink.data = realloc(g_sink.data,
                              g_sink.cap);
    }

    memcpy(g_sink.data + g_sink.len,
           _d,
           _n);
    g_sink.len += _n;

    return D_SFTP_OK;
}

struct d_tests_sftp_source
{
    const unsigned char* data;
    size_t               len;
    size_t               pos;
    size_t               step;
};

/*
source_mem
  Gives an upload's octets from a source, a block at a time.
*/
static enum d_sftp_error
source_mem(
    void*   _c,
    void*   _b,
    size_t  _cap,
    size_t* _out
)
{
    struct d_tests_sftp_source* s = _c;
    size_t                      n = s->len - s->pos;
    if (n > _cap)
    {
        n = _cap;
    }

    if (s->step && n > s->step)
    {
        n = s->step;
    }

    memcpy(_b,
           s->data + s->pos,
           n);
    s->pos += n;
    *_out = n;

    return D_SFTP_OK;
}

static int g_entries;
static int g_saw_blob;

/*
on_entry
  Counts a listing's entries.
*/
static enum d_sftp_error
on_entry(
    void*                     _c,
    const struct d_sftp_name* _e
)
{
    (void)_c;
    g_entries++;
    g_saw_blob |=
        (_e->filename.length == 4 && !memcmp(_e->filename.data,
                                             "blob",
                                             4) &&
         _e->attributes.size == 300000);

    return D_SFTP_OK;
}

static unsigned char g_blob[300000];

/*
fake_transfer_round
  One round against the in-process server: an upload, its attributes, a
download compared with it, a listing, and the file removed.
  It runs past 75 lines because one fixture's state runs through every
check, and no cut leaves both halves whole.
*/
static void
fake_transfer_round(
    int      _reorder,
    uint32_t _short_read
)
{
    struct d_sftp_attributes   a   = { 0 };
    struct d_tests_sftp_source s   = { g_blob, sizeof g_blob, 0, 0 };
    char                       path[64];  // filled before it is read
    struct d_sftp_handle       h   = { 0 };
    size_t                     got = 0;
    unsigned char              small[100];  // filled before it is read
    if (!fake_start(3,
                    1,
                    _reorder,
                    _short_read))
    {
        D_TESTS_SFTP_CHECK(0);

        return;
    }

    D_TESTS_SFTP_CHECK( (fake_open() == D_SFTP_OK) &&
                        (g_client.version == 3) );
    D_TESTS_SFTP_CHECK(
        g_client.extensions ==
        (D_SFTP_EXT_POSIX_RENAME | D_SFTP_EXT_FSYNC | D_SFTP_EXT_LIMITS));
    D_TESTS_SFTP_CHECK( (g_client.read_size == 20000) &&
                        (g_client.write_size == 30000) );
    D_TESTS_SFTP_CHECK(
        d_sftp_client_put(&g_client,
                          "blob",
                          NULL,
                          source_mem,
                          &s) ==
        D_SFTP_OK);
    D_TESTS_SFTP_CHECK( (g_fake.files[0].size == sizeof g_blob) &&
                        (!memcmp(g_fake.files[0].data,
                                 g_blob,
                                 sizeof g_blob)) );
    g_sink.len = 0;
    D_TESTS_SFTP_CHECK( (d_sftp_client_get(&g_client,
                                           "blob",
                                           sink_mem,
                                           NULL) == D_SFTP_OK) &&
                        (g_sink.len == sizeof g_blob)          &&
                        (!memcmp(g_sink.data,
                                 g_blob,
                                 sizeof g_blob)) );
    D_TESTS_SFTP_CHECK( (d_sftp_client_stat(&g_client,
                                            "blob",
                                            &a) == D_SFTP_OK) &&
                        (a.size == sizeof g_blob)             &&
                        (d_sftp_attributes_is_regular(&a)) );
    D_TESTS_SFTP_CHECK( (d_sftp_client_stat(&g_client,
                                            "missing",
                                            &a) == D_SFTP_ERROR_NO_SUCH_FILE) &&
                        (g_client.status_code == 2)                           &&
                        (!strcmp(g_client.status_message,
                                 "No such file")) );
    D_TESTS_SFTP_CHECK( (d_sftp_client_mkdir(&g_client,
                                             "dir",
                                             NULL) == D_SFTP_OK) &&
                        (d_sftp_client_mkdir(&g_client,
                                             "dir",
                                             NULL) == D_SFTP_ERROR_FAILURE) );
    g_entries  = 0;
    g_saw_blob = 0;
    D_TESTS_SFTP_CHECK( (d_sftp_client_list(&g_client,
                                            ".",
                                            on_entry,
                                            NULL) == D_SFTP_OK) &&
                        (g_entries == 2)                        &&
                        (g_saw_blob) );
    D_TESTS_SFTP_CHECK( (d_sftp_client_realpath(&g_client,
                                                ".",
                                                path,
                                                sizeof path) == D_SFTP_OK) &&
                        (!strcmp(path,
                                 "/home/test")) );
    D_TESTS_SFTP_CHECK(d_sftp_client_realpath(&g_client,
                                              ".",
                                              path,
                                              5) ==
                       D_SFTP_ERROR_MESSAGE_TOO_LARGE);
    D_TESTS_SFTP_CHECK( (d_sftp_client_readlink(&g_client,
                                                "l",
                                                path,
                                                sizeof path) == D_SFTP_OK) &&
                        (!strcmp(path,
                                 "target.txt")) );
    D_TESTS_SFTP_CHECK(d_sftp_client_symlink(&g_client,
                                             "blob",
                                             "l") ==
                       D_SFTP_OK);
    D_TESTS_SFTP_CHECK(d_sftp_client_rename(&g_client,
                                            "blob",
                                            "dir",
                                            false) ==
                       D_SFTP_ERROR_FAILURE);
    D_TESTS_SFTP_CHECK( (d_sftp_client_rename(&g_client,
                                              "blob",
                                              "dir",
                                              true) == D_SFTP_OK) &&
                        (d_sftp_client_stat(&g_client,
                                            "dir",
                                            &a) == D_SFTP_OK)     &&
                        (a.size == sizeof g_blob) );
    D_TESTS_SFTP_CHECK(
        d_sftp_client_open_file(&g_client,
                                "dir",
                                D_SFTP_OPEN_READ,
                                NULL,
                                &h) ==
        D_SFTP_OK);
    D_TESTS_SFTP_CHECK( (d_sftp_client_read(&g_client,
                                            &h,
                                            1000,
                                            small,
                                            sizeof small,
                                            &got) == D_SFTP_OK) &&
                        (got > 0)                               &&
                        (!memcmp(small,
                                 g_blob + 1000,
                                 got)) );
    D_TESTS_SFTP_CHECK( (d_sftp_client_read(&g_client,
                                            &h,
                                            sizeof g_blob,
                                            small,
                                            sizeof small,
                                            &got) == D_SFTP_ERROR_EOF) &&
                        (got == 0) );
    D_TESTS_SFTP_CHECK( (d_sftp_client_write(&g_client,
                                             &h,
                                             5,
                                             "XYZ",
                                             3) == D_SFTP_OK) &&
                        (!memcmp(g_fake.files[0].data + 5,
                                 "XYZ",
                                 3)) );
    D_TESTS_SFTP_CHECK( (d_sftp_client_fstat(&g_client,
                                             &h,
                                             &a) == D_SFTP_OK) &&
                        (a.size == sizeof g_blob) );
    D_TESTS_SFTP_CHECK( (d_sftp_client_fsync(&g_client,
                                             &h) == D_SFTP_OK) &&
                        (d_sftp_client_close_file(&g_client,
                                                  &h) == D_SFTP_OK) );
    // a sink that gives up: its own error, and the stream still in step
    g_sink.len        = 0;
    g_sink.fail_after = 50000;
    D_TESTS_SFTP_CHECK(d_sftp_client_get(&g_client,
                                         "dir",
                                         sink_mem,
                                         NULL) ==
                       D_SFTP_ERROR_FAILURE + 0x200);
    g_sink.fail_after = 0;
    D_TESTS_SFTP_CHECK( (d_sftp_client_stat(&g_client,
                                            "dir",
                                            &a) == D_SFTP_OK) &&
                        (a.size == sizeof g_blob) );
    D_TESTS_SFTP_CHECK( (d_sftp_client_remove(&g_client,
                                              "dir") == D_SFTP_OK) &&
                        (d_sftp_client_remove(&g_client,
                                              "dir") ==
                         D_SFTP_ERROR_NO_SUCH_FILE) );
    D_TESTS_SFTP_CHECK(
        d_sftp_client_put(&g_client,
                          "empty",
                          NULL,
                          source_mem,
                          &(struct d_tests_sftp_source){ g_blob, 0, 0, 0 }) ==
        D_SFTP_OK);
    g_sink.len = 0;
    D_TESTS_SFTP_CHECK( (d_sftp_client_get(&g_client,
                                           "empty",
                                           sink_mem,
                                           NULL) == D_SFTP_OK) &&
                        (g_sink.len == 0) );
    fake_stop();
    D_TESTS_SFTP_CHECK(
        (g_fake.shortened > 0) ==
        (_short_read != 0));  // short reads exactly when asked for
    if (_reorder)
    {
        D_TESTS_SFTP_CHECK(g_fake.reordered > 0);
    }

    if (_short_read)
    {
        D_TESTS_SFTP_CHECK(g_fake.reads > (int)(sizeof g_blob / _short_read));
    }

    return;
}

/*
t_fake
  Checks its scenario.
*/
static void
t_fake(
    void
)
{
    for (size_t i = 0; i < sizeof g_blob; i++)
    {
        g_blob[i] = (unsigned char)(i * 131u + (i >> 11));
    }

    fake_transfer_round(0,
                        0);     // in order, whole chunks
    fake_transfer_round(1,
                        0);     // replies in reverse batches
    fake_transfer_round(0,
                        7000);  // short reads
    fake_transfer_round(1,
                        7000);  // both
    // no limits extension: default chunks
    if (fake_start(3,
                   0,
                   0,
                   0))
    {
        D_TESTS_SFTP_CHECK( (fake_open() == D_SFTP_OK)                &&
                            (g_client.read_size == D_SFTP_CHUNK_SIZE) &&
                            (g_client.write_size == D_SFTP_CHUNK_SIZE) );
        fake_stop();
    }

    return;
}

/*
t_fake_2
  t_fake, continued: its checks
  It runs past 75 lines because one fixture's state runs through every
check, and no cut leaves both halves whole.
*/
static void
t_fake_2(
    void
)
{
    struct d_sftp_attributes a = { 0 };

    // a version-2 server is refused, and the client left closed
    if (fake_start(2,
                   0,
                   0,
                   0))
    {
        D_TESTS_SFTP_CHECK( (fake_open() == D_SFTP_ERROR_VERSION) &&
                            (!g_client.open)                      &&
                            (!g_client.buffer) );
        fake_stop();
    }

    // a reply to another request, attributes that cannot be read, a length past
    // the limit
    if (fake_start(3,
                   0,
                   0,
                   0))
    {
        g_fake.cfg.bad_id = 1;
        D_TESTS_SFTP_CHECK(fake_open() == D_SFTP_OK);
        {
            struct d_tests_sftp_source s = { g_blob, 10, 0, 0 };
            D_TESTS_SFTP_CHECK(
                d_sftp_client_put(&g_client,
                                  "f",
                                  NULL,
                                  source_mem,
                                  &s) ==
                D_SFTP_OK);
        }
        D_TESTS_SFTP_CHECK(d_sftp_client_stat(&g_client,
                                              "f",
                                              &a) ==
                           D_SFTP_ERROR_PROTOCOL);
        fake_stop();
    }

    if (fake_start(3,
                   0,
                   0,
                   0))
    {
        g_fake.cfg.bad_attrs = 1;
        D_TESTS_SFTP_CHECK(fake_open() == D_SFTP_OK);
        {
            struct d_tests_sftp_source s = { g_blob, 10, 0, 0 };
            D_TESTS_SFTP_CHECK(
                d_sftp_client_put(&g_client,
                                  "f",
                                  NULL,
                                  source_mem,
                                  &s) ==
                D_SFTP_OK);
        }
        D_TESTS_SFTP_CHECK(d_sftp_client_stat(&g_client,
                                              "f",
                                              &a) ==
                           D_SFTP_ERROR_PROTOCOL);
        fake_stop();
    }

    if (fake_start(3,
                   0,
                   0,
                   0))
    {
        D_TESTS_SFTP_CHECK(fake_open() == D_SFTP_OK);
        g_fake.cfg.huge = 1;
        D_TESTS_SFTP_CHECK(d_sftp_client_stat(&g_client,
                                              "f",
                                              &a) ==
                           D_SFTP_ERROR_PROTOCOL);
        fake_stop();
    }

    // arguments and sequence
    d_sftp_client_init(&g_client);
    D_TESTS_SFTP_CHECK( (d_sftp_client_stat(&g_client,
                                            "x",
                                            &a) == D_SFTP_ERROR_STATE) &&
                        (d_sftp_client_stat(NULL,
                                            "x",
                                            &a) ==
                         D_SFTP_ERROR_INVALID_ARGUMENT) );

    return;
}

/*
t_fake_3
  t_fake, continued: its checks
*/
static void
t_fake_3(
    void
)
{
    D_TESTS_SFTP_CHECK(d_sftp_client_get(&g_client,
                                         "x",
                                         NULL,
                                         NULL) ==
                       D_SFTP_ERROR_INVALID_ARGUMENT);
    {
        struct d_net_connection                     bare     = { NULL };
        static const struct d_net_connection_vtable no_close = {
            fc_read, fc_write, fc_is_open, NULL, NULL, NULL, NULL, NULL, NULL
        };
        struct d_net_connection half = { &no_close };
        D_TESTS_SFTP_CHECK(d_sftp_client_open_connection(&g_client,
                                                         NULL) ==
                           D_SFTP_ERROR_INVALID_ARGUMENT);
        D_TESTS_SFTP_CHECK(d_sftp_client_open_connection(&g_client,
                                                         &bare) ==
                           D_SFTP_ERROR_INVALID_ARGUMENT);
        D_TESTS_SFTP_CHECK(d_sftp_client_open_connection(&g_client,
                                                         &half) ==
                           D_SFTP_ERROR_INVALID_ARGUMENT);
        D_TESTS_SFTP_CHECK( (d_sftp_client_open_connection(NULL,
                                                           &half) ==
                             D_SFTP_ERROR_INVALID_ARGUMENT) &&
                            (!g_client.open) );
    }
    d_sftp_client_close(&g_client);
    d_sftp_client_close(&g_client);

    return;
}

// over a channel of the toy SSH engine, relaying to the in-process server
static struct d_tests_ssh_toy g_toy;
static struct d_ssh_session   g_session;

/*
toy_start
  Starts the in-process server behind the toy SSH engine, and opens an SSH
session and an SFTP channel to it.
*/
static int
toy_start(
    void
)
{
    if (!fake_start(3,
                    1,
                    0,
                    0))
    {
        return 0;
    }

    d_tests_ssh_toy_init(&g_toy);
    g_toy.channels[0].relay = g_client_fd;

    return d_tests_ssh_toy_connect(&g_toy,
                                   &g_session);
}

/*
toy_stop
  Stops the in-process server and disconnects the SSH session.
*/
static void
toy_stop(
    void
)
{
    fake_stop();
    (void)d_ssh_disconnect(&g_session,
                           NULL);
    d_ssh_destroy(&g_session);

    return;
}

/*
t_over_ssh
  Checks d_sftp_client_close, d_sftp_client_get, d_sftp_client_mkdir,
d_sftp_client_open, d_sftp_client_put, d_sftp_client_stat, d_ssh_disconnect,
and more.
  It runs past 75 lines because one fixture's state runs through every
check, and no cut leaves both halves whole.
*/
static void
t_over_ssh(
    void
)
{
    struct d_sftp_attributes   a = { 0 };
    struct d_tests_sftp_source s = { g_blob, 100000, 0, 0 };
    // a whole session: the subsystem asked for, data both ways, and a close
    // that sends EOF and closes and frees the channel
    if (toy_start())
    {
        D_TESTS_SFTP_CHECK( (d_sftp_client_open(&g_client,
                                                &g_session) == D_SFTP_OK) &&
                            (g_client.owns_channel)                       &&
                            (g_client.connection == &g_client.channel.base) );
        D_TESTS_SFTP_CHECK( (!strcmp(g_toy.channels[0].request,
                                     "subsystem")) &&
                            (!strcmp(g_toy.channels[0].value,
                                     "sftp")) );
        D_TESTS_SFTP_CHECK( (g_client.extensions == (D_SFTP_EXT_POSIX_RENAME |
                                                     D_SFTP_EXT_FSYNC        |
                                                     D_SFTP_EXT_LIMITS)) &&
                            (g_client.read_size == 20000) );
        D_TESTS_SFTP_CHECK(
            d_sftp_client_put(&g_client,
                              "blob",
                              NULL,
                              source_mem,
                              &s) ==
            D_SFTP_OK);
        g_sink.len = 0;
        D_TESTS_SFTP_CHECK( (d_sftp_client_get(&g_client,
                                               "blob",
                                               sink_mem,
                                               NULL) == D_SFTP_OK) &&
                            (g_sink.len == 100000)                 &&
                            (!memcmp(g_sink.data,
                                     g_blob,
                                     100000)) );
        D_TESTS_SFTP_CHECK(d_sftp_client_open(&g_client,
                                              &g_session) ==
                           D_SFTP_ERROR_STATE);
        d_sftp_client_close(&g_client);
        D_TESTS_SFTP_CHECK( (!g_client.owns_channel) &&
                            (!g_client.open)         &&
                            (!g_client.connection) );
        D_TESTS_SFTP_CHECK( (g_toy.channels[0].eofs == 1)   &&
                            (g_toy.channels[0].closes == 1) &&
                            (g_toy.channels[0].freed) );
    }
    else
    {
        D_TESTS_SFTP_CHECK(0);
    }

    toy_stop();
    // the stream failing beneath a request: the channel's status beneath the
    // connection's error, the same on every later request
    if (toy_start())
    {
        D_TESTS_SFTP_CHECK(d_sftp_client_open(&g_client,
                                              &g_session) ==
                           D_SFTP_OK);
        g_toy.channels[0].read_fails = D_SSH_ERROR_TRANSPORT;
        D_TESTS_SFTP_CHECK( (d_sftp_client_stat(&g_client,
                                                "blob",
                                                &a) ==
                             D_SFTP_ERROR_TRANSPORT)                    &&
                            (g_client.net_error == D_NET_ERROR_UNKNOWN) &&
                            (g_client.ssh_status == D_SSH_ERROR_TRANSPORT) );
        D_TESTS_SFTP_CHECK( (d_sftp_client_stat(&g_client,
                                                "blob",
                                                &a) ==
                             D_SFTP_ERROR_TRANSPORT)                    &&
                            (g_client.net_error == D_NET_ERROR_UNKNOWN) &&
                            (g_client.ssh_status == D_SSH_ERROR_TRANSPORT) );
        D_TESTS_SFTP_CHECK(d_ssh_get_state(&g_session) == D_SSH_STATE_FAILED);
        d_sftp_client_close(&g_client);
        D_TESTS_SFTP_CHECK( (g_toy.channels[0].closes == 0) &&
                            (g_toy.channels[0].freed)       &&
                            (g_client.ssh_status == D_SSH_ERROR_TRANSPORT) );
    }
    else
    {
        D_TESTS_SFTP_CHECK(0);
    }

    toy_stop();
    // the channel's output ending, and a channel the server timed out on
    if (toy_start())
    {
        D_TESTS_SFTP_CHECK(d_sftp_client_open(&g_client,
                                              &g_session) ==
                           D_SFTP_OK);
        g_toy.channels[0].read_fails = D_SSH_ERROR_CLOSED;
        D_TESTS_SFTP_CHECK( (d_sftp_client_stat(&g_client,
                                                "blob",
                                                &a) ==
                             D_SFTP_ERROR_CONNECTION_LOST)             &&
                            (g_client.net_error == D_NET_ERROR_CLOSED) &&
                            (g_client.ssh_status == D_SSH_ERROR_CLOSED) );
        D_TESTS_SFTP_CHECK(d_ssh_get_state(&g_session) ==
                           D_SSH_STATE_AUTHENTICATED);
        d_sftp_client_close(&g_client);
    }
    else
    {
        D_TESTS_SFTP_CHECK(0);
    }

    toy_stop();
    if (toy_start())
    {
        D_TESTS_SFTP_CHECK(d_sftp_client_open(&g_client,
                                              &g_session) ==
                           D_SFTP_OK);
        g_toy.channels[0].write_fails = D_SSH_ERROR_TIMED_OUT;
        D_TESTS_SFTP_CHECK( (d_sftp_client_mkdir(&g_client,
                                                 "d",
                                                 NULL) ==
                             D_SFTP_ERROR_TRANSPORT)                      &&
                            (g_client.net_error == D_NET_ERROR_TIMED_OUT) &&
                            (g_client.ssh_status == D_SSH_ERROR_TIMED_OUT) );
        d_sftp_client_close(&g_client);
    }
    else
    {
        D_TESTS_SFTP_CHECK(0);
    }

    toy_stop();
    // the session ending under the client: a closed connection, its channel
    // gone with the session
    if (toy_start())
    {
        D_TESTS_SFTP_CHECK(d_sftp_client_open(&g_client,
                                              &g_session) ==
                           D_SFTP_OK);
        D_TESTS_SFTP_CHECK(d_ssh_disconnect(&g_session,
                                            "going") == D_SSH_OK);
        D_TESTS_SFTP_CHECK( (d_sftp_client_stat(&g_client,
                                                "blob",
                                                &a) ==
                             D_SFTP_ERROR_CONNECTION_LOST)             &&
                            (g_client.net_error == D_NET_ERROR_CLOSED) &&
                            (g_client.ssh_status == D_SSH_ERROR_STATE) );
        d_sftp_client_close(&g_client);
        D_TESTS_SFTP_CHECK( (!g_client.owns_channel) &&
                            (g_toy.channels[0].closes == 0) );
    }
    else
    {
        D_TESTS_SFTP_CHECK(0);
    }

    return;
}

/*
t_over_ssh_2
  t_over_ssh, continued: its checks
*/
static void
t_over_ssh_2(
    void
)
{
    toy_stop();

    return;
}

/*
toy_refused
  Refusals before any SFTP is spoken: the channel released, the client closed.
*/
static void
toy_refused(
    enum d_ssh_error  _open_fails,
    enum d_ssh_error  _request_fails,
    enum d_sftp_error _expected
)
{
    struct d_tests_ssh_toy toy     = { 0 };
    struct d_ssh_session   session = { 0 };
    d_tests_ssh_toy_init(&toy);
    toy.open_fails                = _open_fails;
    toy.channels[0].request_fails = _request_fails;
    // the causes of a session that failed before, which a new open forgets
    {
        struct d_tests_sftp_failing f = { 0 };
        fc_make(&f,
                NULL,
                0,
                D_NET_ERROR_TIMED_OUT,
                1000,
                D_NET_OK);
        d_sftp_client_init(&g_client);
        D_TESTS_SFTP_CHECK(d_sftp_client_open_connection(&g_client,
                                                         &f.base) ==
                           D_SFTP_ERROR_TRANSPORT);
        d_sftp_client_close(&g_client);
        D_TESTS_SFTP_CHECK(g_client.net_error == D_NET_ERROR_TIMED_OUT);
    }
    D_TESTS_SFTP_CHECK(d_tests_ssh_toy_connect(&toy,
                                               &session));
    D_TESTS_SFTP_CHECK(d_sftp_client_open(&g_client,
                                          &session) == _expected);
    D_TESTS_SFTP_CHECK( (g_client.ssh_status == ((_open_fails != D_SSH_OK) ?
                                                 _open_fails :
                                                 _request_fails)) &&
                        (g_client.net_error == D_NET_OK) );
    D_TESTS_SFTP_CHECK( (!g_client.open)         &&
                        (!g_client.owns_channel) &&
                        (!g_client.buffer)       &&
                        (!g_client.connection) );
    D_TESTS_SFTP_CHECK(
        (_open_fails != D_SSH_OK)
            ? toy.opened == 0
            : (toy.channels[0].freed && toy.channels[0].requests == 1));
    d_sftp_client_close(&g_client);
    (void)d_ssh_disconnect(&session,
                           NULL);
    d_ssh_destroy(&session);

    return;
}

/*
t_ssh_refusals
  Checks d_sftp_client_init, d_sftp_client_open.
*/
static void
t_ssh_refusals(
    void
)
{
    toy_refused(D_SSH_OK,
                D_SSH_ERROR_CHANNEL,
                D_SFTP_ERROR_UNSUPPORTED);  // no sftp subsystem
    toy_refused(D_SSH_OK,
                D_SSH_ERROR_TRANSPORT,
                D_SFTP_ERROR_TRANSPORT);
    toy_refused(D_SSH_ERROR_CHANNEL,
                D_SSH_OK,
                D_SFTP_ERROR_UNSUPPORTED);  // no channel
    toy_refused(D_SSH_ERROR_TIMED_OUT,
                D_SSH_OK,
                D_SFTP_ERROR_TRANSPORT);
    // a session not authenticated, and arguments
    {
        struct d_ssh_session never = { 0 };
        memset(&never,
               0,
               sizeof never);
        d_sftp_client_init(&g_client);
        D_TESTS_SFTP_CHECK( (d_sftp_client_open(&g_client,
                                                &never) ==
                             D_SFTP_ERROR_TRANSPORT)                   &&
                            (g_client.ssh_status == D_SSH_ERROR_STATE) &&
                            (!g_client.owns_channel) );
        D_TESTS_SFTP_CHECK( (d_sftp_client_open(NULL,
                                                &never) ==
                             D_SFTP_ERROR_INVALID_ARGUMENT) &&
                            (d_sftp_client_open(&g_client,
                                                NULL) ==
                             D_SFTP_ERROR_INVALID_ARGUMENT) );
    }

    return;
}

/*
on_real_entry
  Against a real server, through the SSH module.
*/
static enum d_sftp_error
on_real_entry(
    void*                     _c,
    const struct d_sftp_name* _e
)
{
    int* found = _c;
    *found |=
        (_e->filename.length == 8 && !memcmp(_e->filename.data,
                                             "blob.bin",
                                             8));

    return D_SFTP_OK;
}

/*
t_real
  Checks d_sftp_attributes_is_regular, d_sftp_attributes_is_symlink,
d_sftp_client_close, d_sftp_client_close_file, d_sftp_client_fsync,
d_sftp_client_get, d_sftp_client_init, and more.
  It runs past 75 lines because one fixture's state runs through every
check, and no cut leaves both halves whole.
*/
static void
t_real(
    void
)
{
    const char *port = getenv("SFTP_TEST_PORT"),
               *user = getenv("SFTP_TEST_USER"), *key = getenv("SFTP_TEST_KEY"),
               *dir = getenv("SFTP_TEST_DIR"),
               *kh  = getenv("SFTP_TEST_KNOWN_HOSTS");
    if (!port || !user || !key || !dir || !kh)
    {
        printf("integration: skipped (SFTP_TEST_* not set)\n");

        return;
    }

    static struct d_ssh_session s     = { 0 };
    struct d_ssh_options        o     = d_ssh_options_default();
    struct d_ssh_credentials    c     = d_ssh_credentials_default();
    struct sockaddr_in          addr  = { 0 };
    char                        base[256];  // filled before it is read
    char                        p1[300];  // filled before it is read
    char                        p2[300];  // filled before it is read
    char                        link[300];  // filled before it is read
    char                        out[512];  // filled before it is read
    struct d_sftp_attributes    a     = { 0 };
    int                         found = 0;
    int                         fd    = socket(AF_INET,
                                               SOCK_STREAM,
                                               0);
    memset(&addr,
           0,
           sizeof addr);
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((uint16_t)atoi(port));
    addr.sin_addr.s_addr = htonl(0x7f000001);
    if (fd < 0 || connect(fd,
                          (struct sockaddr*)&addr,
                          sizeof addr))
    {
        printf("integration: cannot connect\n");
        D_TESTS_SFTP_CHECK(0);

        return;
    }

    D_TESTS_SFTP_CHECK(d_ssh_library_init() == D_SSH_OK);
    o.engine          = d_ssh_engine_default();
    o.host            = "127.0.0.1";
    o.port            = (unsigned)atoi(port);
    o.host_key_policy = D_SSH_HOST_KEY_ACCEPT_NEW;
    o.known_hosts     = kh;
    o.timeout_ms      = 10000;
    D_TESTS_SFTP_CHECK(d_ssh_init(&s,
                                  &o,
                                  fd,
                                  true) == D_SSH_OK);
    D_TESTS_SFTP_CHECK(d_ssh_connect(&s) == D_SSH_OK);
    D_TESTS_SFTP_CHECK( (d_ssh_host_key_type(&s) != NULL) &&
                        (d_ssh_host_key_fingerprint(&s) != NULL) );
    c.user        = user;
    c.use_agent   = false;
    c.private_key = key;
    D_TESTS_SFTP_CHECK(d_ssh_authenticate(&s,
                                          &c) == D_SSH_OK);
    d_sftp_client_init(&g_client);
    D_TESTS_SFTP_CHECK( (d_sftp_client_open(&g_client,
                                            &s) == D_SFTP_OK) &&
                        (g_client.owns_channel) );
    printf("integration: version %u, extensions 0x%x, read %u, write %u\n",
           g_client.version,
           g_client.extensions,
           g_client.read_size,
           g_client.write_size);
    snprintf(base,
             sizeof base,
             "%s/sftp_it_%d",
             dir,
             (int)getpid());
    snprintf(p1,
             sizeof p1,
             "%s/blob.bin",
             base);
    snprintf(p2,
             sizeof p2,
             "%s/moved.bin",
             base);
    snprintf(link,
             sizeof link,
             "%s/link",
             base);
    D_TESTS_SFTP_CHECK(d_sftp_client_mkdir(&g_client,
                                           base,
                                           NULL) == D_SFTP_OK);
    D_TESTS_SFTP_CHECK(d_sftp_client_mkdir(&g_client,
                                           base,
                                           NULL) ==
                       D_SFTP_ERROR_FAILURE);
    {
        static unsigned char       big[1 << 20];  // filled before it is read
        struct d_tests_sftp_source src = { big, sizeof big, 0, 0 };
        for (size_t i = 0; i < sizeof big; i++)
        {
            big[i] = (unsigned char)(i * 7u + (i >> 9));
        }

        struct d_sftp_attributes perm = {
            D_SFTP_ATTR_PERMISSIONS, 0, 0, 0, 0600, 0, 0
        };
        D_TESTS_SFTP_CHECK(
            d_sftp_client_put(&g_client,
                              p1,
                              &perm,
                              source_mem,
                              &src) ==
            D_SFTP_OK);
        D_TESTS_SFTP_CHECK( (d_sftp_client_stat(&g_client,
                                                p1,
                                                &a) == D_SFTP_OK) &&
                            (a.size == sizeof big)                &&
                            ((a.permissions & 0777) == 0600)      &&
                            (d_sftp_attributes_is_regular(&a)) );
        g_sink.len = 0;
        D_TESTS_SFTP_CHECK( (d_sftp_client_get(&g_client,
                                               p1,
                                               sink_mem,
                                               NULL) == D_SFTP_OK) &&
                            (g_sink.len == sizeof big)             &&
                            (!memcmp(g_sink.data,
                                     big,
                                     sizeof big)) );
    }
    D_TESTS_SFTP_CHECK( (d_sftp_client_list(&g_client,
                                            base,
                                            on_real_entry,
                                            &found) == D_SFTP_OK) &&
                        (found) );
    D_TESTS_SFTP_CHECK( (d_sftp_client_realpath(&g_client,
                                                base,
                                                out,
                                                sizeof out) == D_SFTP_OK) &&
                        (out[0] == '/') );
    D_TESTS_SFTP_CHECK(d_sftp_client_symlink(&g_client,
                                             p1,
                                             link) == D_SFTP_OK);
    D_TESTS_SFTP_CHECK( (d_sftp_client_readlink(&g_client,
                                                link,
                                                out,
                                                sizeof out) == D_SFTP_OK) &&
                        (!strcmp(out,
                                 p1)) );
    D_TESTS_SFTP_CHECK( (d_sftp_client_lstat(&g_client,
                                             link,
                                             &a) == D_SFTP_OK) &&
                        (d_sftp_attributes_is_symlink(&a)) );
    D_TESTS_SFTP_CHECK( (d_sftp_client_stat(&g_client,
                                            link,
                                            &a) == D_SFTP_OK) &&
                        (d_sftp_attributes_is_regular(&a)) );
    {
        struct d_sftp_attributes perm = {
            D_SFTP_ATTR_PERMISSIONS, 0, 0, 0, 0640, 0, 0
        };
        D_TESTS_SFTP_CHECK( (d_sftp_client_setstat(&g_client,
                                                   p1,
                                                   &perm) == D_SFTP_OK) &&
                            (d_sftp_client_stat(&g_client,
                                                p1,
                                                &a) == D_SFTP_OK)       &&
                            ((a.permissions & 0777) == 0640) );
    }
    {
        struct d_tests_sftp_source src = { (const unsigned char*)"second",
                                           6,
                                           0,
                                           0 };
        D_TESTS_SFTP_CHECK(
            d_sftp_client_put(&g_client,
                              p2,
                              NULL,
                              source_mem,
                              &src) ==
            D_SFTP_OK);
    }
    D_TESTS_SFTP_CHECK(d_sftp_client_rename(&g_client,
                                            p1,
                                            p2,
                                            false) ==
                       D_SFTP_ERROR_FAILURE);
    D_TESTS_SFTP_CHECK( (d_sftp_client_rename(&g_client,
                                              p1,
                                              p2,
                                              true) == D_SFTP_OK) &&
                        (d_sftp_client_stat(&g_client,
                                            p2,
                                            &a) == D_SFTP_OK)     &&
                        (a.size == (1u << 20)) );
    D_TESTS_SFTP_CHECK(d_sftp_client_stat(&g_client,
                                          p1,
                                          &a) ==
                       D_SFTP_ERROR_NO_SUCH_FILE);
    {
        struct d_sftp_handle h   = { 0 };
        size_t               got = 0;
        unsigned char        b[64];  // filled before it is read
        D_TESTS_SFTP_CHECK(
            d_sftp_client_open_file(&g_client,
                                    p2,
                                    D_SFTP_OPEN_READ | D_SFTP_OPEN_WRITE,
                                    NULL,
                                    &h) == D_SFTP_OK);
        D_TESTS_SFTP_CHECK(
            d_sftp_client_write(&g_client,
                                &h,
                                100,
                                "patched",
                                7) == D_SFTP_OK);
        D_TESTS_SFTP_CHECK( (d_sftp_client_read(&g_client,
                                                &h,
                                                100,
                                                b,
                                                sizeof b,
                                                &got) == D_SFTP_OK) &&
                            (got == sizeof b)                       &&
                            (!memcmp(b,
                                     "patched",
                                     7)) );
        D_TESTS_SFTP_CHECK(d_sftp_client_fsync(&g_client,
                                               &h) == D_SFTP_OK);
        D_TESTS_SFTP_CHECK(
            d_sftp_client_read(&g_client,
                               &h,
                               1u << 21,
                               b,
                               sizeof b,
                               &got) ==
            D_SFTP_ERROR_EOF);
        D_TESTS_SFTP_CHECK(d_sftp_client_close_file(&g_client,
                                                    &h) ==
                           D_SFTP_OK);
    }
    D_TESTS_SFTP_CHECK( (d_sftp_client_remove(&g_client,
                                              link) == D_SFTP_OK) &&
                        (d_sftp_client_remove(&g_client,
                                              p2) == D_SFTP_OK) );
    D_TESTS_SFTP_CHECK( (d_sftp_client_rmdir(&g_client,
                                             base) == D_SFTP_OK) &&
                        (d_sftp_client_stat(&g_client,
                                            base,
                                            &a) == D_SFTP_ERROR_NO_SUCH_FILE) );
    D_TESTS_SFTP_CHECK(d_sftp_client_open_file(&g_client,
                                               "/nonexistent/x",
                                               D_SFTP_OPEN_READ,
                                               NULL,
                                               &(struct d_sftp_handle){ 0 }) ==
                       D_SFTP_ERROR_NO_SUCH_FILE);
    d_sftp_client_close(&g_client);
    D_TESTS_SFTP_CHECK( (!g_client.owns_channel) &&
                        (!g_client.open) );
    D_TESTS_SFTP_CHECK(d_ssh_disconnect(&s,
                                        "done") == D_SSH_OK);

    return;
}

/*
t_real_2
  t_real, continued: its checks
*/
static void
t_real_2(
    void
)
{
    d_ssh_library_cleanup();

    return;
}

/*
t_sftp_errors_to_net
  Checks d_sftp_error_to_net.
*/
static void
t_sftp_errors_to_net(
    void
)
{
    static const struct
    {
        enum d_sftp_error from;
        enum d_net_error  to;
    } MAP[] = { { D_SFTP_OK, D_NET_OK },
                { D_SFTP_ERROR_EOF, D_NET_ERROR_CLOSED },
                { D_SFTP_ERROR_NO_SUCH_FILE, D_NET_ERROR_UNKNOWN },
                { D_SFTP_ERROR_PERMISSION_DENIED, D_NET_ERROR_ACCESS_DENIED },
                { D_SFTP_ERROR_FAILURE, D_NET_ERROR_UNKNOWN },
                { D_SFTP_ERROR_BAD_MESSAGE, D_NET_ERROR_PROTOCOL },
                { D_SFTP_ERROR_NO_CONNECTION, D_NET_ERROR_CLOSED },
                { D_SFTP_ERROR_CONNECTION_LOST, D_NET_ERROR_CONNECTION_RESET },
                { D_SFTP_ERROR_UNSUPPORTED, D_NET_ERROR_UNKNOWN },
                { D_SFTP_ERROR_INVALID_ARGUMENT, D_NET_ERROR_INVALID_ARGUMENT },
                { D_SFTP_ERROR_OUT_OF_MEMORY, D_NET_ERROR_OUT_OF_MEMORY },
                { D_SFTP_ERROR_PROTOCOL, D_NET_ERROR_PROTOCOL },
                { D_SFTP_ERROR_VERSION, D_NET_ERROR_PROTOCOL },
                { D_SFTP_ERROR_TRANSPORT, D_NET_ERROR_UNKNOWN },
                { D_SFTP_ERROR_STATE, D_NET_ERROR_INVALID_ARGUMENT },
                { D_SFTP_ERROR_MESSAGE_TOO_LARGE,
                  D_NET_ERROR_MESSAGE_TOO_LARGE } };

    for (size_t i = 0; i < sizeof MAP / sizeof MAP[0]; i++)
    {
        D_TESTS_SFTP_CHECK(d_sftp_error_to_net(MAP[i].from) == MAP[i].to);
    }

    D_TESTS_SFTP_CHECK(d_sftp_error_to_net((enum d_sftp_error)99) ==
                       D_NET_ERROR_UNKNOWN);

    return;
}

/*
t_sftp_errors_from_net
  Checks d_sftp_error_from_net.
*/
static void
t_sftp_errors_from_net(
    void
)
{
    static const struct
    {
        enum d_net_error  from;
        enum d_sftp_error to;
    } MAP[] = { { D_NET_OK, D_SFTP_OK },
                { D_NET_ERROR_CLOSED, D_SFTP_ERROR_CONNECTION_LOST },
                { D_NET_ERROR_WOULD_BLOCK, D_SFTP_ERROR_TRANSPORT },
                { D_NET_ERROR_TIMED_OUT, D_SFTP_ERROR_TRANSPORT },
                { D_NET_ERROR_INTERRUPTED, D_SFTP_ERROR_TRANSPORT },
                { D_NET_ERROR_CONNECTION_RESET, D_SFTP_ERROR_CONNECTION_LOST },
                { D_NET_ERROR_CONNECTION_REFUSED, D_SFTP_ERROR_TRANSPORT },
                { D_NET_ERROR_CONNECTION_ABORTED,
                  D_SFTP_ERROR_CONNECTION_LOST },
                { D_NET_ERROR_ADDRESS_IN_USE, D_SFTP_ERROR_TRANSPORT },
                { D_NET_ERROR_ADDRESS_INVALID, D_SFTP_ERROR_TRANSPORT },
                { D_NET_ERROR_HOST_UNREACHABLE, D_SFTP_ERROR_TRANSPORT },
                { D_NET_ERROR_NETWORK_DOWN, D_SFTP_ERROR_TRANSPORT },
                { D_NET_ERROR_ACCESS_DENIED, D_SFTP_ERROR_TRANSPORT },
                { D_NET_ERROR_INVALID_ARGUMENT, D_SFTP_ERROR_TRANSPORT },
                { D_NET_ERROR_MESSAGE_TOO_LARGE, D_SFTP_ERROR_TRANSPORT },
                { D_NET_ERROR_TOO_MANY_OPEN_FILES, D_SFTP_ERROR_TRANSPORT },
                { D_NET_ERROR_OUT_OF_MEMORY, D_SFTP_ERROR_OUT_OF_MEMORY },
                { D_NET_ERROR_UNKNOWN, D_SFTP_ERROR_TRANSPORT },
                { D_NET_ERROR_PROTOCOL, D_SFTP_ERROR_TRANSPORT } };

    for (size_t i = 0; i < sizeof MAP / sizeof MAP[0]; i++)
    {
        D_TESTS_SFTP_CHECK(d_sftp_error_from_net(MAP[i].from) == MAP[i].to);
    }

    D_TESTS_SFTP_CHECK(d_sftp_error_from_net((enum d_net_error)99) ==
                       D_SFTP_ERROR_TRANSPORT);

    return;
}

/*
t_ssh_errors_to_net
  Checks d_ssh_error_to_net.
*/
static void
t_ssh_errors_to_net(
    void
)
{
    static const struct
    {
        enum d_ssh_error from;
        enum d_net_error to;
    } MAP[] = { { D_SSH_OK, D_NET_OK },
                { D_SSH_ERROR_INVALID_ARGUMENT, D_NET_ERROR_INVALID_ARGUMENT },
                { D_SSH_ERROR_STATE, D_NET_ERROR_INVALID_ARGUMENT },
                { D_SSH_ERROR_OUT_OF_MEMORY, D_NET_ERROR_OUT_OF_MEMORY },
                { D_SSH_ERROR_TRANSPORT, D_NET_ERROR_UNKNOWN },
                { D_SSH_ERROR_CLOSED, D_NET_ERROR_CLOSED },
                { D_SSH_ERROR_TIMED_OUT, D_NET_ERROR_TIMED_OUT },
                { D_SSH_ERROR_PROTOCOL, D_NET_ERROR_PROTOCOL },
                { D_SSH_ERROR_MALFORMED, D_NET_ERROR_INVALID_ARGUMENT },
                { D_SSH_ERROR_NO_BACKEND, D_NET_ERROR_UNKNOWN },
                { D_SSH_ERROR_UNSUPPORTED, D_NET_ERROR_UNKNOWN },
                { D_SSH_ERROR_HANDSHAKE, D_NET_ERROR_PROTOCOL },
                { D_SSH_ERROR_HOST_UNKNOWN, D_NET_ERROR_ACCESS_DENIED },
                { D_SSH_ERROR_HOST_CHANGED, D_NET_ERROR_ACCESS_DENIED },
                { D_SSH_ERROR_HOST_REVOKED, D_NET_ERROR_ACCESS_DENIED },
                { D_SSH_ERROR_KNOWN_HOSTS, D_NET_ERROR_INVALID_ARGUMENT },
                { D_SSH_ERROR_AUTH, D_NET_ERROR_ACCESS_DENIED },
                { D_SSH_ERROR_NO_AUTH_METHOD, D_NET_ERROR_ACCESS_DENIED },
                { D_SSH_ERROR_KEY, D_NET_ERROR_INVALID_ARGUMENT },
                { D_SSH_ERROR_CHANNEL, D_NET_ERROR_UNKNOWN } };

    for (size_t i = 0; i < sizeof MAP / sizeof MAP[0]; i++)
    {
        D_TESTS_SFTP_CHECK(d_ssh_error_to_net(MAP[i].from) == MAP[i].to);
    }

    D_TESTS_SFTP_CHECK(d_ssh_error_to_net((enum d_ssh_error)99) ==
                       D_NET_ERROR_UNKNOWN);

    return;
}

/*
main
  Runs the codec, in-process and failing-connection tests, then the
integration test when SFTP_TEST_PORT names a server; the exit status is
whether any check failed.
*/
int
main(
    void
)
{
    signal(SIGPIPE,
           SIG_IGN);  // a peer that vanished is a failed check, not a dead test
    t_codec();
    t_sftp_errors_to_net();
    t_sftp_errors_from_net();
    t_ssh_errors_to_net();
    t_fake();
    t_fake_2();
    t_fake_3();
    t_failing();
    t_failing_2();
    t_over_ssh();
    t_over_ssh_2();
    t_ssh_refusals();
    t_real();
    t_real_2();
    free(g_sink.data);
    printf("%d checks, %d failures\n",
           g_checks,
           g_fail);

    return g_fail != 0;
}
