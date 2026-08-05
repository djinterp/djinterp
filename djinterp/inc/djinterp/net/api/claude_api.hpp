/******************************************************************************
* djinterp [web]                                                claude_api.hpp
*
*   Higher-level programmatic-workflow layer for Anthropic's Claude, built on
* the typed synchronous surface in claude.hpp. Its centerpiece is the Message
* Batches API -- the supported path for queueing large volumes of requests for
* asynchronous, server-side processing (at reduced cost) rather than driving
* the interactive product. It also provides a small sequential conversation
* runner for dependent multi-turn workloads.
*
* CONTENTS:
*   0.  batches endpoint constant
*   I.   json_int (added to claude::internal; reuses the readers in claude.hpp)
*   namespace djinterp::web::vendor::claude::batch
*     II.  processing_state, request_counts, result_type (+ string helpers)
*     III. status  (a batch object view over its JSON)
*     IV.  result  (one line of the JSONL results file) + parsing/indexing
*     V.   item (custom_id + params) + create-body serialization + helpers
*     VI.  poll_config, outcome
*     VII. client -- create / retrieve / cancel / delete / list / results /
*          poll / submit_and_wait
*   namespace djinterp::web::vendor::claude::queue
*     VIII. run_conversation -- linear dependent-turn runner over claude::client
*
*   WHY BATCHES: a batch request is exactly {custom_id, params} where params is
* the same object claude::message_request::to_json() already emits, so this
* layer is a thin wrapper -- submit a queue, poll (most batches finish within
* an hour; 24-hour ceiling), then stream the JSONL results and reconcile them
* by custom_id (batch results are not returned in submission order).
*
*   Requires:  web/vendor/claude.hpp (and therefore curl.hpp + libcurl).
*
* path:      /inc/djinterp/web/vendor/claude_api.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.17
******************************************************************************/

#ifndef DJINTERP_WEB_VENDOR_CLAUDE_API_
#define DJINTERP_WEB_VENDOR_CLAUDE_API_ 1

// std
#include <chrono>
#include <cstddef>
#include <string>
#include <thread>
#include <vector>
// djinterp -- typed synchronous Claude surface (pulls curl.hpp + web.hpp)
#include "claude.hpp"


NS_DJINTERP
NS_WEB
D_NAMESPACE(D_KEYWORD_VENDOR)
D_NAMESPACE(D_KEYWORD_CLAUDE)


///////////////////////////////////////////////////////////////////////////////
///                        0.   CONSTANTS                                  ///
///////////////////////////////////////////////////////////////////////////////

// batches_path
//   constant: the Message Batches API resource path.
D_CONSTEXPR const char* const batches_path = "/v1/messages/batches";


///////////////////////////////////////////////////////////////////////////////
///           I.   INTEGER JSON READER (added to claude::internal)          ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // json_int_value
    //   function: from `_s`, finds the first occurrence of `"key"` used as a
    // key (followed by ':') whose value is an integer, decodes it into `_out`,
    // and returns true. An optional leading '+'/'-' is honored. Returns false
    // when no such key/integer pair is found. A targeted reader matching the
    // string helpers in claude.hpp -- adequate for the small, uniquely-named
    // numeric fields (request counts) this layer reads.
    inline bool
    json_int_value(
        const std::string& _s,
        const std::string& _key,
        std::size_t        _from,
        long&              _out
    )
    {
        const std::string needle = "\"" + _key + "\"";
        std::size_t       pos    = _s.find(needle, _from);

        // scan each candidate key occurrence
        while (pos != std::string::npos)
        {
            std::size_t i = pos + needle.size();

            // skip whitespace before the ':'
            while ( (i < _s.size()) &&
                    ( (_s[i] == ' ')  || (_s[i] == '\t') ||
                      (_s[i] == '\n') || (_s[i] == '\r') ) )
            {
                ++i;
            }

            // confirm this is a key by the following ':'
            if ( (i < _s.size()) &&
                 (_s[i] == ':') )
            {
                ++i;

                // skip whitespace before the value
                while ( (i < _s.size()) &&
                        ( (_s[i] == ' ')  || (_s[i] == '\t') ||
                          (_s[i] == '\n') || (_s[i] == '\r') ) )
                {
                    ++i;
                }

                bool negative = false;

                // optional sign
                if ( (i < _s.size()) &&
                     ((_s[i] == '-') || (_s[i] == '+')) )
                {
                    negative = (_s[i] == '-');
                    ++i;
                }

                // require at least one digit
                if ( (i < _s.size()) &&
                     (_s[i] >= '0') && (_s[i] <= '9') )
                {
                    long value = 0;

                    while ( (i < _s.size()) &&
                            (_s[i] >= '0') && (_s[i] <= '9') )
                    {
                        value = (value * 10) + (_s[i] - '0');
                        ++i;
                    }

                    _out = negative ? -value : value;

                    return true;
                }
            }

            pos = _s.find(needle, pos + needle.size());
        }

        return false;
    }

    // json_int
    //   function: convenience -- the first integer value for `_key`, or
    // `_fallback`.
    inline long
    json_int(
        const std::string& _s,
        const std::string& _key,
        long               _fallback = 0
    )
    {
        long value = 0;

        return json_int_value(_s, _key, 0, value) ? value : _fallback;
    }

