// SFTP tests: the codec on its own; the client against an in-process server
// over a socket pair, which can answer out of order, read short, and break
// the protocol on purpose; and, when SFTP_TEST_PORT is set, the client over
// the SSH module against a real server.
//
// Integration variables: SFTP_TEST_PORT (on 127.0.0.1), SFTP_TEST_USER,
// SFTP_TEST_KEY (a private key file), SFTP_TEST_DIR (a scratch directory the
// user may write), and SFTP_TEST_KNOWN_HOSTS (a file to record the host in).
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../../../../inc/djinterp/net/sftp/sftp.h"
#include "../../../../inc/djinterp/net/ssh/ssh.h"

static int g_fail = 0, g_checks = 0;
#define CHECK(c) do { g_checks++; if (!(c)) { g_fail++; printf("FAIL %d: %s\n", __LINE__, #c); } } while (0)

// ---------------------------------------------------------------------------
// the codec
// ---------------------------------------------------------------------------
static void t_codec(void)
{
    struct d_ssh_writer w; struct d_ssh_reader r; struct d_sftp_attributes a, b; size_t mark = 0, size = 0;
    struct d_sftp_packet p; struct d_sftp_handle h; struct d_pack_text msg; uint32_t code = 0, ext = 0;
    d_ssh_writer_init(&w);
    d_sftp_attributes_init(&a);
    a.flags = D_SFTP_ATTR_SIZE | D_SFTP_ATTR_UIDGID | D_SFTP_ATTR_PERMISSIONS | D_SFTP_ATTR_ACMODTIME;
    a.size = 0x123456789ull; a.uid = 1000; a.gid = 100; a.permissions = 0100644; a.atime = 11; a.mtime = 22;
    CHECK(d_sftp_write_attributes(&w, &a));
    d_ssh_reader_init(&r, w.data, w.size);
    CHECK(d_sftp_read_attributes(&r, &b) && d_ssh_reader_done(&r) && !memcmp(&a, &b, sizeof a));
    CHECK(d_sftp_attributes_is_regular(&b) && !d_sftp_attributes_is_directory(&b) && !d_sftp_attributes_is_symlink(&b));
    b.permissions = 0040755; CHECK(d_sftp_attributes_is_directory(&b));
    b.flags = 0; CHECK(!d_sftp_attributes_is_directory(&b));
    // undefined flag bits cannot be skipped, so the record is refused
    { static const unsigned char bad[] = { 0, 0, 0, 0x40, 1, 2, 3, 4 }; d_ssh_reader_init(&r, bad, sizeof bad); CHECK(!d_sftp_read_attributes(&r, &b) && r.failed && b.flags == 0); }
    // extended pairs are skipped, and the flag dropped
    { static const unsigned char ex[] = { 0x80, 0, 0, 0x01, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 1, 0, 0, 0, 1, 'k', 0, 0, 0, 1, 'v' };
      d_ssh_reader_init(&r, ex, sizeof ex); CHECK(d_sftp_read_attributes(&r, &b) && d_ssh_reader_done(&r) && b.flags == D_SFTP_ATTR_SIZE && b.size == 9); }
    a.flags |= D_SFTP_ATTR_EXTENDED; w.size = 0; w.failed = false; CHECK(!d_sftp_write_attributes(&w, &a));
    // packets: begin/end, frame, parse
    w.size = 0; w.failed = false;
    CHECK(d_sftp_packet_begin(&w, D_SFTP_FXP_STAT, 77, &mark) && d_ssh_write_cstring(&w, "/x") && d_sftp_packet_end(&w, mark));
    CHECK(d_sftp_packet_frame(w.data, w.size, &size) == D_SFTP_FRAME_COMPLETE && size == w.size);
    CHECK(d_sftp_packet_frame(w.data, w.size - 1, &size) == D_SFTP_FRAME_INCOMPLETE && d_sftp_packet_frame(w.data, 3, &size) == D_SFTP_FRAME_INCOMPLETE);
    { static const unsigned char zero[] = { 0, 0, 0, 0 }, huge[] = { 0, 0x04, 0, 1 };
      CHECK(d_sftp_packet_frame(zero, 4, &size) == D_SFTP_FRAME_MALFORMED && d_sftp_packet_frame(huge, 4, &size) == D_SFTP_FRAME_MALFORMED); }
    CHECK(d_sftp_packet_parse(w.data + 4, w.size - 4, &p) == D_SFTP_OK && p.type == D_SFTP_FXP_STAT && p.id == 77 && d_ssh_reader_remaining(&p.body) == 6);
    CHECK(d_sftp_packet_parse(w.data + 4, 3, &p) == D_SFTP_ERROR_PROTOCOL);
    // status with and without its message
    { static const unsigned char st[] = { 0, 0, 0, 2, 0, 0, 0, 4, 'g', 'o', 'n', 'e', 0, 0, 0, 0 }, bare[] = { 0, 0, 0, 1 };
      d_ssh_reader_init(&r, st, sizeof st); CHECK(d_sftp_read_status(&r, &code, &msg) == D_SFTP_OK && code == 2 && msg.length == 4 && !memcmp(msg.data, "gone", 4));
      d_ssh_reader_init(&r, bare, sizeof bare); CHECK(d_sftp_read_status(&r, &code, &msg) == D_SFTP_OK && code == 1 && msg.length == 0); }
    CHECK(d_sftp_error_from_status(3) == D_SFTP_ERROR_PERMISSION_DENIED && d_sftp_error_from_status(11) == D_SFTP_ERROR_FAILURE);
    // a handle longer than 256 bytes is malformed
    w.size = 0; w.failed = false; { unsigned char big[300]; memset(big, 'h', sizeof big);
      CHECK(d_ssh_write_string(&w, big, 256)); d_ssh_reader_init(&r, w.data, w.size); CHECK(d_sftp_read_handle(&r, &h) == D_SFTP_OK && h.length == 256);
      w.size = 0; CHECK(d_ssh_write_string(&w, big, 257)); d_ssh_reader_init(&r, w.data, w.size); CHECK(d_sftp_read_handle(&r, &h) == D_SFTP_ERROR_PROTOCOL); }
    // extensions: known names set bits, unknown ones are skipped
    w.size = 0; w.failed = false;
    CHECK(d_ssh_write_cstring(&w, "posix-rename@openssh.com") && d_ssh_write_cstring(&w, "1") && d_ssh_write_cstring(&w, "x@y") && d_ssh_write_cstring(&w, "") &&
          d_ssh_write_cstring(&w, "limits@openssh.com") && d_ssh_write_cstring(&w, "1"));
    d_ssh_reader_init(&r, w.data, w.size); CHECK(d_sftp_read_extensions(&r, &ext) == D_SFTP_OK && ext == (D_SFTP_EXT_POSIX_RENAME | D_SFTP_EXT_LIMITS));
    d_ssh_reader_init(&r, w.data, w.size - 1); CHECK(d_sftp_read_extensions(&r, &ext) == D_SFTP_ERROR_PROTOCOL);
    for (int e = 0; e <= 8; e++) CHECK(d_sftp_error_string((enum d_sftp_error)e)[0] != '\0');
    CHECK(strcmp(d_sftp_error_string(D_SFTP_ERROR_TOO_LARGE), "too large") == 0);
    d_ssh_writer_free(&w);
}

