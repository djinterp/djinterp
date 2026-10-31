#!/usr/bin/env python3
"""gen_db_face.py -- write the C++ faces of the database layers.

Each layer's face, inc/djinterp/c/db/<vendor>/<vendor>_face.hpp, is a class
with a member for every function of the layer, the connection argument
dropped and every other argument in the C order, and a class for each object
the layer hands out -- the SQL layers' prepared statement, MongoDB's cursor
and change stream -- with a member for every function that takes one first,
the object argument dropped, and its name's prefix too where the object's
functions share one (d_mongodb_cursor_fetch is cursor.fetch). Those members
are generated here,
from the prototypes in the layer's module headers -- the ones its umbrella,
<vendor>.h, includes -- into the face's template,
scripts/db_face/<vendor>_face.hpp.in, which holds everything written by hand:
the banner, ownership, the format alias, and the functions that open,
connect, close or finalize. A template line "@@MEMBERS <header> <which>@@"
becomes that header's members, in the header's order, <which> being
"static" (those that take neither a connection nor an object first),
"instance" (those that take a connection), "all" (both), or an object's
name -- "statement", "cursor", "change_stream" -- for those that take that
object.

How a C parameter crosses (db_face.hpp says why):
  const char*                    string_arg, which takes a std::string too
  char**                         text&; and a std::string& form, and from
                                 C++17 a std::optional<std::string>& form,
                                 each taking every text the function
                                 writes
  struct dj_db_name_list*        name_list&, and a std::vector form
  struct dj_db_column_list*      column_list&
  struct dj_db_rows*             rows&
  struct dj_db_pending*          dj_db_pending&: what a call that does not
                                 block waits for, carried between its
                                 continuations; never a result, so no
                                 std::expected form takes it
  struct <object>**              the object's face, &: a prepare, or an
                                 opening, fills it, finishing what it held
  const struct dj_db_column*,    a pointer and a count; and a form taking
  size_t                         an array, whose size is the count, and
                                 from C++20 one taking a std::span
  any other T*, void* aside      T&
  ...                            from C++11, a variadic template whose
                                 format is the face's format<Args...> --
                                 checked at compile time from C++20 -- and
                                 whose arguments must be what C's ...
                                 carries
A function returning char* -- text the layer allocated -- returns a
std::string, and has an exact form that hands the text to a text& instead.
A function with one result written through a parameter has, from C++23, a
form returning std::expected<the result, int> instead -- unless that
parameter describes a failure (FAILURE_TEXTS), which is no result, or the
function does not block, D_DB_WAIT being no failure. A function whose
name says it only asks -- has_, list_, get_, is_, ... (CONST_NAMES) -- is a
const member. Every form is the C call, inline: noexcept unless it makes a
std::string or a std::vector, and [[nodiscard]] when its return value is
its only result. The functions in SPECIAL are written by hand in the
template.

  usage:  scripts/gen_db_face.py [--check] [sqlite | mariadb | postgres |
                                 mongodb] ...
          (no vendor: all four)
          --check  writes nothing, and fails if a face is not what this
                   script would write
  exit:   0 when every face was written, or is current; 1 otherwise
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VENDORS = ('sqlite', 'mariadb', 'postgres', 'mongodb')

# the connection each vendor's functions take first
HANDLE = {'sqlite': 'sqlite3', 'mariadb': 'dj_mariadb', 'postgres': 'pg_conn',
          'mongodb': 'dj_mongodb'}

# the objects each vendor hands out, which their own functions take first:
# for each, the C struct, its face, the owner's name a template line names,
# the face's member that holds it, and the prefix its functions' names share,
# which its members' names drop
class Object(object):
    """an object a layer hands out, and its face"""

    def __init__(self, ctype, face, key, member, prefix=''):
        self.ctype, self.face, self.key = ctype, face, key
        self.member, self.prefix = member, prefix


OBJECTS = {
    'sqlite': [Object('sqlite3_stmt', 'sqlite_statement', 'statement',
                      'm_statement')],
    'mariadb': [Object('dj_mariadb_statement', 'mariadb_statement',
                       'statement', 'm_statement')],
    'postgres': [Object('dj_postgres_statement', 'postgres_statement',
                        'statement', 'm_statement')],
    'mongodb': [Object('dj_mongodb_cursor', 'mongodb_cursor', 'cursor',
                       'm_cursor', 'cursor_'),
                Object('dj_mongodb_change_stream', 'mongodb_change_stream',
                       'change_stream', 'm_stream', 'change_stream_')],
}

# written by hand in the template: the functions that make or end a
# connection or a statement, or that need two, or whose C form a member
# cannot mirror
SPECIAL = {
    'sqlite': {'open', 'open_readonly', 'open_memory', 'open_with', 'close',
               'copy_database', 'finalize'},
    'mariadb': {'connect', 'connect_with', 'adopt', 'close',
                'connect_start', 'connect_with_start', 'connect_continue',
                'cancel_start', 'cancel_continue', 'finalize'},
    'postgres': {'connect', 'connect_string', 'connect_with', 'close',
                 'connect_start', 'connect_string_start',
                 'connect_with_start', 'connect_continue', 'cancel_finish',
                 'result_sqlstate', 'finalize'},
    'mongodb': {'connect', 'connect_uri', 'connect_with', 'adopt', 'close',
                'cursor_close', 'change_stream_close'},
}

# names the template declares, which no function of a layer may take
RESERVED = {'format', 'transaction_mode', 'default_transaction_mode',
            'handle', 'is_open', 'is_prepared', 'out', 'release', 'reset',
            'swap'}

# members that only ask, which are const
CONST_NAMES = re.compile(
    r'^(has_|list_|get_|current_|is_|count_rows$|database_path$|'
    r'last_insert_id$|changes$|total_changes$|error_|server_|supports$|'
    r'connection_id$|warning_count$|backend_pid$|in_transaction$|'
    r'in_failed_transaction$|sqlstate$|integrity_check$|quick_check$|'
    r'check_table$|checksum_table$|table_size$|database_size$|'
    r'quote_literal$|native$|parameter_count$|parameter_index$|'
    r'statement_connection$|parameter_status$|transaction_status$|'
    r'pipeline_status$|count_documents$|estimated_document_count$|'
    r'collection_size$|collection_info$|describe_indexes$|validate$|'
    r'resume_token$)')

# static members that only ask, whose return value is their answer
ASKING_STATIC = re.compile(
    r'^(library_|header_|client_|bson_|result_|quote_|is_|supports$|'
    r'get_log_level$)')

MEMBERS = re.compile(r'^\s*@@MEMBERS (\S+) (\w+)@@\s*$')

# the levels a form needs (db_face.hpp, 1)
CPP11 = 'D_ENV_LANG_IS_CPP11_OR_HIGHER'
OPTIONAL = 'D_INTERNAL_DB_FACE_OPTIONAL'
SPAN = 'D_INTERNAL_DB_FACE_SPAN'
EXPECTED = 'D_INTERNAL_DB_FACE_EXPECTED'

# what a result written through a parameter is, by its kind
OUTPUTS = ('text', 'names', 'columns_out', 'rows', 'statement', 'out')

# text parameters that describe a failure rather than give a result, which a
# std::expected form would lose: such a function has no such form
FAILURE_TEXTS = ('_message',)


class Param(object):
    """a face parameter: its declaration, how it reaches the C call, and
    what kind of parameter it is"""

    def __init__(self, decl, arg, kind='plain', face=None):
        self.decl, self.arg, self.kind = decl, arg, kind
        self.face = face


def prototypes(vendor, text):
    """(return type, C name, parameters) for each prototype in a header,
    each declared D_NOTHROW, as every function of a layer is"""
    proto = re.compile(
        r'^((?:const\s+)?(?:struct\s+|enum\s+)?[A-Za-z_][\w ]*?\**)\s*\n?\s*'
        r'(d_' + vendor + r'_\w+)\(([^;]*?)\)(\s*D_NOTHROW)?;', re.M | re.S)
    for m in proto.finditer(text):
        if not m.group(4):
            raise SystemExit('%s is not declared D_NOTHROW' % m.group(2))
        yield (' '.join(m.group(1).split()), m.group(2),
               parameters(m.group(3)))


def parameters(text):
    """(type, name) for each parameter; ('...', None) for the varargs"""
    out = []
    text = ' '.join(text.split())
    if text == 'void':
        return out
    for raw in text.split(','):
        p = raw.strip()
        if p == '...':
            out.append(('...', None))
            continue
        m = re.match(r'^(.*?)(\**)\s*(_\w+)(\[\d+\])?$', p)
        if not m:
            raise SystemExit('cannot read parameter: %r' % p)
        base, stars, name, array = (m.group(1).strip(), m.group(2),
                                    m.group(3), m.group(4))
        ty = re.sub(r'\s*\*', '*', base + stars)
        out.append((ty + (array or ''), name))
    return out


def cxx(ty):
    """the C++ spelling of a C type: no struct or enum keyword, the
    fixed-width integers from re_std"""
    ty = re.sub(r'\b(struct|enum)\s+', '', ty)
    ty = re.sub(r'\b(u?int(?:8|16|32|64)_t)\b', r're_std::\1', ty)
    ty = re.sub(r'\bsize_t\b', 'std::size_t', ty)
    return ty


def is_handle(vendor, ty):
    return re.match(r'^(const )?(struct )?%s\*$' % HANDLE[vendor],
                    ty) is not None


def object_of(vendor, ty):
    """the object a parameter of a type is, or None"""
    for obj in OBJECTS[vendor]:
        if re.match(r'^(const )?(struct )?%s\*$' % obj.ctype, ty):
            return obj
    return None


def object_slot(vendor, ty):
    """the object a parameter of a type fills, struct <object>**, or None"""
    for obj in OBJECTS[vendor]:
        if ty == 'struct %s**' % obj.ctype:
            return obj
    return None


def convert(vendor, params, variadic):
    """the face's parameters, and how each reaches the C call"""
    out = []
    i = 0
    while i < len(params):
        ty, name = params[i]
        if name is None:
            out.append(Param(None, None, 'varargs'))
        elif ty == 'const char*' and variadic and name == '_format':
            out.append(Param(('format<Args...>', name),
                             '%s.c_str()' % name, 'format'))
        elif ty == 'const char*':
            out.append(Param(('string_arg', name), '%s.c_str()' % name))
        elif ty == 'char**':
            out.append(Param(('text&', name), '%s.out()' % name, 'text'))
        elif ty == 'struct dj_db_name_list*':
            out.append(Param(('name_list&', name), '%s.out()' % name,
                             'names'))
        elif ty == 'struct dj_db_column_list*':
            out.append(Param(('column_list&', name), '%s.out()' % name,
                             'columns_out'))
        elif ty == 'struct dj_db_rows*':
            out.append(Param(('rows&', name), '%s.out()' % name, 'rows'))
        elif ty == 'struct dj_db_pending*':
            out.append(Param(('dj_db_pending&', name), '&%s' % name,
                             'pending'))
        elif object_slot(vendor, ty):
            obj = object_slot(vendor, ty)
            out.append(Param((obj.face + '&', name), '%s.out()' % name,
                             'statement', obj.face))
        elif (ty == 'const struct dj_db_column*' and i + 1 < len(params) and
              params[i + 1][0] == 'size_t'):
            out.append(Param(('const dj_db_column*', name), name, 'columns'))
            out.append(Param(('std::size_t', params[i + 1][1]),
                             params[i + 1][1], 'count'))
            i += 1
        elif ty == 'const struct dj_db_column*':
            out.append(Param(('const dj_db_column&', name), '&%s' % name))
        elif ty.endswith('*') and not ty.startswith('const ') and \
                ty != 'void*':
            out.append(Param((cxx(ty[:-1]) + '&', name), '&%s' % name,
                             'out'))
        else:
            out.append(Param((cxx(ty), name), name))
        i += 1
    return out


