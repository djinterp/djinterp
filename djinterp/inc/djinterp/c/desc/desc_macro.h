/*******************************************************************************
* djinterp [c]                                                      desc_macro.h
*
*   desc's macro layer: descriptions the preprocessor makes, as string
* literals, of what a type and the knobs fix. No code runs, so a description
* is a constant: a const array, a static_assert's message, a #pragma message,
* or a unit's text, so the run-time units and the macro layer share words.
*
*   A description is put together as a person says one: the determiner; the
* adjectives by kind -- condition, size, quality, shape, then origin -- the
* noun; what completes the noun; and remarks after it, each a sentence. Words
* of one kind are coordinate and take commas between them; words of
* different kinds take none: "a full fixed-size ordered tree". Each word's
* place says what goes before it, so nothing is joined and then changed.
*
*   Section 1 needs nothing but #if and D_CONCAT, so it is built in every
* language, C++98 included. The kit of section 2 needs variadic macros (C99,
* C++11), and is built on dmacro.h's tools: D_CONCAT, D_IIF, D_CHECK and
* D_PROBE.
* A word that a knob alone decides is better made with #if, as section 1's
* are, and then needs no kit.
*
*
* path:      /inc/djinterp/c/desc/desc_macro.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DESC'S OWN BUILD
    ----------------
    1.  D_DESC_MACRO_RING_*
    2.  D_DESC_MACRO_BUILD
2.  THE KIT
    -------
    1.  D_DESC_MACRO_KIT
    2.  D_DESC_MACRO_WHEN
    3.  D_DESC_MACRO_IS_ZERO, D_DESC_MACRO_IS_ONE
    4.  D_DESC_MACRO_PLURAL
    5.  D_DESC_MACRO_SENTENCE
    6.  D_DESC_MACRO_NAME
*/

#ifndef DJINTERP_C_DESC_MACRO_H
#define DJINTERP_C_DESC_MACRO_H 1

// djinterp
#include "../dmacro_token.h"   // D_CONCAT, D_TOSTR: ISO C++98 included
#include "./desc_units.h"      // D_DESC_BUILT, the knobs


//==============================================================================
// 1.  DESC'S OWN BUILD
//==============================================================================

#if D_CFG_DESC_ENABLED && D_CFG_DESC_MACRO

// 1.1
// D_DESC_MACRO_RING_*
//   constants: the rings by name, for a description of a build.
#define D_DESC_MACRO_RING_0  "name"
#define D_DESC_MACRO_RING_1  "line"
#define D_DESC_MACRO_RING_2  "sentence"
#define D_DESC_MACRO_RING_3  "detailed"

// a layer's place among the layers built: the first follows "the", the last
// " and ", the others ", "
#define D_INTERNAL_DESC_MACRO_BITS     ((D_CFG_DESC_LAYERS_BUILT) & 0x1FF)
#define D_INTERNAL_DESC_MACRO_HAS(i)                                           \
    (((D_INTERNAL_DESC_MACRO_BITS >> (i)) & 1) != 0)
#define D_INTERNAL_DESC_MACRO_BEFORE(i)                                        \
    ((D_INTERNAL_DESC_MACRO_BITS & ((1 << (i)) - 1)) != 0)
#define D_INTERNAL_DESC_MACRO_AFTER(i)                                         \
    ((D_INTERNAL_DESC_MACRO_BITS >> ((i) + 1)) != 0)

#if !D_INTERNAL_DESC_MACRO_HAS(0)
    #define D_INTERNAL_DESC_MACRO_LAYER_0
#else
    #define D_INTERNAL_DESC_MACRO_LAYER_0  "the identity"
#endif

#if !D_INTERNAL_DESC_MACRO_HAS(1)
    #define D_INTERNAL_DESC_MACRO_LAYER_1
#elif !D_INTERNAL_DESC_MACRO_BEFORE(1)
    #define D_INTERNAL_DESC_MACRO_LAYER_1  "the type"
#elif D_INTERNAL_DESC_MACRO_AFTER(1)
    #define D_INTERNAL_DESC_MACRO_LAYER_1  ", type"