// ---------------------------------------------------------------------------
// the in-process server
// ---------------------------------------------------------------------------
struct fs_file { char name[128]; unsigned char* data; size_t size, cap; uint32_t perm; int used, dir; };
struct fake
{
    int fd; pthread_t thread;
    struct { uint32_t version, read_limit, short_read; int limits, reorder, bad_id, huge, bad_attrs; } cfg;
    struct fs_file files[16]; int listed;
    unsigned char* queue[64]; size_t queue_size[64], queued;
    int reads, writes, reordered;
};
static uint32_t be32(const unsigned char* p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }
static int read_exact(int fd, void* b, size_t n)
{
    size_t got = 0;
    while (got < n) { ssize_t r = read(fd, (char*)b + got, n - got); if (r < 0 && errno == EINTR) continue; if (r <= 0) return 0; got += (size_t)r; }
    return 1;
}
static int write_all(int fd, const void* b, size_t n)
{
    size_t done = 0;
    while (done < n) { ssize_t r = write(fd, (const char*)b + done, n - done); if (r < 0 && errno == EINTR) continue; if (r <= 0) return 0; done += (size_t)r; }
    return 1;
}
static void fs_begin(struct d_ssh_writer* w, uint8_t type, uint32_t id)
{ w->size = 0; w->failed = false; (void)d_ssh_write_uint32(w, 0); (void)d_ssh_write_byte(w, type); (void)d_ssh_write_uint32(w, id); }
static void fs_flush(struct fake* f)
{
    for (size_t i = f->queued; i > 0; i--) { write_all(f->fd, f->queue[i - 1], f->queue_size[i - 1]); free(f->queue[i - 1]); }
    f->reordered += (f->queued > 1); f->queued = 0;
}
static void fs_send(struct fake* f, struct d_ssh_writer* w, int reorderable)
{
    size_t n = w->size - 4; w->data[0] = (unsigned char)(n >> 24); w->data[1] = (unsigned char)(n >> 16); w->data[2] = (unsigned char)(n >> 8); w->data[3] = (unsigned char)n;
    if (f->cfg.reorder && reorderable && f->queued < 64) { f->queue[f->queued] = malloc(w->size); memcpy(f->queue[f->queued], w->data, w->size); f->queue_size[f->queued++] = w->size; return; }
    fs_flush(f); write_all(f->fd, w->data, w->size);
}
static void fs_status(struct fake* f, struct d_ssh_writer* w, uint32_t id, uint32_t code, const char* text, int reorderable)
{ fs_begin(w, D_SFTP_FXP_STATUS, id); (void)d_ssh_write_uint32(w, code); (void)d_ssh_write_cstring(w, text); (void)d_ssh_write_cstring(w, ""); fs_send(f, w, reorderable); }
static struct fs_file* fs_find(struct fake* f, const unsigned char* name, size_t len)
{
    for (int i = 0; i < 16; i++) if (f->files[i].used && strlen(f->files[i].name) == len && !memcmp(f->files[i].name, name, len)) return &f->files[i];
    return NULL;
}
static struct fs_file* fs_create(struct fake* f, const unsigned char* name, size_t len, int dir)
{
    for (int i = 0; i < 16; i++) if (!f->files[i].used && len < 128)
    { struct fs_file* x = &f->files[i]; memset(x, 0, sizeof *x); memcpy(x->name, name, len); x->used = 1; x->dir = dir; x->perm = dir ? 0040755 : 0100644; return x; }
    return NULL;
}
static void fs_attrs(struct d_ssh_writer* w, const struct fs_file* x, int bad)
{
    (void)d_ssh_write_uint32(w, bad ? 0x40u : (D_SFTP_ATTR_SIZE | D_SFTP_ATTR_PERMISSIONS));
    (void)d_ssh_write_uint64(w, x->size); (void)d_ssh_write_uint32(w, x->perm);
}
static struct fs_file* fs_handle(struct fake* f, const unsigned char* h, size_t n)
{ return (h && n == 2 && h[0] == 'F' && h[1] < 16 && f->files[h[1]].used) ? &f->files[h[1]] : NULL; }
static int input_pending(int fd) { struct pollfd p = { fd, POLLIN, 0 }; return poll(&p, 1, 30) > 0; }
static void* fs_main(void* arg)
{
    struct fake* f = arg; struct d_ssh_writer w; d_ssh_writer_init(&w);
    for (;;)
    {
        unsigned char head[4]; if (!read_exact(f->fd, head, 4)) break;
        size_t n = be32(head); unsigned char* pkt = malloc(n);
        if (!pkt || !read_exact(f->fd, pkt, n)) { free(pkt); break; }
        struct d_ssh_reader r; uint8_t type = 0; uint32_t id = 0; d_ssh_reader_init(&r, pkt, n);
        (void)d_ssh_read_byte(&r, &type); (void)d_ssh_read_uint32(&r, &id);
        const unsigned char *s1 = NULL, *s2 = NULL; size_t l1 = 0, l2 = 0; struct fs_file* x = NULL;
        if (type == D_SFTP_FXP_INIT)
        {
            fs_begin(&w, D_SFTP_FXP_VERSION, f->cfg.version);
            (void)d_ssh_write_cstring(&w, "posix-rename@openssh.com"); (void)d_ssh_write_cstring(&w, "1");
            (void)d_ssh_write_cstring(&w, "fsync@openssh.com"); (void)d_ssh_write_cstring(&w, "1");
            if (f->cfg.limits) { (void)d_ssh_write_cstring(&w, "limits@openssh.com"); (void)d_ssh_write_cstring(&w, "1"); }
            fs_send(f, &w, 0);
        }
        else if (f->cfg.huge) { static const unsigned char h[4] = { 0x7f, 0xff, 0xff, 0xff }; write_all(f->fd, h, 4); }
        else if (type == D_SFTP_FXP_EXTENDED)
        {
            (void)d_ssh_read_string(&r, &s1, &l1);
            if (l1 == 18 && !memcmp(s1, "limits@openssh.com", 18))
            { fs_begin(&w, D_SFTP_FXP_EXTENDED_REPLY, id); (void)d_ssh_write_uint64(&w, 34000); (void)d_ssh_write_uint64(&w, f->cfg.read_limit);
              (void)d_ssh_write_uint64(&w, 30000); (void)d_ssh_write_uint64(&w, 64); fs_send(f, &w, 0); }
            else if (l1 == 24 && !memcmp(s1, "posix-rename@openssh.com", 24))
            {
                (void)d_ssh_read_string(&r, &s1, &l1); (void)d_ssh_read_string(&r, &s2, &l2);
                struct fs_file* from = fs_find(f, s1, l1); struct fs_file* to = fs_find(f, s2, l2);
                if (!from) fs_status(f, &w, id, 2, "no such file", 0);
                else { if (to && to != from) { free(to->data); to->used = 0; } memset(from->name, 0, 128); memcpy(from->name, s2, l2); fs_status(f, &w, id, 0, "ok", 0); }
            }
            else fs_status(f, &w, id, 0, "ok", 0);   // fsync
        }
        else if (type == D_SFTP_FXP_OPEN)
        {
            uint32_t pflags = 0; struct d_sftp_attributes a;
            (void)d_ssh_read_string(&r, &s1, &l1); (void)d_ssh_read_uint32(&r, &pflags); (void)d_sftp_read_attributes(&r, &a);
            x = fs_find(f, s1, l1);
            if (!x && (pflags & D_SFTP_OPEN_CREATE)) { x = fs_create(f, s1, l1, 0); if (x && (a.flags & D_SFTP_ATTR_PERMISSIONS)) x->perm = 0100000u | (a.permissions & 07777u); }
            if (!x) { fs_status(f, &w, id, 2, "no such file", 0); goto next; }
            if (pflags & D_SFTP_OPEN_TRUNCATE) x->size = 0;
            fs_begin(&w, D_SFTP_FXP_HANDLE, id); { unsigned char hd[2] = { 'F', (unsigned char)(x - f->files) }; (void)d_ssh_write_string(&w, hd, 2); } fs_send(f, &w, 0);
        }
        else if (type == D_SFTP_FXP_READ)
        {
            uint64_t off = 0; uint32_t len = 0; (void)d_ssh_read_string(&r, &s1, &l1); (void)d_ssh_read_uint64(&r, &off); (void)d_ssh_read_uint32(&r, &len);
            x = fs_handle(f, s1, l1); f->reads++;
            if (!x) { fs_status(f, &w, id, 4, "bad handle", 0); goto next; }
            if (off >= x->size) { fs_status(f, &w, id, 1, "eof", 1); goto next; }
            size_t give = x->size - off; if (give > len) give = len; if (f->cfg.short_read && give > f->cfg.short_read) give = f->cfg.short_read;
            fs_begin(&w, D_SFTP_FXP_DATA, id); (void)d_ssh_write_string(&w, x->data + off, give); fs_send(f, &w, 1);
        }
        else if (type == D_SFTP_FXP_WRITE)
        {
            uint64_t off = 0; (void)d_ssh_read_string(&r, &s1, &l1); (void)d_ssh_read_uint64(&r, &off); (void)d_ssh_read_string(&r, &s2, &l2);
            x = fs_handle(f, s1, l1); f->writes++;
            if (!x) { fs_status(f, &w, id, 4, "bad handle", 0); goto next; }
            if (off + l2 > x->cap) { x->cap = (size_t)(off + l2) * 2; x->data = realloc(x->data, x->cap); }
            memcpy(x->data + off, s2, l2); if (off + l2 > x->size) x->size = (size_t)(off + l2);
            fs_status(f, &w, id, 0, "ok", 1);
        }
        else if (type == D_SFTP_FXP_CLOSE || type == D_SFTP_FXP_SETSTAT || type == D_SFTP_FXP_SYMLINK) fs_status(f, &w, id, 0, "ok", 0);
        else if (type == D_SFTP_FXP_STAT || type == D_SFTP_FXP_LSTAT || type == D_SFTP_FXP_FSTAT)
        {
            (void)d_ssh_read_string(&r, &s1, &l1);
            x = (type == D_SFTP_FXP_FSTAT) ? fs_handle(f, s1, l1) : fs_find(f, s1, l1);
            if (!x) { fs_status(f, &w, id, 2, "No such file", 0); goto next; }
            fs_begin(&w, D_SFTP_FXP_ATTRS, f->cfg.bad_id ? id + 1000 : id); fs_attrs(&w, x, f->cfg.bad_attrs); fs_send(f, &w, 0);
        }
        else if (type == D_SFTP_FXP_OPENDIR) { f->listed = 0; fs_begin(&w, D_SFTP_FXP_HANDLE, id); (void)d_ssh_write_cstring(&w, "D"); fs_send(f, &w, 0); }
        else if (type == D_SFTP_FXP_READDIR)
        {
            if (f->listed) { fs_status(f, &w, id, 1, "eof", 0); goto next; }
            uint32_t count = 0; for (int i = 0; i < 16; i++) count += (uint32_t)f->files[i].used;
            fs_begin(&w, D_SFTP_FXP_NAME, id); (void)d_ssh_write_uint32(&w, count);
            for (int i = 0; i < 16; i++) if (f->files[i].used) { (void)d_ssh_write_cstring(&w, f->files[i].name); (void)d_ssh_write_cstring(&w, "-rw-r--r-- long"); fs_attrs(&w, &f->files[i], 0); }
            f->listed = 1; fs_send(f, &w, 0);
        }
        else if (type == D_SFTP_FXP_MKDIR) { (void)d_ssh_read_string(&r, &s1, &l1); if (fs_find(f, s1, l1)) fs_status(f, &w, id, 4, "exists", 0); else { fs_create(f, s1, l1, 1); fs_status(f, &w, id, 0, "ok", 0); } }
        else if (type == D_SFTP_FXP_REMOVE || type == D_SFTP_FXP_RMDIR)
        { (void)d_ssh_read_string(&r, &s1, &l1); x = fs_find(f, s1, l1); if (!x) fs_status(f, &w, id, 2, "no such file", 0); else { free(x->data); x->used = 0; fs_status(f, &w, id, 0, "ok", 0); } }
        else if (type == D_SFTP_FXP_RENAME)
        {
            (void)d_ssh_read_string(&r, &s1, &l1); (void)d_ssh_read_string(&r, &s2, &l2);
            x = fs_find(f, s1, l1);
            if (!x) fs_status(f, &w, id, 2, "no such file", 0); else if (fs_find(f, s2, l2)) fs_status(f, &w, id, 4, "target exists", 0);
            else { memset(x->name, 0, 128); memcpy(x->name, s2, l2); fs_status(f, &w, id, 0, "ok", 0); }
        }
        else if (type == D_SFTP_FXP_REALPATH || type == D_SFTP_FXP_READLINK)
        {
            (void)d_ssh_read_string(&r, &s1, &l1);
            fs_begin(&w, D_SFTP_FXP_NAME, id); (void)d_ssh_write_uint32(&w, 1);
            (void)d_ssh_write_cstring(&w, type == D_SFTP_FXP_REALPATH ? "/home/test" : "target.txt"); (void)d_ssh_write_cstring(&w, ""); (void)d_ssh_write_uint32(&w, 0);
            fs_send(f, &w, 0);
        }
        else fs_status(f, &w, id, 8, "unsupported", 0);
    next:
        free(pkt);
        if (f->queued && !input_pending(f->fd)) fs_flush(f);
    }
    fs_flush(f); d_ssh_writer_free(&w);
    return NULL;
}