def wrap_call(indent, head, args, tail=');'):
    """a call, on one line when it fits, else one argument a line, aligned
    after its parenthesis; a head may carry its own indentation"""
    lead = ' ' * indent + head
    base = len(lead) - len(lead.lstrip())
    one = '%s%s%s' % (lead, ', '.join(args), tail)
    if len(one) <= 80 or not args:
        return [one]
    col = len(lead)
    lines = [lead + args[0] + ',']
    lines += [' ' * col + a + ',' for a in args[1:-1]]
    lines.append(' ' * col + args[-1] + tail)
    if max(len(l) for l in lines) <= 80:
        return lines
    lines = [lead.rstrip()]
    for k, a in enumerate(args):
        lines.append(' ' * (base + 4) + a +
                     (',' if k < len(args) - 1 else tail))
    return lines


def signature(ret, name, decls, const, static=False, template=None,
              nodiscard=False, noexcept=False):
    """a member's declaration, in the style guide's definition shape"""
    lines = []
    if template:
        lines.append('    template<%s>' % template)
    lines.append('    %s%s%s' % ('D_NODISCARD ' if nodiscard else '',
                                 'static ' if static else '', ret))
    tail = '%s%s' % (' const' if const else '',
                     ' D_NOEXCEPT' if noexcept else '')
    if not decls:
        lines.append('    %s()%s' % (name, tail))
        return lines
    width = max(len(t) for (t, n) in decls)
    lines.append('    %s(' % name)
    for k, (t, n) in enumerate(decls):
        lines.append('        %s %s%s' % (t.ljust(width), n,
                                         ',' if k < len(decls) - 1 else ''))
    lines.append('    )%s' % tail)
    return lines


