/*******************************************************************************
* vparse [vparse]                                                      store.cpp
*
* Run folders on disk.
*   Everything goes through std::filesystem's error_code overloads and
* checked streams, so a bad path or a full disk becomes an error message, not
* an exception. The grammar and input are written in binary mode so the bytes
* that were run are the bytes that are saved, line endings included.
*   A path that becomes text -- a run's origin, its session's name, a message
* -- becomes UTF-8 (utf8_from_path): path::string() is the system's code page
* on Windows, which cannot spell every name, and throws where it cannot.
*
* path:      /src/djinterp/tools/vparse/store.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.22
*                                                            revised: 2026.10.17
*******************************************************************************/
#include "../../../../inc/djinterp/tools/vparse/store.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the header has
// std
#include <algorithm>     // std::max, std::sort
#include <cerrno>        // errno
#include <cstdio>        // std::snprintf
#include <cstdlib>       // std::atoi, std::atol
#include <ctime>         // std::time, std::localtime, std::strftime
#include <fstream>       // std::ifstream, std::ofstream
#include <iterator>      // std::istreambuf_iterator
#include <map>           // std::map
#include <sstream>       // std::ostringstream
#include <system_error>  // std::error_code, std::errc,
                         // std::generic_category
#include <utility>       // std::move
// vparse
#include "../../../../inc/djinterp/tools/vparse/heat.hpp"  // heat_svg


namespace vparse
{
namespace workbench
{

namespace
{

namespace fs = std::filesystem;

// write_file
//   writes _text to _path in binary mode; false on any failure.
bool write_file(
    const fs::path&    _path,
    const std::string& _text
)
{
    std::ofstream out(_path, std::ios::binary | std::ios::trunc);

    if (!out)
    {
        return false;
    }

    out.write(_text.data(), static_cast<std::streamsize>(_text.size()));

    return static_cast<bool>(out);
}

// read_file
//   the whole of _path in binary mode; false if it cannot be read.
bool read_file(
    const fs::path& _path,
    std::string&    _text
)
{
    return read_whole_file(_path,
                           _text,
                           nullptr);
}

// read_fields
//   run.txt's "key: value" lines as a map.
std::map<std::string, std::string> read_fields(
    const fs::path& _path
)
{
    std::map<std::string, std::string> fields;
    std::string                        text;

    if (!read_file(_path, text))
    {
        return fields;
    }

    std::istringstream lines(text);
    std::string        line;

    while (std::getline(lines, line))
    {
        const std::size_t colon = line.find(": ");

        // lines without a key are prose
        if (colon == std::string::npos)
        {
            continue;
        }

        fields[line.substr(0, colon)] = line.substr(colon + 2);
    }

    return fields;
}

// run_folder_name
//   "run-007".
std::string run_folder_name(
    int _id
)
{
    char name[32];
    std::snprintf(name, sizeof(name), "run-%03d", _id);

    return name;
}

// run_fields
//   run.txt for _run.
std::string run_fields(
    const run_record& _run
)
{
    std::ostringstream out;

    out << "vparse workbench run\n"
        << "id: " << _run.id << "\n"
        << "when: " << _run.when << "\n"
        << "frontend: " << _run.request.frontend << "\n"
        << "trace: " << (_run.request.trace ? "on" : "off") << "\n"
        << "step_limit: " << _run.request.step_limit << "\n"
        << "memoize: " << _run.request.memoize << "\n"
        << "grammar_bytes: " << _run.request.grammar.size() << "\n"
        << "input_bytes: " << _run.request.input.size() << "\n"
        << "result: " << headline(_run) << "\n";

    return out.str();
}

// update_index
//   rewrites the session's index.tsv with _run's line added or replaced,
// ordered by run id.
bool update_index(
    const fs::path&   _session,
    const run_record& _run,
    const fs::path&   _folder
)
{
    const fs::path             index = _session / "index.tsv";
    std::map<int, std::string> lines;
    std::string                text;

    // keep every other run's line
    if (read_file(index, text))
    {
        std::istringstream in(text);
        std::string        line;

        while (std::getline(in, line))
        {
            if ( (line.empty()) ||
                 (line.rfind("id\t", 0) == 0) )
            {
                continue;
            }

            lines[std::atoi(line.c_str())] = line;
        }
    }

    lines[_run.id] = std::to_string(_run.id) + "\t" + _run.when + "\t" +
                     _run.request.frontend + "\t" +
                     utf8_from_path(_folder.filename()) + "\t" +
                     headline(_run);

    std::string out = "id\twhen\tfrontend\tfolder\tresult\n";

    for (const auto& entry : lines)
    {
        out += entry.second + "\n";
    }

    return write_file(index, out);
}

/**
 * @brief Says why a path names nothing whose bytes can be read: nothing by
 *        that name, a name the system cannot look up, or a folder.
 *
 * @param[in] _path  the path.
 * @return the reason, in the system's words; empty when the path names
 *         something to try to open.
 */
std::string unreadable(
    const fs::path& _path
)
{
    std::error_code       code;
    const fs::file_status status = fs::status(_path,
                                              code);

    // nothing by that name
    if (status.type() == fs::file_type::not_found)
    {
        return std::make_error_code(
                   std::errc::no_such_file_or_directory).message();
    }

    // a name the system cannot look up
    if (code)
    {
        return code.message();
    }

    // a folder has no bytes of its own to read
    if (fs::is_directory(status))
    {
        return std::make_error_code(std::errc::is_a_directory).message();
    }

    return std::string();
}

/**
 * @brief Gives a caller that asked the reason a file was not read.
 *
 * @param[out] _error   receives `_reason`; may be NULL, for a caller that
 *                      did not ask.
 * @param[in]  _reason  the reason.
 * @return `false`, for the reader to return.
 */
bool not_read(
    std::string*       _error,
    const std::string& _reason
)
{
    // a caller that asked
    if (_error != nullptr)
    {
        *_error = _reason;
    }

    return false;
}

}  // anonymous namespace


/*
session_name
  Timestamped so sessions sort chronologically by name.
*/
std::string session_name()
{
    const std::time_t now = std::time(nullptr);
    char              name[40];

    std::strftime(name, sizeof(name), "session-%Y%m%d-%H%M%S",
                  std::localtime(&now));

    return name;
}

/*
save_run
  The folder is created first; every file after it is written whole, so a
failure part-way leaves earlier files intact and reports which file failed.
*/
std::filesystem::path save_run(
    const run_record&            _run,
    const std::filesystem::path& _session,
    std::string*                 _error
)
{
    const fs::path  folder = _session / run_folder_name(_run.id);
    std::error_code code;

    fs::create_directories(folder, code);

    if (code)
    {
        if (_error)
        {
            *_error = "could not create " + utf8_from_path(folder) + ": " +
                      code.message();
        }

        return {};
    }

    std::vector<std::pair<std::string, std::string>> files = {
        { "run.txt",     run_fields(_run)     },
        { "grammar.txt", _run.request.grammar },
        { "input.txt",   _run.request.input   } };

    // the grammar part is grammar.txt with line numbers; the raw file wins
    for (const std::string& part : part_names())
    {
        if (part == "grammar")
        {
            continue;
        }

        files.push_back({ part + ".txt",
                          listing_text(part_listing(_run, part)) });
    }

    // the heat map as a picture, when the run has one
    if (_run.heat)
    {
        files.push_back({ "heat.svg", heat_svg(*_run.heat, _run) });
    }

    for (const auto& file : files)
    {
        if (!write_file(folder / file.first, file.second))
        {
            if (_error)
            {
                *_error = "could not write " +
                          utf8_from_path(folder / file.first);
            }

            return {};
        }
    }

    // the index is a convenience; the run folder is the record
    if ( (!update_index(_session, _run, folder)) &&
         (_error) )
    {
        *_error = "saved, but could not update " +
                  utf8_from_path(_session / "index.tsv");
    }

    return folder;
}

/*
list_saved
  Sessions and runs both sort by name, which is chronological by construction;
reversing puts the newest first.
*/
std::vector<saved_run> list_saved(
    const std::filesystem::path& _root
)
{
    std::vector<saved_run> found;
    std::error_code        code;

    if (!fs::is_directory(_root, code))
    {
        return found;
    }

    for (fs::recursive_directory_iterator it(_root, code), end;
         (!code) && (it != end);
         it.increment(code))
    {
        // runs are at most two levels down: session / run
        if (it.depth() > 1)
        {
            it.disable_recursion_pending();
        }

        const fs::path marker = it->path() / "run.txt";

        if ( (!it->is_directory(code)) ||
             (!fs::exists(marker, code)) )
        {
            continue;
        }

        const std::map<std::string, std::string> fields = read_fields(marker);
        const auto get = [&fields](const std::string& _key)
        {
            const auto hit = fields.find(_key);

            return (hit == fields.end()) ? std::string() : hit->second;
        };

        saved_run run;
        run.folder   = it->path();
        run.session  = utf8_from_path(it->path().parent_path().filename());
        run.id       = std::atoi(get("id").c_str());
        run.frontend = get("frontend");
        run.when     = get("when");
        run.result   = get("result");
        found.push_back(run);
    }

    std::sort(found.begin(), found.end(),
              [](const saved_run& _a, const saved_run& _b)
              {
                  return _a.folder > _b.folder;
              });

    return found;
}

/*
load_request
  Only the three source files are read; the settings come from run.txt, with
the defaults of a fresh request for anything missing.
*/
bool load_request(
    const std::filesystem::path& _folder,
    run_request&                 _request,
    std::string*                 _error
)
{
    const std::map<std::string, std::string> fields =
        read_fields(_folder / "run.txt");

    run_request request;

    if ( (fields.empty()) ||
         (!read_file(_folder / "grammar.txt", request.grammar)) ||
         (!read_file(_folder / "input.txt", request.input)) )
    {
        if (_error)
        {
            *_error = utf8_from_path(_folder) + " is not a saved run";
        }

        return false;
    }

    const auto get = [&fields](const std::string& _key,
                               const std::string& _fallback)
    {
        const auto hit = fields.find(_key);

        return (hit == fields.end()) ? _fallback : hit->second;
    };

    request.frontend   = (get("frontend", "notation") == "ebnf") ? "ebnf"
                                                                  : "notation";
    request.trace      = (get("trace", "on") != "off");
    request.step_limit = std::max(1L, std::atol(get("step_limit",
                                                    "100000").c_str()));
    request.memoize    = get("memoize",
                             "none");
    request.origin     = utf8_from_path(_folder);

    _request = request;

    return true;
}

/*
listing_text
  Columns are padded to their widest cell, capped at 48 characters so one long
set does not push every other row off the page; wider cells simply overflow.
*/
std::string listing_text(
    const listing& _listing
)
{
    if (_listing.rows.empty())
    {
        return _listing.note.empty() ? std::string("(empty)\n")
                                     : _listing.note + "\n";
    }

    const std::size_t        columns = _listing.columns.size();
    std::vector<std::size_t> width(columns, 0);

    for (std::size_t c = 0; c < columns; ++c)
    {
        width[c] = std::min<std::size_t>(48, _listing.columns[c].size());
    }

    for (const std::vector<std::string>& row : _listing.rows)
    {
        for (std::size_t c = 0; ( (c < columns) && (c < row.size()) ); ++c)
        {
            width[c] = std::max(width[c],
                                std::min<std::size_t>(48, row[c].size()));
        }
    }

    const auto line = [&](const std::vector<std::string>& _cells)
    {
        std::string out;

        for (std::size_t c = 0; c < columns; ++c)
        {
            const std::string cell = (c < _cells.size()) ? _cells[c] : "";

            out += cell;

            if ((c + 1) < columns)
            {
                const std::size_t pad =
                    (cell.size() < width[c]) ? (width[c] - cell.size()) : 0;
                out += std::string(pad + 2, ' ');
            }
        }

        while ( (!out.empty()) &&
                (out.back() == ' ') )
        {
            out.pop_back();
        }

        return out + "\n";
    };

    std::string out = line(_listing.columns);
    std::string rule;

    for (std::size_t c = 0; c < columns; ++c)
    {
        rule += std::string(width[c], '-') + (((c + 1) < columns) ? "  " : "");
    }

    out += rule + "\n";

    for (const std::vector<std::string>& row : _listing.rows)
    {
        out += line(row);
    }

    return out;
}

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

/*
path_from_utf8
  From C++20 a path is made from UTF-8 as a std::u8string; the characters are
copied across one for one, since char8_t may not alias a char's storage.
*/
std::filesystem::path path_from_utf8(
    const std::string& _name
)
{
    return std::filesystem::path(std::u8string(_name.begin(),
                                               _name.end()));
}

/*
utf8_from_path
  From C++20 a path's UTF-8 name is a std::u8string, copied back into a
std::string one character for one.
*/
std::string utf8_from_path(
    const std::filesystem::path& _path
)
{
    const std::u8string name = _path.u8string();

    return std::string(name.begin(),
                       name.end());
}

#else

/*
path_from_utf8
  C++17's own conversion, which C++20 deprecates for the one above.
*/
std::filesystem::path path_from_utf8(
    const std::string& _name
)
{
    return std::filesystem::u8path(_name);
}

/*
utf8_from_path
  C++17's u8string() is a std::string holding UTF-8.
*/
std::string utf8_from_path(
    const std::filesystem::path& _path
)
{
    return _path.u8string();
}

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

/*
read_whole_file
  The file's status is asked first, so that a name that names nothing, or a
directory, is told apart from a file that cannot be opened, in the system's
own words: a stream that fails to open says nothing about why, though the C
library it opens through leaves its reason in errno, which is read when it
is set. Reading stops at the end of the file, and a stream left bad by it is
a failure; the bytes are handed over only once all of them are read.
*/
bool read_whole_file(
    const std::filesystem::path& _path,
    std::string&                 _bytes,
    std::string*                 _error
)
{
    const std::string unfit = unreadable(_path);

    // nothing there to open
    if (!unfit.empty())
    {
        return not_read(_error,
                        unfit);
    }

    errno = 0;

    std::ifstream in(_path,
                     std::ios::binary);

    // there, but not to be opened: the C library's reason, where it gave one
    if (!in)
    {
        return not_read(_error,
                        (errno != 0)
                            ? std::generic_category().message(errno)
                            : std::string("the file could not be opened"));
    }

    std::string bytes((std::istreambuf_iterator<char>(in)),
                      std::istreambuf_iterator<char>());

    // opened, and then failed part-way
    if (in.bad())
    {
        return not_read(_error,
                        std::make_error_code(std::errc::io_error).message());
    }

    _bytes = std::move(bytes);

    return true;
}

}  // namespace workbench
}  // namespace vparse

#endif  // floor, for now