static enum d_ssh_status fd_read(void* c, void* b, size_t cap, size_t* n)
{
    ssize_t r; do r = read(*(int*)c, b, cap); while (r < 0 && errno == EINTR);
    *n = r > 0 ? (size_t)r : 0; return r > 0 ? D_SSH_OK : (r == 0 ? D_SSH_ERR_CLOSED : D_SSH_ERR_IO);
}
static enum d_ssh_status fd_write(void* c, const void* d, size_t n) { return write_all(*(int*)c, d, n) ? D_SSH_OK : D_SSH_ERR_IO; }

static struct fake g_fake; static int g_client_fd;
static struct d_sftp_client g_client;
static int fake_start(uint32_t version, int limits, int reorder, uint32_t short_read)
{
    int sv[2]; memset(&g_fake, 0, sizeof g_fake);
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv)) return 0;
    g_fake.fd = sv[1]; g_client_fd = sv[0];
    g_fake.cfg.version = version; g_fake.cfg.limits = limits; g_fake.cfg.reorder = reorder; g_fake.cfg.short_read = short_read; g_fake.cfg.read_limit = 20000;
    if (pthread_create(&g_fake.thread, NULL, fs_main, &g_fake)) return 0;
    d_sftp_client_init(&g_client);
    return 1;
}
static enum d_sftp_error fake_open(void) { struct d_sftp_transport t = { fd_read, fd_write, &g_client_fd }; return d_sftp_client_open_transport(&g_client, t); }
static void fake_stop(void)
{
    d_sftp_client_close(&g_client); shutdown(g_client_fd, SHUT_RDWR); close(g_client_fd);
    pthread_join(g_fake.thread, NULL); close(g_fake.fd);
    for (int i = 0; i < 16; i++) if (g_fake.files[i].used) free(g_fake.files[i].data);
}

