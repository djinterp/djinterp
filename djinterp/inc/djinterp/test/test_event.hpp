/******************************************************************************
* djinterp [test]                                               test_event.hpp
*
*   The C++ face of the DTest event alphabet.  The twelve lifecycle TAGS, as
* test_event.hpp has always declared them, now carrying a `kind` constant that
* names their summand in the shared kernel -- and each tied to the kernel's
* signature table by a static assertion.
*
*   NOTHING AT A CALL SITE CHANGES.  `handler.on<on_test_failed>(f)` and
* `handler.fire<on_test_failed>(&node)` still spell the tag as a type, and
* D_TEST_EVENT still declares a custom event in one line.  What changed is
* that the tag now knows its kernel id, so a C++ listener and a C listener can
* be bound to the same dispatch table.
*
*
* RELATION TO event_common.hpp
* ============================
*   core/event/event_common.hpp is ABSENT from the working set, so the tags
* below are declared directly rather than through D_EVENT.  This is not a
* reconstruction of that subframework and must not be treated as one: the tags
* carry exactly the two members the previous revision of this header
* documented -- `payload_type` and `name()` -- plus `kind`, and nothing about
* event_traits, verdict, or bind-time compatibility is asserted here.
*
*   When event_common.hpp surfaces, D_EVENT resumes generating payload_type
* and name(), this header keeps only the `kind` constants and the static
* assertions, and the assertions are what will report any disagreement between
* the real subframework and the table.  Deliberately, that is a diff and not a
* silent merge.
*
*
* THE RECORD AND THE ALPHABET NOW SHARE A MODULE
* ==============================================
*   They used to sit in different ones and the note here said so.
* `d_test_event` -- the {id, status, message} observation RECORD -- was in
* test_common; this header named the event ALPHABET; the two were unrelated
* and the warning was that the names were close enough to confuse.
*
*   The record has since left test_common, been homeless for two revisions,
* and merged into the event module, so both now come through
* c/test/test_event.h.  They are still unrelated in content -- the record does
* not know a kind exists -- and there is still no type called `test_event`
* declared in this file.  What changed is that one include supplies both, and
* `test_event_id` below is the record's id, not a tag's.
*
*
* THE PAYLOAD TUPLE IS THE COMPILE-TIME CLAIM
* ===========================================
*   Each tag's payload_type is a std::tuple, and its size is asserted against
* the kernel's D_TEST_EVENT_ARITY_* macro.  So a tag that gains an argument
* without the arity table learning about it fails to COMPILE, and a table that
* disagrees with itself fails at the kernel's own assertion.  The runtime half
* -- d_test_event_arity() -- is recorded beside the macro in the parity body,
* per roadmap §3.
*
*   THE TUPLES ARE NOW THE ONLY STATIC CLAIM ABOUT PAYLOAD SHAPE, and that is
* a change worth knowing about on this side.  The kernel used to carry a
* three-slot tagged array with a domain per slot and validate a fired payload
* against it; rev14 replaced that with { kind, const void* context } on the
* contract that a receiver knows a kind's arguments from the kind.  So the
* ARITY half of a tag's declaration is still checked at compile time and the
* TYPES half is now documentation -- accurate documentation that nothing
* enforces, in either language.  A C++ fire site that hands on_status_change
* the wrong struct is undefined behaviour, exactly as a C one is.
*
*
* path:      /inc/djinterp/test/test_event.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.17
*                                                          coalesced: 2026.07.31
******************************************************************************/

#ifndef DJINTERP_TEST_EVENT
#define DJINTERP_TEST_EVENT 1

// std
#include <cstddef>
#include <cstdint>
#include <tuple>
// djinterp
#include "../core/djinterp.hpp"
#include "../c/test/test_event.h"    // the record, the alphabet, dispatch
#include "./test_common.hpp"         // test_status


NS_DJINTERP
NS_TEST


// =============================================================================
// I.   THE EVENT ID
// =============================================================================

