/*******************************************************************************
* djinterp [tools]                                                        dxml.h
*
* XML, read as a stream of tokens.
*   Enough of XML to find elements and read their attributes and text in the
* parts of a package: a scanner over a buffer yields start tags, end tags,
* empty-element tags and text in document order, each with its place in the
* buffer, and nothing is built. A caller that wants a tree keeps its own
* stack; one that wants to change a value edits the buffer at the place the
* token gives, which leaves every other byte of the part as it was.
*   Not done: validation, DTDs, and namespace resolution. A prefixed name is
* read as written, and d_xml_is matches an element by its local name whatever
* its prefix, which is what reading a known format needs.
*   A stand-in, local to dawk: XML belongs to the parse tier, and when that
* tier reads it, this unit gives way to it.
*
* path:      /inc/djinterp/tools/dawk/dxml.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Tokens
         1.  d_xml_kind
         2.  d_xml_token
    2.  Scanner
         1.  d_xml
2.  OPERATIONS
    ----------
    1.  Scanning
    2.  Names and attributes
    3.  Text
*/

#ifndef DJINTERP_TOOLS_DAWK_DXML_H
#define DJINTERP_TOOLS_DAWK_DXML_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Tokens
//------------------------------------------------------------------------------
// 1.1.1
// d_xml_kind
//   enum: what a token is.  A comment, a processing instruction and a
//   declaration are all D_XML_OTHER: a reader of a known format skips them.
enum d_xml_kind
{
    D_XML_END = 0,   // the buffer is exhausted
    D_XML_START,     // <name ...>
    D_XML_EMPTY,     // <name .../>, which no end tag follows
    D_XML_CLOSE,     // </name>
    D_XML_TEXT,      // character data, or a CDATA section's contents
    D_XML_OTHER,     // <!-- -->, <? ?>, <!DOCTYPE ...>
    D_XML_ERROR      // markup that is not well formed
};

// 1.1.2
// d_xml_token
//   struct: one token, pointing into the scanned buffer.  `start` and `end`
//   bound the whole token; `name` is the element's for the three tag kinds,
//   `attributes` the raw text between the name and the tag's close, and
//   `text` the raw character data, entities still encoded unless `cdata`.
struct d_xml_token
{
    enum d_xml_kind kind;
    size_t          start;
    size_t          end;
    const char*     name;
    size_t          name_length;
    const char*     attributes;
    size_t          attributes_length;
    const char*     text;
    size_t          text_length;
    bool            cdata;
};

// 1.2    Scanner
//------------------------------------------------------------------------------
// 1.2.1
// d_xml
//   struct: a cursor over a buffer the caller owns and keeps unchanged while
//   it scans.  Fields are the scanner's.
struct d_xml
{
    const char* text;
    size_t      length;
    size_t      at;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Scanning
//------------------------------------------------------------------------------
// d_xml_next yields the token at the cursor and moves past it; it returns
// false at the end of the buffer and on malformed markup, which the token's
// kind tells apart, and keeps returning it.
void         d_xml_begin(struct d_xml* _xml,
                         const char*  _text,
                         size_t       _length);
bool         d_xml_next(struct d_xml*       _xml,
                        struct d_xml_token* _out);

// 2.2    Names and attributes
//------------------------------------------------------------------------------
// d_xml_is compares an element's local name: `sld` matches <sld> and <p:sld>.
// d_xml_attribute finds an attribute by its name as written, `r:id`
// included, and gives its value raw, between the quotes, entities encoded.
bool         d_xml_is(const struct d_xml_token* _token,
                      const char*               _local_name);
bool         d_xml_attribute(const struct d_xml_token* _token,
                             const char*               _name,
                             const char**              _out_value,
                             size_t*                   _out_length);

// 2.3    Text
//------------------------------------------------------------------------------
/**
 * @brief Decodes the five predefined entities and character references.
 *
 * @note A reference that is not one of those is copied as it stands, and a
 *       character reference is written as UTF-8. Decoding never lengthens.
 *
 * @param[in]  _raw     the encoded text; need not be terminated.
 * @param[in]  _length  its length.
 * @param[out] _out     receives the decoded text, unterminated; it must hold
 *                      `_length` bytes.
 * @return the number of bytes written.
 */
size_t       d_xml_decode(const char* _raw,
                          size_t      _length,
                          char*       _out);


#endif  // DJINTERP_TOOLS_DAWK_DXML_H