// sinks and sources
static struct { unsigned char* data; size_t len, cap, fail_after; } g_sink;
static enum d_sftp_error sink_mem(void* c, const void* d, size_t n)
{
    (void)c;
    if (g_sink.fail_after && g_sink.len >= g_sink.fail_after) return D_SFTP_ERROR_FAILURE + 0x200;   // a caller's own error code
    if (g_sink.len + n > g_sink.cap) { g_sink.cap = (g_sink.len + n) * 2; g_sink.data = realloc(g_sink.data, g_sink.cap); }
    memcpy(g_sink.data + g_sink.len, d, n); g_sink.len += n; return D_SFTP_OK;
}
struct src { const unsigned char* data; size_t len, pos, step; };
static enum d_sftp_error source_mem(void* c, void* b, size_t cap, size_t* out)
{
    struct src* s = c; size_t n = s->len - s->pos; if (n > cap) n = cap; if (s->step && n > s->step) n = s->step;
    memcpy(b, s->data + s->pos, n); s->pos += n; *out = n; return D_SFTP_OK;
}
static int g_entries, g_saw_blob;
static enum d_sftp_error on_entry(void* c, const struct d_sftp_name* e)
{ (void)c; g_entries++; g_saw_blob |= (e->filename.length == 4 && !memcmp(e->filename.data, "blob", 4) && e->attributes.size == 300000); return D_SFTP_OK; }

