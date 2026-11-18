/*******************************************************************************
* djinterp [c]                                                     log_writers.c
*
*   The log's three writers, and its times. Every value goes through the
* stream's writer of values, so JSON is escaped by its one escaper; a
* record's facts are given by desc's replay to a machine: desc's text
* machine's line for text, desc's JSON machine (or a dotting wrapper) for
* JSON lines, and an attribute machine of OTLP's own for OTLP/JSON.
*
*   OTLP/JSON follows the specification's JSON encoding: field names in
* lowerCamelCase, a 64-bit integer as a decimal string, an enum as its
* number, a double's NaN and infinities as protobuf's strings.
*
*
* path:      /src/djinterp/c/log/log_writers.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.17
*******************************************************************************/

#include "../../../../inc/djinterp/c/log/log.h"  // the writers' header

#if RE_STD_HAS_INT64

// std
#include <string.h>  // strlen
#include <time.h>    // localtime_r, localtime_s, time_t, struct tm
#if defined(__unix__) || defined(__APPLE__)
    #include <unistd.h>  // getpid
    #define D_INTERNAL_LOG_PID  1
#elif defined(_WIN32)
    #include <process.h>  // _getpid
    #define D_INTERNAL_LOG_PID  1
#else
    #define D_INTERNAL_LOG_PID  0
#endif


//==============================================================================
// WRITING
//==============================================================================

/*
d_internal_log_digits
  A number of a fixed width, zero-padded.
*/
static void
d_internal_log_digits(
    struct dj_ostream* _os,
    uint64_t           _number,
    uint32_t           _width
)
{
    char     digits[20];
    uint32_t at = _width;

    while (at > 0u)
    {
        digits[--at] = (char)('0' + (int)(_number % 10u));
        _number     /= 10u;
    }

    (void)d_ostream_write(_os, digits, _width);

    return;
}

/*
d_internal_log_value
  A value, raw or converted, by the stream.
*/
static void
d_internal_log_value(
    struct dj_ostream*             _os,
    const struct dj_ostream_value* _value,
    uint32_t                       _convert
)
{
    struct dj_ostream_spec spec;

    d_ostream_spec_init(&spec);
    spec.convert = (uint8_t)_convert;
    (void)d_ostream_write_value(_os, _value, &spec);

    return;
}

#if (D_CFG_LOG_FORMATS & 0x6)
/*
d_internal_log_string
  A text as a JSON string.
*/
static void
d_internal_log_string(
    struct dj_ostream* _os,
    const char*        _text,
    size_t             _size
)
{
    const struct dj_ostream_value value =
        d_ostream_value_text_n((_text != NULL) ? _text : "", _size);

    d_internal_log_value(_os, &value, (uint32_t)D_OSTREAM_CONVERT_JSON);

    return;
}

/*
d_internal_log_member
  A JSON member's key, as "key":, after a comma unless it is the first.
*/
static void
d_internal_log_member(
    struct dj_ostream* _os,
    const char*        _key,
    int                _first
)
{
    if (_first == 0)
    {
        (void)d_ostream_write_char(_os, ',');
    }

    d_internal_log_string(_os, _key, strlen(_key));
    (void)d_ostream_write_char(_os, ':');

    return;
}
#endif  // D_CFG_LOG_FORMATS & 0x6

/*
d_internal_log_code
  A code's name as a style says: "net.connect_timeout", "net:207", or both
as "net.connect_timeout(207)"; by number where names are not built.
*/
static void
d_internal_log_code(
    struct dj_ostream*        _os,
    const struct dj_log_code* _code,
    uint32_t                  _style
)
{
    const struct dj_ostream_value number = d_ostream_value_long(
                                               (long)_code->number);

    (void)d_ostream_write_cstr(_os, _code->module->name);

    if ( (_code->name == NULL) || (_style == 1u) )
    {
        (void)d_ostream_write_char(_os, ':');
        d_internal_log_value(_os, &number, (uint32_t)D_OSTREAM_CONVERT_RAW);

        return;
    }

    (void)d_ostream_write_char(_os, '.');
    (void)d_ostream_write_cstr(_os, _code->name);

    if (_style == 2u)
    {
        (void)d_ostream_write_char(_os, '(');
        d_internal_log_value(_os, &number, (uint32_t)D_OSTREAM_CONVERT_RAW);
        (void)d_ostream_write_char(_os, ')');
    }

    return;
}

