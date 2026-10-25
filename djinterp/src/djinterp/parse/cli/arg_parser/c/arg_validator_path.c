/*******************************************************************************
* djinterp [parse]                                          arg_validator_path.c
*
* The validators that ask the file system, declared in arg_validator_path.h.
*   CLI11's ExistingFile, ExistingDirectory, ExistingPath, NonexistentPath,
* FileOnDefaultPath, permission validators and FileSizeValidator, each
* answered by one status query of the system layer (arg_internal.h, 9.1),
* through a copy of the value that ends in a NUL, so that the query sees the
* path CLI11's c_str() hands the system -- cut at a NUL the value holds. A
* size the system cannot give, a directory's or a device's, is refused with
* the error text libstdc++ and libc++ give, the same on every system.
*   Without D_INTERNAL_ARG_PARSER_PATH_VALIDATORS nothing here is compiled.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_validator_path.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator_path.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free
#include <string.h>  // memcpy, strlen
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // d_arg_validator, d_arg_validation
#include "./arg_internal.h"  // path queries, number text, validator parts
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint64_t, uintmax_t


#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

//==============================================================================
// 1.  QUERIES
//==============================================================================

// d_arg_internal_path_access
//   enum: the access a permission validator asks the file's bits for.
enum d_arg_internal_path_access
{
    ACCESS_READ  = 0,
    ACCESS_WRITE = 1,
    ACCESS_EXEC  = 2
};

// PATH_INFO_NONE
//   constant: the answer for a path not yet examined, or not there.
static const struct d_arg_internal_path_info PATH_INFO_NONE =
{
    0,      // kind
    false,  // regular
    0u,     // size
    false,  // readable
    false,  // writable
    false   // executable
};

D_STATIC_ASSERT( (sizeof(uint64_t) <= sizeof(union d_arg_validator_value)),
                 "a file size's limits are kept in a validator's numbers" );

/**
 * @brief Queries the status of the path a value names.
 *
 * @param[in]  _value  the value.
 * @param[out] _info   receives the answer; its kind 0 for a path that does not
 *                     exist or cannot be examined.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY when the value could not be
 *         copied, `*_info` then unwritten.
 *
 * One status query of the value, through a NUL-terminated copy of it, the
 * answer in `*_info`; the kind is 0 for a path that does not exist or cannot
 * be examined. Only the copy can fail.
 */
D_STATIC int
d_arg_internal_path_examine(
    struct d_pack_text               _value,
    struct d_arg_internal_path_info* _info
)
{
    struct d_arg_buffer path   = D_ARG_BUFFER_INIT;
    const int           status = d_arg_buffer_append_text(&path,
                                                          _value);

    // the query, once the copy is made
    if (status == D_ARG_OK)
    {
        (void)d_arg_internal_path_query(d_arg_buffer_text(&path).data,
                                        _info);
    }

    d_arg_buffer_release(&path);

    return status;
}

/**
 * @brief Describes a path validator.
 *
 * @param[in]     _validator  the validator, whose parameter is its description.
 * @param[in,out] _out        the buffer that gains the description.
 * @return D_ARG_OK, or the buffer's status.
 *
 * The description a path validator keeps as its parameter, a literal.
 */
D_STATIC int
d_arg_internal_path_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    return d_arg_buffer_append_cstr(_out,
                                    (const char*)_validator->parameter);
}

/**
 * @brief Makes a path validator.
 *
 * @param[in] _validate     the function.
 * @param[in] _description  the description, a literal it keeps as its
 *                          parameter.
 * @param[in] _kind         the completion it offers: D_ARG_VALIDATOR_PATH or
 *                          D_ARG_VALIDATOR_DIRECTORY.
 * @return the validator.
 *
 * A path validator: the function, the description it keeps as its
 * parameter, and the completion it offers; it holds nothing else.
 */
D_STATIC struct d_arg_validator
d_arg_internal_path_validator(
    d_arg_validate_fn _validate,
    const char*       _description,
    uint32_t          _kind
)
{
    struct d_arg_validator validator = d_arg_validator_custom(_validate,
                                                              NULL,
                                                              NULL);

    validator.describe  = d_arg_internal_path_describe;
    validator.parameter = _description;
    d_arg_validator_set_value_kind(&validator,
                                   _kind);

    return validator;
}