static unsigned char g_blob[300000];

static void fake_transfer_round(int reorder, uint32_t short_read)
{
    struct d_sftp_attributes a; struct src s = { g_blob, sizeof g_blob, 0, 0 }; char path[64]; struct d_sftp_handle h; size_t got = 0; unsigned char small[100];
    if (!fake_start(3, 1, reorder, short_read)) { CHECK(0); return; }
    CHECK(fake_open() == D_SFTP_OK && g_client.version == 3);
    CHECK(g_client.extensions == (D_SFTP_EXT_POSIX_RENAME | D_SFTP_EXT_FSYNC | D_SFTP_EXT_LIMITS));
    CHECK(g_client.read_size == 20000 && g_client.write_size == 30000);   // limits, then the 34000-byte packet cap
    CHECK(d_sftp_client_put(&g_client, "blob", NULL, source_mem, &s) == D_SFTP_OK);
    CHECK(g_fake.files[0].size == sizeof g_blob && !memcmp(g_fake.files[0].data, g_blob, sizeof g_blob));
    g_sink.len = 0;
    CHECK(d_sftp_client_get(&g_client, "blob", sink_mem, NULL) == D_SFTP_OK && g_sink.len == sizeof g_blob && !memcmp(g_sink.data, g_blob, sizeof g_blob));
    CHECK(d_sftp_client_stat(&g_client, "blob", &a) == D_SFTP_OK && a.size == sizeof g_blob && d_sftp_attributes_is_regular(&a));
    CHECK(d_sftp_client_stat(&g_client, "missing", &a) == D_SFTP_ERROR_NO_SUCH_FILE && g_client.status_code == 2 && !strcmp(g_client.status_message, "No such file"));
    CHECK(d_sftp_client_mkdir(&g_client, "dir", NULL) == D_SFTP_OK && d_sftp_client_mkdir(&g_client, "dir", NULL) == D_SFTP_ERROR_FAILURE);
    g_entries = 0; g_saw_blob = 0;
    CHECK(d_sftp_client_list(&g_client, ".", on_entry, NULL) == D_SFTP_OK && g_entries == 2 && g_saw_blob);
    CHECK(d_sftp_client_realpath(&g_client, ".", path, sizeof path) == D_SFTP_OK && !strcmp(path, "/home/test"));
    CHECK(d_sftp_client_realpath(&g_client, ".", path, 5) == D_SFTP_ERROR_TOO_LARGE);
    CHECK(d_sftp_client_readlink(&g_client, "l", path, sizeof path) == D_SFTP_OK && !strcmp(path, "target.txt"));
    CHECK(d_sftp_client_symlink(&g_client, "blob", "l") == D_SFTP_OK);
    CHECK(d_sftp_client_rename(&g_client, "blob", "dir", false) == D_SFTP_ERROR_FAILURE);
    CHECK(d_sftp_client_rename(&g_client, "blob", "dir", true) == D_SFTP_OK && d_sftp_client_stat(&g_client, "dir", &a) == D_SFTP_OK && a.size == sizeof g_blob);
    CHECK(d_sftp_client_open_file(&g_client, "dir", D_SFTP_OPEN_READ, NULL, &h) == D_SFTP_OK);
    CHECK(d_sftp_client_read(&g_client, &h, 1000, small, sizeof small, &got) == D_SFTP_OK && got > 0 && !memcmp(small, g_blob + 1000, got));
    CHECK(d_sftp_client_read(&g_client, &h, sizeof g_blob, small, sizeof small, &got) == D_SFTP_ERROR_EOF && got == 0);
    CHECK(d_sftp_client_write(&g_client, &h, 5, "XYZ", 3) == D_SFTP_OK && !memcmp(g_fake.files[0].data + 5, "XYZ", 3));
    CHECK(d_sftp_client_fstat(&g_client, &h, &a) == D_SFTP_OK && a.size == sizeof g_blob);
    CHECK(d_sftp_client_fsync(&g_client, &h) == D_SFTP_OK && d_sftp_client_close_file(&g_client, &h) == D_SFTP_OK);
    // a sink that gives up: its own error, and the stream still in step
    g_sink.len = 0; g_sink.fail_after = 50000;
    CHECK(d_sftp_client_get(&g_client, "dir", sink_mem, NULL) == D_SFTP_ERROR_FAILURE + 0x200);
    g_sink.fail_after = 0;
    CHECK(d_sftp_client_stat(&g_client, "dir", &a) == D_SFTP_OK && a.size == sizeof g_blob);
    CHECK(d_sftp_client_remove(&g_client, "dir") == D_SFTP_OK && d_sftp_client_remove(&g_client, "dir") == D_SFTP_ERROR_NO_SUCH_FILE);
    CHECK(d_sftp_client_put(&g_client, "empty", NULL, source_mem, &(struct src){ g_blob, 0, 0, 0 }) == D_SFTP_OK);
    g_sink.len = 0; CHECK(d_sftp_client_get(&g_client, "empty", sink_mem, NULL) == D_SFTP_OK && g_sink.len == 0);
    fake_stop();
    if (reorder) CHECK(g_fake.reordered > 0);
    if (short_read) CHECK(g_fake.reads > (int)(sizeof g_blob / short_read));
}