#else
    #define D_INTERNAL_DESC_MACRO_LAYER_1  " and type"
#endif

#if !D_INTERNAL_DESC_MACRO_HAS(2)
    #define D_INTERNAL_DESC_MACRO_LAYER_2
#elif !D_INTERNAL_DESC_MACRO_BEFORE(2)
    #define D_INTERNAL_DESC_MACRO_LAYER_2  "the state"
#elif D_INTERNAL_DESC_MACRO_AFTER(2)
    #define D_INTERNAL_DESC_MACRO_LAYER_2  ", state"
#else
    #define D_INTERNAL_DESC_MACRO_LAYER_2  " and state"
#endif

#if !D_INTERNAL_DESC_MACRO_HAS(3)
    #define D_INTERNAL_DESC_MACRO_LAYER_3
#elif !D_INTERNAL_DESC_MACRO_BEFORE(3)
    #define D_INTERNAL_DESC_MACRO_LAYER_3  "the intent"
#elif D_INTERNAL_DESC_MACRO_AFTER(3)
    #define D_INTERNAL_DESC_MACRO_LAYER_3  ", intent"
#else
    #define D_INTERNAL_DESC_MACRO_LAYER_3  " and intent"
#endif

#if !D_INTERNAL_DESC_MACRO_HAS(4)
    #define D_INTERNAL_DESC_MACRO_LAYER_4
#elif !D_INTERNAL_DESC_MACRO_BEFORE(4)
    #define D_INTERNAL_DESC_MACRO_LAYER_4  "the policy"
#elif D_INTERNAL_DESC_MACRO_AFTER(4)
    #define D_INTERNAL_DESC_MACRO_LAYER_4  ", policy"
#else
    #define D_INTERNAL_DESC_MACRO_LAYER_4  " and policy"
#endif

#if !D_INTERNAL_DESC_MACRO_HAS(5)
    #define D_INTERNAL_DESC_MACRO_LAYER_5
#elif !D_INTERNAL_DESC_MACRO_BEFORE(5)
    #define D_INTERNAL_DESC_MACRO_LAYER_5  "the health"
#elif D_INTERNAL_DESC_MACRO_AFTER(5)
    #define D_INTERNAL_DESC_MACRO_LAYER_5  ", health"
#else
    #define D_INTERNAL_DESC_MACRO_LAYER_5  " and health"
#endif

#if !D_INTERNAL_DESC_MACRO_HAS(6)
    #define D_INTERNAL_DESC_MACRO_LAYER_6
#elif !D_INTERNAL_DESC_MACRO_BEFORE(6)
    #define D_INTERNAL_DESC_MACRO_LAYER_6  "the provenance"
#elif D_INTERNAL_DESC_MACRO_AFTER(6)
    #define D_INTERNAL_DESC_MACRO_LAYER_6  ", provenance"
#else
    #define D_INTERNAL_DESC_MACRO_LAYER_6  " and provenance"
#endif

#if !D_INTERNAL_DESC_MACRO_HAS(7)
    #define D_INTERNAL_DESC_MACRO_LAYER_7
#elif !D_INTERNAL_DESC_MACRO_BEFORE(7)
    #define D_INTERNAL_DESC_MACRO_LAYER_7  "the contents"
#elif D_INTERNAL_DESC_MACRO_AFTER(7)
    #define D_INTERNAL_DESC_MACRO_LAYER_7  ", contents"
#else
    #define D_INTERNAL_DESC_MACRO_LAYER_7  " and contents"
#endif

#if !D_INTERNAL_DESC_MACRO_HAS(8)
    #define D_INTERNAL_DESC_MACRO_LAYER_8
#elif !D_INTERNAL_DESC_MACRO_BEFORE(8)
    #define D_INTERNAL_DESC_MACRO_LAYER_8  "the cost"
#else
    #define D_INTERNAL_DESC_MACRO_LAYER_8  " and cost"
#endif

#if (D_INTERNAL_DESC_MACRO_BITS & (D_INTERNAL_DESC_MACRO_BITS - 1)) == 0
    #define D_INTERNAL_DESC_MACRO_LAYERS_NOUN  " layer"