def declare(pairs):
    """consecutive local declarations, their names aligned"""
    width = max(len(t) for (t, rest) in pairs)
    return ['        %s %s' % (t.ljust(width), rest) for (t, rest) in pairs]


def guarded(condition, lines):
    """a form that exists from the level a condition names"""
    if not condition:
        return lines
    return ['#if %s' % condition] + lines + ['#endif  // %s' % condition]


def owner(vendor, params):
    """whose member a function is: the connection's, an object's (its key),
    or the face's own, static"""
    if params and is_handle(vendor, params[0][0]):
        return 'connection'
    if params and object_of(vendor, params[0][0]):
        return object_of(vendor, params[0][0]).key
    return 'static'


class Function(object):
    """one C function, and what its face members need to know of it"""

    def __init__(self, vendor, ret_c, cname, params):
        self.short = cname[len('d_%s_' % vendor):]
        self.vendor = vendor
        self.cname = cname
        self.ret_c = ret_c
        self.owner = owner(vendor, params)
        self.static = (self.owner == 'static')
        obj = object_of(vendor, params[0][0]) if params else None
        if self.owner == 'connection':
            self.self_arg = 'm_db'
        elif obj:
            self.self_arg = obj.member
            if obj.prefix and self.short.startswith(obj.prefix):
                self.short = self.short[len(obj.prefix):]
        else:
            self.self_arg = None
        if self.short in RESERVED:
            raise SystemExit('%s: its name is the template\'s' % cname)
        rest = params if self.static else params[1:]
        self.variadic = any(name is None for (ty, name) in rest)
        self.conv = convert(vendor, rest, self.variadic)
        self.const = ((not self.static) and
                      CONST_NAMES.match(self.short) is not None)
        self.outs = [p for p in self.conv if p.kind in OUTPUTS]
        self.template = 'typename... Args' if self.variadic else None
        self.condition = CPP11 if self.variadic else None

    def decls(self, replace=None, drop=()):
        """the member's parameters, one replaced or some dropped"""
        replace = replace or {}
        out = []
        for p in self.conv:
            if p.kind in drop or (p.decl and p.decl[1] in drop):
                continue
            if p.kind == 'varargs':
                out.append(('Args...', '_args'))
            elif p.decl and p.decl[1] in replace:
                out.append(replace[p.decl[1]])
            elif p.decl:
                out.append(p.decl)
        return out

    def args(self, replace=None, drop=()):
        """the C call's arguments, some replaced, some dropped"""
        replace = replace or {}
        out = [] if self.static else [self.self_arg]
        for p in self.conv:
            if p.kind in drop or (p.decl and p.decl[1] in drop):
                continue
            if p.kind == 'varargs':
                out.append('_args...')
            elif p.decl and p.decl[1] in replace:
                out.append(replace[p.decl[1]])
            else:
                out.append(p.arg)
        return out

    def prologue(self):
        """what an f form checks before it calls"""
        if not self.variadic:
            return []
        return ['        internal::check_varargs<Args...>();', '']