static void t_fake(void)
{
    struct d_sftp_attributes a;
    for (size_t i = 0; i < sizeof g_blob; i++) g_blob[i] = (unsigned char)(i * 131u + (i >> 11));
    fake_transfer_round(0, 0);        // in order, whole chunks
    fake_transfer_round(1, 0);        // replies in reverse batches
    fake_transfer_round(0, 7000);     // short reads
    fake_transfer_round(1, 7000);     // both
    // no limits extension: default chunks
    if (fake_start(3, 0, 0, 0)) { CHECK(fake_open() == D_SFTP_OK && g_client.read_size == D_SFTP_CHUNK_SIZE && g_client.write_size == D_SFTP_CHUNK_SIZE); fake_stop(); }
    // a version-2 server is refused, and the client left closed
    if (fake_start(2, 0, 0, 0)) { CHECK(fake_open() == D_SFTP_ERROR_VERSION && !g_client.open && !g_client.buffer); fake_stop(); }
    // a reply to another request, attributes that cannot be read, a length past the limit
    if (fake_start(3, 0, 0, 0)) { g_fake.cfg.bad_id = 1; CHECK(fake_open() == D_SFTP_OK);
        { struct src s = { g_blob, 10, 0, 0 }; CHECK(d_sftp_client_put(&g_client, "f", NULL, source_mem, &s) == D_SFTP_OK); }
        CHECK(d_sftp_client_stat(&g_client, "f", &a) == D_SFTP_ERROR_PROTOCOL); fake_stop(); }
    if (fake_start(3, 0, 0, 0)) { g_fake.cfg.bad_attrs = 1; CHECK(fake_open() == D_SFTP_OK);
        { struct src s = { g_blob, 10, 0, 0 }; CHECK(d_sftp_client_put(&g_client, "f", NULL, source_mem, &s) == D_SFTP_OK); }
        CHECK(d_sftp_client_stat(&g_client, "f", &a) == D_SFTP_ERROR_PROTOCOL); fake_stop(); }
    if (fake_start(3, 0, 0, 0)) { CHECK(fake_open() == D_SFTP_OK); g_fake.cfg.huge = 1; CHECK(d_sftp_client_stat(&g_client, "f", &a) == D_SFTP_ERROR_PROTOCOL); fake_stop(); }
    // arguments and sequence
    d_sftp_client_init(&g_client);
    CHECK(d_sftp_client_stat(&g_client, "x", &a) == D_SFTP_ERROR_BAD_SEQUENCE && d_sftp_client_stat(NULL, "x", &a) == D_SFTP_ERROR_INVALID_ARGUMENT);
    CHECK(d_sftp_client_get(&g_client, "x", NULL, NULL) == D_SFTP_ERROR_INVALID_ARGUMENT);
    { struct d_sftp_transport t = { NULL, fd_write, NULL }; CHECK(d_sftp_client_open_transport(&g_client, t) == D_SFTP_ERROR_INVALID_ARGUMENT); }
    d_sftp_client_close(&g_client); d_sftp_client_close(&g_client);
}

