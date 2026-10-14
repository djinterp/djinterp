/*******************************************************************************
* djinterp [parse]                                               arg_validator.h
*
* Validators: the checks and transformations an option runs on its values.
*   A `struct d_arg_validator` is CLI11's Validator as a C value: a function
* that judges -- and may rewrite -- one value, a description help prints
* beside the option, a name to find it by, the position of the value it
* applies to, and whether it is active and whether its rewriting counts.
* Options hold validators by value, so a validator is built, configured and
* handed over; every call that takes one by value consumes it, and
* d_arg_validator_copy keeps a caller's own.
*   Validators combine as CLI11's do with `&`, `|` and `!`, through
* d_arg_validator_and, d_arg_validator_or and d_arg_validator_not, with
* CLI11's descriptions and messages word for word.
*   This header holds the record, the running of a validator over a value,
* composition, custom validators, and CLI11's default validators: ranges,
* the two number signs, and escaped strings. CLI11's extra validators are in
* arg_validator_extra.h, and those that ask the file system in
* arg_validator_path.h.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/c/arg_validator.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VALIDATION
    ----------
    1.  The record
         1.  d_arg_validation
    2.  Operations
         1.  d_arg_validation_init / d_arg_validation_release
         2.  d_arg_validation_replace
         3.  d_arg_validation_reject / d_arg_validation_rejectf
         4.  d_arg_validation_passed
2.  VALIDATORS
    ----------
    1.  Functions
         1.  d_arg_validate_fn
         2.  d_arg_describe_fn
         3.  d_arg_choice_fn
         4.  d_arg_choices_fn
    2.  The record
         1.  Validator flags
              1.  D_ARG_VALIDATOR_ACTIVE
              2.  D_ARG_VALIDATOR_MODIFYING
              3.  D_ARG_VALIDATOR_PATH
              4.  D_ARG_VALIDATOR_DIRECTORY
              5.  D_ARG_VALIDATOR_NUMBER
         2.  d_arg_validator_value
         3.  d_arg_validator
    3.  Lifetime
         1.  d_arg_validator_init / d_arg_validator_release
         2.  d_arg_validator_copy
    4.  Settings
         1.  d_arg_validator_set_name / d_arg_validator_set_description
         2.  d_arg_validator_set_active and kin
    5.  Use
         1.  d_arg_validator_run
         2.  d_arg_validator_describe
         3.  d_arg_validator_each_choice
3.  COMPOSITION
    -----------
    1.  Operators
         1.  d_arg_validator_and / d_arg_validator_or
         2.  d_arg_validator_not
4.  CUSTOM VALIDATORS
    -----------------
    1.  Construction
         1.  d_arg_validator_custom
         2.  d_arg_validator_own_context
         3.  d_arg_validator_set_describe
         4.  d_arg_validator_set_choices / d_arg_validator_set_value_kind
5.  DEFAULT VALIDATORS
    ------------------
    1.  Ranges
         1.  d_arg_validator_range
         2.  d_arg_validator_range_long and kin
    2.  Number signs
         1.  d_arg_validator_positive_number
         2.  d_arg_validator_non_negative_number
    3.  Strings
         1.  d_arg_validator_escaped_string
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_H 1

// djinterp
#include "../../../../c/djinterp.h"          // framework root
#include "../../../../c/util/sink_common.h"  // d_pack_text
#include "./arg_common.h"                    // d_arg_buffer, d_arg_copy_fn,
                                             // d_arg_release_fn
#include "./arg_type.h"                      // d_arg_type
#include "../../../../env/c/env_vendor_attributes.h"  // D_FORMAT_PRINTF
// re_std
#include "../../../../../re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  VALIDATION
//==============================================================================
// What a validator works on: one value, the value it may become, and the
// reason it is rejected, if it is.


// 1.1    The record
//------------------------------------------------------------------------------
// 1.1.1
// d_arg_validation
//   struct: one value on its way through a validator. `value` is the value
// as it stands: the text it started as, then whatever the last
// transformation made of it, held in `storage`. `message` collects why the
// value was rejected, and stays empty while it passes. `option` is the
// option the value belongs to, NULL outside a parse, and `integer_rule` the
// rule a validator's conversions use: the option's during a parse.
//   Public-payload layout: read every field directly; change `value` only
// through d_arg_validation_replace and `message` only through
// d_arg_validation_reject, which keep `storage` consistent with them.
struct d_arg_validation
{
    struct d_pack_text         value;
    struct d_arg_buffer        storage;
    struct d_arg_buffer        message;
    const struct d_arg_option* option;
    int                        integer_rule;
};


D_EXTERN_C_BEGIN

// 1.2    Operations
//------------------------------------------------------------------------------
// 1.2.1
// d_arg_validation_init / d_arg_validation_release
//   lifetime: init starts a validation of `_value`, which it borrows, with no
// option and the configured D_INTERNAL_ARG_PARSER_INTEGER_RULE; release
// frees what the validation came to own -- a replaced value, a message --
// after which `value` must not be read. Neither fails, and release accepts
// NULL.
void d_arg_validation_init(struct d_arg_validation* _validation,
                           struct d_pack_text       _value);
void d_arg_validation_release(struct d_arg_validation* _validation);

// 1.2.2
// d_arg_validation_replace
/**
 * @brief Replaces the value under validation, as a transformation does.
 *
 * @param[in,out] _validation  the validation.
 * @param[in]     _value       the new value, which is copied; it may span
 *                             the current value or a part of it.
 * @post   on success `_validation->value` spans the copy, which is kept
 *         NUL-terminated in `_validation->storage`; on failure the value is
 *         unchanged.
 * @retval D_ARG_OK                   replaced.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  the copy could not be stored.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   `_validation` is NULL, or `_value` is
 *                                    NULL with a nonzero length.
 */