#else
    #define D_INTERNAL_DESC_MACRO_LAYERS_NOUN  " layers"
#endif

#if D_INTERNAL_DESC_MACRO_BITS == 0
    #define D_INTERNAL_DESC_MACRO_LAYERS  "no layer"
#elif D_INTERNAL_DESC_MACRO_BITS == 0x1FF
    #define D_INTERNAL_DESC_MACRO_LAYERS  "every layer"
#else
    #define D_INTERNAL_DESC_MACRO_LAYERS                                       \
        D_INTERNAL_DESC_MACRO_LAYER_0 D_INTERNAL_DESC_MACRO_LAYER_1            \
        D_INTERNAL_DESC_MACRO_LAYER_2 D_INTERNAL_DESC_MACRO_LAYER_3            \
        D_INTERNAL_DESC_MACRO_LAYER_4 D_INTERNAL_DESC_MACRO_LAYER_5            \
        D_INTERNAL_DESC_MACRO_LAYER_6 D_INTERNAL_DESC_MACRO_LAYER_7            \
        D_INTERNAL_DESC_MACRO_LAYER_8 D_INTERNAL_DESC_MACRO_LAYERS_NOUN
#endif

#if !D_CFG_DESC_UNITS
    #define D_INTERNAL_DESC_MACRO_UNITS                                        \
        "No units are built"
#else
    #define D_INTERNAL_DESC_MACRO_UNITS                                        \
        "Units are built to the "                                              \
        D_CONCAT(D_DESC_MACRO_RING_, D_CFG_DESC_RING_MAX)                      \
        " ring, in " D_INTERNAL_DESC_MACRO_LAYERS
#endif

#if D_CFG_DESC_FACTS
    #define D_INTERNAL_DESC_MACRO_FACTS    "; facts are derived"
#else
    #define D_INTERNAL_DESC_MACRO_FACTS    "; facts are not built"
#endif

#if D_CFG_DESC_ARTICLES
    #define D_INTERNAL_DESC_MACRO_ARTICLES "Articles are on"
#else
    #define D_INTERNAL_DESC_MACRO_ARTICLES "Articles are off"
#endif

#if D_CFG_DESC_OPTIONS
    #define D_INTERNAL_DESC_MACRO_OPTIONS  "options are read at run time"
#else
    #define D_INTERNAL_DESC_MACRO_OPTIONS  "only the knobs decide"
#endif

// 1.2
// D_DESC_MACRO_BUILD
//   constant: desc's own build, in sentences: "Units are built to the
// detailed ring, in every layer; facts are derived. Articles are on, and
// options are read at run time."
#define D_DESC_MACRO_BUILD                                                     \
    D_INTERNAL_DESC_MACRO_UNITS D_INTERNAL_DESC_MACRO_FACTS ". "               \
    D_INTERNAL_DESC_MACRO_ARTICLES ", and " D_INTERNAL_DESC_MACRO_OPTIONS "."

#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_MACRO


//==============================================================================
// 2.  THE KIT
//==============================================================================

// 2.1
// D_DESC_MACRO_KIT
//   constant: 1 when the kit is built: desc and its macro knob are on, and
// the language has variadic macros: C99, or C++11. A C++98 compiler that has
// them as an extension still warns of each under -Wpedantic, so the kit
// goes by the language, not the compiler.
#if D_CFG_DESC_ENABLED && D_CFG_DESC_MACRO &&                                  \
    ( D_ENV_LANG_IS_CPP11_OR_HIGHER ||                                         \
      (!defined(__cplusplus) && D_ENV_LANG_IS_C99_OR_HIGHER) )
    #define D_DESC_MACRO_KIT 1
#else
    #define D_DESC_MACRO_KIT 0
#endif

#if D_DESC_MACRO_KIT

// dmacro.h's tools, which need variadic macros
#include "../dmacro.h"         // D_IIF, D_CHECK, D_PROBE

// 2.2
// D_DESC_MACRO_WHEN
//   macro: words when a bit, 0 or 1, is 1; else nothing. A bit a knob
// decides is made with #if D_DESC_BUILT(ring, layer).
#define D_DESC_MACRO_WHEN(bit, words)  D_IIF(bit)(words, )