//==============================================================================
// 2.  EXISTENCE
//==============================================================================

/**
 * @brief Judges whether a value names a file, as CLI11's ExistingFile.
 *
 * @param[in]     _validator   the validator; unused.
 * @param[in,out] _validation  the validation, rejected for an empty value, a
 *                             path that does not exist, or a directory.
 * @return D_ARG_OK once a verdict is reached; D_ARG_ERROR_OUT_OF_MEMORY when
 *         the value could not be copied or the rejection kept.
 *
 * CLI11's ExistingFile: an empty value is "File name is missing.", a path
 * that does not exist "File does not exist: <path>", and a directory "File is
 * actually a directory: <path>".
 */
D_STATIC int
d_arg_internal_path_file_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    struct d_arg_internal_path_info info = PATH_INFO_NONE;

    (void)_validator;

    // no name at all
    if (_validation->value.length == 0u)
    {
        return d_arg_validation_reject(_validation,
                                       "File name is missing.");
    }

    const int status = d_arg_internal_path_examine(_validation->value,
                                                   &info);

    // a refused copy, or a file
    if ( (status != D_ARG_OK) ||
         (info.kind == 1) )
    {
        return status;
    }

    return d_arg_internal_validator_reject(
               _validation,
               (info.kind == 0) ? "File does not exist: "
                                : "File is actually a directory: ",
               _validation->value,
               "");
}

/**
 * @brief Judges whether a value names a directory, as CLI11's
 *        ExistingDirectory.
 *
 * @param[in]     _validator   the validator; unused.
 * @param[in,out] _validation  the validation, rejected for a path that does not
 *                             exist, or one that is not a directory.
 * @return D_ARG_OK once a verdict is reached; D_ARG_ERROR_OUT_OF_MEMORY when
 *         the value could not be copied or the rejection kept.
 *
 * CLI11's ExistingDirectory: a path that does not exist is "Directory does
 * not exist: <path>", and one that is not a directory "Directory is actually
 * a file: <path>".
 */
D_STATIC int
d_arg_internal_path_directory_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    struct d_arg_internal_path_info info   = PATH_INFO_NONE;
    const int                       status =
        d_arg_internal_path_examine(_validation->value,
                                    &info);

    (void)_validator;

    // a refused copy, or a directory
    if ( (status != D_ARG_OK) ||
         (info.kind == 2) )
    {
        return status;
    }

    return d_arg_internal_validator_reject(
               _validation,
               (info.kind == 0) ? "Directory does not exist: "
                                : "Directory is actually a file: ",
               _validation->value,
               "");
}

/**
 * @brief Judges whether a value names anything, as CLI11's ExistingPath.
 *
 * @param[in]     _validator   the validator; unused.
 * @param[in,out] _validation  the validation, rejected for a path that does not
 *                             exist.
 * @return D_ARG_OK once a verdict is reached; D_ARG_ERROR_OUT_OF_MEMORY when
 *         the value could not be copied or the rejection kept.
 *
 * CLI11's ExistingPath: a path that does not exist is "Path does not exist:
 * <path>".
 */
D_STATIC int
d_arg_internal_path_existing_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    struct d_arg_internal_path_info info   = PATH_INFO_NONE;
    const int                       status =
        d_arg_internal_path_examine(_validation->value,
                                    &info);

    (void)_validator;

    // a refused copy, or something there
    if ( (status != D_ARG_OK) ||
         (info.kind != 0) )
    {
        return status;
    }

    return d_arg_internal_validator_reject(_validation,
                                           "Path does not exist: ",
                                           _validation->value,
                                           "");
}

/**
 * @brief Judges whether a value names nothing, as CLI11's NonexistentPath.
 *
 * @param[in]     _validator   the validator; unused.
 * @param[in,out] _validation  the validation, rejected for a path that exists.
 * @return D_ARG_OK once a verdict is reached; D_ARG_ERROR_OUT_OF_MEMORY when
 *         the value could not be copied or the rejection kept.
 *
 * CLI11's NonexistentPath: a path that exists is "Path already exists:
 * <path>".
 */