int  d_arg_validation_replace(struct d_arg_validation* _validation,
                              struct d_pack_text       _value);

// 1.2.3
// d_arg_validation_reject / d_arg_validation_rejectf
/**
 * @brief Records why the value under validation is rejected.
 *
 * @note A rejection is a verdict, not an error: the validator that records
 *       one still returns D_ARG_OK, as CLI11's returns its message, and a
 *       composite combines it with its other operand's verdict. Returning
 *       an error status instead -- CLI11's throw -- stops a composite at
 *       once. An empty message records nothing, as an empty string is a
 *       pass in CLI11.
 *
 * @param[in,out] _validation  the validation.
 * @param[in]     _message     the reason, which is copied; NULL records
 *                             nothing.
 * @retval D_ARG_OK                   recorded.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  the message could not be stored.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   `_validation` is NULL.
 */
int  d_arg_validation_reject(struct d_arg_validation* _validation,
                             const char*              _message);
/**
 * @brief Records a `printf`-style reason, as d_arg_validation_reject.
 *
 * @param[in,out] _validation  the validation.
 * @param[in]     _format      the reason's format string.
 * @return a status, as d_arg_validation_reject's.
 */
int  d_arg_validation_rejectf(struct d_arg_validation* _validation,
                              const char*              _format,
                              ...) D_FORMAT_PRINTF(2, 3);

// 1.2.4
// d_arg_validation_passed
/**
 * @brief Determines whether the value under validation has passed so far.
 *
 * @param[in] _validation  the validation.
 * @return `true` while no rejection is recorded; `false` after one, or for
 *         NULL.
 */
D_INLINE bool
d_arg_validation_passed(
    const struct d_arg_validation* _validation
)
{
    return ( (_validation != NULL) &&
             (_validation->message.length == 0) );
}

D_EXTERN_C_END


//==============================================================================
// 2.  VALIDATORS
//==============================================================================