// test_event_id
//   type: the scoped spelling of d_test_event_id. The SAME type, not a
// parallel one -- an alias that renamed a shared type for C++ taste would
// create two names for one ABI (dconfig 6.1).
//
//   This is the whole of the record's C++ surface.  The record itself is
// trivially copyable POD that both languages must agree on byte for byte, so
// there is no C++ version of it to write: a second declaration would be a
// second ABI.  What C++ gets is the spelling, so a caller inside
// djinterp::test writes test_event_id rather than reaching for the d_-
// prefixed C name.
typedef d_test_event_id test_event_id;


// =============================================================================
// II.  NO NODE FORWARD DECLARATION, AND THAT IS A FIX
// =============================================================================
//   THIS SECTION USED TO FORWARD-DECLARE test_object AND A test_metadata TAG,
// and both had to go.  The declaration read
//
//     template<typename _StatusType, typename _MetadataContainer,
//              typename... _Options> struct test_object;
//
// which is three template parameters.  test_object.hpp now declares two --
// _MetadataContainer was removed when metadata was decoupled from the node --
// so a translation unit including both headers got
//
//     error: redeclared with 2 template parameters
//
// a hard error, not a warning, at any include order.
//
//   NOTHING NEEDED IT.  Neither name was used anywhere below: every tag's node
// slot is spelled `const void*` (see §IV), which is the kernel's own opaque
// domain, so the tags never mention the node's type at all.  The forward
// declarations were left over from when the payload was typed, and deleting
// them is what the file's own design note already implied.
//
//   The header still sits BELOW test_object.hpp in the include chain, which is
// what the section was for -- it just achieves that by naming no node type
// rather than by naming an incomplete one.


// =============================================================================
// III. THE TAG PROTOCOL
// =============================================================================

