/*******************************************************************************
* djinterp [cpp][container]                          radix_tree_tests_common.cpp
*
*   radix_tree_common.hpp: the key traits and the node probes.
*
*
* path:      /tests/djinterp/cpp/container/tree/radix/radix_tree_tests_common.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include "radix_tree_tests.hpp"  // D_RX_CHECK, the declarations
#include <djinterp/cpp/container/tree/radix/radix_tree_common.hpp>  // under test

// std
#include <string>       // std::string
#include <type_traits>  // std::is_same, std::is_unsigned
#include <vector>       // std::vector


NS_DJINTERP
NS_TESTING

namespace
{
    // DRxNode
    //   fixture: a value_type and children, what a radix node has.
    struct DRxNode
    {
        using value_type = int;

        std::vector<DRxNode*> children;
    };

    // DRxValueOnly
    //   fixture: a value_type and no children.
    struct DRxValueOnly
    {
        using value_type = int;
    };
}

/*
tests_radix_tree_common_key_kinds
  which keys are text, which binary, and their category
  Tests the following:
  - const char*, a char array and re_std::string_view are text keys; a
    std::string is one exactly when it converts to re_std::string_view (it
    does not, today)
  - const char* is a binary key too, as every pointer; text wins its
    category
  - integers and pointers are binary keys, and not text
  - a double is neither, and its category is custom
  - key_bit_width is the key's width in bits, and 0 for a text key
*/
bool tests_radix_tree_common_key_kinds()
{
    D_RX_CHECK(is_text_key<const char*>::value);
    D_RX_CHECK(is_text_key<char[6]>::value);
    D_RX_CHECK(is_text_key<re_std::string_view>::value);
    D_RX_CHECK(is_binary_key<const char*>::value);
    D_RX_CHECK(radix_key_category_of<const char*>::value ==
               DRadixKeyCategory::text);
    D_RX_CHECK(is_text_key<std::string>::value ==
               std::is_convertible<const std::string&,
                                   re_std::string_view>::value);
    D_RX_CHECK(!is_binary_key<std::string>::value);

    D_RX_CHECK(is_binary_key<int>::value);
    D_RX_CHECK(is_binary_key<unsigned long long>::value);
    D_RX_CHECK(is_binary_key<int*>::value);
    D_RX_CHECK(!is_text_key<int>::value);

    D_RX_CHECK(!is_text_key<double>::value);
    D_RX_CHECK(!is_binary_key<double>::value);

    D_RX_CHECK(radix_key_category_of<re_std::string_view>::value ==
               DRadixKeyCategory::text);
    D_RX_CHECK(radix_key_category_of<unsigned>::value ==
               DRadixKeyCategory::binary);
    D_RX_CHECK(radix_key_category_of<double>::value ==
               DRadixKeyCategory::custom);

    D_RX_CHECK(key_bit_width<re_std::uint32_t>::value == 32u);
    D_RX_CHECK(key_bit_width<re_std::uint8_t>::value == 8u);
    D_RX_CHECK(key_bit_width<std::string>::value == 0u);

    return true;
}

/*
tests_radix_tree_common_key_bits
  the bit carrier
  Tests the following:
  - a signed key's carrier is its unsigned twin, so a shift is logical
  - a pointer's carrier is uintptr_t; bool's is unsigned char
  - key_bits keeps the key's bit pattern
*/
bool tests_radix_tree_common_key_bits()
{
    D_RX_CHECK(std::is_same<key_bits_t<int>, unsigned int>::value);
    D_RX_CHECK(std::is_same<key_bits_t<re_std::uint16_t>,
                            re_std::uint16_t>::value);
    D_RX_CHECK(std::is_same<key_bits_t<int*>, re_std::uintptr_t>::value);
    D_RX_CHECK(std::is_same<key_bits_t<bool>, unsigned char>::value);
    D_RX_CHECK(std::is_unsigned<key_bits_t<long>>::value);

    D_RX_CHECK(key_bits(static_cast<re_std::uint32_t>(0x80000001u)) ==
               0x80000001u);
    D_RX_CHECK(key_bits(-1) == static_cast<unsigned int>(-1));

    return true;
}

/*
tests_radix_tree_common_node_probes
  the node probes
  Tests the following:
  - a type with a value_type and children is a radix node
  - a value_type alone is not; int is not
*/
bool tests_radix_tree_common_node_probes()
{
    D_RX_CHECK(has_radix_value<DRxNode>::value);
    D_RX_CHECK(has_radix_children<DRxNode>::value);
    D_RX_CHECK(is_radix_node<DRxNode>::value);

    D_RX_CHECK(has_radix_value<DRxValueOnly>::value);
    D_RX_CHECK(!has_radix_children<DRxValueOnly>::value);
    D_RX_CHECK(!is_radix_node<DRxValueOnly>::value);
    D_RX_CHECK(!is_radix_node<int>::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