// 2.3
// D_DESC_MACRO_IS_ZERO, D_DESC_MACRO_IS_ONE
//   macros: 1 when a plain number (64, not 64u) is 0, or 1; else 0. A probe
// the number names gives 1; any other, 0.
#define D_DESC_MACRO_IS_ZERO(n)                                                \
    D_CHECK(D_CONCAT(D_INTERNAL_DESC_MACRO_ZERO_, n))
#define D_DESC_MACRO_IS_ONE(n)                                                 \
    D_CHECK(D_CONCAT(D_INTERNAL_DESC_MACRO_ONE_, n))
#define D_INTERNAL_DESC_MACRO_ZERO_0  D_PROBE(~)
#define D_INTERNAL_DESC_MACRO_ONE_1   D_PROBE(~)

// 2.4
// D_DESC_MACRO_PLURAL
//   macro: the "s" a count of a plain number takes: "" for 1, "s" else.
#define D_DESC_MACRO_PLURAL(n)                                                 \
    D_IIF(D_DESC_MACRO_IS_ONE(n))("", "s")

// 2.5
// D_DESC_MACRO_SENTENCE
//   macro: a description, its parts given in the order a person says them:
//     condition    how it is now:      (an, "empty")        or nothing
//     size         its size:           (a, "fixed-size")    or nothing
//     quality      its qualities:      (an, "immutable")    or nothing
//     shape        its arrangement:    (an, "ordered")      or nothing
//     origin       its origin, style:  (a, "Windows-style") or nothing
//     noun         what it is:         (a, "file tree"), always said
//     complements  what completes it:  ("mapping int to int") or nothing
//     remarks      sentences after it: ("It holds at most 8 keys.") or
//                                      nothing
//   An adjective part may hold several words, (a, "x")(a, "y"). The
// determiner, "a" or "an", is the first word's own.
#define D_DESC_MACRO_SENTENCE(condition, size, quality, shape, origin, noun,   \
                              complements, remarks)                            \
    D_INTERNAL_DESC_MACRO_JOIN(D_INTERNAL_DESC_MACRO_KIND(condition)           \
                               D_INTERNAL_DESC_MACRO_KIND(size)                \
                               D_INTERNAL_DESC_MACRO_KIND(quality)             \
                               D_INTERNAL_DESC_MACRO_KIND(shape)               \
                               D_INTERNAL_DESC_MACRO_KIND(origin)              \
                               D_INTERNAL_DESC_MACRO_NOUN noun)                \
    D_INTERNAL_DESC_MACRO_COMPLEMENTS(complements)                             \
    D_INTERNAL_DESC_MACRO_REMARKS(remarks)

// 2.6
// D_DESC_MACRO_NAME
//   macro: the name ring: the noun and what completes it, with no
// determiner: "binary radix tree mapping uint32_t to int".
#define D_DESC_MACRO_NAME(noun, complements)                                   \
    D_INTERNAL_DESC_MACRO_TEXT noun                                            \
    D_INTERNAL_DESC_MACRO_COMPLEMENTS(complements)

// a kind's words, each marked with what goes before it: a space before the
// kind's first word, a comma before the others
#define D_INTERNAL_DESC_MACRO_KIND(words)                                      \
    D_CONCAT(D_INTERNAL_DESC_MACRO_KIND_HEAD words, _END)
#define D_INTERNAL_DESC_MACRO_KIND_HEAD(art, text)                             \
    (space, art, text) D_INTERNAL_DESC_MACRO_KIND_A
#define D_INTERNAL_DESC_MACRO_KIND_A(art, text)                                \
    (comma, art, text) D_INTERNAL_DESC_MACRO_KIND_B
#define D_INTERNAL_DESC_MACRO_KIND_B(art, text)                                \
    (comma, art, text) D_INTERNAL_DESC_MACRO_KIND_A