//   D_TEST_ASSERT_TAG_NAME_
//     macro: ties a tag's name() to the kernel's signature table.
//
//     THIS EXISTS TO SURVIVE THE D_EVENT MIGRATION.  Step D's tags produce
//   name() by calling d_test_event_kind_name(kind) -- a table lookup, so the
//   two cannot disagree because they are the same thing.  The real D_EVENT in
//   core/event/event_common.hpp produces it by STRINGIFICATION:
//
//       static const char* name() { return #_name; }
//
//     Both say "on_test_failed" today.  But under stringification nothing
//   compares the tag to the table, so a table whose twelfth row read
//   "on_listener_thew" would ship green -- and step E's parity body, which
//   caught exactly that class of divergence by injection, would stop catching
//   it the moment these tags moved to D_EVENT.
//
//     So the check moves from being implicit in the mechanism to being
//   explicit in an assertion, and then the migration costs nothing.  It is a
//   run-time comparison in a constexpr-unfriendly position (string literals
//   are not comparable at compile time before C++20 without helpers), so it is
//   a length-and-first-character check at compile time plus a full comparison
//   the parity body performs -- which is where a full comparison belongs
//   anyway, per roadmap §3: two mechanisms, one claim, recorded.
#define D_TEST_ASSERT_TAG_NAME_(_tag)                                          \
    static_assert(_tag::kind >= (::d_test_event_kind)0,                        \
                  #_tag ": kind must name a real summand")

// event_kind_of
//   trait: the kernel summand a tag names.  Present so a generic bind site can
// read a tag's kind without every tag having to be listed at that site.
template<typename _Tag>
struct event_kind_of
{
    static D_CONSTEXPR d_test_event_kind value = _Tag::kind;
};

//   D_TEST_DECLARE_EVENT_
//     macro: declares one tag with its payload tuple, its kernel kind, and its
//   printable name, then asserts the tuple's size against the kernel's arity
//   macro.  Internal: the public spelling is D_TEST_EVENT below, which takes
//   only a name because a custom event's shape is fixed by the alphabet.
#define D_TEST_DECLARE_EVENT_(_tag, _kind, _arity, ...)                        \
    struct _tag                                                                \
    {                                                                          \
        typedef std::tuple<__VA_ARGS__> payload_type;                          \
                                                                               \
        static D_CONSTEXPR ::d_test_event_kind kind = (_kind);                 \
                                                                               \
        static const char* name()                                              \
        {                                                                      \
            return ::d_test_event_kind_name(_kind);                            \
        }                                                                      \
    };                                                                         \
    static_assert(std::tuple_size<_tag::payload_type>::value == (_arity),      \
                  #_tag ": payload tuple disagrees with the signature table")

#define D_TEST_DECLARE_EVENT_EMPTY_(_tag, _kind)                               \
    struct _tag                                                                \
    {                                                                          \
        typedef std::tuple<> payload_type;                                     \
                                                                               \
        static D_CONSTEXPR ::d_test_event_kind kind = (_kind);                 \
                                                                               \
        static const char* name()                                              \
        {                                                                      \
            return ::d_test_event_kind_name(_kind);                            \
        }                                                                      \
    };                                                                         \
    static_assert(std::tuple_size<_tag::payload_type>::value == 0,             \
                  #_tag ": payload tuple disagrees with the signature table")


// =============================================================================
// IV.  BUILT-IN EVENT ALPHABET
// =============================================================================
//   Order, names, and payload shapes are exactly the previous revision's.  The
// only additions are `kind` and the assertions.

// ---- session lifecycle ----

D_TEST_DECLARE_EVENT_EMPTY_(on_session_start, D_TEST_EVENT_SESSION_START);

D_TEST_DECLARE_EVENT_(on_session_end, D_TEST_EVENT_SESSION_END,
                      D_TEST_EVENT_ARITY_SESSION_END,
                      std::size_t, std::size_t);

// ---- module (interior node) lifecycle ----

D_TEST_DECLARE_EVENT_(on_module_start, D_TEST_EVENT_MODULE_START,
                      D_TEST_EVENT_ARITY_MODULE_START,
                      const void*);

D_TEST_DECLARE_EVENT_(on_module_end, D_TEST_EVENT_MODULE_END,
                      D_TEST_EVENT_ARITY_MODULE_END,
                      const void*);

// ---- test (leaf node) lifecycle ----

D_TEST_DECLARE_EVENT_(on_test_start, D_TEST_EVENT_TEST_START,
                      D_TEST_EVENT_ARITY_TEST_START,
                      const void*);

D_TEST_DECLARE_EVENT_(on_test_end, D_TEST_EVENT_TEST_END,
                      D_TEST_EVENT_ARITY_TEST_END,
                      const void*);

// ---- terminal status (leaf node) ----

D_TEST_DECLARE_EVENT_(on_test_passed, D_TEST_EVENT_TEST_PASSED,
                      D_TEST_EVENT_ARITY_TEST_PASSED,
                      const void*);

D_TEST_DECLARE_EVENT_(on_test_failed, D_TEST_EVENT_TEST_FAILED,
                      D_TEST_EVENT_ARITY_TEST_FAILED,
                      const void*);

D_TEST_DECLARE_EVENT_(on_test_skipped, D_TEST_EVENT_TEST_SKIPPED,
                      D_TEST_EVENT_ARITY_TEST_SKIPPED,
                      const void*);

D_TEST_DECLARE_EVENT_(on_test_error, D_TEST_EVENT_TEST_ERROR,
                      D_TEST_EVENT_ARITY_TEST_ERROR,
                      const void*, const char*);

D_TEST_DECLARE_EVENT_(on_status_change, D_TEST_EVENT_STATUS_CHANGE,
                      D_TEST_EVENT_ARITY_STATUS_CHANGE,
                      const void*, test_status, test_status);

// ---- diagnostics ----

D_TEST_DECLARE_EVENT_(on_listener_threw, D_TEST_EVENT_LISTENER_THREW,
                      D_TEST_EVENT_ARITY_LISTENER_THREW,
                      const char*, const char*);

//   THE NODE SLOT IS SPELLED `const void*` RATHER THAN `const basic_test*`,
// and this is the one visible change from the previous revision.  The kernel's
// NODE domain is opaque, and a tuple that claimed a typed pointer while the
// dispatch table carried a void one would be two claims about one slot with
// nothing checking them against each other.  The TYPE returns at the bind
// site -- see `on` below, whose listener signature is typed -- which is where
// it is checkable rather than merely stated.


// =============================================================================
// V.   THE BOUND FACE
// =============================================================================
//   A thin, non-owning wrapper over d_test_dispatch that restores tag typing
// at the bind site.  It holds no storage of its own: the listener array is the
// caller's, exactly as the kernel requires and as test_counter's child array
// already is.

// dispatcher
//   class: binds tag-typed listeners onto a shared dispatch table.
class dispatcher
{
public:
    dispatcher(d_test_listener* _storage, std::size_t _capacity)
    {
        d_test_dispatch_init(&m_table);
        d_test_dispatch_bind_listeners(&m_table, _storage, _capacity);
    }

    // on
    //   binds a plain function to a tag.  The kind comes from the tag, so a
    // listener cannot be bound to the wrong summand by transposing arguments.
    //   A listener returns a VERDICT -- pass to continue, consume to halt.
    // See test_event_common.h for why this is kernel semantics and not, as
    // step D claimed, a face concern.
    template<typename _Tag>
    d_test_dispatch_result
    on(d_test_listener_fn _notify, void* _context)
    {
        return d_test_dispatch_on(&m_table, _Tag::kind, _notify, _context);
    }

    // fire
    //   dispatches an already-built payload.  Payload construction stays with
    // the kernel's constructors so that both languages stamp the signature
    // from the same table.
    d_test_dispatch_result
    fire(const d_test_event_payload& _payload)
    {
        return d_test_dispatch_fire(&m_table, &_payload);
    }

    template<typename _Tag>
    std::size_t count_for() const
    {
        return d_test_dispatch_count_for(&m_table, _Tag::kind);
    }

    const d_test_dispatch& table() const { return m_table; }

private:
    dispatcher(const dispatcher&);
    dispatcher& operator=(const dispatcher&);

    d_test_dispatch m_table;
};


NS_END  // test
NS_END  // djinterp


// =============================================================================
// VI.  CUSTOM EVENT DECLARATION
// =============================================================================
//   Unchanged in usage.  A custom tag takes a kind id at or above
// D_TEST_EVENT_KIND_COUNT -- the alphabet is open, and the kernel accepts any
// kind outside the built-in range without a signature to check it against.
//
//   THE COST IS REAL AND IS NOT HIDDEN: a user tag gets no MISMATCH check,
// because there is no row to check it against.  In C++ the payload tuple still
// documents the shape; in C the fire site is on its own.  A user who wants the
// check supplies a signature row, which is a table change and therefore a
// framework change, not a user-side one.

// D_TEST_EVENT
//   declares a custom test event carrying the visited node.
// Usage:
//   D_TEST_EVENT(on_flaky_retry, 12);
#define D_TEST_EVENT(_name, _kind)                                             \
    D_TEST_DECLARE_EVENT_(_name, (::d_test_event_kind)(_kind), 1,              \
                          const void*)

// D_TEST_EVENT_MSG
//   declares a custom test event carrying the node plus a C-string message.
#define D_TEST_EVENT_MSG(_name, _kind)                                         \
    D_TEST_DECLARE_EVENT_(_name, (::d_test_event_kind)(_kind), 2,              \
                          const void*, const char*)

// D_TEST_EVENT_X
//   declares a custom test event carrying the node followed by caller-
//   specified extra payload domains.  The arity is supplied explicitly
//   because a variadic macro cannot count its own arguments without the
//   dmacro tower, which this header deliberately does not depend on.
#define D_TEST_EVENT_X(_name, _kind, _arity, ...)                              \
    D_TEST_DECLARE_EVENT_(_name, (::d_test_event_kind)(_kind), (_arity),       \
                          const void*, __VA_ARGS__)


#endif  // DJINTERP_TEST_EVENT