NS_END  // internal


D_NAMESPACE(batch)

///////////////////////////////////////////////////////////////////////////////
///          II.   PROCESSING STATE / COUNTS / RESULT TYPE                 ///
///////////////////////////////////////////////////////////////////////////////

// processing_state
//   enum: the stage a Message Batch is in. `unknown` covers an unrecognized or
// absent status string.
enum class processing_state : unsigned char
{
    unknown = 0,
    in_progress,
    canceling,
    ended
};

// to_string(processing_state)
//   function: the API token for a processing state.
D_NODISCARD inline const char*
to_string(
    processing_state _state
)
{
    switch (_state)
    {
        case processing_state::in_progress: return "in_progress";
        case processing_state::canceling:   return "canceling";
        case processing_state::ended:       return "ended";
        case processing_state::unknown:     return "";
    }

    return "";
}

// processing_state_from_string
//   function: maps a status token to a processing_state.
D_NODISCARD inline processing_state
processing_state_from_string(
    const std::string& _token
)
{
    if (_token == "in_progress") { return processing_state::in_progress; }
    if (_token == "canceling")   { return processing_state::canceling;   }
    if (_token == "ended")       { return processing_state::ended;       }

    return processing_state::unknown;
}

// request_counts
//   struct: the tally of per-request outcomes reported on a batch object.
struct request_counts
{
    long processing;
    long succeeded;
    long errored;
    long canceled;
    long expired;

    request_counts()
        : processing(0),
          succeeded(0),
          errored(0),
          canceled(0),
          expired(0)
    {
    }

    // total
    //   function: the sum of all counted requests.
    D_NODISCARD long
    total() const
    {
        return processing + succeeded + errored + canceled + expired;
    }
};

// result_type
//   enum: the outcome of a single request within a completed batch. `unknown`
// covers an unrecognized type string.
enum class result_type : unsigned char
{
    unknown = 0,
    succeeded,
    errored,
    canceled,
    expired
};

// to_string(result_type)
//   function: the API token for a result type.
D_NODISCARD inline const char*
to_string(
    result_type _type
)
{
    switch (_type)
    {
        case result_type::succeeded: return "succeeded";
        case result_type::errored:   return "errored";
        case result_type::canceled:  return "canceled";
        case result_type::expired:   return "expired";
        case result_type::unknown:   return "";
    }

    return "";
}

// result_type_from_string
//   function: maps a result-type token to a result_type.
D_NODISCARD inline result_type
result_type_from_string(
    const std::string& _token
)
{
    if (_token == "succeeded") { return result_type::succeeded; }
    if (_token == "errored")   { return result_type::errored;   }
    if (_token == "canceled")  { return result_type::canceled;  }
    if (_token == "expired")   { return result_type::expired;   }

    return result_type::unknown;
}


///////////////////////////////////////////////////////////////////////////////
///                        III.   STATUS                                   ///
///////////////////////////////////////////////////////////////////////////////