#define D_INTERNAL_DESC_MACRO_KIND_HEAD_END
#define D_INTERNAL_DESC_MACRO_KIND_A_END
#define D_INTERNAL_DESC_MACRO_KIND_B_END
#define D_INTERNAL_DESC_MACRO_NOUN(art, text)  (space, art, text)
#define D_INTERNAL_DESC_MACRO_TEXT(art, text)  text

// the determiner, then each word after its mark
#define D_INTERNAL_DESC_MACRO_JOIN(words)                                      \
    D_CONCAT(D_INTERNAL_DESC_MACRO_JOIN_HEAD words, _END)
#define D_INTERNAL_DESC_MACRO_JOIN_HEAD(mark, art, text)                       \
    D_INTERNAL_DESC_MACRO_DETERMINER(art) text D_INTERNAL_DESC_MACRO_JOIN_A
#define D_INTERNAL_DESC_MACRO_JOIN_A(mark, art, text)                          \
    D_INTERNAL_DESC_MACRO_MARK_##mark text D_INTERNAL_DESC_MACRO_JOIN_B
#define D_INTERNAL_DESC_MACRO_JOIN_B(mark, art, text)                          \
    D_INTERNAL_DESC_MACRO_MARK_##mark text D_INTERNAL_DESC_MACRO_JOIN_A
#define D_INTERNAL_DESC_MACRO_JOIN_HEAD_END
#define D_INTERNAL_DESC_MACRO_JOIN_A_END
#define D_INTERNAL_DESC_MACRO_JOIN_B_END
#define D_INTERNAL_DESC_MACRO_MARK_space  " "
#define D_INTERNAL_DESC_MACRO_MARK_comma  ", "
#define D_INTERNAL_DESC_MACRO_ART_a       "a"
#define D_INTERNAL_DESC_MACRO_ART_an      "an"

#if D_CFG_DESC_ARTICLES
    #define D_INTERNAL_DESC_MACRO_DETERMINER(art)                              \
        D_CONCAT(D_INTERNAL_DESC_MACRO_ART_, art) " "
#else
    #define D_INTERNAL_DESC_MACRO_DETERMINER(art)
#endif

// complements follow the noun, with commas between them
#define D_INTERNAL_DESC_MACRO_COMPLEMENTS(texts)                               \
    D_CONCAT(D_INTERNAL_DESC_MACRO_COMPLEMENT_HEAD texts, _END)
#define D_INTERNAL_DESC_MACRO_COMPLEMENT_HEAD(text)                            \
    " " text D_INTERNAL_DESC_MACRO_COMPLEMENT_A
#define D_INTERNAL_DESC_MACRO_COMPLEMENT_A(text)                               \
    ", " text D_INTERNAL_DESC_MACRO_COMPLEMENT_B
#define D_INTERNAL_DESC_MACRO_COMPLEMENT_B(text)                               \
    ", " text D_INTERNAL_DESC_MACRO_COMPLEMENT_A
#define D_INTERNAL_DESC_MACRO_COMPLEMENT_HEAD_END
#define D_INTERNAL_DESC_MACRO_COMPLEMENT_A_END
#define D_INTERNAL_DESC_MACRO_COMPLEMENT_B_END

// remarks follow the rest, each a sentence of its own
#define D_INTERNAL_DESC_MACRO_REMARKS(texts)                                   \
    D_CONCAT(D_INTERNAL_DESC_MACRO_REMARK_HEAD texts, _END)
#define D_INTERNAL_DESC_MACRO_REMARK_HEAD(text)                                \
    ". " text D_INTERNAL_DESC_MACRO_REMARK_A
#define D_INTERNAL_DESC_MACRO_REMARK_A(text)                                   \
    " " text D_INTERNAL_DESC_MACRO_REMARK_B
#define D_INTERNAL_DESC_MACRO_REMARK_B(text)                                   \
    " " text D_INTERNAL_DESC_MACRO_REMARK_A
#define D_INTERNAL_DESC_MACRO_REMARK_HEAD_END
#define D_INTERNAL_DESC_MACRO_REMARK_A_END
#define D_INTERNAL_DESC_MACRO_REMARK_B_END

#endif  // D_DESC_MACRO_KIT


#endif  // DJINTERP_C_DESC_MACRO_H
