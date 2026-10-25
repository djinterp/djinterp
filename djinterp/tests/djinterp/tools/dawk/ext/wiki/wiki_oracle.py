# wiki_oracle.py -- the wiki reader's records, compared with other readers'.
#
# Reads each page named, or found beneath a directory named, with
# test_dwiki --dump and with another reader, and compares what both make of
# it: the headings, by level, line and title; the links and images, by
# destination; and, where the other reader says, code blocks, by line and
# content. A page that reads the same both ways is counted; one that does
# not is shown, its first difference of each kind with it.
#
#   python3 -I tests/djinterp/tools/dawk/ext/wiki/wiki_oracle.py <reader> \
#       <test_dwiki> [-show N] [-parser-tests] <page or directory>...
#
#   markdown-it   Markdown, against markdown-it-py's CommonMark with tables
#                 and strikethrough: headings, links and code. Front matter
#                 is taken away first and GitHub's :emoji: shown, as a
#                 GitHub wiki does, from pymdownx's table of them. A page
#                 that holds [[ is passed over: Gollum's links are no
#                 CommonMark's.
#   pandoc-gfm    Markdown, against pandoc's gfm with source positions:
#                 headings, and the links and images whose text is not their
#                 destination; a page that holds [[ passed over. Pandoc's
#                 own extensions, $math$ among them, are its departures from
#                 GitHub's.
#   pandoc-mediawiki
#                 MediaWiki's, against pandoc's mediawiki reader: headings,
#                 and the links and images, in any order. Pandoc departs
#                 from MediaWiki in places -- a template it leaves out, a
#                 link's trail -- so a difference here is a case to look at,
#                 not a failure by itself.
#   mediawiki     MediaWiki's, against MediaWiki itself -- 1.43, installed
#                 at MW_INSTALL_PATH, run with php through
#                 wiki_mw_records.php, beside this script, whose comment
#                 says how the wiki is set up: the sections, by level,
#                 anchor and line; the pages, templates, files and
#                 categories linked to, as MediaWiki resolves their titles;
#                 and the external links. With -parser-tests, the pages are
#                 MediaWiki's own parser tests besides, each test's wikitext
#                 a page. What the wiki itself decides is left out: what a
#                 template it does not have would have shown, a file it
#                 does not have, the pages a function reads.
#
# Exit status: 0 when every page read the same both ways, 1 when one did
# not, 2 when the other reader is not installed.
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from collections import Counter


def pages(paths, extensions):
    """The pages named, and those beneath the directories named, sorted."""
    found = []
    for path in paths:
        if os.path.isdir(path):
            for root, dirs, files in os.walk(path):
                dirs[:] = sorted(d for d in dirs if not d.startswith('.'))
                found.extend(os.path.join(root, name) for name in sorted(files)
                             if name.lower().endswith(extensions))
        else:
            found.append(path)
    return found


def has_gollum(path):
    """Whether a page holds [[, Gollum's links, no other reader's."""
    try:
        return '[[' in open(path, encoding='utf-8').read()
    except (OSError, UnicodeDecodeError):
        return True


def dump(reader, path, dialect):
    """test_dwiki's records of a page, a dict to a record; None when the
    page does not read."""
    out = subprocess.run([reader, '--dump', path, dialect],
                         capture_output=True, text=True)
    if out.returncode != 0:
        return None
    return [json.loads(line) for line in out.stdout.splitlines()]


# -- markdown-it ---------------------------------------------------------------

def markdown_it_reader():
    """markdown-it-py set as GitHub's wiki reads, and the emoji it shows."""
    from markdown_it import MarkdownIt
    from pymdownx import gemoji_db
    emoji = {}
    for key, entry in gemoji_db.emoji.items():
        code = entry.get('unicode_alt') or entry.get('unicode') or ''
        emoji[key.strip(':')] = ''.join(chr(int(part, 16))
                                        for part in code.split('-') if part)
    for alias, key in gemoji_db.aliases.items():
        if key in gemoji_db.emoji:
            emoji[alias.strip(':')] = emoji[key.strip(':')]
    return MarkdownIt('commonmark').enable(['table', 'strikethrough']), emoji