#if (D_CFG_LOG_FORMATS & 0x6)
/*
d_internal_log_code_text
  A code's name as d_internal_log_code writes it, into a buffer.
*/
static const char*
d_internal_log_code_text(
    const struct dj_log_code* _code,
    uint32_t                  _style,
    char*                     _buffer,
    size_t                    _capacity
)
{
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;

    (void)d_ostream_init_fixed(&os, &fixed, _buffer, _capacity, 1);
    d_internal_log_code(&os, _code, _style);
    (void)d_ostream_close(&os);

    return _buffer;
}
#endif  // D_CFG_LOG_FORMATS & 0x6


//==============================================================================
// TIMES
//==============================================================================

/*
d_internal_log_civil
  A day's date from its number since 1970-01-01: Howard Hinnant's
civil_from_days, exact for every day of the proleptic Gregorian calendar.
*/
static void
d_internal_log_civil(
    int64_t   _days,
    int64_t*  _year,
    uint32_t* _month,
    uint32_t* _day
)
{
    const int64_t  z   = _days + 719468;
    const int64_t  era = ((z >= 0) ? z : (z - 146096)) / 146097;
    const uint64_t doe = (uint64_t)(z - era * 146097);
    const uint64_t yoe = (doe - doe / 1460u + doe / 36524u - doe / 146096u) /
                         365u;
    const uint64_t doy = doe - (365u * yoe + yoe / 4u - yoe / 100u);
    const uint64_t mp  = (5u * doy + 2u) / 153u;

    *_day   = (uint32_t)(doy - (153u * mp + 2u) / 5u + 1u);
    *_month = (uint32_t)((mp < 10u) ? (mp + 3u) : (mp - 9u));
    *_year  = (int64_t)yoe + era * 400 + ((*_month <= 2u) ? 1 : 0);

    return;
}

/*
d_internal_log_days
  A date's number of days since 1970-01-01: Hinnant's days_from_civil.
*/
static int64_t
d_internal_log_days(
    int64_t  _year,
    uint32_t _month,
    uint32_t _day
)
{
    const int64_t  y   = _year - ((_month <= 2u) ? 1 : 0);
    const int64_t  era = ((y >= 0) ? y : (y - 399)) / 400;
    const uint64_t yoe = (uint64_t)(y - era * 400);
    const uint64_t mp  = (_month > 2u) ? (_month - 3u) : (_month + 9u);
    const uint64_t doy = (153u * mp + 2u) / 5u + _day - 1u;
    const uint64_t doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;

    return era * 146097 + (int64_t)doe - 719468;
}

/*
d_internal_log_offset
  The local time's offset from UTC at a moment, in seconds: the local date
and time, as days and seconds, less the UTC ones. 0 where the C library has
no reentrant localtime.
*/
static int64_t
d_internal_log_offset(
    int64_t _seconds
)
{
    const time_t moment = (time_t)_seconds;
    struct tm    local;
    int          ok = 0;

#if defined(_WIN32)
    ok = (localtime_s(&local, &moment) == 0) ? 1 : 0;
#elif defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE) ||                   \
      defined(__unix__) || defined(__APPLE__)
    ok = (localtime_r(&moment, &local) != NULL) ? 1 : 0;
#endif

    if (ok == 0)
    {
        return 0;
    }

    return d_internal_log_days((int64_t)local.tm_year + 1900,
                               (uint32_t)local.tm_mon + 1u,
                               (uint32_t)local.tm_mday) * 86400 +
           (int64_t)local.tm_hour * 3600 + (int64_t)local.tm_min * 60 +
           (int64_t)local.tm_sec - _seconds;
}