def form_main(f):
    """the member that forwards to the C function"""
    owned = (f.ret_c == 'char*')
    ret = 'std::string' if owned else cxx(f.ret_c)
    nodiscard = ((not f.outs) and f.ret_c != 'void' and
                 (owned or f.const or
                  (f.static and ASKING_STATIC.match(f.short) is not None)))
    body = f.prologue()
    if owned:
        body += wrap_call(8, 'const text result(%s(' % f.cname, f.args(),
                          '));')
        body += ['', '        return result.str();']
    else:
        body += wrap_call(8, 'return %s(' % f.cname, f.args())
    lines = signature(ret, f.short, f.decls(), f.const, f.static,
                      f.template, nodiscard, not owned)
    return guarded(f.condition, lines + ['    {'] + body + ['    }'])


def form_owned(f):
    """text a function returns, which the layer allocated, handed to a text
    the caller gives: the exact form, which the std::string form copies"""
    if f.variadic:
        raise SystemExit('%s: returns char* and takes ...' % f.cname)
    name = '_quoted' if f.short.startswith('quote_') else '_result'
    lines = signature('void', f.short, f.decls() + [('text&', name)],
                      f.const, f.static, None, False, True)
    body = wrap_call(8, '%s.reset(%s(' % (name, f.cname), f.args(), '));')
    body += ['', '        return;']
    return lines + ['    {'] + body + ['    }']


