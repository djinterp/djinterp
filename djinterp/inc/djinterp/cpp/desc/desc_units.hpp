/*******************************************************************************
* djinterp [cpp]                                                  desc_units.hpp
*
*   desc's C++ face: the C units engine, with a type's description chosen by
* the type. A type T gives its description as units<T> -- a specialization
* whose type() returns its struct dj_desc_type -- and its name in prose as
* type_name<T>, which a container's units name its parameters by: "mapping
* uint32_t to string". Describing a type that gives none fails to compile.
*
*   What the type decides, it decides when it is compiled: pick<Keep, L>
* keeps a literal only for a type that keeps it, so a literal a type rules
* out is never referenced and is not in the binary, and D_DESC_RING_IF puts
* the unit that would say it in no ring. What the value decides, the plan
* decides, at run time, as in C.
*
*   From C++98; static_assert's message, where a type has no name, from
* C++11. Nothing here allocates but describe(), which returns a std::string.
*
*
* path:      /inc/djinterp/cpp/desc/desc_units.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  NAMES AND LITERALS
    ------------------
    1.  type_name
    2.  D_DESC_TYPE_NAME
    3.  D_DESC_LITERAL
    4.  pick
    5.  D_DESC_RING_IF
2.  DESCRIBING
    ----------
    1.  units
    2.  say
    3.  describe
    4.  facts
*/

#ifndef DJINTERP_DESC_UNITS_HPP
#define DJINTERP_DESC_UNITS_HPP 1

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
// djinterp
#include "../djinterp.hpp"                   // framework root
#include "../../c/desc/desc_units.h"         // the engine
#include "../../../re_std/cpp/cstdint/cstdint.hpp"  // uint32_t, uint64_t


#if D_CFG_DESC_ENABLED


NS_DJINTERP
D_NAMESPACE(desc)


//==============================================================================
// 1.  NAMES AND LITERALS
//==============================================================================

// 1.1
// type_name
//   trait: a type's name in prose, from text(). A type without one fails to
// compile when it is named; from C++11, the message says what to add.
template<typename T>
struct type_name
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static_assert(sizeof(T) == 0u,
                  "desc: give this type a name with D_DESC_TYPE_NAME");
#else
    // below C++11: the name of this member is the message
    typedef typename T::desc_give_this_type_a_name_with_D_DESC_TYPE_NAME
        missing;
#endif
};

// 1.2
// D_DESC_TYPE_NAME
//   macro: names a type in prose, at namespace djinterp::desc.
#define D_DESC_TYPE_NAME(type_, text_)                                         \
    template<>                                                                 \
    struct type_name< type_ >                                                  \
    {                                                                          \
        static const char* text() { return (text_); }                          \
    }

D_DESC_TYPE_NAME(bool,        "bool");
D_DESC_TYPE_NAME(int,         "int");
D_DESC_TYPE_NAME(double,      "double");
D_DESC_TYPE_NAME(uint32_t,    "uint32_t");
D_DESC_TYPE_NAME(uint64_t,    "uint64_t");
D_DESC_TYPE_NAME(std::string, "string");

// 1.3
// D_DESC_LITERAL
//   macro: a literal as a type, for pick.
#define D_DESC_LITERAL(name_, text_)                                           \
    struct name_                                                               \
    {                                                                          \
        static const char* text() { return (text_); }                          \
    }

// 1.4
// pick
//   trait: a literal the type keeps, or none. A literal a type rules out is
// never referenced, so it is not in the binary.
template<bool Keep, typename Literal>
struct pick
{
    static const char* text() { return Literal::text(); }
};

template<typename Literal>
struct pick<false, Literal>
{
    static const char* text() { return NULL; }
};

// 1.5
// D_DESC_RING_IF
//   macro: a unit's ring when the type keeps it; else no ring shows it. The
// row stays in the table; its own text, through pick, does not.
#define D_DESC_RING_IF(keep_, ring_)                                           \
    ((keep_) ? (ring_) : D_DESC_RING_NEVER)


//==============================================================================
// 2.  DESCRIBING
//==============================================================================

// 2.1
// units
//   trait: a type's description. A specialization gives
//     static const struct dj_desc_type& type();
// Describing a type without one fails to compile: units<T> is incomplete.
template<typename T>
struct units;

// d_internal_desc_put_string
//   function: a put into a std::string, its context.
inline int
d_internal_desc_put_string(
    void*       _context,
    const char* _text,
    std::size_t _size
)
{
    static_cast<std::string*>(_context)->append(_text, _size);

    return 0;
}

// 2.2
// say
//   function: describes an object to a put, as d_desc_say.
template<typename T>
int
say(
    const T&    _object,
    fn_desc_put _put,
    void*       _context,
    uint32_t    _ring   = D_DESC_RING_OPTION,
    uint32_t    _layers = D_DESC_LAYERS_OPTION,
    uint32_t    _flags  = 0u
)
{
    return d_desc_say(&units<T>::type(), &_object, _ring, _layers, _flags,
                      _put, _context);
}

// 2.3
// describe
//   function: an object's description, as a std::string.
template<typename T>
std::string
describe(
    const T& _object,
    uint32_t _ring   = D_DESC_RING_OPTION,
    uint32_t _layers = D_DESC_LAYERS_OPTION,
    uint32_t _flags  = 0u
)
{
    std::string text;

    (void)d_desc_say(&units<T>::type(), &_object, _ring, _layers, _flags,
                     d_internal_desc_put_string, &text);

    return text;
}

// 2.4
// facts
//   function: an object's facts, from its type's field list, into a sink,
// as d_desc_facts.
template<typename T>
int
facts(
    const T&             _object,
    struct dj_desc_sink* _sink
)
{
    return d_desc_facts(&units<T>::type(), &_object, _sink);
}


NS_END  // desc
NS_END  // djinterp


#endif  // D_CFG_DESC_ENABLED

#endif  // DJINTERP_DESC_UNITS_HPP