D_STATIC int
d_arg_internal_path_nonexistent_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    struct d_arg_internal_path_info info   = PATH_INFO_NONE;
    const int                       status =
        d_arg_internal_path_examine(_validation->value,
                                    &info);

    (void)_validator;

    // a refused copy, or nothing there
    if ( (status != D_ARG_OK) ||
         (info.kind == 0) )
    {
        return status;
    }

    return d_arg_internal_validator_reject(_validation,
                                           "Path already exists: ",
                                           _validation->value,
                                           "");
}

/*
d_arg_validator_existing_file
  ExistingFile, described as CLI11 describes it.
*/
struct d_arg_validator
d_arg_validator_existing_file(void)
{
    return d_arg_internal_path_validator(d_arg_internal_path_file_validate,
                                         "FILE",
                                         D_ARG_VALIDATOR_PATH);
}

/*
d_arg_validator_existing_directory
  ExistingDirectory, which completes as a directory.
*/
struct d_arg_validator
d_arg_validator_existing_directory(void)
{
    return d_arg_internal_path_validator(
               d_arg_internal_path_directory_validate,
               "DIR",
               D_ARG_VALIDATOR_DIRECTORY);
}

/*
d_arg_validator_existing_path
  ExistingPath, described as CLI11 describes it.
*/
struct d_arg_validator
d_arg_validator_existing_path(void)
{
    return d_arg_internal_path_validator(
               d_arg_internal_path_existing_validate,
               "PATH(existing)",
               D_ARG_VALIDATOR_PATH);
}

/*
d_arg_validator_nonexistent_path
  NonexistentPath, described as CLI11 describes it.
*/
struct d_arg_validator
d_arg_validator_nonexistent_path(void)
{
    return d_arg_internal_path_validator(
               d_arg_internal_path_nonexistent_validate,
               "PATH(non-existing)",
               D_ARG_VALIDATOR_PATH);
}

//==============================================================================
// 3.  SEARCH
//==============================================================================

/**
 * @brief Joins a value to a default path, as CLI11's test path.
 *
 * @param[in]     _directory  the default path.
 * @param[in]     _value      the value.
 * @param[in,out] _out        the buffer that gains the joined path.
 * @return D_ARG_OK, or the buffer's status.
 *
 * CLI11's test path: the default path, then `/` unless the path is empty or
 * ends in `/` or `\`, then the value.
 */
D_STATIC int
d_arg_internal_path_join(
    const char*          _directory,
    struct d_pack_text   _value,
    struct d_arg_buffer* _out
)
{
    const size_t length    = strlen(_directory);
    const bool   separated = ( (length == 0u)                      ||
                               (_directory[length - 1u] == '/')    ||
                               (_directory[length - 1u] == '\\') );

    (void)d_arg_buffer_append(_out,
                              _directory,
                              length);
    (void)d_arg_buffer_append_cstr(_out,
                                   (separated) ? "" : "/");

    return d_arg_buffer_append_text(_out,
                                    _value);
}

/**
 * @brief Looks for a file under a default path, as CLI11's FileOnDefaultPath.
 *
 * @param[in]     _validator   the validator: the default path in its data, and
 *                             in `low` whether errors are returned.
 * @param[in,out] _validation  the validation, whose value is replaced by the
 *                             joined path naming a file, or rejected when
 *                             errors are returned and it names nothing.
 * @return D_ARG_OK once a verdict is reached; D_ARG_ERROR_OUT_OF_MEMORY when a
 *         copy, the joined path, the replacement, or the rejection could not be
 *         made.
 *
 * CLI11's FileOnDefaultPath: a value naming anything stands; one naming
 * nothing is tried under the default path and, naming a file there, replaced
 * by the joined path; otherwise it is rejected with "File does not exist:
 * <value>" when errors are returned, and stands when they are not.
 */
D_STATIC int
d_arg_internal_path_default_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    struct d_arg_internal_path_info info   = PATH_INFO_NONE;
    struct d_arg_buffer             joined = D_ARG_BUFFER_INIT;
    int                             status =
        d_arg_internal_path_examine(_validation->value,
                                    &info);

    // a refused copy, or a value naming something
    if ( (status != D_ARG_OK) ||
         (info.kind != 0) )
    {
        return status;
    }

    status = d_arg_internal_path_join((const char*)_validator->data,
                                      _validation->value,
                                      &joined);

    // the joined path examined: a file there replaces the value
    if (status == D_ARG_OK)
    {
        (void)d_arg_internal_path_query(d_arg_buffer_text(&joined).data,
                                        &info);
        status = (info.kind == 1)
                 ? d_arg_validation_replace(_validation,
                                            d_arg_buffer_text(&joined))
                 : ( (_validator->low.integer != 0L)
                     ? d_arg_internal_validator_reject(
                           _validation,
                           "File does not exist: ",
                           _validation->value,
                           "")
                     : D_ARG_OK );
    }

    d_arg_buffer_release(&joined);

    return status;
}

