/*******************************************************************************
* djinterp [cpp][container]                       file_tree_tests_attributes.cpp
*
*   file_attributes.hpp: the encodings and their byte-order marks, the
* metadata record and its flags, and the detected populator, run against the
* suite's directory.
*
*
* path:      /tests/djinterp/cpp/container/tree/file/file_tree_tests_attributes.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include "file_tree_tests.hpp"  // D_FT_CHECK, DFtScratch, the declarations
#include <djinterp/cpp/container/tree/file/file_attributes.hpp>  // under test

// std
#include <cstring>  // std::strcmp


NS_DJINTERP
NS_TESTING

/*
tests_file_tree_attributes_encoding
  the byte-order marks, and each encoding's name
  Tests the following:
  - UTF-32 LE and BE, UTF-8, UTF-16 LE and BE marks, the longest first
  - no mark, and too few bytes, are unknown
  - names, and which encodings are Unicode
*/
bool tests_file_tree_attributes_encoding()
{
    const unsigned char u32le[] = { 0xFF, 0xFE, 0x00, 0x00 };
    const unsigned char u32be[] = { 0x00, 0x00, 0xFE, 0xFF };
    const unsigned char u8[]    = { 0xEF, 0xBB, 0xBF, 'a' };
    const unsigned char u16le[] = { 0xFF, 0xFE, 'a', 0x00 };
    const unsigned char u16be[] = { 0xFE, 0xFF };
    const unsigned char plain[] = { 'a', 'b', 'c' };

    D_FT_CHECK(detect_bom(u32le, 4) == file_encoding_utf32_le);
    D_FT_CHECK(detect_bom(u32be, 4) == file_encoding_utf32_be);
    D_FT_CHECK(detect_bom(u8, 4) == file_encoding_utf8_bom);
    D_FT_CHECK(detect_bom(u16le, 4) == file_encoding_utf16_le);
    D_FT_CHECK(detect_bom(u16be, 2) == file_encoding_utf16_be);
    D_FT_CHECK(detect_bom(plain, 3) == file_encoding_unknown);
    D_FT_CHECK(detect_bom(u8, 1) == file_encoding_unknown);

    D_FT_CHECK(std::strcmp(file_encoding_name(file_encoding_utf8),
                           "UTF-8") == 0);
    D_FT_CHECK(file_encoding_is_unicode(file_encoding_utf16_be));
    D_FT_CHECK(file_encoding_is_unicode(file_encoding_utf8_bom));
    D_FT_CHECK(!file_encoding_is_unicode(file_encoding_latin1));
    D_FT_CHECK(!file_encoding_is_unicode(file_encoding_binary));

    return true;
}

/*
tests_file_tree_attributes_metadata
  the metadata record
  Tests the following:
  - a default record is empty, with no flags and an unknown encoding
  - each query reads its flag, and only its flag
*/
bool tests_file_tree_attributes_metadata()
{
    DFileMetadata m;

    D_FT_CHECK(m.size == 0);
    D_FT_CHECK(m.attr_flags == file_attr_none);
    D_FT_CHECK(m.encoding == file_encoding_unknown);
    D_FT_CHECK(m.timestamps.modified == 0);
    D_FT_CHECK(!m.is_directory());

    m.attr_flags = file_attr_directory | file_attr_hidden;

    D_FT_CHECK(m.is_directory());
    D_FT_CHECK(m.is_hidden());
    D_FT_CHECK(m.has_flag(file_attr_hidden));
    D_FT_CHECK(!m.is_readonly());
    D_FT_CHECK(!m.is_symlink());
    D_FT_CHECK(!m.is_executable());

    return true;
}

/*
tests_file_tree_attributes_populate
  the detected populator, against the suite's directory
  Tests the following:
  - a file's size, and that it is not a directory
  - a directory is one
  - a missing path fails
  - the none backend's populator fails, and touches nothing
*/
bool tests_file_tree_attributes_populate()
{
    DFtScratch    s("attributes");
    DFileMetadata file_meta;
    DFileMetadata dir_meta;
    DFileMetadata missing;
    DFileMetadata nothing;

    D_FT_CHECK(s.ok);
    D_FT_CHECK(DFileAttributesDefault::populate(s.path() + "/a.txt",
                                                file_meta));
    D_FT_CHECK(file_meta.size == 5);
    D_FT_CHECK(!file_meta.is_directory());

    D_FT_CHECK(DFileAttributesDefault::populate(s.path() + "/sub", dir_meta));
    D_FT_CHECK(dir_meta.is_directory());

    D_FT_CHECK(!DFileAttributesDefault::populate(s.path() + "/not_there",
                                                 missing));

    D_FT_CHECK(!DFileAttributes<DOperatingSystem::none>::populate(
                   s.path() + "/a.txt", nothing));
    D_FT_CHECK(nothing.size == 0);

    return true;
}

NS_END  // testing
NS_END  // djinterp
