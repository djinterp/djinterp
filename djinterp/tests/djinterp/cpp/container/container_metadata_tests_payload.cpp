/*******************************************************************************
* djinterp [cpp]                            container_metadata_tests_payload.cpp
*
*
* path:      /tests/djinterp/cpp/container/container_metadata_tests_payload.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "container_metadata_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_container_metadata_payload
  The two payload() overloads.
  Tests the following:
  - the non-const overload returns a mutable reference, and a write
    through it is visible on the object afterwards - so it is a handle on
    the stored payload rather than a copy of it
  - the const overload returns a const reference, so a const wrapper
    cannot be used to modify what it holds
  - the reference is to the payload actually stored, checked by address
    rather than by value, since equal values would not distinguish a
    reference from a copy
  - the payload is owned: it survives the container being rebound or
    unbound, which is what separates the owned half of this type from the
    borrowed half
*/
bool tests_container_metadata_payload()
{
    DCmBox  box;
    cm_meta meta(box, DCmNote{10});

    // reading through both overloads
    D_CM_CHECK(meta.payload().v == 10);

    {
        const cm_meta& frozen = meta;
        D_CM_CHECK(frozen.payload().v == 10);
    }

    // the non-const overload is a mutable handle on the stored payload
    meta.payload().v = 20;
    D_CM_CHECK(meta.payload().v == 20);

    // and it really is the stored object, not a copy
    D_CM_CHECK(&meta.payload() == &meta.payload());

    // return types: mutable through a mutable wrapper, const through a
    // const one
    D_CM_CHECK(
     !std::is_const<std::remove_reference_t<
         decltype(meta.payload())>>::value);
    D_CM_CHECK(
     std::is_const<std::remove_reference_t<
         decltype(std::declval<const cm_meta&>().payload())>>::value);
    D_CM_CHECK(
     std::is_reference<decltype(meta.payload())>::value);

    // the payload is owned, and outlives changes to the binding
    meta.bind(box);
    D_CM_CHECK(meta.payload().v == 20);
    meta.unbind();
    D_CM_CHECK(meta.payload().v == 20);

    return true;
}

/*
tests_container_metadata_set_payload
  Replacing the payload, by copy and by move.
  Tests the following:
  - the copy overload leaves the argument intact and installs its value
  - the move overload takes an rvalue and moves rather than copies, which
    is measured with a counting payload rather than assumed from the
    signature
  - an lvalue argument selects the copy overload and an rvalue the move
    overload, so the two are distinguished by value category and not by
    the caller remembering which to call
  - replacing the payload does not disturb the binding, the other half of
    the object's state
*/
bool tests_container_metadata_set_payload()
{
    DCmBox box;

    // copy: the source survives with its value
    {
        cm_meta       meta(box, DCmNote{1});
        const DCmNote replacement{55};

        meta.set_payload(replacement);
        D_CM_CHECK(meta.payload().v == 55);
        D_CM_CHECK(replacement.v == 55);
        D_CM_CHECK(meta.is_bound());
    }

    // move: an rvalue is moved, not copied
    {
        using counted_meta = DContainerMetadata<DCmBox, DCmCounted>;
        counted_meta meta(box, DCmCounted{1});

        DCmCounted::reset();
        meta.set_payload(DCmCounted{77});
        D_CM_CHECK(DCmCounted::moves == 1);
        D_CM_CHECK(DCmCounted::copies == 0);
        D_CM_CHECK(meta.payload().v == 77);
    }

    // lvalue: an lvalue is copied, not moved
    {
        using counted_meta = DContainerMetadata<DCmBox, DCmCounted>;
        counted_meta meta(box, DCmCounted{1});
        DCmCounted   source{88};

        DCmCounted::reset();
        meta.set_payload(source);
        D_CM_CHECK(DCmCounted::copies == 1);
        D_CM_CHECK(DCmCounted::moves == 0);
        D_CM_CHECK(meta.payload().v == 88);
        D_CM_CHECK(source.v == 88);
    }

    // an explicit move on an lvalue reaches the move overload
    {
        using counted_meta = DContainerMetadata<DCmBox, DCmCounted>;
        counted_meta meta(box, DCmCounted{1});
        DCmCounted   source{99};

        DCmCounted::reset();
        meta.set_payload(std::move(source));
        D_CM_CHECK(DCmCounted::moves == 1);
        D_CM_CHECK(DCmCounted::copies == 0);
        D_CM_CHECK(meta.payload().v == 99);
    }

    // the binding is untouched by either overload
    {
        cm_meta meta(box, DCmNote{1});
        meta.set_payload(DCmNote{2});
        D_CM_CHECK(meta.is_bound());
        D_CM_CHECK(meta.get_container() == &box);
    }

    return true;
}

/*
tests_container_metadata_comparison
  Equality compares payloads and ignores the container entirely.
  Tests the following:
  - two objects bound to DIFFERENT containers with equal payloads compare
    EQUAL.  This is the module's most surprising behaviour and it is
    deliberate: the container half is a borrowed pointer, and a non-owning
    reference has no value identity to compare.  A caller who expects
    equality to mean "the same association" will be wrong
  - an unbound object equals a bound one whose payload matches, which is
    the same rule seen from the other side
  - the same container with different payloads compares UNEQUAL, so the
    payload really is what is being compared
  - operator!= is the negation of operator== on every case above rather
    than a separately-written and separately-wrong predicate
*/
bool tests_container_metadata_comparison()
{
    DCmBox first;
    DCmBox second;

    // different containers, equal payloads: EQUAL
    {
        const cm_meta a(first,  DCmNote{5});
        const cm_meta b(second, DCmNote{5});
        D_CM_CHECK(a.get_container() != b.get_container());
        D_CM_CHECK(a == b);
        D_CM_CHECK(!(a != b));
    }

    // unbound versus bound, equal payloads: EQUAL
    {
        const cm_meta unbound(DCmNote{5});
        const cm_meta bound(first, DCmNote{5});
        D_CM_CHECK(unbound.is_bound() != bound.is_bound());
        D_CM_CHECK(unbound == bound);
        D_CM_CHECK(!(unbound != bound));
    }

    // same container, different payloads: UNEQUAL
    {
        const cm_meta a(first, DCmNote{5});
        const cm_meta b(first, DCmNote{6});
        D_CM_CHECK(a.get_container() == b.get_container());
        D_CM_CHECK(!(a == b));
        D_CM_CHECK(a != b);
    }

    // both unbound, equal payloads: EQUAL
    {
        const cm_meta a(DCmNote{9});
        const cm_meta b(DCmNote{9});
        D_CM_CHECK(a == b);
    }

    // an object equals itself
    {
        const cm_meta a(first, DCmNote{3});
        D_CM_CHECK(a == a);
        D_CM_CHECK(!(a != a));
    }

    // changing only the payload changes the answer; changing only the
    // binding does not
    {
        cm_meta       a(first, DCmNote{4});
        const cm_meta b(first, DCmNote{4});

        D_CM_CHECK(a == b);
        a.bind(second);
        D_CM_CHECK(a == b);
        a.unbind();
        D_CM_CHECK(a == b);
        a.set_payload(DCmNote{5});
        D_CM_CHECK(a != b);
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