/**
 * @brief Copies a default path, as FileOnDefaultPath's copy function.
 *
 * @param[in] _context  the default path; NULL for none.
 * @return the copy, which the caller frees; NULL for none, or when it could not
 *         be allocated.
 *
 * The validator's copy function: a copy of the default path, or NULL.
 */
D_STATIC void*
d_arg_internal_path_default_copy(
    const void* _context
)
{
    return (_context != NULL)
           ? d_arg_internal_copy_cstr((const char*)_context)
           : NULL;
}

/*
d_arg_validator_file_on_default_path
  The default path copied as the validator's context, owned, copied and
freed with it, and whether errors are returned in `low`.
*/
struct d_arg_validator
d_arg_validator_file_on_default_path(
    const char* _default_path,
    bool        _error_return
)
{
    // validate the parameters
    if (!_default_path)
    {
        return d_arg_internal_validator_failed(D_ARG_ERROR_BAD_ARGUMENT,
                                               NULL);
    }

    char* const directory = d_arg_internal_copy_cstr(_default_path);

    // check if memory allocation was successful
    if (!directory)
    {
        return d_arg_internal_validator_failed(D_ARG_ERROR_OUT_OF_MEMORY,
                                               NULL);
    }

    struct d_arg_validator validator =
        d_arg_validator_custom(d_arg_internal_path_default_validate,
                               directory,
                               NULL);

    validator.describe    = d_arg_internal_path_describe;
    validator.parameter   = "FILE";
    validator.low.integer = (_error_return) ? 1L : 0L;
    d_arg_validator_set_value_kind(&validator,
                                   D_ARG_VALIDATOR_PATH);
    d_arg_validator_own_context(&validator,
                                d_arg_internal_path_default_copy,
                                free);

    return validator;
}

//==============================================================================
// 4.  PERMISSIONS
//==============================================================================

/**
 * @brief Judges whether a path grants an access, as CLI11's permission
 *        validators.
 *
 * @param[in]     _validator   the validator, the access asked for in its `low`.
 * @param[in,out] _validation  the validation, rejected for a path that does not
 *                             exist, or one whose permission bits grant the
 *                             access to no one.
 * @return D_ARG_OK once a verdict is reached; D_ARG_ERROR_OUT_OF_MEMORY when
 *         the value could not be copied or the rejection kept.
 *
 * CLI11's permission validators: a path that does not exist is "Path does
 * not exist: <path>", and one whose permission bits grant the access to no one
 * "Path does not have required permissions: <path>". The access asked for is
 * kept in `low`.
 */
D_STATIC int
d_arg_internal_path_permission_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    struct d_arg_internal_path_info info   = PATH_INFO_NONE;
    const int                       status =
        d_arg_internal_path_examine(_validation->value,
                                    &info);

    // a refused copy
    if (status != D_ARG_OK)
    {
        return status;
    }

    // nothing there
    if (info.kind == 0)
    {
        return d_arg_internal_validator_reject(_validation,
                                               "Path does not exist: ",
                                               _validation->value,
                                               "");
    }

    const long access  = _validator->low.integer;
    const bool granted = (access == ACCESS_READ)
                         ? info.readable
                         : ( (access == ACCESS_WRITE)
                             ? info.writable
                             : info.executable );

    return (granted)
           ? D_ARG_OK
           : d_arg_internal_validator_reject(
                 _validation,
                 "Path does not have required permissions: ",
                 _validation->value,
                 "");
}

/**
 * @brief Makes a permission validator for one access.
 *
 * @param[in] _access       the access: ACCESS_READ, ACCESS_WRITE or
 *                          ACCESS_EXEC.
 * @param[in] _description  the description, as CLI11 gives it.
 * @return the validator.
 *
 * A permission validator for one access, described as CLI11 describes it.
 */