// ---------------------------------------------------------------------------
// against a real server, through the SSH module
// ---------------------------------------------------------------------------
static enum d_sftp_error on_real_entry(void* c, const struct d_sftp_name* e)
{ int* found = c; *found |= (e->filename.length == 8 && !memcmp(e->filename.data, "blob.bin", 8)); return D_SFTP_OK; }

static void t_real(void)
{
    const char *port = getenv("SFTP_TEST_PORT"), *user = getenv("SFTP_TEST_USER"), *key = getenv("SFTP_TEST_KEY"), *dir = getenv("SFTP_TEST_DIR"), *kh = getenv("SFTP_TEST_KNOWN_HOSTS");
    if (!port || !user || !key || !dir || !kh) { printf("integration: skipped (SFTP_TEST_* not set)\n"); return; }
    static struct d_ssh_session s; struct d_ssh_options o = d_ssh_options_default(); struct d_ssh_credentials c = d_ssh_credentials_default();
    struct sockaddr_in addr; char base[256], p1[300], p2[300], link[300], out[512]; struct d_sftp_attributes a; int found = 0;
    int fd = socket(AF_INET, SOCK_STREAM, 0); memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET; addr.sin_port = htons((uint16_t)atoi(port)); addr.sin_addr.s_addr = htonl(0x7f000001);
    if (fd < 0 || connect(fd, (struct sockaddr*)&addr, sizeof addr)) { printf("integration: cannot connect\n"); CHECK(0); return; }
    CHECK(d_ssh_library_init() == D_SSH_OK);
    o.engine = d_ssh_engine_default(); o.host = "127.0.0.1"; o.port = (unsigned)atoi(port); o.host_key_policy = D_SSH_HOST_KEY_ACCEPT_NEW; o.known_hosts = kh; o.timeout_ms = 10000;
    CHECK(d_ssh_init(&s, &o, fd, true) == D_SSH_OK);
    CHECK(d_ssh_connect(&s) == D_SSH_OK);
    CHECK(d_ssh_host_key_type(&s) != NULL && d_ssh_host_key_fingerprint(&s) != NULL);
    c.user = user; c.use_agent = false; c.private_key = key;
    CHECK(d_ssh_authenticate(&s, &c) == D_SSH_OK);
    d_sftp_client_init(&g_client);
    CHECK(d_sftp_client_open(&g_client, &s) == D_SFTP_OK && g_client.owns_channel);
    printf("integration: version %u, extensions 0x%x, read %u, write %u\n", g_client.version, g_client.extensions, g_client.read_size, g_client.write_size);
    snprintf(base, sizeof base, "%s/sftp_it_%d", dir, (int)getpid());
    snprintf(p1, sizeof p1, "%s/blob.bin", base); snprintf(p2, sizeof p2, "%s/moved.bin", base); snprintf(link, sizeof link, "%s/link", base);
    CHECK(d_sftp_client_mkdir(&g_client, base, NULL) == D_SFTP_OK);
    CHECK(d_sftp_client_mkdir(&g_client, base, NULL) == D_SFTP_ERROR_FAILURE);
    { static unsigned char big[1 << 20]; struct src src = { big, sizeof big, 0, 0 };
      for (size_t i = 0; i < sizeof big; i++) big[i] = (unsigned char)(i * 7u + (i >> 9));
      struct d_sftp_attributes perm = { D_SFTP_ATTR_PERMISSIONS, 0, 0, 0, 0600, 0, 0 };
      CHECK(d_sftp_client_put(&g_client, p1, &perm, source_mem, &src) == D_SFTP_OK);
      CHECK(d_sftp_client_stat(&g_client, p1, &a) == D_SFTP_OK && a.size == sizeof big && (a.permissions & 0777) == 0600 && d_sftp_attributes_is_regular(&a));
      g_sink.len = 0; CHECK(d_sftp_client_get(&g_client, p1, sink_mem, NULL) == D_SFTP_OK && g_sink.len == sizeof big && !memcmp(g_sink.data, big, sizeof big)); }
    CHECK(d_sftp_client_list(&g_client, base, on_real_entry, &found) == D_SFTP_OK && found);
    CHECK(d_sftp_client_realpath(&g_client, base, out, sizeof out) == D_SFTP_OK && out[0] == '/');
    CHECK(d_sftp_client_symlink(&g_client, p1, link) == D_SFTP_OK);
    CHECK(d_sftp_client_readlink(&g_client, link, out, sizeof out) == D_SFTP_OK && !strcmp(out, p1));
    CHECK(d_sftp_client_lstat(&g_client, link, &a) == D_SFTP_OK && d_sftp_attributes_is_symlink(&a));
    CHECK(d_sftp_client_stat(&g_client, link, &a) == D_SFTP_OK && d_sftp_attributes_is_regular(&a));
    { struct d_sftp_attributes perm = { D_SFTP_ATTR_PERMISSIONS, 0, 0, 0, 0640, 0, 0 };
      CHECK(d_sftp_client_setstat(&g_client, p1, &perm) == D_SFTP_OK && d_sftp_client_stat(&g_client, p1, &a) == D_SFTP_OK && (a.permissions & 0777) == 0640); }
    { struct src src = { (const unsigned char*)"second", 6, 0, 0 };
      CHECK(d_sftp_client_put(&g_client, p2, NULL, source_mem, &src) == D_SFTP_OK); }
    CHECK(d_sftp_client_rename(&g_client, p1, p2, false) == D_SFTP_ERROR_FAILURE);
    CHECK(d_sftp_client_rename(&g_client, p1, p2, true) == D_SFTP_OK && d_sftp_client_stat(&g_client, p2, &a) == D_SFTP_OK && a.size == (1u << 20));
    CHECK(d_sftp_client_stat(&g_client, p1, &a) == D_SFTP_ERROR_NO_SUCH_FILE);
    { struct d_sftp_handle h; size_t got = 0; unsigned char b[64];
      CHECK(d_sftp_client_open_file(&g_client, p2, D_SFTP_OPEN_READ | D_SFTP_OPEN_WRITE, NULL, &h) == D_SFTP_OK);
      CHECK(d_sftp_client_write(&g_client, &h, 100, "patched", 7) == D_SFTP_OK);
      CHECK(d_sftp_client_read(&g_client, &h, 100, b, sizeof b, &got) == D_SFTP_OK && got == sizeof b && !memcmp(b, "patched", 7));
      CHECK(d_sftp_client_fsync(&g_client, &h) == D_SFTP_OK);
      CHECK(d_sftp_client_read(&g_client, &h, 1u << 21, b, sizeof b, &got) == D_SFTP_ERROR_EOF);
      CHECK(d_sftp_client_close_file(&g_client, &h) == D_SFTP_OK); }
    CHECK(d_sftp_client_remove(&g_client, link) == D_SFTP_OK && d_sftp_client_remove(&g_client, p2) == D_SFTP_OK);
    CHECK(d_sftp_client_rmdir(&g_client, base) == D_SFTP_OK && d_sftp_client_stat(&g_client, base, &a) == D_SFTP_ERROR_NO_SUCH_FILE);
    CHECK(d_sftp_client_open_file(&g_client, "/nonexistent/x", D_SFTP_OPEN_READ, NULL, &(struct d_sftp_handle){ 0 }) == D_SFTP_ERROR_NO_SUCH_FILE);
    d_sftp_client_close(&g_client);
    CHECK(!g_client.owns_channel && !g_client.open);
    CHECK(d_ssh_disconnect(&s, "done") == D_SSH_OK);
    d_ssh_library_cleanup();
}

int main(void)
{
    signal(SIGPIPE, SIG_IGN);   // a peer that vanished is a failed check, not a dead test
    t_codec(); t_fake(); t_real();
    free(g_sink.data);
    printf("%d checks, %d failures\n", g_checks, g_fail);
    return g_fail != 0;
}