void
d_log_write_time(
    struct dj_ostream* _os,
    uint64_t           _time,
    uint32_t           _how
)
{
    static const uint32_t DIGITS[4] = { 0u, 3u, 6u, 9u };
    const int64_t  seconds = (int64_t)(_time / 1000000000u);
    const uint64_t nanos   = _time % 1000000000u;
    const int64_t  offset  = (_how >= 4u) ? d_internal_log_offset(seconds)
                                          : 0;
    const int64_t  local   = seconds + offset;
    const int64_t  days    = ((local >= 0) ? local : (local - 86399)) / 86400;
    const int64_t  second  = local - days * 86400;
    int64_t        year;
    uint32_t       month;
    uint32_t       day;
    uint32_t       digits  = DIGITS[_how & 3u];
    uint64_t       part    = nanos;

    d_internal_log_civil(days, &year, &month, &day);
    d_internal_log_digits(_os, (uint64_t)((year >= 0) ? year : 0), 4u);
    (void)d_ostream_write_char(_os, '-');
    d_internal_log_digits(_os, month, 2u);
    (void)d_ostream_write_char(_os, '-');
    d_internal_log_digits(_os, day, 2u);
    (void)d_ostream_write_char(_os, 'T');
    d_internal_log_digits(_os, (uint64_t)(second / 3600), 2u);
    (void)d_ostream_write_char(_os, ':');
    d_internal_log_digits(_os, (uint64_t)((second / 60) % 60), 2u);
    (void)d_ostream_write_char(_os, ':');
    d_internal_log_digits(_os, (uint64_t)(second % 60), 2u);

    if (digits != 0u)
    {
        uint32_t cut;

        for (cut = digits; cut < 9u; ++cut)
        {
            part /= 10u;
        }

        (void)d_ostream_write_char(_os, '.');
        d_internal_log_digits(_os, part, digits);
    }

    if (_how < 4u)
    {
        (void)d_ostream_write_char(_os, 'Z');

        return;
    }

    (void)d_ostream_write_char(_os, (offset < 0) ? '-' : '+');
    d_internal_log_digits(_os, (uint64_t)(((offset < 0) ? -offset : offset) /
                                          3600), 2u);
    (void)d_ostream_write_char(_os, ':');
    d_internal_log_digits(_os, (uint64_t)((((offset < 0) ? -offset : offset) /
                                           60) % 60), 2u);

    return;
}


//==============================================================================
// THE TEXT WRITER
//==============================================================================

#if (D_CFG_LOG_FORMATS & 0x1)

/*
d_internal_log_text_record
  time level code: body key=value ..., one line.
*/
static int
d_internal_log_text_record(
    void*                      _context,
    struct dj_ostream*         _os,
    const struct dj_log_entry* _entry
)
{
    struct dj_desc_text_state line;

    (void)_context;
    d_log_write_time(_os, _entry->time, _entry->settings->time);
    (void)d_ostream_write_char(_os, ' ');
    (void)d_ostream_write_cstr(_os, _entry->level);
    (void)d_ostream_write_char(_os, ' ');
    d_internal_log_code(_os, _entry->code, _entry->settings->code_style);

    if (_entry->body_size != 0u)
    {
        (void)d_ostream_write(_os, ": ", 2u);
        (void)d_ostream_write(_os, _entry->body, _entry->body_size);
    }

    if (_entry->facts->count != 0u)
    {
        (void)d_ostream_write_char(_os, ' ');
        d_desc_text_init(&line, _os, D_DESC_TEXT_LINE, 0u);
        (void)d_desc_replay(_entry->facts, d_desc_text_machine(), &line);
        d_desc_text_machine()->end(&line);
    }

    if ( (_entry->settings->dropped_count != 0u) && (_entry->dropped != 0u) )
    {
        const struct dj_ostream_value dropped =
            d_ostream_value_size(_entry->dropped);

        (void)d_ostream_write(_os, " truncated=", 11u);
        d_internal_log_value(_os, &dropped, (uint32_t)D_OSTREAM_CONVERT_RAW);
    }

    (void)d_ostream_write_char(_os, '\n');

    return 0;
}