// 2.1    Functions
//------------------------------------------------------------------------------
// 2.1.1
// d_arg_validate_fn
//   type: judges, and may rewrite, the value of `_validation`, reading its own
// parameters from `_validator`. It returns D_ARG_OK once it has reached a
// verdict -- the value passes, or is rejected through
// d_arg_validation_reject -- and an error status when it cannot reach one
// (CLI11's throw): that stops a composite at once and fails the option, with
// the message recorded, if any. It may rewrite the value with
// d_arg_validation_replace; whether the rewriting counts is decided by the
// validator's D_ARG_VALIDATOR_MODIFYING flag, not by the function.
typedef int (*d_arg_validate_fn)(const struct d_arg_validator* _validator,
                                 struct d_arg_validation*      _validation);

// 2.1.2
// d_arg_describe_fn
//   type: appends a description that is worked out when asked for rather than
// fixed -- a range's, from its bounds; a composite's, from its operands'.
// Returns D_ARG_OK or the buffer's status.
typedef int (*d_arg_describe_fn)(const struct d_arg_validator* _validator,
                                 struct d_arg_buffer*          _out);

// 2.1.3
// d_arg_choice_fn
//   type: receives one value a validator accepts, from a d_arg_choices_fn;
// a status other than D_ARG_OK stops the listing, which returns it.
typedef int (*d_arg_choice_fn)(struct d_pack_text _choice,
                               void*              _context);

// 2.1.4
// d_arg_choices_fn
//   type: lists the values a validator accepts, when they are few enough to
// list -- an is_member's set, a transformer's keys -- for shell completion
// and whatever else offers them: calls `_each` once for each, in order, and
// returns D_ARG_OK or the first other status `_each` returned.
typedef int (*d_arg_choices_fn)(const struct d_arg_validator* _validator,
                                d_arg_choice_fn               _each,
                                void*                         _context);

// 2.2    The record
//------------------------------------------------------------------------------
// 2.2.1
// Validator flags
//   constant: the switches of d_arg_validator's `flags`.

// 2.2.1.1
// D_ARG_VALIDATOR_ACTIVE
//   constant: the validator runs and is described. Cleared, it passes every
// value and describes nothing, as CLI11's active(false). Set by every
// constructor.
#define D_ARG_VALIDATOR_ACTIVE    0x0001u

// 2.2.1.2
// D_ARG_VALIDATOR_MODIFYING
//   constant: what the validator makes of a value replaces the value.
// Cleared, the validator runs on a copy, so its verdict counts and its
// rewriting does not, as CLI11's non_modifying(). Set by every constructor;
// an option's check clears it, and its transform keeps it.
#define D_ARG_VALIDATOR_MODIFYING 0x0002u

// 2.2.1.3
// D_ARG_VALIDATOR_PATH
//   constant: the values the validator accepts are paths, for which shell
// completion offers files. Set by the path validators.
#define D_ARG_VALIDATOR_PATH      0x0004u

// 2.2.1.4
// D_ARG_VALIDATOR_DIRECTORY
//   constant: the values are directories, for which shell completion offers
// directories; set with D_ARG_VALIDATOR_PATH by
// d_arg_validator_existing_directory.
#define D_ARG_VALIDATOR_DIRECTORY 0x0008u

// 2.2.1.5
// D_ARG_VALIDATOR_NUMBER
//   constant: the values are numbers -- or made of them, as an IPv4 address
// is -- for which shell completion offers nothing. Set by the ranges, the
// number signs, and, in arg_validator_extra.h, the bounds, the type
// validators of arithmetic types, the IPv4 check and the unit validators.
#define D_ARG_VALIDATOR_NUMBER    0x0010u

// 2.2.2
// d_arg_validator_value
//   union: a parameter a validator keeps inside itself rather than on the
// heap, such as a range's bound. It is large and aligned enough for every
// arithmetic type a built-in descriptor converts to -- the C sources store
// the integers wider than `long` in it by copying their bytes -- so the
// built-in validators that need nothing else never allocate.
union d_arg_validator_value
{
    long          integer;
    unsigned long natural;
    double        real;
    long double   extended;
};