D_STATIC struct d_arg_validator
d_arg_internal_path_permission(
    long        _access,
    const char* _description
)
{
    struct d_arg_validator validator =
        d_arg_internal_path_validator(d_arg_internal_path_permission_validate,
                                      _description,
                                      D_ARG_VALIDATOR_PATH);

    validator.low.integer = _access;

    return validator;
}

/*
d_arg_validator_read_permissions
  ReadPermissions.
*/
struct d_arg_validator
d_arg_validator_read_permissions(void)
{
    return d_arg_internal_path_permission(ACCESS_READ,
                                          "Path with read permission");
}

/*
d_arg_validator_write_permissions
  WritePermissions.
*/
struct d_arg_validator
d_arg_validator_write_permissions(void)
{
    return d_arg_internal_path_permission(ACCESS_WRITE,
                                          "Path with write permission");
}

/*
d_arg_validator_exec_permissions
  ExecPermissions.
*/
struct d_arg_validator
d_arg_validator_exec_permissions(void)
{
    return d_arg_internal_path_permission(ACCESS_EXEC,
                                          "Path with exec permission");
}

//==============================================================================
// 5.  SIZE
//==============================================================================

/**
 * @brief Reads a file-size validator's limits.
 *
 * @param[in]  _validator  the validator, its limits in `low` and `high`.
 * @param[out] _minimum    receives the least size.
 * @param[out] _maximum    receives the greatest size; 0 for no maximum.
 *
 * A file-size validator's limits, kept as 64-bit integers in `low` and
 * `high`, read back by copying their bytes.
 */
D_STATIC void
d_arg_internal_path_limits(
    const struct d_arg_validator* _validator,
    uintmax_t*                    _minimum,
    uintmax_t*                    _maximum
)
{
    uint64_t minimum = 0u;
    uint64_t maximum = 0u;

    memcpy(&minimum,
           &_validator->low,
           sizeof(minimum));
    memcpy(&maximum,
           &_validator->high,
           sizeof(maximum));
    *_minimum = (uintmax_t)minimum;
    *_maximum = (uintmax_t)maximum;

    return;
}

/**
 * @brief Appends words and a number of bytes in decimal.
 *
 * @param[in,out] _out     the buffer that gains them.
 * @param[in]     _before  the words.
 * @param[in]     _bytes   the number.
 * @return D_ARG_OK, or the buffer's status.
 *
 * A number of bytes, in decimal, after `_before`.
 */
D_STATIC int
d_arg_internal_path_bytes(
    struct d_arg_buffer* _out,
    const char*          _before,
    uintmax_t            _bytes
)
{
    struct d_arg_internal_number_text text = D_INTERNAL_ARG_NUMBER_TEXT_INIT;

    d_arg_internal_text_unsigned(&text,
                                 _bytes);
    (void)d_arg_buffer_append_cstr(_out,
                                   _before);

    return d_arg_buffer_append(_out,
                               text.data,
                               text.length);
}

/**
 * @brief Checks a file's size against a validator's limits.
 *
 * @param[in]     _validator   the validator, its limits in `low` and `high`.
 * @param[in]     _size        the size.
 * @param[in,out] _validation  the validation, rejected for a size outside the
 *                             limits.
 * @return D_ARG_OK once a verdict is reached; D_ARG_ERROR_OUT_OF_MEMORY when
 *         the rejection could not be kept.
 *
 * A file's size against the limits, 0 standing for no maximum: "File size
 * <size> bytes is less than minimum <min> bytes", or "File size <size> bytes
 * exceeds maximum <max> bytes".
 */
D_STATIC int
d_arg_internal_path_size_check(
    const struct d_arg_validator* _validator,
    uintmax_t                     _size,
    struct d_arg_validation*      _validation
)
{
    uintmax_t minimum = 0u;
    uintmax_t maximum = 0u;

    d_arg_internal_path_limits(_validator,
                               &minimum,
                               &maximum);

    // within the limits
    if ( (_size >= minimum) &&
         ( (maximum == 0u) ||
           (_size <= maximum) ) )
    {
        return D_ARG_OK;
    }

    const bool small = (_size < minimum);

    d_arg_buffer_clear(&_validation->message);
    (void)d_arg_internal_path_bytes(&_validation->message,
                                    "File size ",
                                    _size);
    (void)d_arg_internal_path_bytes(&_validation->message,
                                    (small)
                                    ? " bytes is less than minimum "
                                    : " bytes exceeds maximum ",
                                    (small) ? minimum : maximum);

    return d_arg_buffer_append_cstr(&_validation->message,
                                    " bytes");
}