// status
//   struct: the outcome of a batch lifecycle call (create / retrieve / cancel).
// It carries the neutral HTTP response and reads the batch object's fields on
// demand from the JSON body. The complete body is available via raw().
struct status
{
    response http;

    // ok
    //   function: whether the call completed transport-cleanly with a 2xx
    // status (i.e. the batch object was returned).
    D_NODISCARD bool
    ok() const
    {
        return ( (http.error == transport_error::none) &&
                 http.ok() );
    }

    // http_status / transport / raw
    D_NODISCARD int
    http_status() const
    {
        return http.status;
    }

    D_NODISCARD transport_error
    transport() const
    {
        return http.error;
    }

    D_NODISCARD const std::string&
    raw() const
    {
        return http.body;
    }

    // id
    //   function: the batch id (e.g. "msgbatch_..."), or "".
    D_NODISCARD std::string
    id() const
    {
        return internal::json_string(http.body, "id");
    }

    // processing_status
    //   function: the raw processing-status token ("in_progress" / "canceling"
    // / "ended"), or "".
    D_NODISCARD std::string
    processing_status() const
    {
        return internal::json_string(http.body, "processing_status");
    }

    // state
    //   function: the processing status as an enum.
    D_NODISCARD processing_state
    state() const
    {
        return processing_state_from_string(processing_status());
    }

    // is_ended
    //   function: whether processing has finished and results are ready.
    D_NODISCARD bool
    is_ended() const
    {
        return (processing_status() == "ended");
    }

    // is_processing
    //   function: whether the batch is still in progress or canceling.
    D_NODISCARD bool
    is_processing() const
    {
        const std::string s = processing_status();

        return ( (s == "in_progress") ||
                 (s == "canceling") );
    }

    // counts
    //   function: the request_counts tally parsed from the batch object.
    D_NODISCARD request_counts
    counts() const
    {
        request_counts c;
        c.processing = internal::json_int(http.body, "processing");
        c.succeeded  = internal::json_int(http.body, "succeeded");
        c.errored    = internal::json_int(http.body, "errored");
        c.canceled   = internal::json_int(http.body, "canceled");
        c.expired    = internal::json_int(http.body, "expired");

        return c;
    }

    // results_url
    //   function: the URL to download the JSONL results, or "" when not yet
    // available (still processing, or the value is null).
    D_NODISCARD std::string
    results_url() const
    {
        return internal::json_string(http.body, "results_url");
    }

    // has_results
    //   function: whether a results URL is present.
    D_NODISCARD bool
    has_results() const
    {
        return !results_url().empty();
    }

    // created_at / expires_at / ended_at
    //   function: the corresponding timestamp strings, or "".
    D_NODISCARD std::string
    created_at() const
    {
        return internal::json_string(http.body, "created_at");
    }

    D_NODISCARD std::string
    expires_at() const
    {
        return internal::json_string(http.body, "expires_at");
    }

    D_NODISCARD std::string
    ended_at() const
    {
        return internal::json_string(http.body, "ended_at");
    }

    // error_type / error_message
    //   function: for a failed lifecycle call carrying an error document, the
    // error subtype / message, or "".
    D_NODISCARD std::string
    error_type() const
    {
        return internal::json_string_after(http.body, "error", "type");
    }

    D_NODISCARD std::string
    error_message() const
    {
        return internal::json_string_after(http.body, "error", "message");
    }
};


///////////////////////////////////////////////////////////////////////////////
///                        IV.   RESULT                                    ///
///////////////////////////////////////////////////////////////////////////////

// result
//   struct: the outcome of one request within a completed batch, parsed from a
// single line of the JSONL results file. Accessors read the line's JSON on
// demand; the full line is available via raw().
struct result
{
    std::string custom_id;
    result_type type;
    std::string line;  // the raw JSON for this result

    result()
        : custom_id(),
          type(result_type::unknown),
          line()
    {
    }

    // ok
    //   function: whether this request succeeded and produced a message.
    D_NODISCARD bool
    ok() const
    {
        return (type == result_type::succeeded);
    }

    // raw
    //   function: the raw JSON line for this result.
    D_NODISCARD const std::string&
    raw() const
    {
        return line;
    }

    // text
    //   function: the assistant reply as the concatenation of every text block,
    // for a succeeded result; "" otherwise.
    D_NODISCARD std::string
    text() const
    {
        return ok() ? internal::concat_string_values(line, "text")
                    : std::string();
    }

    // stop_reason
    //   function: the message stop reason for a succeeded result, or "".
    D_NODISCARD std::string
    stop_reason() const
    {
        return internal::json_string(line, "stop_reason");
    }

    // message_id
    //   function: the id of the produced message (from inside the "message"
    // object), or "".
    D_NODISCARD std::string
    message_id() const
    {
        return internal::json_string_after(line, "message", "id");
    }

    // model
    //   function: the model reported on the produced message, or "".
    D_NODISCARD std::string
    model() const
    {
        return internal::json_string(line, "model");
    }

    // error_type / error_message
    //   function: for an errored result, the error subtype / message from the
    // error object, or "".
    D_NODISCARD std::string
    error_type() const
    {
        return internal::json_string_after(line, "error", "type");
    }

    D_NODISCARD std::string
    error_message() const
    {
        return internal::json_string_after(line, "error", "message");
    }
};

// parse_results
//   function: parses a JSONL results payload into one result per non-empty
// line. Each line's custom_id and result type are extracted eagerly; the rest
// is read lazily by the result accessors.
D_NODISCARD inline std::vector<result>
parse_results(
    const std::string& _jsonl
)
{
    std::vector<result> out;
    std::size_t         start = 0;

    // split on newlines and parse each line
    while (start <= _jsonl.size())
    {
        std::size_t nl  = _jsonl.find('\n', start);
        std::size_t end = (nl == std::string::npos) ? _jsonl.size() : nl;

        if (end > start)
        {
            std::string line = _jsonl.substr(start, end - start);

            // tolerate CRLF endings
            while ( (!line.empty()) &&
                    (line.back() == '\r') )
            {
                line.pop_back();
            }

            // parse non-blank lines
            if (!line.empty())
            {
                result r;
                r.line      = line;
                r.custom_id = internal::json_string(line, "custom_id");
                r.type      = result_type_from_string(
                                  internal::json_string(line, "type"));

                out.push_back(r);
            }
        }

        if (nl == std::string::npos)
        {
            break;
        }

        start = nl + 1;
    }

    return out;
}

// find_result
//   function: a pointer to the result with the given custom_id, or nullptr.
// Since batch results are unordered, callers reconcile by custom_id.
D_NODISCARD inline const result*
find_result(
    const std::vector<result>& _results,
    const std::string&         _custom_id
)
{
    // linear search over the parsed results
    for (std::size_t i = 0; i < _results.size(); ++i)
    {
        if (_results[i].custom_id == _custom_id)
        {
            return &_results[i];
        }
    }

    return nullptr;
}


///////////////////////////////////////////////////////////////////////////////
///                 V.   ITEM + CREATE-BODY SERIALIZATION                  ///
///////////////////////////////////////////////////////////////////////////////

// item
//   struct: one request in a batch -- a caller-chosen custom_id plus the
// message parameters (a claude::message_request). custom_id must match
// ^[a-zA-Z0-9_-]{1,64}$ (see is_valid_custom_id) and be unique within a batch.
struct item
{
    std::string     custom_id;
    message_request params;

    item()
        : custom_id(),
          params()
    {
    }

    item(
        const std::string&     _custom_id,
        const message_request& _params
    )
        : custom_id(_custom_id),
          params(_params)
    {
    }
};

// is_valid_custom_id
//   function: whether `_id` satisfies the API's custom_id constraint
// (1 to 64 characters of ASCII letters, digits, hyphen, or underscore).
D_NODISCARD inline bool
is_valid_custom_id(
    const std::string& _id
)
{
    // enforce the length bounds
    if ( _id.empty() ||
         (_id.size() > 64) )
    {
        return false;
    }

    // enforce the character set
    for (std::size_t i = 0; i < _id.size(); ++i)
    {
        const char c = _id[i];

        const bool allowed = ( ( (c >= 'a') && (c <= 'z') ) ||
                               ( (c >= 'A') && (c <= 'Z') ) ||
                               ( (c >= '0') && (c <= '9') ) ||
                               (c == '-') || (c == '_') );

        if (!allowed)
        {
            return false;
        }
    }

    return true;
}