// 2.2.3
// d_arg_validator
//   struct: CLI11's Validator. `validate` judges a value, and NULL passes
// every value. `description` is owned text fixing the description; while it
// is NULL, `describe` works it out, and NULL there describes nothing.
// `choices`, when set, lists the values the validator accepts.
// `data` is the validator's context: owned when `release` is set, which
// frees it with the validator, and duplicated by `copy` when the validator
// is copied; borrowed when `release` is NULL. `parameter` is constant data
// a built-in validator reads and does not own, such as a type's
// descriptor, and `low` and `high` hold its numbers. `name` (owned, or
// NULL) is what an option finds the validator by, and `application_index`
// the position of the one value it applies to, -1 for every value.
//   `status` is D_ARG_OK, or why building the validator failed -- a refused
// allocation, a bad argument -- with the reason in `description` when one
// could be kept, and nothing else held. A failed validator runs as an error
// and composes into a failed one, and an option given one records the
// failure as a definition error, so a chain of constructors needs no check
// at each step.
//   Public-payload layout: read every field directly; write only through
// the calls below, which keep ownership consistent.
struct d_arg_validator
{
    d_arg_validate_fn           validate;
    d_arg_describe_fn           describe;
    d_arg_choices_fn            choices;
    void*                       data;
    d_arg_copy_fn               copy;
    d_arg_release_fn            release;
    const void*                 parameter;
    union d_arg_validator_value low;
    union d_arg_validator_value high;
    char*                       name;
    char*                       description;
    int                         application_index;
    int                         status;
    uint32_t                    flags;
};


D_EXTERN_C_BEGIN

// 2.3    Lifetime
//------------------------------------------------------------------------------
// 2.3.1
// d_arg_validator_init / d_arg_validator_release
//   lifetime: init makes CLI11's default Validator -- active, modifying,
// applying to every value, passing every value, describing nothing; release
// frees whatever the validator owns and leaves it as init makes it. Neither
// fails, and each accepts NULL.
void d_arg_validator_init(struct d_arg_validator* _validator);
void d_arg_validator_release(struct d_arg_validator* _validator);

// 2.3.2
// d_arg_validator_copy
/**
 * @brief Copies a validator and everything it owns.
 *
 * @param[in] _source  the validator to copy.
 * @return the copy, which the caller releases or hands over; on failure, a
 *         validator holding only the failure: D_ARG_ERROR_OUT_OF_MEMORY,
 *         D_ARG_ERROR_UNSUPPORTED when `_source` owns a context it has no
 *         way to copy, D_ARG_ERROR_BAD_ARGUMENT for NULL, or `_source`'s
 *         own status when it had failed.
 */
struct d_arg_validator
     d_arg_validator_copy(const struct d_arg_validator* _source);

// 2.4    Settings
//------------------------------------------------------------------------------
// 2.4.1
// d_arg_validator_set_name / d_arg_validator_set_description
/**
 * @brief Names a validator, so that an option can find it; CLI11's name().
 *
 * @param[in,out] _validator  the validator.
 * @param[in]     _name       the name, which is copied; NULL removes it.
 * @post   on failure the validator's `status` is the failure's.
 * @retval D_ARG_OK                   named.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  the copy could not be made.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   `_validator` is NULL.
 */
int  d_arg_validator_set_name(struct d_arg_validator* _validator,
                              const char*             _name);
/**
 * @brief Fixes a validator's description; CLI11's description().
 *
 * @param[in,out] _validator    the validator.
 * @param[in]     _description  the description, which is copied; NULL
 *                              returns the validator to the description it
 *                              works out for itself.
 * @post   on failure the validator's `status` is the failure's.
 * @return a status, as d_arg_validator_set_name's.
 */
int  d_arg_validator_set_description(struct d_arg_validator* _validator,
                                     const char*             _description);

// 2.4.2
// d_arg_validator_set_active and kin
//   settings: CLI11's active(), non_modifying() -- with its sense reversed --
// and application_index(), in which -1 means every value. None fails, and
// each ignores NULL.
void d_arg_validator_set_active(struct d_arg_validator* _validator,
                                bool                    _active);
void d_arg_validator_set_modifying(struct d_arg_validator* _validator,
                                   bool                    _modifying);