static const struct dj_log_writer TEXT_WRITER =
{
    NULL, d_internal_log_text_record, NULL, NULL
};

#endif  // D_CFG_LOG_FORMATS & 0x1

const struct dj_log_writer*
d_log_writer_text(void)
{
#if (D_CFG_LOG_FORMATS & 0x1)
    return &TEXT_WRITER;
#else
    return NULL;
#endif
}


//==============================================================================
// THE JSON LINES WRITER
//==============================================================================

#if (D_CFG_LOG_FORMATS & 0x2)

// dj_internal_log_dotted
//   struct: a machine over desc's JSON machine that writes nested facts with
// dotted keys at one level, for log.nesting=dotted.
struct dj_internal_log_dotted
{
    struct dj_desc_json_state* json;
    const char*                path[D_CFG_LOG_DEPTH_MAX + 1];
};

static int
d_internal_log_dotted_fact(
    void*                      _context,
    const struct dj_desc_fact* _fact
)
{
    struct dj_internal_log_dotted* const dotted =
        (struct dj_internal_log_dotted*)_context;
    char                    key[256];
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;
    struct dj_desc_fact     flat = *_fact;
    uint32_t                level;

    (void)d_ostream_init_fixed(&os, &fixed, key, sizeof(key), 1);

    for (level = 0u; (level < _fact->depth) &&
                     (level <= (uint32_t)D_CFG_LOG_DEPTH_MAX); ++level)
    {
        (void)d_ostream_write_cstr(&os, (dotted->path[level] != NULL)
                                            ? dotted->path[level] : "?");
        (void)d_ostream_write_char(&os, '.');
    }

    (void)d_ostream_write_cstr(&os, (_fact->key != NULL) ? _fact->key : "?");
    (void)d_ostream_close(&os);
    flat.key   = key;
    flat.depth = 0u;

    return d_desc_json_machine()->fact(dotted->json, &flat);
}

static int
d_internal_log_dotted_enter(
    void*       _context,
    const char* _key,
    const char* _label,
    uint32_t    _depth
)
{
    struct dj_internal_log_dotted* const dotted =
        (struct dj_internal_log_dotted*)_context;

    (void)_label;

    if (_depth <= (uint32_t)D_CFG_LOG_DEPTH_MAX)
    {
        dotted->path[_depth] = _key;
    }

    return 0;
}

static const struct dj_desc_machine DOTTED =
{
    NULL, d_internal_log_dotted_fact, d_internal_log_dotted_enter, NULL,
    NULL, NULL
};

/*
d_internal_log_json_attributes
  "attributes":{"code":...,"module":...,facts...}: the facts by desc's JSON
machine, bare, nested or dotted.
*/
static void
d_internal_log_json_attributes(
    struct dj_ostream*         _os,
    const struct dj_log_entry* _entry
)
{
    char                          code[160];
    struct dj_desc_json_state     json;
    struct dj_internal_log_dotted dotted;

    d_internal_log_member(_os, "attributes", 0);
    (void)d_ostream_write(_os, "{\"code\":", 8u);
    (void)d_internal_log_code_text(_entry->code, 0u, code, sizeof(code));
    d_internal_log_string(_os, code, strlen(code));
    d_internal_log_member(_os, "module", 0);
    d_internal_log_string(_os, _entry->code->module->name,
                          strlen(_entry->code->module->name));

    d_desc_json_init(&json, _os,
                     D_DESC_JSON_BARE |
                     ((_entry->settings->json_integers != 0u)
                          ? D_DESC_JSON_INTEGER_STRINGS : 0u) |
                     ((_entry->settings->json_nonfinite != 0u)
                          ? D_DESC_JSON_NONFINITE_STRINGS : 0u));
    (void)d_desc_json_machine()->begin(&json);
    json.empty = 0u;  // code and module came first

    if (_entry->settings->nesting != 0u)
    {
        dotted.json = &json;
        (void)d_desc_replay(_entry->facts, &DOTTED, &dotted);
    }
    else
    {
        (void)d_desc_replay(_entry->facts, d_desc_json_machine(), &json);
    }

    d_desc_json_machine()->end(&json);
    (void)d_ostream_write_char(_os, '}');

    return;
}

