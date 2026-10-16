/*******************************************************************************
* djinterp [test]                                       archive_tests_traits.cpp
*
*   Tests for the format tags, the format_traits identity and capability
* flags, and the format_is_writable / format_is_readable queries.
*
*   Part of the archive.hpp / archive.cpp unit-test suite; the declarations,
* shared helpers, and per-suite check macros live in archive_tests.hpp.
* These functions are flat in djinterp::testing and are called by the
* suite runner.
*
*
* path:      /tests/djinterp/core/util/archive/archive_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./archive_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_format_traits_identity
  Verifies the per-format runtime identity carried by format_traits.
  Tests the following:
  - id() returns the matching format_id for every tag
  - name() returns the expected display string for every tag
  - the six format ids are distinct
*/
bool
tests_format_traits_identity()
{
    using namespace ::djinterp::formats;

    bool _ok = true;

    D_AR_CHECK(format_traits<zip>::id()      == format_id_zip);
    D_AR_CHECK(format_traits<tar>::id()      == format_id_tar);
    D_AR_CHECK(format_traits<gz>::id()       == format_id_gz);
    D_AR_CHECK(format_traits<tar_gz>::id()   == format_id_tar_gz);
    D_AR_CHECK(format_traits<sevenzip>::id() == format_id_sevenzip);
    D_AR_CHECK(format_traits<rar>::id()      == format_id_rar);

    D_AR_CHECK(std::string(format_traits<zip>::name())      == "zip");
    D_AR_CHECK(std::string(format_traits<tar>::name())      == "tar");
    D_AR_CHECK(std::string(format_traits<gz>::name())       == "gz");
    D_AR_CHECK(std::string(format_traits<tar_gz>::name())   == "tar.gz");
    D_AR_CHECK(std::string(format_traits<sevenzip>::name()) == "7z");
    D_AR_CHECK(std::string(format_traits<rar>::name())      == "rar");

    // the ids partition six distinct values
    D_AR_CHECK(format_id_zip      != format_id_tar);
    D_AR_CHECK(format_id_tar      != format_id_gz);
    D_AR_CHECK(format_id_gz       != format_id_tar_gz);
    D_AR_CHECK(format_id_tar_gz   != format_id_sevenzip);
    D_AR_CHECK(format_id_sevenzip != format_id_rar);

    return _ok;
}

