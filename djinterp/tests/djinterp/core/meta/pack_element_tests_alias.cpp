/*******************************************************************************
* djinterp [core]                                   pack_element_tests_alias.cpp
*
*
* path:      /tests/djinterp/core/meta/pack_element_tests_alias.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "pack_element_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_pack_element_alias_equivalence
  pack_element_t against the struct it abbreviates.
  Tests the following:
  - the two agree at every index of an ordinary pack
  - they agree at every index of the whole adversarial zoo, checked by
    rebuilding the pack through each route and comparing the results in
    one expression, so no index is left unvisited
  - they agree with std::tuple_element_t, an independent oracle for the
    same mapping - which is what rules out a shared bug in the two djinterp
    spellings
  - the alias is transparent: it introduces no wrapper, so its result can
    be used anywhere the element type itself could be
*/
bool
tests_pack_element_alias_equivalence()
{
    bool ok = true;

    // spot equivalence at each index of an ordinary pack
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<0, pe_first, pe_second, pe_third>,
                       typename pack_element<0, pe_first, pe_second,
                                             pe_third>::type>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<1, pe_first, pe_second, pe_third>,
                       typename pack_element<1, pe_first, pe_second,
                                             pe_third>::type>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<2, pe_first, pe_second, pe_third>,
                       typename pack_element<2, pe_first, pe_second,
                                             pe_third>::type>);

    // every index of the whole zoo, in one claim per route
    using zoo = pe_rebuild<D_TEST_HOSTILE_TYPES>;

    D_PE_CHECK(ok, std::is_same_v<zoo::via_alias, zoo::via_struct>);
    D_PE_CHECK(ok, std::is_same_v<zoo::via_alias,
                                  pe_origin<D_TEST_HOSTILE_TYPES>>);

    // ... and against an independent oracle
    D_PE_CHECK(ok, std::is_same_v<zoo::via_alias, zoo::via_tuple>);
    D_PE_CHECK(ok, std::is_same_v<zoo::via_struct, zoo::via_tuple>);

    // the same three routes over a long generated pack
    using tags = pe_rebuild<pe_tag<0>, pe_tag<1>, pe_tag<2>, pe_tag<3>,
                            pe_tag<4>, pe_tag<5>, pe_tag<6>, pe_tag<7>>;

    D_PE_CHECK(ok, std::is_same_v<tags::via_alias, tags::via_struct>);
    D_PE_CHECK(ok, std::is_same_v<tags::via_alias, tags::via_tuple>);
    D_PE_CHECK(ok, std::is_same_v<tags::via_alias, pe_tag_list<8>>);

    // the alias is transparent - no wrapper is introduced
    D_PE_CHECK(ok, std::is_same_v<pack_element_t<0, int>*, int*>);
    D_PE_CHECK(ok, std::is_same_v<pack_element_t<1, int, char>&, char&>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<0, pack_element_t<1, int, char>>,
                       char>);

    return ok;
}

/*
tests_pack_element_alias_arity
  The two templates' SIGNATURES, and the asymmetry between them.
  Tests the following:
  - the struct is template<std::size_t, typename, typename...> - the index
    is a std::size_t and one element is REQUIRED
  - the alias is template<std::size_t, typename...> - no separately
    required head, so it accepts an empty pack syntactically even though
    resolving one could never succeed
  - both are proved by BINDING each template as a template template
    argument, which checks the signature without instantiating anything
  - the practical consequence: naming the struct with an empty pack is a
    substitution failure and can therefore be detected, which is the one
    piece of the out-of-range story that is testable at all
*/
bool
tests_pack_element_alias_arity()
{
    bool ok = true;

    // the struct requires an index and at least one element
    D_PE_CHECK(ok, pe_binds_indexed<pack_element>::value);

    // the alias requires only an index
    D_PE_CHECK(ok, pe_binds_alias<pack_element_t>::value);

    // the asymmetry, observed: an empty pack cannot even be NAMED through
    // the struct, because too-few-template-arguments is diagnosed while
    // naming rather than during instantiation
    D_PE_CHECK(ok, !pe_is_nameable<0>::value);
    D_PE_CHECK(ok, !pe_is_nameable<1>::value);
    D_PE_CHECK(ok, !pe_is_resolvable<0>::value);

    // whereas one element is enough to name it, at any index
    D_PE_CHECK(ok, pe_is_nameable<0, int>::value);
    D_PE_CHECK(ok, pe_is_nameable<0, int, char>::value);

    // the index is a std::size_t, so an unsuffixed literal converts
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<0, pe_first, pe_second>,
                       pack_element_t<static_cast<std::size_t>(0),
                                      pe_first, pe_second>>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<1u, pe_first, pe_second>,
                       pe_second>);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