void d_arg_validator_set_application_index(struct d_arg_validator* _validator,
                                           int                     _index);

// 2.5    Use
//------------------------------------------------------------------------------
// 2.5.1
// d_arg_validator_run
/**
 * @brief Runs a validator over a value; CLI11's operator().
 *
 * @note An inactive validator passes without running. One whose
 *       D_ARG_VALIDATOR_MODIFYING flag is clear runs on a copy of the value,
 *       so its verdict is kept and its rewriting dropped.
 *
 * @param[in]     _validator   the validator.
 * @param[in,out] _validation  the value, which may be rewritten, and where
 *                             the verdict is recorded.
 * @return D_ARG_OK when a verdict was reached -- read it with
 *         d_arg_validation_passed -- or the error status the validator
 *         stopped with; a failed validator's own status;
 *         D_ARG_ERROR_BAD_ARGUMENT for a NULL argument.
 */
int  d_arg_validator_run(const struct d_arg_validator* _validator,
                         struct d_arg_validation*      _validation);

// 2.5.2
// d_arg_validator_describe
/**
 * @brief Appends a validator's description; CLI11's get_description().
 *
 * @param[in]     _validator  the validator.
 * @param[in,out] _out        receives the description, appended: the fixed
 *                            one, or the one worked out; nothing for an
 *                            inactive or failed validator.
 * @return D_ARG_OK, or the buffer's status; D_ARG_ERROR_BAD_ARGUMENT for a
 *         NULL argument.
 */
int  d_arg_validator_describe(const struct d_arg_validator* _validator,
                              struct d_arg_buffer*          _out);

// 2.5.3
// d_arg_validator_each_choice
/**
 * @brief Lists the values a validator accepts, when it knows them.
 *
 * @note A composite lists its operands': both of an OR's, and, of an AND's,
 *       the first operand's that has any. A NOT, an inactive or failed
 *       validator, and one without a `choices` function list nothing.
 *
 * @param[in] _validator  the validator.
 * @param[in] _each       called once for each value.
 * @param[in] _context    `_each`'s context.
 * @return D_ARG_OK, or the first other status `_each` returned;
 *         D_ARG_ERROR_BAD_ARGUMENT for a NULL validator or function.
 */
int  d_arg_validator_each_choice(const struct d_arg_validator* _validator,
                                 d_arg_choice_fn               _each,
                                 void*                         _context);


//==============================================================================
// 3.  COMPOSITION
//==============================================================================
// Each operator consumes its operands, so composites nest as expressions do
// in CLI11 (`Range(0, 9) & !ExistingFile`). An operand is run through its
// `validate` directly: its own active and modifying flags do not apply
// inside a composite, whose flags are its own.


// 3.1    Operators
//------------------------------------------------------------------------------
// 3.1.1
// d_arg_validator_and / d_arg_validator_or
/**
 * @brief Combines two validators so that a value must pass both; CLI11's
 *        `a & b`.
 *
 * @note Both run, the second on whatever the first made of the value. When
 *       both reject, the message is "(first) AND (second)"; otherwise it is
 *       the one rejection, if any. The description is "(first) AND
 *       (second)" when both operands have one, and the one they have
 *       otherwise. The result is active when both operands are, applies
 *       where `_first` applies, and is modifying. It lists the values of
 *       the first operand that lists any. It completes as a number when
 *       either operand does, and otherwise as a path when either does -- as
 *       a directory when either completes as one.
 *
 * @param[in] _first   the first validator, which is consumed.
 * @param[in] _second  the second validator, which is consumed.
 * @return the combination; on failure, a failed validator with the status
 *         of the first operand that had failed or of the refused
 *         allocation, both operands released.
 */
struct d_arg_validator d_arg_validator_and(struct d_arg_validator _first,
                                           struct d_arg_validator _second);
