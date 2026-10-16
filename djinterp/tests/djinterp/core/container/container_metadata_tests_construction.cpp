/*******************************************************************************
* djinterp [core]                      container_metadata_tests_construction.cpp
*
*
* path:      /tests/djinterp/core/container/container_metadata_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "container_metadata_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_container_metadata_construction
  All six constructors produce the state they advertise.
  Tests the following:
  - the default constructor leaves the object UNBOUND with a
    value-initialised payload, so a default-constructed instance is usable
    rather than merely declared
  - the two payload-only constructors take a payload and leave the object
    unbound, in copy and in move form
  - the two container-and-payload constructors bind and store together
  - the container-only constructor binds and value-initialises the payload
  - the payload-only and container-only forms are explicit, so neither can
    be selected by an implicit conversion; that is checked with
    is_convertible against is_constructible, which separates "can be built
    from" from "converts from"
  - the move-taking forms are noexcept exactly when the payload's move is,
    which is what the conditional noexcept on them claims
*/
bool
tests_container_metadata_construction()
{
    cm_box box;
    box.n = 11;

    // default: unbound, payload value-initialised
    {
        cm_meta meta;
        D_CM_CHECK(!meta.is_bound());
        D_CM_CHECK(meta.get_container() == nullptr);
        D_CM_CHECK(meta.payload().v == 0);
    }

    // payload only, by copy: holds the payload, still unbound
    {
        const cm_note note{5};
        cm_meta       meta(note);
        D_CM_CHECK(!meta.is_bound());
        D_CM_CHECK(meta.payload().v == 5);
    }

    // payload only, by move
    {
        cm_meta meta(cm_note{6});
        D_CM_CHECK(!meta.is_bound());
        D_CM_CHECK(meta.payload().v == 6);
    }

    // container and payload, by copy
    {
        const cm_note note{7};
        cm_meta       meta(box, note);
        D_CM_CHECK(meta.is_bound());
        D_CM_CHECK(meta.get_container() == &box);
        D_CM_CHECK(meta.payload().v == 7);
    }

    // container and payload, by move
    {
        cm_meta meta(box, cm_note{8});
        D_CM_CHECK(meta.is_bound());
        D_CM_CHECK(meta.get_container() == &box);
        D_CM_CHECK(meta.payload().v == 8);
    }

    // container only: bound, payload value-initialised
    {
        cm_meta meta(box);
        D_CM_CHECK(meta.is_bound());
        D_CM_CHECK(meta.get_container() == &box);
        D_CM_CHECK(meta.payload().v == 0);
    }

    // the single-argument forms are explicit
    D_CM_CHECK((std::is_constructible<cm_meta, const cm_note&>::value));
    D_CM_CHECK((!std::is_convertible<const cm_note&, cm_meta>::value));
    D_CM_CHECK((std::is_constructible<cm_meta, cm_note&&>::value));
    D_CM_CHECK((!std::is_convertible<cm_note&&, cm_meta>::value));
    D_CM_CHECK((std::is_constructible<cm_meta, cm_box&>::value));
    D_CM_CHECK((!std::is_convertible<cm_box&, cm_meta>::value));

    // the move-taking forms are noexcept exactly when the payload is
    D_CM_CHECK(noexcept(cm_meta(cm_note{})));
    D_CM_CHECK(noexcept(cm_meta(box, cm_note{})));
    D_CM_CHECK(
     std::is_nothrow_move_constructible<cm_note>::value ==
     noexcept(cm_meta(cm_note{})));

    // ... and not noexcept when the payload's move can throw
    {
        using throwing_meta = container_metadata<cm_box, cm_throwing>;
        D_CM_CHECK(!noexcept(throwing_meta(cm_throwing{})));
        D_CM_CHECK(!noexcept(throwing_meta(box, cm_throwing{})));
        D_CM_CHECK(
         std::is_nothrow_move_constructible<cm_throwing>::value ==
         noexcept(throwing_meta(cm_throwing{})));
    }

    return true;
}

/*
tests_container_metadata_construction_hazards
  Two places where construction does not behave as it reads.
  Tests the following:
  - std::is_default_constructible reports TRUE for a specialisation whose
    payload has no default constructor, and default construction of that
    specialisation then FAILS TO COMPILE.  The trait inspects only the
    constructor's declaration, which is unconstrained; the failure occurs
    in the mem-initialiser `m_payload()` inside the definition, which is
    not part of the immediate context and therefore not SFINAE-visible.
    No probe can detect this - that IS the defect - so the assertion here
    records the trait's answer and this comment records the truth.  The
    container-only constructor has the identical shape and the identical
    problem.  Making either SFINAE-friendly would need a constraint on the
    payload being default-constructible
  - when the container and payload types are the SAME, the single-argument
    overloads do not become ambiguous - they silently diverge.  A non-const
    lvalue binds the CONTAINER, because `_Container&` is an exact match and
    beats the qualification conversion to `const _Payload&`; a const lvalue
    can only bind the payload overload, and so is copied into the payload.
    Two calls that read alike do entirely different things, and a caller
    gets no diagnostic either way
*/
bool
tests_container_metadata_construction_hazards()
{
    // --- the default-constructibility false positive ---
    {
        using awkward = container_metadata<cm_box, cm_no_default>;

        // the payload genuinely has no default constructor
        D_CM_CHECK(!std::is_default_constructible<cm_no_default>::value);

        // ... and yet the trait reports the wrapper as default
        // constructible.  Writing `awkward meta;` here would not compile,
        // so the disagreement cannot be asserted directly
        D_CM_CHECK(std::is_default_constructible<awkward>::value);

        // the constructors that DO work for such a payload
        cm_box  box;
        awkward from_payload(cm_no_default{3});
        awkward from_both(box, cm_no_default{4});
        D_CM_CHECK(from_payload.payload().v == 3);
        D_CM_CHECK(from_both.payload().v == 4);
        D_CM_CHECK(from_both.is_bound());

        // the container-only form has the same trap: the trait admits it
        // while the definition would not compile
        D_CM_CHECK((std::is_constructible<awkward, cm_box&>::value));
    }

    // --- same container and payload type ---
    {
        using same = container_metadata<cm_box, cm_box>;

        cm_box       thing;
        const cm_box frozen{99};
        thing.n = 42;

        // a NON-const lvalue selects the container overload: bound, and
        // the payload is left value-initialised
        same as_container(thing);
        D_CM_CHECK(as_container.is_bound());
        D_CM_CHECK(as_container.get_container() == &thing);
        D_CM_CHECK(as_container.payload().n == 0);

        // a CONST lvalue cannot bind `_Container&`, so it is copied into
        // the payload instead - unbound, payload populated
        same as_payload(frozen);
        D_CM_CHECK(!as_payload.is_bound());
        D_CM_CHECK(as_payload.get_container() == nullptr);
        D_CM_CHECK(as_payload.payload().n == 99);

        // the two spellings differ in every observable
        D_CM_CHECK(as_container.is_bound() != as_payload.is_bound());
        D_CM_CHECK(
         as_container.payload().n != as_payload.payload().n);
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