/**
 * @brief Judges a file's size, as CLI11's FileSizeValidator.
 *
 * @param[in]     _validator   the validator, its limits in `low` and `high`.
 * @param[in,out] _validation  the validation, rejected for a path that does not
 *                             exist, one that is not a regular file, or a size
 *                             outside the limits.
 * @return D_ARG_OK once a verdict is reached; D_ARG_ERROR_OUT_OF_MEMORY when
 *         the value could not be copied or the rejection kept.
 *
 * CLI11's FileSizeValidator: a path that does not exist is "File does not
 * exist: <path>"; one that is not a regular file has no size to give, refused
 * as libstdc++ and libc++ refuse it -- "Error getting file size: Is a
 * directory" for a directory, "Error getting file size: Operation not
 * supported" for anything else; then the size is checked against the limits.
 */
D_STATIC int
d_arg_internal_path_size_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    struct d_arg_internal_path_info info   = PATH_INFO_NONE;
    const int                       status =
        d_arg_internal_path_examine(_validation->value,
                                    &info);

    // a refused copy
    if (status != D_ARG_OK)
    {
        return status;
    }

    // nothing there
    if (info.kind == 0)
    {
        return d_arg_internal_validator_reject(_validation,
                                               "File does not exist: ",
                                               _validation->value,
                                               "");
    }

    // no size to give
    if (!info.regular)
    {
        return d_arg_validation_reject(
                   _validation,
                   (info.kind == 2)
                   ? "Error getting file size: Is a directory"
                   : "Error getting file size: Operation not supported");
    }

    return d_arg_internal_path_size_check(_validator,
                                          info.size,
                                          _validation);
}

/**
 * @brief Describes a FileSizeValidator, as CLI11 does.
 *
 * @param[in]     _validator  the validator, its limits in `low` and `high`.
 * @param[in,out] _out        the buffer that gains the description.
 * @return D_ARG_OK, or the buffer's status.
 *
 * CLI11's FileSizeValidator description: "File size at least <min> bytes",
 * or "File size between <min> and <max> bytes" when there is a maximum.
 */
D_STATIC int
d_arg_internal_path_size_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    uintmax_t minimum = 0u;
    uintmax_t maximum = 0u;

    d_arg_internal_path_limits(_validator,
                               &minimum,
                               &maximum);

    // no maximum
    if (maximum == 0u)
    {
        (void)d_arg_internal_path_bytes(_out,
                                        "File size at least ",
                                        minimum);

        return d_arg_buffer_append_cstr(_out,
                                        " bytes");
    }

    (void)d_arg_internal_path_bytes(_out,
                                    "File size between ",
                                    minimum);
    (void)d_arg_internal_path_bytes(_out,
                                    " and ",
                                    maximum);

    return d_arg_buffer_append_cstr(_out,
                                    " bytes");
}

/*
d_arg_validator_file_size
  The limits copied into the validator's numbers, so that nothing is
allocated.
*/
struct d_arg_validator
d_arg_validator_file_size(
    uint64_t _minimum,
    uint64_t _maximum
)
{
    struct d_arg_validator validator =
        d_arg_validator_custom(d_arg_internal_path_size_validate,
                               NULL,
                               NULL);

    validator.describe = d_arg_internal_path_size_describe;
    memcpy(&validator.low,
           &_minimum,
           sizeof(_minimum));
    memcpy(&validator.high,
           &_maximum,
           sizeof(_maximum));
    d_arg_validator_set_value_kind(&validator,
                                   D_ARG_VALIDATOR_PATH);

    return validator;
}

/*
d_arg_validator_non_empty_file
  CLI11's NonEmptyFile: at least one byte, and no maximum.
*/
struct d_arg_validator
d_arg_validator_non_empty_file(void)
{
    return d_arg_validator_file_size(1u,
                                     0u);
}

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS
