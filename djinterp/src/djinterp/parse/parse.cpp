#include "parse.hpp"


NS_PARSE


// ================================================================
//  parse_error
// ================================================================

/*
parse_error (default)
  Constructs an empty parse_error with zeroed status, zero offset, and
a null message.

Parameter(s):
  none.
Return:
  none.
*/
parse_error::parse_error
()
    : m_status  (0),
      m_offset  (0),
      m_message (nullptr)
{
}

/*
parse_error (parameterized)
  Constructs a parse_error from a status code, input offset, and an
optional human-readable message.

Parameter(s):
  _status:  the status code classifying the failure.
  _offset:  the byte/element offset at which the error was detected.
  _message: an optional diagnostic string (not owned; must outlive the
            error object).
Return:
  none.
*/
parse_error::parse_error
(
    parse_status _status,
    std::size_t  _offset,
    const char*  _message
)
    : m_status  (_status),
      m_offset  (_offset),
      m_message (_message)
{
}

/*
parse_error (copy)
  Copy-constructs a parse_error from another instance.

Parameter(s):
  _other: the parse_error to copy.
Return:
  none.
*/
parse_error::parse_error
(
    const parse_error& _other
)
    : m_status  (_other.m_status),
      m_offset  (_other.m_offset),
      m_message (_other.m_message)
{
}

/*
operator= (copy)
  Copy-assigns a parse_error from another instance.

Parameter(s):
  _other: the parse_error to copy.
Return:
  a reference to this parse_error.
*/
parse_error&
parse_error::operator=
(
    const parse_error& _other
)
{
    m_status  = _other.m_status;
    m_offset  = _other.m_offset;
    m_message = _other.m_message;

    return *this;
}

/*
status
  Returns the status code classifying the parse failure.

Parameter(s):
  none.
Return:
  the parse_status held by this error.
*/
parse_status
parse_error::status() const
{
    return m_status;
}

/*
offset
  Returns the byte/element offset at which the error was detected.

Parameter(s):
  none.
Return:
  the offset into the input at which the failure occurred.
*/
std::size_t
parse_error::offset() const
{
    return m_offset;
}

/*
message
  Returns the optional human-readable diagnostic message, or nullptr
if none was provided.

Parameter(s):
  none.
Return:
  a pointer to the diagnostic string, or nullptr.
*/
const char*
parse_error::message() const
{
    return m_message;
}


NS_END  // parse