/**
 * @brief Combines two validators so that a value must pass either; CLI11's
 *        `a | b`.
 *
 * @note Both run, the second on whatever the first made of the value. The
 *       result passes when either operand passes, and otherwise rejects
 *       with "(first) OR (second)". The description is "(first) OR
 *       (second)" when both operands have one, and the one they have
 *       otherwise. Flags and application are those of d_arg_validator_and.
 *       It lists the values of both operands, and completes as a number
 *       when both operands do, and otherwise as a path when either does --
 *       as a directory when every operand that completes as a path
 *       completes as a directory.
 *
 * @param[in] _first   the first validator, which is consumed.
 * @param[in] _second  the second validator, which is consumed.
 * @return the combination, or a failed validator, as d_arg_validator_and.
 */
struct d_arg_validator d_arg_validator_or(struct d_arg_validator _first,
                                          struct d_arg_validator _second);

// 3.1.2
// d_arg_validator_not
/**
 * @brief Inverts a validator; CLI11's `!a`.
 *
 * @note The inversion passes when the operand rejects, and otherwise
 *       rejects with "check <description> succeeded improperly". It is
 *       described as "NOT " and the operand's description when the operand
 *       has one. It is active when the operand is and applies where the
 *       operand applies.
 *
 * @param[in] _operand  the validator to invert, which is consumed.
 * @return the inversion; on failure, a failed validator, as
 *         d_arg_validator_and.
 */
struct d_arg_validator d_arg_validator_not(struct d_arg_validator _operand);


//==============================================================================
// 4.  CUSTOM VALIDATORS
//==============================================================================


// 4.1    Construction
//------------------------------------------------------------------------------
// 4.1.1
// d_arg_validator_custom
/**
 * @brief Makes a validator from a function; CLI11's Validator(op, desc).
 *
 * @param[in] _validate     the function; NULL makes a validator that passes
 *                          every value.
 * @param[in] _context      the function's context, as the validator's
 *                          `data`; borrowed until d_arg_validator_own_context
 *                          gives it to the validator.
 * @param[in] _description  the description, which is copied; NULL for none.
 * @return the validator; on failure, a failed validator holding
 *         D_ARG_ERROR_OUT_OF_MEMORY.
 */
struct d_arg_validator d_arg_validator_custom(d_arg_validate_fn _validate,
                                              void*             _context,
                                              const char*       _description);

// 4.1.2
// d_arg_validator_own_context
/**
 * @brief Gives a validator its context, to free and to copy with it.
 *
 * @note Calling it again replaces the two functions; the context itself is
 *       kept, and is never freed by this call.
 *
 * @param[in,out] _validator  the validator, whose `data` it now owns.
 * @param[in]     _copy       duplicates the context when the validator is
 *                            copied; NULL makes copying the validator fail
 *                            with D_ARG_ERROR_UNSUPPORTED, though it can
 *                            still be handed over.
 * @param[in]     _release    frees the context with the validator; NULL
 *                            leaves the context borrowed.
 */
void d_arg_validator_own_context(struct d_arg_validator* _validator,
                                 d_arg_copy_fn           _copy,
                                 d_arg_release_fn        _release);

// 4.1.3
// d_arg_validator_set_describe
//   setting: makes a validator work out its description with `_describe`
// whenever no fixed description is set; NULL describes nothing. Does not
// fail, and ignores a NULL validator.
void d_arg_validator_set_describe(struct d_arg_validator* _validator,
                                  d_arg_describe_fn       _describe);

// 4.1.4
// d_arg_validator_set_choices / d_arg_validator_set_value_kind
//   settings: tell shell completion what a custom validator accepts --
// `_choices` lists its values (NULL for none), and `_kind` is 0,
// D_ARG_VALIDATOR_PATH, D_ARG_VALIDATOR_DIRECTORY, which sets the path flag
// too, or D_ARG_VALIDATOR_NUMBER. Neither fails, and each ignores a NULL
// validator.
void d_arg_validator_set_choices(struct d_arg_validator* _validator,
                                 d_arg_choices_fn        _choices);
void d_arg_validator_set_value_kind(struct d_arg_validator* _validator,
                                    uint32_t                _kind);