/*
d_internal_log_resource_json
  "resource":{"service.name":...,"process.pid":...}.
*/
static void
d_internal_log_resource_json(
    struct dj_ostream*            _os,
    const struct dj_log_settings* _settings
)
{
    const char* const service = (_settings->service != NULL)
                                    ? _settings->service : "unknown_service";

    d_internal_log_member(_os, "resource", 0);
    (void)d_ostream_write(_os, "{\"service.name\":", 16u);
    d_internal_log_string(_os, service, strlen(service));
#if D_INTERNAL_LOG_PID
    {
#if defined(_WIN32)
        const struct dj_ostream_value pid = d_ostream_value_long(
                                                (long)_getpid());
#else
        const struct dj_ostream_value pid = d_ostream_value_long(
                                                (long)getpid());
#endif

        d_internal_log_member(_os, "process.pid", 0);
        d_internal_log_value(_os, &pid, (uint32_t)D_OSTREAM_CONVERT_JSON);
    }
#endif
    (void)d_ostream_write_char(_os, '}');

    return;
}

/*
d_internal_log_json_record
  One object a line, with the data model's names.
*/
static int
d_internal_log_json_record(
    void*                      _context,
    struct dj_ostream*         _os,
    const struct dj_log_entry* _entry
)
{
    const struct dj_log_settings* const settings = _entry->settings;

    (void)_context;
    (void)d_ostream_write(_os, "{\"timestamp\":\"", 14u);
    d_log_write_time(_os, _entry->time, settings->time);
    (void)d_ostream_write_char(_os, '"');

    if (settings->observed_time != 0u)
    {
        d_internal_log_member(_os, "observedTimestamp", 0);
        (void)d_ostream_write_char(_os, '"');
        d_log_write_time(_os, _entry->observed, settings->time);
        (void)d_ostream_write_char(_os, '"');
    }

    d_internal_log_member(_os, "severityText", 0);
    d_internal_log_string(_os, _entry->level, strlen(_entry->level));

    if (settings->severity_number != 0u)
    {
        const struct dj_ostream_value number = d_ostream_value_ulong(
                                                   _entry->severity);

        d_internal_log_member(_os, "severityNumber", 0);
        d_internal_log_value(_os, &number, (uint32_t)D_OSTREAM_CONVERT_JSON);
    }

    d_internal_log_member(_os, "body", 0);
    d_internal_log_string(_os, _entry->body, _entry->body_size);
    d_internal_log_json_attributes(_os, _entry);

    if ( (settings->dropped_count != 0u) && (_entry->dropped != 0u) )
    {
        const struct dj_ostream_value dropped = d_ostream_value_size(
                                                    _entry->dropped);

        d_internal_log_member(_os, "droppedAttributesCount", 0);
        d_internal_log_value(_os, &dropped, (uint32_t)D_OSTREAM_CONVERT_JSON);
    }

    if (settings->scope != 0u)
    {
        d_internal_log_member(_os, "scope", 0);
        (void)d_ostream_write(_os, "{\"name\":", 8u);
        d_internal_log_string(_os, _entry->code->module->name,
                              strlen(_entry->code->module->name));
        (void)d_ostream_write_char(_os, '}');
    }

    if (settings->resource != 0u)
    {
        d_internal_log_resource_json(_os, settings);
    }

    (void)d_ostream_write(_os, "}\n", 2u);

    return 0;
}