def without_front_matter(text):
    """A page's text with its YAML front matter blank, lines kept."""
    lines = text.split('\n')
    if len(lines) > 1 and lines[0].rstrip(' \t') == '---':
        for at in range(1, len(lines)):
            if lines[at].rstrip(' \t') in ('---', '...'):
                return '\n' * (at + 1) + '\n'.join(lines[at + 1:])
    return text


def markdown_it_records(md, emoji, text):
    """markdown-it's headings, links and code of a page."""
    def shown(children):
        out = []
        for token in children:
            if token.type == 'text':
                out.append(re.sub(r':([A-Za-z0-9_+-]+):',
                                  lambda m: emoji.get(m.group(1), m.group(0)),
                                  token.content))
            elif token.type == 'code_inline':
                out.append(token.content)
            elif token.type in ('softbreak', 'hardbreak'):
                out.append('\n')
        return ''.join(out)

    def flat(tokens):
        for token in tokens:
            yield token
            if token.children:
                yield from flat(token.children)

    tokens = md.parse(text)
    heads, links, codes = [], [], []
    for at, token in enumerate(tokens):
        if token.type == 'heading_open':
            heads.append((int(token.tag[1]), token.map[0] + 1,
                          shown(tokens[at + 1].children)))
        elif token.type in ('fence', 'code_block'):
            content = token.content
            codes.append((token.map[0] + 1, content[:-1]
                          if content.endswith('\n') else content))
    for token in flat(tokens):
        if token.type == 'link_open':
            links.append(('link', token.attrs.get('href', '')))
        elif token.type == 'image':
            links.append(('image', token.attrs.get('src', '')))
    return heads, links, codes


def compare_markdown_it(reader, paths, show):
    """Each page against markdown-it's reading."""
    try:
        md, emoji = markdown_it_reader()
    except ImportError:
        print('markdown-it-py and pymdownx are not installed')
        return 2
    counts = Counter()
    for path in pages(paths, ('.md', '.markdown')):
        if has_gollum(path):
            counts['passed over'] += 1
            continue
        text = open(path, encoding='utf-8').read()
        records = dump(reader, path, 'markdown')
        if records is None:
            counts['not read'] += 1
            print('NOT READ', path)
            continue
        counts['pages'] += 1
        theirs = markdown_it_records(md, emoji,
                                     without_front_matter(
                                         text.replace('\r\n', '\n')))
        heads = [(r['level'], r['line'], r['title']) for r in records
                 if r['t'] == 'heading']
        links = [('image' if r['embedded'] else 'link',
                  md.normalizeLink(r['target'])) for r in records
                 if r['t'] == 'link' and
                 r['form'] in ('inline', 'reference', 'auto')]
        codes = [(r['line'], r['content']) for r in records
                 if r['t'] == 'block' and r['kind'] == 'code']
        report(path, (heads, links, codes), theirs,
               ('headings', 'links', 'code'), counts, show)
    return summary(counts)


# -- pandoc --------------------------------------------------------------------

def pandoc(path, reader):
    """Pandoc's JSON of a page; None when pandoc fails, and an error when it
    is not there."""
    out = subprocess.run(['pandoc', '-f', reader, '-t', 'json', path],
                         capture_output=True, text=True)
    return json.loads(out.stdout)['blocks'] if out.returncode == 0 else None


def inline_text(inlines, out, breaks):
    """The text pandoc's inlines show."""
    for node in inlines:
        kind, content = node['t'], node.get('c')
        if kind == 'Str':
            out.append(content)
        elif kind == 'Space':
            out.append(' ')
        elif kind in ('SoftBreak', 'LineBreak'):
            out.append(breaks)
        elif kind == 'Code':
            out.append(content[1])
        elif kind in ('Emph', 'Strong', 'Strikeout', 'Underline', 'SmallCaps',
                      'Superscript', 'Subscript'):
            inline_text(content, out, breaks)
        elif kind in ('Span', 'Link'):
            inline_text(content[1], out, breaks)
        elif kind == 'Quoted':
            quote = '"' if content[0]['t'] == 'DoubleQuote' else "'"
            out.append(quote)
            inline_text(content[1], out, breaks)
            out.append(quote)
    return ''.join(out)


def source_line(attr):
    """The line pandoc's sourcepos gives a node."""
    for key, value in attr[2]:
        if key == 'data-pos':
            return int(value.split(';')[0].rsplit('@', 1)[-1].split(':')[0])
    return None


def shows_itself(shown, url):
    """Whether a link's text is its destination, as a bare link's is."""
    return shown in (url, url[7:] if url.startswith(('mailto:', 'http://'))
                     else url)


def pandoc_records(node, heads, links, gfm):
    """Pandoc's headings and links of a page, walked."""
    if isinstance(node, dict):
        kind, content = node.get('t'), node.get('c')
        if kind == 'Header':
            title = inline_text(content[2], [], '\n' if gfm else ' ')
            heads.append((content[0], source_line(content[1]), title) if gfm
                         else (content[0], ' '.join(title.split())))
        elif kind == 'Link':
            url = content[2][0]
            if not gfm:
                links.append(('link', url))
            elif not shows_itself(inline_text(content[1], [], '\n'), url):
                links.append(('link', url, source_line(content[0])))
        elif kind == 'Image':
            links.append(('image', content[2][0], source_line(content[0]))
                         if gfm else ('image', content[2][0]))
        for value in node.values():
            pandoc_records(value, heads, links, gfm)
    elif isinstance(node, list):
        for value in node:
            pandoc_records(value, heads, links, gfm)


def compare_pandoc(reader, paths, show, gfm):
    """Each page against pandoc's reading."""
    counts = Counter()
    extensions = ('.md', '.markdown') if gfm else ('.mediawiki', '.wiki',
                                                     '.wikitext')
    for path in pages(paths, extensions):
        if gfm and has_gollum(path):
            counts['passed over'] += 1
            continue
        try:
            blocks = pandoc(path, 'gfm+sourcepos' if gfm else 'mediawiki')
        except FileNotFoundError:
            print('pandoc is not installed')
            return 2
        if blocks is None:
            counts['pandoc failed'] += 1
            continue
        records = dump(reader, path, 'markdown' if gfm else 'mediawiki')
        if records is None:
            counts['not read'] += 1
            print('NOT READ', path)
            continue
        counts['pages'] += 1
        heads, links = [], []
        pandoc_records(blocks, heads, links, gfm)
        ours = ours_for_pandoc(records, gfm)
        if gfm:
            theirs = ([(level, ' '.join(title.split()))
                       for level, _, title in heads], links)
            ours = ([(level, ' '.join(title.split()))
                     for level, _, title in ours[0]], ours[1])
        else:
            theirs = (heads, sorted(links))
            ours = (ours[0], sorted(ours[1]))
        report(path, ours, theirs, ('headings', 'links'), counts, show)
    return summary(counts)


def ours_for_pandoc(records, gfm):
    """The reader's headings and links, as pandoc's are compared."""
    heads, links = [], []
    for r in records:
        if r['t'] == 'heading':
            heads.append((r['level'], r['line'], r['title']) if gfm
                         else (r['level'], ' '.join(r['title'].split())))
        elif r['t'] != 'link':
            continue
        elif gfm:
            if r['form'] in ('inline', 'reference') and \
                    (r['embedded'] or not shows_itself(r['label'],
                                                       r['target'])):
                links.append(('image' if r['embedded'] else 'link',
                              r['target'], r['line']))
        elif r['to'] == 'image':
            page = r['page']
            links.append(('image', page.split(':', 1)[1] if ':' in page
                          else page))
        else:
            links.append(('link', r['target'].replace(' ', '_')
                          if r['form'] == 'wiki' else r['target']))
    return heads, links


# -- MediaWiki -----------------------------------------------------------------

RECORDS = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       'wiki_mw_records.php')

# Parser tests whose difference is known: what the reader does not do, or
# what the wiki holds of its own.
KNOWN = {
    'T21451_Links_should_refer_to_the_normalized_form':
        'titles are not put in Unicode NFC',
    'Template_from_main_namespace':
        "the wiki's own Main Page is transcluded",
    'Various_link_types_in_alt_and_link_options':
        'a link= of [[Media:...]] links to the file page',
}

# MediaWiki's functions that make text of their arguments: a link within
# one is none.
TEXTUAL = {'anchorencode', 'urlencode', 'fullurl', 'fullurle', 'localurl',
           'localurle', 'canonicalurl', 'canonicalurle', 'filepath', 'ns',
           'nse', '#special', '#speciale', 'int', '#language', '#bcp47',
           '#dir', 'pagesincategory', 'pagesincat', 'pagesize', 'pageid'}


def mediawiki_php(args, stdin=None):
    """The lines wiki_mw_records.php prints, run in the MediaWiki at
    MW_INSTALL_PATH."""
    root = os.environ['MW_INSTALL_PATH']
    out = subprocess.run(['php', os.path.join(root, 'maintenance', 'run.php'),
                          RECORDS] + args, input=stdin, capture_output=True,
                         text=True, env=dict(os.environ))
    return out.stdout.splitlines()


def parser_tests(root, into):
    """MediaWiki's parser tests, the wikitext of each a page in `into`:
    <file>-<number>-<name>.wiki."""
    count = 0
    for name in sorted(os.listdir(os.path.join(root, 'tests', 'parser'))):
        if not name.endswith('.txt'):
            continue
        lines = open(os.path.join(root, 'tests', 'parser', name),
                     encoding='utf-8').read().split('\n')
        at = 0
        while at < len(lines):
            if lines[at].strip().lower() != '!! test':
                at += 1
                continue
            title, sections, current, at = lines[at + 1], {}, None, at + 2
            while at < len(lines) and lines[at].strip().lower() != '!! end':
                match = re.match(r'^!!\s*(\S+)', lines[at])
                if match:
                    current = match.group(1).lower()
                    sections[current] = []
                elif current is not None:
                    sections[current].append(lines[at])
                at += 1
            text = '\n'.join(sections.get('wikitext', []))
            if text.strip():
                count += 1
                page = '%s-%04d-%s.wiki' % (name[:-4], count,
                                           re.sub(r'[^A-Za-z0-9]+', '_',
                                                  title)[:50])
                with open(os.path.join(into, page), 'w',
                          encoding='utf-8') as out:
                    out.write(text + '\n')
    return into


def resolve(asked, urls):
    """Each title asked, [text, namespace], as MediaWiki resolves it, and
    each URL as it records a link to it."""
    keys = sorted(asked)
    lines = mediawiki_php(['--titles'], ''.join(
        json.dumps({'text': text, 'ns': space}) + '\n'
        for text, space in keys))
    titles = {key: (tuple(json.loads(line)) if json.loads(line) else None)
              for key, line in zip(keys, lines)}
    keys = sorted(urls)
    lines = mediawiki_php(['--titles'], ''.join(json.dumps({'url': url}) +
                                                '\n' for url in keys))
    return titles, {key: json.loads(line) for key, line in zip(keys, lines)}


def opaque(record, titles, functions):
    """Whether MediaWiki leaves a template's arguments unexpanded here: a
    template it does not have -- but not one whose name is no title, which
    it shows as text -- or a function none of its own."""
    if record['kind'] == 0:
        return '{{' in record['name'] or \
            titles.get((record['name'], 10)) is not None
    if record['kind'] == 1:
        return record['name'] not in functions and \
            record['name'].lower() not in functions
    return False


def within_unexpanded(records, record, titles, functions):
    """Whether a record is in the arguments of a template MediaWiki does
    not expand -- or, for a link, of a function that makes text of them."""
    templates = {r['index']: r for r in records if r['t'] == 'template'}
    parent = record.get('parent', -1)
    while parent != -1:
        held = templates[parent]
        if (opaque(held, titles, functions) or
                (record['t'] == 'link' and held['kind'] == 1 and
                 held['name'].lower() in TEXTUAL)) and \
                record['start'] >= held['body']:
            return True
        parent = held['parent']
    return False


def mediawiki_asked(records):
    """The titles a page's records name, [text, namespace], and its URLs."""
    asked, urls = set(), set()
    for r in records:
        if r['t'] == 'link' and r['form'] in ('wiki', 'option') and \
                r['to'] in ('page', 'category', 'image', 'file'):
            asked.add((r['page'], 0))
        elif r['t'] == 'link' and r['form'] == 'gallery':
            asked.add((r['page'], 6))
        elif r['t'] == 'link' and r['to'] in ('url', 'mail'):
            urls.add(r['target'])
        elif r['t'] == 'template' and r['kind'] == 0:
            asked.add((r['name'], 10))
        elif r['t'] == 'template' and r['kind'] == 1 and r['args']:
            asked.add((r['args'][0]['value'].strip(), 0))
    return asked, urls


def mediawiki_ours(records, text, titles, urls, functions):
    """The reader's records of a page as MediaWiki's are compared, with the
    lines whose headings the wiki decides, and the titles either may have
    that it decides."""
    ours = {kind: set() for kind in ('pages', 'templates', 'files',
                                     'categories', 'external')}
    unstable, either, read = set(), set(), set()
    for r in records:
        if r['t'] == 'template' and r['parent'] == -1 and \
                opaque(r, titles, functions):
            unstable.add(r['line'])
        elif r['t'] == 'link' and r['to'] == 'image':
            unstable.add(r['line'])
        if r['t'] == 'template' and r['kind'] == 1 and r['args'] and \
                titles.get((r['args'][0]['value'].strip(), 0)):
            read.add(titles[(r['args'][0]['value'].strip(), 0)])
        if r['t'] == 'link' and r['form'] == 'wiki' and \
                titles.get((r['page'], 0)) and \
                any(t['t'] == 'template' and opaque(t, titles, functions) and
                    r['start'] <= t['start'] and t['end'] <= r['end']
                    for t in records):
            either.update((titles[(r['page'], 0)],
                           titles[(r['page'], 0)][1]))
    sections = []
    for r in records:
        if r['t'] in ('link', 'template') and \
                within_unexpanded(records, r, titles, functions):
            continue
        if r['t'] == 'heading' and r['top'] and r['line'] not in unstable:
            sections.append((r['level'], r['anchor'], r['line']))
        elif r['t'] == 'template' and r['kind'] == 0 and \
                titles.get((r['name'], 10)) and \
                titles[(r['name'], 10)][0] != -1:
            ours['templates'].add(titles[(r['name'], 10)])
        elif r['t'] != 'link':
            continue
        elif r['form'] == 'gallery':
            title = titles.get((r['page'], 6))
            if title and title[0] == 6:
                ours['files'].add(title[1])
        elif r['to'] in ('url', 'mail'):
            ours['external'].add(urls.get(r['target'], r['target']))
        elif r['form'] in ('wiki', 'option') and titles.get((r['page'], 0)):
            title = titles[(r['page'], 0)]
            if r['to'] == 'page':
                ours['pages'].add(title)
            elif r['to'] == 'category':
                ours['categories'].add(title[1])
            elif r['to'] in ('image', 'file'):
                ours['files'].add(title[1])
    return ours, sections, unstable, either, read


def mediawiki_theirs(found, text, unstable, read, named):
    """MediaWiki's records of a page, as the reader's are compared: what it
    links to as a template's page left out of the pages."""
    def line(offset):
        return text[:offset].count('\n') + 1
    templates = {tuple(x) for x in found['templates']} - read - \
        {(0, 'Oracle_test', '')}
    pages = {tuple(x) for x in found['links'] + found['interwiki'] +
             found['language'] + found['special']}
    return {
        'pages': pages - templates - named - read,
        'templates': templates,
        'files': {x[1] for x in found['images']},
        'categories': {x[1] for x in found['categories']},
        'external': set(found['external']),
    }, [(s['level'], s['anchor'], line(s['offset']))
        for s in found['sections']
        if isinstance(s['offset'], int) and line(s['offset']) not in unstable]


def compare_mediawiki(reader, paths, show, tests):
    """Each page against MediaWiki's own reading."""
    if not os.environ.get('MW_INSTALL_PATH') or not shutil.which('php'):
        print('MediaWiki is not installed: set MW_INSTALL_PATH, and php')
        return 2
    temporary = tempfile.mkdtemp() if tests else None
    if tests:
        paths = list(paths) + [parser_tests(os.environ['MW_INSTALL_PATH'],
                                            temporary)]
    try:
        return mediawiki_pages(reader, pages(paths, ('.mediawiki', '.wiki',
                                                     '.wikitext')), show)
    finally:
        if temporary:
            shutil.rmtree(temporary)