def form_text_as(f, outs, kind):
    """text results as std::strings, or from C++17 as
    std::optional<std::string>s: every text the function writes, in one form,
    each into a text of its own first"""
    names = [o.decl[1] for o in outs]
    locals_ = (['text_value'] if len(names) == 1 else
               ['%s_value' % n.lstrip('_') for n in names])
    if kind == 'string':
        decl, condition = 'std::string&', f.condition
        assign = ['']
        for (name, local) in zip(names, locals_):
            assign.append('        %s = %s.str();' % (name, local))
    else:
        decl, condition = 'std::optional<std::string>&', OPTIONAL
        assign = []
        for (name, local) in zip(names, locals_):
            assign += ['',
                       '        // the text; nothing for SQL NULL, and for a '
                       'failure',
                       '        if (%s.is_null())' % local,
                       '        {',
                       '            %s.reset();' % name,
                       '        }',
                       '        else',
                       '        {',
                       '            %s = %s.str();' % (name, local),
                       '        }']
    lines = signature('int', f.short,
                      f.decls(dict((n, (decl, n)) for n in names)),
                      f.const, False, f.template)
    # the texts and the result, declared together, their names aligned
    heads = declare([('text', '%s;' % local) for local in locals_] +
                    [('const int', 'result = %s(' % f.cname)])
    call = wrap_call(0, heads[-1],
                     f.args(dict((n, '%s.out()' % local)
                                 for (n, local) in zip(names, locals_))))
    body = (f.prologue() + heads[:-1] + call + assign +
            ['', '        return result;'])
    return guarded(condition, lines + ['    {'] + body + ['    }'])