static const struct dj_log_writer JSON_WRITER =
{
    NULL, d_internal_log_json_record, NULL, NULL
};

#endif  // D_CFG_LOG_FORMATS & 0x2

const struct dj_log_writer*
d_log_writer_json(void)
{
#if (D_CFG_LOG_FORMATS & 0x2)
    return &JSON_WRITER;
#else
    return NULL;
#endif
}


//==============================================================================
// THE OTLP/JSON WRITER
//==============================================================================

#if (D_CFG_LOG_FORMATS & 0x4)

// dj_internal_log_otlp
//   struct: the OTLP attribute machine's state: a KeyValue list a level.
struct dj_internal_log_otlp
{
    struct dj_ostream* os;
    uint32_t           depth;
    uint64_t           empty;  // bit per level: 1 while nothing is in it
};

/*
d_internal_log_otlp_any
  An AnyValue: a string, a 64-bit integer as a decimal string, a bool, a
double (its NaN and infinities as protobuf's strings), or none as {}.
*/
static void
d_internal_log_otlp_any(
    struct dj_ostream*             _os,
    const struct dj_ostream_value* _value
)
{
    switch (_value->kind)
    {
        case D_OSTREAM_VALUE_TEXT:
            (void)d_ostream_write(_os, "{\"stringValue\":", 15u);
            d_internal_log_value(_os, _value,
                                 (uint32_t)D_OSTREAM_CONVERT_JSON);
            break;
        case D_OSTREAM_VALUE_INTEGER:
            (void)d_ostream_write(_os, "{\"intValue\":\"", 13u);
            d_internal_log_value(_os, _value, (uint32_t)D_OSTREAM_CONVERT_RAW);
            (void)d_ostream_write_char(_os, '"');
            break;
        case D_OSTREAM_VALUE_BOOLEAN:
            (void)d_ostream_write(_os, "{\"boolValue\":", 13u);
            d_internal_log_value(_os, _value,
                                 (uint32_t)D_OSTREAM_CONVERT_JSON);
            break;
        case D_OSTREAM_VALUE_REAL:
            (void)d_ostream_write(_os, "{\"doubleValue\":", 15u);

            if (_value->real != _value->real)
            {
                (void)d_ostream_write_cstr(_os, "\"NaN\"");
            }
            else if ( (_value->real > 1.7976931348623157e308) ||
                      (_value->real < -1.7976931348623157e308) )
            {
                (void)d_ostream_write_cstr(_os, (_value->real > 0.0)
                                                    ? "\"Infinity\""
                                                    : "\"-Infinity\"");
            }
            else
            {
                d_internal_log_value(_os, _value,
                                     (uint32_t)D_OSTREAM_CONVERT_JSON);
            }

            break;
        default:
            (void)d_ostream_write_char(_os, '{');
            break;
    }

    (void)d_ostream_write_char(_os, '}');

    return;
}

/*
d_internal_log_otlp_key
  A KeyValue's start, {"key":...,"value":, after a comma unless it is the
level's first.
*/
static void
d_internal_log_otlp_key(
    struct dj_internal_log_otlp* _state,
    const char*                  _key
)
{
    const uint64_t bit = (uint64_t)1u << _state->depth;

    if ((_state->empty & bit) == 0u)
    {
        (void)d_ostream_write_char(_state->os, ',');
    }

    _state->empty &= ~bit;
    (void)d_ostream_write(_state->os, "{\"key\":", 7u);
    d_internal_log_string(_state->os, (_key != NULL) ? _key : "",
                          (_key != NULL) ? strlen(_key) : 0u);
    (void)d_ostream_write(_state->os, ",\"value\":", 9u);

    return;
}

