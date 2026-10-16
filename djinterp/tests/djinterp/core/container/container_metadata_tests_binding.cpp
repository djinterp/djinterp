/*******************************************************************************
* djinterp [core]                           container_metadata_tests_binding.cpp
*
*
* path:      /tests/djinterp/core/container/container_metadata_tests_binding.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "container_metadata_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_container_metadata_binding
  bind, unbind, is_bound and get_container as a lifecycle.
  Tests the following:
  - bind() attaches a container and is_bound() reports it; get_container()
    returns that exact address rather than a copy
  - bind() on an already-bound object REBINDS rather than refusing, so the
    association is a mutable property and not a one-shot commitment
  - unbind() clears the association and leaves the PAYLOAD untouched.
    That is the important half: the two members are independent, and an
    unbind that quietly reset the payload would be a data-loss bug that
    no is_bound() check would reveal
  - unbind() on an already-unbound object is harmless, so callers need not
    guard it
  - all four are noexcept, which is what lets them be used in a destructor
    or a cleanup path
*/
bool
tests_container_metadata_binding()
{
    cm_box first;
    cm_box second;
    first.n  = 1;
    second.n = 2;

    cm_meta meta(cm_note{40});

    // starts unbound, with its payload
    D_CM_CHECK(!meta.is_bound());
    D_CM_CHECK(meta.get_container() == nullptr);
    D_CM_CHECK(meta.payload().v == 40);

    // bind attaches, and returns the very object bound
    meta.bind(first);
    D_CM_CHECK(meta.is_bound());
    D_CM_CHECK(meta.get_container() == &first);

    // rebinding is permitted and switches the association
    meta.bind(second);
    D_CM_CHECK(meta.is_bound());
    D_CM_CHECK(meta.get_container() == &second);
    D_CM_CHECK(meta.get_container() != &first);

    // the payload survived both binds
    D_CM_CHECK(meta.payload().v == 40);

    // unbind clears the association only
    meta.unbind();
    D_CM_CHECK(!meta.is_bound());
    D_CM_CHECK(meta.get_container() == nullptr);
    D_CM_CHECK(meta.payload().v == 40);

    // unbinding again is harmless
    meta.unbind();
    D_CM_CHECK(!meta.is_bound());
    D_CM_CHECK(meta.payload().v == 40);

    // and a bind after an unbind still works
    meta.bind(first);
    D_CM_CHECK(meta.is_bound());
    D_CM_CHECK(meta.get_container() == &first);

    // the whole binding surface is noexcept
    D_CM_CHECK(noexcept(meta.bind(first)));
    D_CM_CHECK(noexcept(meta.unbind()));
    D_CM_CHECK(noexcept(meta.is_bound()));
    D_CM_CHECK(noexcept(meta.get_container()));

    return true;
}

/*
tests_container_metadata_non_owning
  The container is borrowed, not owned.
  Tests the following:
  - get_container() hands back a pointer to the ORIGINAL object, so a
    write made through it is visible on that object afterwards.  Address
    equality alone would not prove this - a copy could share an address
    pattern by luck - so the check is a write followed by a read of the
    original
  - two container_metadata objects bound to one container both see that
    same object, and a write through either is visible through the other
  - binding does not copy the container: mutating the original is visible
    through a previously-bound handle
  - the const overload of get_container() yields a pointer through which
    the container cannot be modified, while the non-const overload does,
    so constness of the wrapper propagates to the borrowed object
*/
bool
tests_container_metadata_non_owning()
{
    cm_box box;
    box.n = 100;

    cm_meta meta(box, cm_note{1});

    // the pointer is to the original object
    D_CM_CHECK(meta.get_container() == &box);

    // a write through the handle lands on the original
    meta.get_container()->n = 200;
    D_CM_CHECK(box.n == 200);

    // a write to the original is visible through the handle
    box.n = 300;
    D_CM_CHECK(meta.get_container()->n == 300);

    // two handles onto one container see each other's writes
    {
        cm_meta other(box, cm_note{2});
        D_CM_CHECK(other.get_container() == meta.get_container());

        other.get_container()->n = 400;
        D_CM_CHECK(meta.get_container()->n == 400);
        D_CM_CHECK(box.n == 400);
    }

    // rebinding leaves the previous container untouched
    {
        cm_box another;
        another.n = 7;
        meta.bind(another);
        D_CM_CHECK(box.n == 400);
        D_CM_CHECK(meta.get_container()->n == 7);
        meta.bind(box);
    }

    // constness of the wrapper reaches the borrowed container
    {
        const cm_meta frozen(box, cm_note{3});
        D_CM_CHECK(frozen.get_container() == &box);
        D_CM_CHECK(
         std::is_const<std::remove_pointer_t<
             decltype(frozen.get_container())>>::value);
        D_CM_CHECK(
         !std::is_const<std::remove_pointer_t<
             decltype(meta.get_container())>>::value);
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