def form_vector(f, out):
    """a name list as a std::vector<std::string>"""
    name = out.decl[1]
    lines = signature('int', f.short,
                      f.decls({name: ('std::vector<std::string>&', name)}),
                      f.const, False, f.template)
    heads = declare([('name_list', 'names;'),
                     ('const int', 'result = %s(' % f.cname)])
    call = wrap_call(0, heads[-1], f.args({name: 'names.out()'}))
    body = (f.prologue() + heads[:-1] + call +
            ['', '        names.to_vector(%s);' % name, '',
             '        return result;'])
    return guarded(f.condition, lines + ['    {'] + body + ['    }'])


def form_columns(f, kind):
    """a column array, its count deduced; or from C++20 a std::span"""
    cols = [p for p in f.conv if p.kind == 'columns'][0]
    count = [p for p in f.conv if p.kind == 'count'][0]
    name = cols.decl[1]
    if kind == 'array':
        decls = f.decls({name: ('const dj_db_column', '(&%s)[Count]' % name)},
                        drop=('count',))
        args = f.args({name: name, count.decl[1]: 'Count'})
        template, condition = 'std::size_t Count', None
    else:
        decls = f.decls({name: ('std::span<const dj_db_column>', name)},
                        drop=('count',))
        args = f.args({name: '%s.data()' % name,
                       count.decl[1]: '%s.size()' % name})
        template, condition = None, SPAN
    lines = signature('int', f.short, decls, f.const, False, template,
                      False, True)
    body = wrap_call(8, 'return %s(' % f.cname, args)
    return guarded(condition, lines + ['    {'] + body + ['    }'])


def form_expected(f, out):
    """the result written through a parameter, returned instead, from
    C++23: std::expected<the result, int>"""
    if out.kind == 'text':
        value, arg = 'text', 'value.out()'
    elif out.kind == 'names':
        value, arg = 'name_list', 'value.out()'
    elif out.kind == 'columns_out':
        value, arg = 'column_list', 'value.out()'
    elif out.kind == 'rows':
        value, arg = 'rows', 'value.out()'
    elif out.kind == 'statement':
        value, arg = out.face, 'value.out()'
    else:
        value, arg = out.decl[0][:-1], '&value'
    ret = 'std::expected<%s, int>' % value
    lines = signature(ret, f.short, f.decls(drop=(out.decl[1],)), f.const,
                      False, f.template, True, True)
    heads = declare([(value, 'value{};'),
                     ('const int', 'result = %s(' % f.cname)])
    call = wrap_call(0, heads[-1], f.args({out.decl[1]: arg}))
    body = (f.prologue() + heads[:-1] + call +
            ['',
             '        // the value, or the layer\'s result when it failed',
             '        if (result != 0)',
             '        {',
             '            return std::unexpected(result);',
             '        }',
             '',
             '        return value;'])
    return guarded(EXPECTED, lines + ['    {'] + body + ['    }'])


def member(vendor, ret_c, cname, params):
    """the members that forward to one C function: the member itself, and
    the forms the levels add"""
    f = Function(vendor, ret_c, cname, params)
    blocks = []
    if f.ret_c == 'char*':
        blocks.append(form_owned(f))
    blocks.append(form_main(f))
    texts = [out for out in f.outs if out.kind == 'text']
    if texts:
        blocks.append(form_text_as(f, texts, 'string'))
        blocks.append(form_text_as(f, texts, 'optional'))
    for out in f.outs:
        if out.kind == 'names':
            blocks.append(form_vector(f, out))
    if any(p.kind == 'columns' for p in f.conv):
        blocks.append(form_columns(f, 'array'))
        blocks.append(form_columns(f, 'span'))
    if ( (len(f.outs) == 1) and (f.ret_c == 'int') and
         not (f.outs[0].decl and f.outs[0].decl[1] in FAILURE_TEXTS) and
         not any(p.kind == 'pending' for p in f.conv) ):
        blocks.append(form_expected(f, f.outs[0]))
    return ['\n'.join(b) for b in blocks]