static int
d_internal_log_otlp_fact(
    void*                      _context,
    const struct dj_desc_fact* _fact
)
{
    struct dj_internal_log_otlp* const state =
        (struct dj_internal_log_otlp*)_context;

    d_internal_log_otlp_key(state, _fact->key);
    d_internal_log_otlp_any(state->os, &_fact->value);
    (void)d_ostream_write_char(state->os, '}');

    return 0;
}

static int
d_internal_log_otlp_enter(
    void*       _context,
    const char* _key,
    const char* _label,
    uint32_t    _depth
)
{
    struct dj_internal_log_otlp* const state =
        (struct dj_internal_log_otlp*)_context;

    (void)_label;
    state->depth = _depth;
    d_internal_log_otlp_key(state, _key);
    (void)d_ostream_write(state->os, "{\"kvlistValue\":{\"values\":[", 26u);
    state->depth  = _depth + 1u;
    state->empty |= (uint64_t)1u << state->depth;

    return 0;
}

static int
d_internal_log_otlp_leave(
    void*    _context,
    uint32_t _depth
)
{
    struct dj_internal_log_otlp* const state =
        (struct dj_internal_log_otlp*)_context;

    (void)d_ostream_write(state->os, "]}}}", 4u);
    state->depth = _depth;

    return 0;
}

static const struct dj_desc_machine OTLP_ATTRIBUTES =
{
    NULL, d_internal_log_otlp_fact, d_internal_log_otlp_enter,
    d_internal_log_otlp_leave, NULL, NULL
};

/*
d_internal_log_otlp_text
  A KeyValue of text, in a list whose state says whether it is the first.
*/
static void
d_internal_log_otlp_text(
    struct dj_internal_log_otlp* _state,
    const char*                  _key,
    const char*                  _text
)
{
    const struct dj_ostream_value value = d_ostream_value_text(_text);

    d_internal_log_otlp_key(_state, _key);
    d_internal_log_otlp_any(_state->os, &value);
    (void)d_ostream_write_char(_state->os, '}');

    return;
}

/*
d_internal_log_otlp_resource
  "resource":{"attributes":[service.name, process.pid]}.
*/
static void
d_internal_log_otlp_resource(
    struct dj_ostream*            _os,
    const struct dj_log_settings* _settings
)
{
    struct dj_internal_log_otlp list;

    list.os    = _os;
    list.depth = 0u;
    list.empty = 1u;
    (void)d_ostream_write(_os, "{\"resource\":{\"attributes\":[", 27u);
    d_internal_log_otlp_text(&list, "service.name",
                             (_settings->service != NULL)
                                 ? _settings->service : "unknown_service");
#if D_INTERNAL_LOG_PID
    {
#if defined(_WIN32)
        const struct dj_ostream_value pid = d_ostream_value_long(
                                                (long)_getpid());
#else
        const struct dj_ostream_value pid = d_ostream_value_long(
                                                (long)getpid());
#endif

        d_internal_log_otlp_key(&list, "process.pid");
        d_internal_log_otlp_any(_os, &pid);
        (void)d_ostream_write_char(_os, '}');
    }
#endif
    (void)d_ostream_write(_os, "]}", 2u);

    return;
}

/*
d_internal_log_otlp_nanos_value
  A time as a JSON string of nanoseconds, as OTLP writes a fixed64.
*/
static int
d_internal_log_otlp_nanos_value(
    struct dj_ostream* _os,
    uint64_t           _time
)
{
    const struct dj_ostream_value time = d_ostream_value_uint64(_time);

    (void)d_ostream_write_char(_os, '"');
    d_internal_log_value(_os, &time, (uint32_t)D_OSTREAM_CONVERT_RAW);

    return d_ostream_write_char(_os, '"');
}