// user_item
//   function: convenience -- a batch item carrying a single user prompt. Ideal
// for bulk independent one-shot requests.
D_NODISCARD inline item
user_item(
    const std::string& _custom_id,
    const std::string& _prompt,
    const std::string& _model      = default_model,
    int                _max_tokens = 1024
)
{
    message_request req;
    req.model      = _model;
    req.max_tokens = _max_tokens;
    req.add_user(_prompt);

    return item(_custom_id, req);
}

// build_create_body
//   function: serializes a list of items into the JSON body the create
// endpoint expects: { "requests": [ { "custom_id", "params" }, ... ] }. The
// streaming flag is forced off on each item's params, since batch requests are
// never streamed.
D_NODISCARD inline std::string
build_create_body(
    const std::vector<item>& _items
)
{
    internal::json_writer w;

    w.begin_object();
    w.key("requests");
    w.begin_array();

    // emit each request as {custom_id, params}
    for (std::size_t i = 0; i < _items.size(); ++i)
    {
        w.begin_object();

        w.key("custom_id");
        w.value(_items[i].custom_id);

        w.key("params");

        // params must be non-streaming; embed the message_request JSON verbatim
        message_request params = _items[i].params;
        params.stream = false;
        w.value_raw(params.to_json());

        w.end_object();
    }

    w.end_array();
    w.end_object();

    return w.str();
}


///////////////////////////////////////////////////////////////////////////////
///                   VI.   POLL CONFIG + OUTCOME                          ///
///////////////////////////////////////////////////////////////////////////////

// poll_config
//   struct: controls the blocking poll loop. `interval_ms` is the pause
// between status checks; `max_wait_ms` caps the total wait (0 = wait until the
// batch ends or the server's 24-hour ceiling expires it).
struct poll_config
{
    long interval_ms;
    long max_wait_ms;

    poll_config()
        : interval_ms(60000),
          max_wait_ms(0)
    {
    }

    poll_config(
        long _interval_ms,
        long _max_wait_ms
    )
        : interval_ms(_interval_ms),
          max_wait_ms(_max_wait_ms)
    {
    }
};

// outcome
//   struct: the combined result of a submit-then-wait run -- the final batch
// status and the parsed per-request results (empty unless the batch ended and
// results were fetched).
struct outcome
{
    status              final_status;
    std::vector<result> results;
};


///////////////////////////////////////////////////////////////////////////////
///                        VII.   CLIENT                                   ///
///////////////////////////////////////////////////////////////////////////////

// client
//   class: a configured entry point to the Message Batches API. Holds the API
// key and endpoint/version settings plus the transport options, and drives the
// batch lifecycle (create, poll, fetch results, cancel, delete, list).
class client
{
public:
    explicit client(
        const std::string& _api_key,
        const std::string& _base_url = api_base_url,
        const std::string& _version  = default_version
    )
        : m_api_key(_api_key),
          m_base_url(_base_url),
          m_version(_version),
          m_beta(),
          m_options(),
          m_extra_headers()
    {
    }

    // set_beta
    //   function: sets the anthropic-beta header value (empty to clear).
    // Returns *this for chaining.
    client&
    set_beta(
        const std::string& _beta
    )
    {
        m_beta = _beta;

        return *this;
    }

    // add_header
    //   function: adds/overrides a default request header applied to every
    // batch API call. Returns *this for chaining.
    client&
    add_header(
        const std::string& _name,
        const std::string& _value
    )
    {
        set_header(m_extra_headers, _name, _value);

        return *this;
    }

    // options
    //   function: mutable access to the transport options.
    D_NODISCARD curl::options&
    options()
    {
        return m_options;
    }

    // options (const)
    //   function: read-only access to the transport options.
    D_NODISCARD const curl::options&
    options() const
    {
        return m_options;
    }