/*
tests_format_traits_capability_flags
  Verifies the compile-time capability enums and their relationship to the
runtime capability queries.
  Tests the following:
  - every can_read / can_write enum is a strict 0 or 1
  - the built-in zip and tar formats are both readable and writable
  - for the formats with no runtime tool probe (zip, tar, gz, tar.gz) the
    compile-time flag equals the runtime query exactly
  - for the tool-augmented formats (7z, rar) the compile-time flag implies the
    runtime query (compile-time capability is a subset of runtime capability)
*/
bool
tests_format_traits_capability_flags()
{
    using namespace ::djinterp::formats;

    bool _ok = true;

    // each flag is a genuine boolean 0/1
    D_AR_CHECK(format_traits<zip>::can_read      == 0 || format_traits<zip>::can_read      == 1);
    D_AR_CHECK(format_traits<zip>::can_write     == 0 || format_traits<zip>::can_write     == 1);
    D_AR_CHECK(format_traits<tar>::can_read      == 0 || format_traits<tar>::can_read      == 1);
    D_AR_CHECK(format_traits<tar>::can_write     == 0 || format_traits<tar>::can_write     == 1);
    D_AR_CHECK(format_traits<gz>::can_read       == 0 || format_traits<gz>::can_read       == 1);
    D_AR_CHECK(format_traits<gz>::can_write      == 0 || format_traits<gz>::can_write      == 1);
    D_AR_CHECK(format_traits<tar_gz>::can_read   == 0 || format_traits<tar_gz>::can_read   == 1);
    D_AR_CHECK(format_traits<tar_gz>::can_write  == 0 || format_traits<tar_gz>::can_write  == 1);
    D_AR_CHECK(format_traits<sevenzip>::can_read == 0 || format_traits<sevenzip>::can_read == 1);
    D_AR_CHECK(format_traits<sevenzip>::can_write== 0 || format_traits<sevenzip>::can_write== 1);
    D_AR_CHECK(format_traits<rar>::can_read      == 0 || format_traits<rar>::can_read      == 1);
    D_AR_CHECK(format_traits<rar>::can_write     == 0 || format_traits<rar>::can_write     == 1);

    // built-in guarantees
    D_AR_CHECK(format_traits<zip>::can_read  == 1);
    D_AR_CHECK(format_traits<zip>::can_write == 1);
    D_AR_CHECK(format_traits<tar>::can_read  == 1);
    D_AR_CHECK(format_traits<tar>::can_write == 1);
    D_AR_CHECK(format_is_writable<zip>());
    D_AR_CHECK(format_is_readable<zip>());
    D_AR_CHECK(format_is_writable<tar>());
    D_AR_CHECK(format_is_readable<tar>());

    // no runtime probe: flag == query exactly
    D_AR_CHECK((format_traits<zip>::can_write    != 0) == format_is_writable<zip>());
    D_AR_CHECK((format_traits<zip>::can_read     != 0) == format_is_readable<zip>());
    D_AR_CHECK((format_traits<tar>::can_write    != 0) == format_is_writable<tar>());
    D_AR_CHECK((format_traits<tar>::can_read     != 0) == format_is_readable<tar>());
    D_AR_CHECK((format_traits<gz>::can_write     != 0) == format_is_writable<gz>());
    D_AR_CHECK((format_traits<gz>::can_read      != 0) == format_is_readable<gz>());
    D_AR_CHECK((format_traits<tar_gz>::can_write != 0) == format_is_writable<tar_gz>());
    D_AR_CHECK((format_traits<tar_gz>::can_read  != 0) == format_is_readable<tar_gz>());

    // tool-augmented: compile-time capability implies runtime capability
    D_AR_CHECK(!(format_traits<sevenzip>::can_write != 0) || format_is_writable<sevenzip>());
    D_AR_CHECK(!(format_traits<sevenzip>::can_read  != 0) || format_is_readable<sevenzip>());
    D_AR_CHECK(!(format_traits<rar>::can_write      != 0) || format_is_writable<rar>());
    D_AR_CHECK(!(format_traits<rar>::can_read       != 0) || format_is_readable<rar>());

    return _ok;
}

/*
tests_format_capability_queries
  Verifies the template capability wrappers forward to the dispatch leaves.
  Tests the following:
  - format_is_writable<F>() equals internal::format_can_write(id) for every F
  - format_is_readable<F>() equals internal::format_can_read(id) for every F
*/
bool
tests_format_capability_queries()
{
    using namespace ::djinterp::formats;

    bool _ok = true;

    D_AR_CHECK(format_is_writable<zip>()      == internal::format_can_write(format_id_zip));
    D_AR_CHECK(format_is_writable<tar>()      == internal::format_can_write(format_id_tar));
    D_AR_CHECK(format_is_writable<gz>()       == internal::format_can_write(format_id_gz));
    D_AR_CHECK(format_is_writable<tar_gz>()   == internal::format_can_write(format_id_tar_gz));
    D_AR_CHECK(format_is_writable<sevenzip>() == internal::format_can_write(format_id_sevenzip));
    D_AR_CHECK(format_is_writable<rar>()      == internal::format_can_write(format_id_rar));

    D_AR_CHECK(format_is_readable<zip>()      == internal::format_can_read(format_id_zip));
    D_AR_CHECK(format_is_readable<tar>()      == internal::format_can_read(format_id_tar));
    D_AR_CHECK(format_is_readable<gz>()       == internal::format_can_read(format_id_gz));
    D_AR_CHECK(format_is_readable<tar_gz>()   == internal::format_can_read(format_id_tar_gz));
    D_AR_CHECK(format_is_readable<sevenzip>() == internal::format_can_read(format_id_sevenzip));
    D_AR_CHECK(format_is_readable<rar>()      == internal::format_can_read(format_id_rar));

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