/*
d_internal_log_otlp_nanos
  A member whose value is a time in nanoseconds.
*/
static void
d_internal_log_otlp_nanos(
    struct dj_ostream* _os,
    const char*        _key,
    uint64_t           _time
)
{
    d_internal_log_member(_os, _key, 0);
    (void)d_internal_log_otlp_nanos_value(_os, _time);

    return;
}

/*
d_internal_log_otlp_record
  One ExportLogsServiceRequest a line: the resource, the module's scope,
and the record.
*/
static int
d_internal_log_otlp_record(
    void*                      _context,
    struct dj_ostream*         _os,
    const struct dj_log_entry* _entry
)
{
    const struct dj_log_settings* const settings = _entry->settings;
    struct dj_internal_log_otlp         list;
    char                                code[160];

    (void)_context;
    (void)d_ostream_write(_os, "{\"resourceLogs\":[", 17u);
    d_internal_log_otlp_resource(_os, settings);
    (void)d_ostream_write(_os, ",\"scopeLogs\":[{\"scope\":{", 24u);

    if (settings->scope != 0u)
    {
        (void)d_ostream_write(_os, "\"name\":", 7u);
        d_internal_log_string(_os, _entry->code->module->name,
                              strlen(_entry->code->module->name));
    }

    (void)d_ostream_write(_os, "},\"logRecords\":[{", 17u);
    (void)d_ostream_write(_os, "\"timeUnixNano\":", 15u);
    (void)d_internal_log_otlp_nanos_value(_os, _entry->time);

    if (settings->observed_time != 0u)
    {
        d_internal_log_otlp_nanos(_os, "observedTimeUnixNano",
                                  _entry->observed);
    }

    if (settings->severity_number != 0u)
    {
        const struct dj_ostream_value number = d_ostream_value_ulong(
                                                   _entry->severity);

        d_internal_log_member(_os, "severityNumber", 0);
        d_internal_log_value(_os, &number, (uint32_t)D_OSTREAM_CONVERT_JSON);
    }

    d_internal_log_member(_os, "severityText", 0);
    d_internal_log_string(_os, _entry->level, strlen(_entry->level));
    d_internal_log_member(_os, "body", 0);
    (void)d_ostream_write(_os, "{\"stringValue\":", 15u);
    d_internal_log_string(_os, _entry->body, _entry->body_size);
    (void)d_ostream_write_char(_os, '}');

    // the attributes: code and module, then the facts
    d_internal_log_member(_os, "attributes", 0);
    (void)d_ostream_write_char(_os, '[');
    list.os    = _os;
    list.depth = 0u;
    list.empty = 1u;
    d_internal_log_otlp_text(&list, "code",
                             d_internal_log_code_text(_entry->code, 0u, code,
                                                      sizeof(code)));
    d_internal_log_otlp_text(&list, "module", _entry->code->module->name);
    (void)d_desc_replay(_entry->facts, &OTLP_ATTRIBUTES, &list);
    (void)d_ostream_write_char(_os, ']');

    if ( (settings->dropped_count != 0u) && (_entry->dropped != 0u) )
    {
        const struct dj_ostream_value dropped = d_ostream_value_size(
                                                    _entry->dropped);

        d_internal_log_member(_os, "droppedAttributesCount", 0);
        d_internal_log_value(_os, &dropped, (uint32_t)D_OSTREAM_CONVERT_JSON);
    }

    (void)d_ostream_write(_os, "}]}]}]}\n", 8u);

    return 0;
}

static const struct dj_log_writer OTLP_WRITER =
{
    NULL, d_internal_log_otlp_record, NULL, NULL
};

#endif  // D_CFG_LOG_FORMATS & 0x4

const struct dj_log_writer*
d_log_writer_otlp(void)
{
#if (D_CFG_LOG_FORMATS & 0x4)
    return &OTLP_WRITER;
#else
    return NULL;
#endif
}

#endif  // RE_STD_HAS_INT64