//==============================================================================
// 5.  DEFAULT VALIDATORS
//==============================================================================
// CLI11's own, which CLI11 compiles in every build. None allocates, so none
// fails but for a bad argument.


// 5.1    Ranges
//------------------------------------------------------------------------------
// 5.1.1
// d_arg_validator_range
/**
 * @brief Makes CLI11's Range: a value must convert to a type and lie
 *        between two bounds, inclusive.
 *
 * @note Rejects with "Value <value> not in range [<low> - <high>]", and is
 *       described as "<TYPE> in [<low> - <high>]", the bounds written as
 *       CLI11 streams them: a character as itself, integers in full --
 *       8-bit ones too, which CLI11 streams as characters -- and floating
 *       point to six significant digits. The value is converted as CLI11's
 *       lexical_cast converts it: as an option of the type would, but for
 *       the empty text, which is out of range, so a fraction is out of
 *       range for an integer type. Shell completion offers nothing for it,
 *       as for a number.
 *
 * @param[in] _type  the descriptor of an arithmetic type: an integer, a
 *                   character or a floating-point type, with a conversion.
 * @param[in] _low   the lower bound, an object of the type, copied.
 * @param[in] _high  the upper bound, an object of the type, copied.
 * @return the validator; on failure, a failed validator holding
 *         D_ARG_ERROR_BAD_ARGUMENT for a NULL argument, a type that is not
 *         arithmetic or has no conversion, or a container's.
 */
struct d_arg_validator d_arg_validator_range(const struct d_arg_type* _type,
                                             const void*              _low,
                                             const void*              _high);

// 5.1.2
// d_arg_validator_range_long and kin
//   constructors: d_arg_validator_range over the built-in `long`, `unsigned
// long` and `double` types, the ones CLI11's Range is most often given,
// described as "INT", "UINT" and "FLOAT" ranges. None fails.
struct d_arg_validator d_arg_validator_range_long(long _low,
                                                  long _high);
struct d_arg_validator d_arg_validator_range_ulong(unsigned long _low,
                                                   unsigned long _high);
struct d_arg_validator d_arg_validator_range_double(double _low,
                                                    double _high);

// 5.2    Number signs
//------------------------------------------------------------------------------
// 5.2.1
// d_arg_validator_positive_number
//   constructor: CLI11's PositiveNumber, a range over `double` from the
// smallest positive value to the largest, described as "POSITIVE". Does not
// fail.
struct d_arg_validator d_arg_validator_positive_number(void);

// 5.2.2
// d_arg_validator_non_negative_number
//   constructor: CLI11's NonNegativeNumber, a range over `double` from 0 to
// the largest value, described as "NONNEGATIVE". Does not fail.
struct d_arg_validator d_arg_validator_non_negative_number(void);

// 5.3    Strings
//------------------------------------------------------------------------------
// 5.3.1
// d_arg_validator_escaped_string
/**
 * @brief Makes CLI11's EscapedString, a transformation that unquotes and
 *        unescapes a value.
 *
 * @note A value in double quotes loses them and has its escapes resolved;
 *       one in single quotes or back quotes loses them and nothing else; an
 *       unquoted value with a backslash has its escapes resolved. The
 *       escapes are `\\b`, `\\t`, `\\n`, `\\f`, `\\r`, `\\"`, `\\\\`, `\\0`,
 *       `\\uXXXX` and `\\UXXXXXXXX`, the last two written out as UTF-8. A
 *       binary string becomes its bytes, each `\\xHH` one byte: one in
 *       single quotes, `'B"(...)"'`, always, and an unquoted `B"(...)"` when
 *       it holds a backslash, as CLI11 decides. A malformed escape rejects
 *       the value with CLI11's message.
 *       Unlike CLI11, a quoted value that looks like `[...]` is not escaped
 *       against being read as a list: that is the parser's concern, not the
 *       value's.
 *
 * @return the validator, described by nothing. Does not fail.
 */
struct d_arg_validator d_arg_validator_escaped_string(void);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_H