    // create
    //   function: submits a new batch of items for asynchronous processing.
    // The returned status carries the initial batch object (typically
    // processing_status "in_progress").
    D_NODISCARD status
    create(
        const std::vector<item>& _items
    ) const
    {
        status st;

        const request req = base_request(http_method::post,
                                         batches_url(),
                                         build_create_body(_items));
        st.http = curl::perform(req, m_options);

        return st;
    }

    // retrieve
    //   function: fetches the current state of a batch by id.
    D_NODISCARD status
    retrieve(
        const std::string& _batch_id
    ) const
    {
        status st;

        const request req = base_request(http_method::get,
                                         batch_url(_batch_id),
                                         std::string());
        st.http = curl::perform(req, m_options);

        return st;
    }

    // cancel
    //   function: requests cancellation of an in-progress batch. The batch
    // moves to "canceling" and then "ended", possibly with partial results.
    D_NODISCARD status
    cancel(
        const std::string& _batch_id
    ) const
    {
        status st;

        const request req = base_request(http_method::post,
                                         batch_url(_batch_id) + "/cancel",
                                         std::string());
        st.http = curl::perform(req, m_options);

        return st;
    }

    // delete_batch
    //   function: deletes a batch (which must not be in progress). Returns the
    // raw neutral response.
    D_NODISCARD response
    delete_batch(
        const std::string& _batch_id
    ) const
    {
        const request req = base_request(http_method::delete_,
                                         batch_url(_batch_id),
                                         std::string());

        return curl::perform(req, m_options);
    }

    // list
    //   function: lists batches in the workspace (most recent first), up to
    // `_limit`. Returns the raw response; the body is a JSON page with a "data"
    // array of batch objects.
    D_NODISCARD response
    list(
        int _limit = 20
    ) const
    {
        const std::string url =
            batches_url() + "?limit=" + std::to_string(_limit);

        const request req = base_request(http_method::get, url, std::string());

        return curl::perform(req, m_options);
    }

    // fetch_results
    //   function: downloads and parses the JSONL results for an ended batch,
    // given its status. The results URL is a pre-signed download link, so it is
    // fetched without the API credentials. Returns an empty vector when no
    // results URL is present (batch not finished).
    D_NODISCARD std::vector<result>
    fetch_results(
        const status& _status
    ) const
    {
        std::vector<result> out;

        const std::string url = _status.results_url();

        // nothing to fetch until the batch has produced results
        if (url.empty())
        {
            return out;
        }

        // the results URL is pre-signed: a bare GET, no auth headers
        request req;
        req.method = http_method::get;
        req.url    = url;

        const response r = curl::perform(req, m_options);

        return parse_results(r.body);
    }

    // results
    //   function: convenience -- retrieves a batch by id and, if it has ended,
    // downloads and parses its results.
    D_NODISCARD std::vector<result>
    results(
        const std::string& _batch_id
    ) const
    {
        return fetch_results(retrieve(_batch_id));
    }

    // poll
    //   function: blocks, re-checking the batch on `_config.interval_ms` until
    // it ends, the optional `_config.max_wait_ms` budget is exhausted, or a
    // retrieve fails at the transport/HTTP level. Returns the last status seen
    // (call is_ended()/ok() on it to distinguish the reasons).
    D_NODISCARD status
    poll(
        const std::string& _batch_id,
        const poll_config& _config = poll_config()
    ) const
    {
        long waited = 0;

        for (;;)
        {
            status st = retrieve(_batch_id);

            // a failed retrieve ends the loop; the caller inspects st.ok()
            if (!st.ok())
            {
                return st;
            }

            // finished -- results are ready
            if (st.is_ended())
            {
                return st;
            }

            // honor the optional wait budget
            if ( (_config.max_wait_ms > 0) &&
                 (waited >= _config.max_wait_ms) )
            {
                return st;
            }

            std::this_thread::sleep_for(
                std::chrono::milliseconds(_config.interval_ms));

            waited += _config.interval_ms;
        }
    }