def records(vendor):
    """one record per function of the layer, in its headers' order"""
    layer = os.path.join(ROOT, 'inc', 'djinterp', 'c', 'db', vendor)
    umbrella = open(os.path.join(layer, vendor + '.h'),
                    encoding='utf-8').read()
    headers = re.findall(r'^#include "\./(%s_[a-z]*\.h)"' % vendor,
                         umbrella, re.M)
    out = []
    for header in headers:
        text = open(os.path.join(layer, header), encoding='utf-8').read()
        for (ret, cname, params) in prototypes(vendor, text):
            short = cname[len('d_%s_' % vendor):]
            if short in SPECIAL[vendor]:
                out.append({'header': header, 'name': short,
                            'special': True})
                continue
            out.append({'header': header, 'name': short, 'special': False,
                        'owner': owner(vendor, params),
                        'code': member(vendor, ret, cname, params)})
    return out


def assemble(vendor, recs):
    """the template, its @@MEMBERS lines filled; every generated member
    placed exactly once"""
    path = os.path.join(ROOT, 'scripts', 'db_face', vendor + '_face.hpp.in')
    template = open(path, encoding='utf-8').read().split('\n')
    if not any(re.match(r'^\s*using format = ', l) for l in template):
        raise SystemExit('%s: no format alias for the f forms' % path)
    placed = set()
    out = []
    for line in template:
        m = MEMBERS.match(line)
        if not m:
            out.append(line)
            continue
        header, which = m.group(1), m.group(2)
        keys = [o.key for o in OBJECTS[vendor]]
        if which not in ('static', 'instance', 'all') and which not in keys:
            raise SystemExit('%s: no owner %s: %s' % (path, which,
                                                      line.strip()))
        owners = {'static': ('static',), 'instance': ('connection',),
                  'all': ('static', 'connection')}.get(which, (which,))
        chosen = [r for r in recs
                  if (r['header'] == header and not r['special'] and
                      r['owner'] in owners)]
        if not chosen:
            raise SystemExit('%s: no members for %s' % (path, line.strip()))
        blocks = []
        for r in chosen:
            key = (r['header'], r['name'])
            if key in placed:
                raise SystemExit('%s: placed twice: %s' % (path, key))
            placed.add(key)
            blocks.extend(r['code'])
        out.append('\n\n'.join(blocks))
    missing = [(r['header'], r['name']) for r in recs
               if not r['special'] and (r['header'], r['name']) not in placed]
    if missing:
        raise SystemExit('%s: not placed: %s' % (path, missing))
    return '\n'.join(out)


def main(argv):
    check = '--check' in argv
    vendors = [a for a in argv if a != '--check'] or list(VENDORS)
    status = 0
    for vendor in vendors:
        if vendor not in VENDORS:
            raise SystemExit('unknown vendor: %s' % vendor)
        text = assemble(vendor, records(vendor))
        face = os.path.join(ROOT, 'inc', 'djinterp', 'cpp', 'db', vendor,
                            vendor + '_face.hpp')
        rel = os.path.relpath(face, ROOT)
        if check:
            current = (open(face, encoding='utf-8', newline='').read()
                       if os.path.exists(face) else None)
            if current != text:
                print('%s: not what scripts/gen_db_face.py writes; run it' % rel)
                status = 1
            else:
                print('%s: current' % rel)
        else:
            open(face, 'w', encoding='utf-8', newline='\n').write(text)
            print('%s: written' % rel)
    return status


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
