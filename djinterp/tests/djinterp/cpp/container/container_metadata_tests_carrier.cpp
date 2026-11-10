/*******************************************************************************
* djinterp [testing]                        container_metadata_tests_carrier.cpp
*
*   The carrier section of container_metadata's suite: DMetadataCarrier, the
* mixin by which a container carries a metadata object, and the
* metadata_traits protocol that detects and extracts it.
*
*
* path:      /tests/djinterp/cpp/container/container_metadata_tests_carrier.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/
#include "container_metadata_tests.hpp"  // corresponding header


NS_DJINTERP
NS_TESTING

/*
tests_container_metadata_carrier
  DMetadataCarrier's surface.
  Tests the following:
  - the const and non-const metadata() reach the same carried object, and a
    write through the non-const one is seen through the const one
  - a carrier constructed with metadata holds that metadata
  - metadata_type and metadata_container_type both name the metadata type
  - the mixin cannot be made or destroyed on its own: its constructors and
    destructor are protected
*/
bool tests_container_metadata_carrier()
{
    DCmCarried carried;
    const DCmCarried& view = carried;
    const DCmCarried seeded(DCmStrings{ { "title", "Q3" } });

    D_CM_CHECK(&carried.metadata() == &view.metadata());
    D_CM_CHECK(view.metadata().empty());

    carried.metadata().set("owner", "ops");

    D_CM_CHECK(view.metadata().size() == 1);
    D_CM_CHECK(view.metadata().at("owner") == "ops");

    D_CM_CHECK(seeded.metadata().at("title") == "Q3");

    D_CM_CHECK(std::is_same<DCmCarried::metadata_type, DCmStrings>::value);
    D_CM_CHECK(std::is_same<DCmCarried::metadata_container_type,
                            DCmStrings>::value);

    D_CM_CHECK(!std::is_default_constructible<
                   DMetadataCarrier<DCmStrings>>::value);
    D_CM_CHECK(!std::is_destructible<DMetadataCarrier<DCmStrings>>::value);
    D_CM_CHECK(std::is_default_constructible<DCmCarried>::value);

    return true;
}

/*
tests_container_metadata_protocol
  The metadata_traits protocol over a carrier.
  Tests the following:
  - a carrier is seen by has_metadata (through the method),
    has_metadata_type and has_metadata_container_type (through the nested
    types)
  - metadata_t, metadata_type_t and metadata_container_type_t name the
    carried metadata type
  - the store itself is no carrier, and neither is a plain type: each scores
    none of the three
*/
bool tests_container_metadata_protocol()
{
    D_CM_CHECK(has_metadata<DCmCarried>::value);
    D_CM_CHECK(has_metadata_method<DCmCarried>::value);
    D_CM_CHECK(has_metadata_type<DCmCarried>::value);
    D_CM_CHECK(has_metadata_container_type<DCmCarried>::value);

    D_CM_CHECK(std::is_same<metadata_t<DCmCarried>, DCmStrings>::value);
    D_CM_CHECK(std::is_same<metadata_type_t<DCmCarried>, DCmStrings>::value);
    D_CM_CHECK(std::is_same<metadata_container_type_t<DCmCarried>,
                            DCmStrings>::value);

    D_CM_CHECK(!has_metadata<DCmStrings>::value);
    D_CM_CHECK(!has_metadata_type<DCmStrings>::value);
    D_CM_CHECK(!has_metadata_container_type<DCmStrings>::value);

    D_CM_CHECK(!has_metadata<DCmNotCarried>::value);
    D_CM_CHECK(!has_metadata_type<DCmNotCarried>::value);
    D_CM_CHECK(!has_metadata_container_type<DCmNotCarried>::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