def mediawiki_pages(reader, found, show):
    """The pages found, each against MediaWiki's reading."""
    magic = json.loads(mediawiki_php(['--magic'])[0])
    functions = {name.lower() for name in magic['insensitive']} | \
        set(magic['sensitive'])
    theirs = {}
    for at in range(0, len(found), 200):
        for line in mediawiki_php(found[at:at + 200]):
            record = json.loads(line)
            theirs[record['file']] = record
    records = {path: dump(reader, path, 'mediawiki') for path in found}
    asked, urls = set(), set()
    for path in found:
        more = mediawiki_asked(records[path] or [])
        asked |= more[0]
        urls |= more[1]
    titles, links = resolve(asked, urls)
    counts = Counter()
    for path in found:
        if any(key in os.path.basename(path) for key in KNOWN):
            counts['known'] += 1
            continue
        if records[path] is None or 'error' in theirs.get(path, {'error': 1}):
            counts['not read'] += 1
            print('NOT READ', path)
            continue
        counts['pages'] += 1
        text = open(path, 'rb').read().decode('utf-8', 'replace')
        ours, sections, unstable, either, read = mediawiki_ours(
            records[path], text, titles, links, functions)
        named = {titles[(r['name'], 10)] for r in records[path]
                 if r['t'] == 'template' and r['kind'] == 0 and
                 titles.get((r['name'], 10))}
        mine, their_sections = mediawiki_theirs(theirs[path], text, unstable,
                                                read, named)
        for kind in ('pages', 'categories'):
            ours[kind] -= either
            mine[kind] -= either
        # a heading the wiki decides numbers those after it its own way
        if unstable & {r['line'] for r in records[path]
                       if r['t'] == 'heading'}:
            sections = [(level, re.sub(r'_\d+$', '', anchor), line)
                        for level, anchor, line in sections]
            their_sections = [(level, re.sub(r'_\d+$', '', anchor), line)
                              for level, anchor, line in their_sections]
        kinds = ('pages', 'templates', 'files', 'categories', 'external')
        report(path, [sections] + [sorted(ours[k]) for k in kinds],
               [their_sections] + [sorted(mine[k]) for k in kinds],
               ('headings', 'page links') + kinds[1:], counts, show)
    return summary(counts)


# -- reporting -----------------------------------------------------------------

def report(path, ours, theirs, kinds, counts, show):
    """A page counted the same, or shown with its first differences."""
    different = [kind for kind, a, b in zip(kinds, ours, theirs) if a != b]
    if not different:
        counts['same'] += 1
        return
    for kind in different:
        counts[kind] += 1
    if counts['shown'] >= show:
        return
    counts['shown'] += 1
    print('DIFFERENT', path, ', '.join(different))
    for kind, a, b in zip(kinds, ours, theirs):
        if a == b:
            continue
        for at in range(max(len(a), len(b))):
            x = a[at] if at < len(a) else None
            y = b[at] if at < len(b) else None
            if x != y:
                print('    %s %d, ours:   %s' % (kind, at, repr(x)[:200]))
                print('    %s %d, theirs: %s' % (kind, at, repr(y)[:200]))
                break


def summary(counts):
    """The counts, and the status they make."""
    shown = ', '.join('%d %s' % (counts[key], key) for key in
                      ('same', 'headings', 'links', 'code', 'page links',
                       'templates', 'files', 'categories', 'external',
                       'passed over', 'pandoc failed', 'not read', 'known')
                      if counts[key])
    print('%d pages: %s' % (counts['pages'], shown or 'none'))
    return 0 if counts['same'] == counts['pages'] and \
        not counts['not read'] else 1


def main():
    args = sys.argv[1:]
    show = 5
    if '-show' in args:
        at = args.index('-show')
        show = int(args[at + 1])
        del args[at:at + 2]
    tests = '-parser-tests' in args
    if tests:
        args.remove('-parser-tests')
    if len(args) < (2 if tests else 3) or \
            args[0] not in ('markdown-it', 'pandoc-gfm', 'pandoc-mediawiki',
                            'mediawiki'):
        print('usage: wiki_oracle.py markdown-it|pandoc-gfm|pandoc-mediawiki|'
              'mediawiki <test_dwiki> [-show N] [-parser-tests] '
              '<page or directory>...')
        return 2
    if args[0] == 'markdown-it':
        return compare_markdown_it(args[1], args[2:], show)
    if args[0] == 'mediawiki':
        return compare_mediawiki(args[1], args[2:], show, tests)
    return compare_pandoc(args[1], args[2:], show, args[0] == 'pandoc-gfm')


sys.exit(main())