    // submit_and_wait
    //   function: the headline convenience -- creates a batch from `_items`,
    // polls until it ends, and fetches its results. On a create failure the
    // outcome carries the failing status and no results; on a poll timeout it
    // carries the last (still-processing) status and no results.
    D_NODISCARD outcome
    submit_and_wait(
        const std::vector<item>& _items,
        const poll_config&       _config = poll_config()
    ) const
    {
        outcome out;

        // submit
        const status created = create(_items);

        if (!created.ok())
        {
            out.final_status = created;

            return out;
        }

        // wait for completion
        out.final_status = poll(created.id(), _config);

        // collect results once ended
        if (out.final_status.is_ended())
        {
            out.results = fetch_results(out.final_status);
        }

        return out;
    }

private:
    // base_request
    //   function: builds a neutral web::request for a batch endpoint, applying
    // the standard Anthropic auth/version headers, any default headers, and --
    // when a body is supplied -- the JSON content type and body.
    D_NODISCARD request
    base_request(
        http_method        _method,
        const std::string& _url,
        const std::string& _body
    ) const
    {
        request req;

        req.method = _method;
        req.url    = _url;

        req.set_header(header_api_key, m_api_key);
        req.set_header(header_version, m_version);

        // optional beta opt-in header
        if (!m_beta.empty())
        {
            req.set_header(header_beta, m_beta);
        }

        // caller-supplied default headers
        for (std::size_t i = 0; i < m_extra_headers.size(); ++i)
        {
            set_header(req.headers,
                       m_extra_headers[i].first,
                       m_extra_headers[i].second);
        }

        // body + content type when present
        if (!_body.empty())
        {
            req.set_header(header_name::content_type, content_type::json);
            req.body = _body;
        }

        return req;
    }

    // batches_url
    //   function: the absolute batches collection URL.
    D_NODISCARD std::string
    batches_url() const
    {
        return m_base_url + batches_path;
    }

    // batch_url
    //   function: the absolute URL of a single batch by id.
    D_NODISCARD std::string
    batch_url(
        const std::string& _batch_id
    ) const
    {
        return batches_url() + "/" + _batch_id;
    }

    std::string   m_api_key;
    std::string   m_base_url;
    std::string   m_version;
    std::string   m_beta;
    curl::options m_options;
    header_list   m_extra_headers;
};

NS_END  // batch


D_NAMESPACE(queue)

///////////////////////////////////////////////////////////////////////////////
///          VIII.   SEQUENTIAL DEPENDENT-TURN RUNNER                      ///
///////////////////////////////////////////////////////////////////////////////

// turn_result
//   struct: one exchanged turn in a run_conversation run -- the user input that
// was sent and the model's response to it.
struct turn_result
{
    std::string      user;
    message_response response;
};

// run_conversation
//   function: drives a linear, dependent multi-turn conversation to completion
// using the synchronous claude::client. Starting from `_seed` (which presets
// model / max_tokens / system, and may already contain prior messages), each
// user turn in `_user_turns` is appended and sent with the full accumulated
// context; the assistant's text reply is threaded back in before the next
// turn. This is the structured equivalent of "answer arrived -> send the next
// input." Stops early if a turn fails (the failing turn is included). Best
// suited to straightforward text conversations; turns whose replies are not
// plain text (e.g. tool use) thread back only their text portion.
D_NODISCARD inline std::vector<turn_result>
run_conversation(
    const client&                   _client,
    message_request                 _seed,
    const std::vector<std::string>& _user_turns
)
{
    std::vector<turn_result> out;

    // send each user turn with the running context
    for (std::size_t i = 0; i < _user_turns.size(); ++i)
    {
        _seed.add_user(_user_turns[i]);

        turn_result turn;
        turn.user     = _user_turns[i];
        turn.response = _client.send(_seed);

        out.push_back(turn);

        // stop the chain on any failure
        if (!turn.response.ok())
        {
            break;
        }

        // thread the assistant reply back in for the next turn
        _seed.add_assistant(turn.response.text());
    }

    return out;
}

NS_END  // queue


NS_END  // claude
NS_END  // vendor
NS_END  // web
NS_END  // djinterp


#endif  // DJINTERP_WEB_VENDOR_CLAUDE_API_
