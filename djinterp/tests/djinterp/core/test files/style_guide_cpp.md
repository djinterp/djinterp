# C++ Coding Style Guide

---

# Part I: Common (C and C++)

## General Formatting

### Spacing

#### Whitespace

-   Use spaces, not tabs
-   Align related elements vertically where appropriate

### Brackets

-   All opening braces `{` on their own line (see Part II for C++ exception)
-   No bracketless `if`-statements (always use brackets) e.g.

```c
if (<condition>)
{
    ...
}
```

-   Empty brackets `{};` should be on their own line

**Correct:**

```c
struct empty_struct
{
};
```

**Incorrect:**

```c
struct empty_struct
{};
```

### Boolean Expressions

Boolean expressions with multiple conditions should be broken up into individual
lines, parenthesized, and enclosed in one set of top-level parentheses. Each
condition should be on its own line with the boolean operators aligned for
consecutive terms of similar length.

**Format:**

```c
if ( (!_array)    ||
     (!_elements) ||
     ( (some_condition) && (other_condition) )        ||
     (!d_array_common_validate_params(_element_size)) ||
     (_count == 0) )
{
    // handle condition
}
```

For `static constexpr bool` expressions:

```cpp
static constexpr bool value =
    ( is_array_like_interface<clean_type>::value &&
      has_tuple_protocol<clean_type>::value      &&
      has_constexpr_size<clean_type>::value );
```

**Rules:**

-   Enclose the entire expression in one set of top-level parentheses
-   Each condition on its own line
-   Line up the opening parentheses of conditions
-   Align boolean operators (`||`, `&&`) for readability
-   Add space between the operator and the next condition
-   Use judgment for what is easiest to read when aligning operators
-   Always parenthesize multi-condition boolean expressions

### Function Calls and Returns

Function calls and return statements should be broken across multiple lines
when:

1.  The function has more than 1 parameter, OR
2.  The full statement exceeds the 80-character line limit

**Format:**

```c
    return d_array_common_resize_amount(_ptr_array->elements,
                                        _ptr_array->count,
                                        sizeof(void*),
                                        _amount);
```

**Rules:**

-   Each parameter on its own line
-   All parameters should line up/start in the same column
-   Align parameters with the first parameter after the opening parenthesis

### Variable Declarations and Initialization

Variables should be declared at the beginning of the function and initialized
only AFTER parameter verification.

**Format:**

```c
struct some_struct*
d_some_struct_new_copy(
    const struct some_struct* _other
)
{
    struct some_struct* result;
    void*               elements;
    size_t              count;
    size_t              capacity;

    // parameter validation first
    if (!_other)
    {
        return NULL;
    }

    // then initialize variables
    result   = NULL;
    elements = NULL;
    count    = 0;
    capacity = 0;

    // rest of function logic...
}
```

**Rules:**

-   Declare all variables at the top of the function
-   Align variable names vertically when possible
-   Perform parameter validation before variable initialization
-   Initialize variables after parameter checks pass

### Control Flow and Spacing

#### if-statements and loops

-   Most `if`-statements and loops should have a comment explaining their
    purpose.
-   `if`-statements should be followed by an empty line, unless the next line
    contains another closing bracket for an outer `if`-statement or `for`-loop.

**Example:**

```c
// check if memory allocation was successful
if (!result)
{
    return NULL;
}

// initialize the array elements
elements = malloc(size * sizeof(void*));
```

#### Return Statements

-   Return statements should be preceded by an empty line
-   Void functions must have an explicit empty `return;` statement at the end

**Example:**

```c
void
d_some_struct_free(
    struct some_struct* _some_struct
)
{
    if (_some_struct)
    {
        if (_some_struct->member)
        {
            free(_some_struct->member);
        }

        free(_some_struct);
    }

    return;
}
```

## Naming Conventions

### Prefixes

-   **Constants/Macros**: prefixed with `D_`, with the rest in ALL CAPS

### C-Specific Naming

-   **Struct/Union types**: prefixed with `d_`

### C++-Specific Naming

-   **Classes and types**: use `snake_case` (e.g., `fixed_array`, `nary_tree`)

## Comments

### Comment Style in Headers

-   Use `//` comments in header files always, except in initial header comment
    block
-   First letter should be lower-case

### Comment Formatting

-   Only indent the first line of each paragraph in a comment/function
    definition header, NOT every line
-   Subsequent lines in the same paragraph should align with the text, not the
    comment marker

**Example:**

```c
/*
d_ptr_array_new
  Creates a `d_ptr_array` with the initial capacity specified.
Note: an initial size of 0 is valid and will result in an empty array that can
contain no elements.

Parameter(s):
  _initial_size: the initial capacity of the array in number of pointer
                 elements.
*/
```

### Brief Comment Format

For structs, unions, classes, and `#define` items:

```c
// <n>
//   <category>: <brief description>
```

e.g.

```c
// D_KEYWORD_FRAMEWORK_NAME
//   constant: keyword corresponding to the name of this framework.
#define D_KEYWORD_FRAMEWORK_NAME    djinterp
```

## Functions

### Function Declarations

Function declarations in header files must:

1.  Include parameter names, not just types
2.  **Multi-line parameters** — when a declaration has more than one parameter,
    break across multiple lines. Align all parameters with the first parameter
    after the opening parenthesis. Align parameter names (the `_` character)
    vertically within each declaration
3.  Line up on the function name (all `d_` prefixes should align in the same
    column)
4.  **No comments** — do not add comments above or beside function declarations

**Example:**

```c
bool        d_some_boolean_fn(const void*       _param1,
                              const void* const _param2);
const char* d_get_name(int _id);
int         d_arg_parser_init(struct d_arg_parser* _parser);
```

**Incorrect (do NOT do this):**

```c
// d_arg_parser_init
//   function: initialize a parser with default settings.
// Returns 0 on success, non-zero on failure.
int                  d_arg_parser_init(struct d_arg_parser* _parser);
```

### Function Definitions

#### Standard Functions

For non-unit test functions, use the following format for commenting above
function definitions:

```c
/*
d_some_boolean_fn
  <description>

Parameter(s):
  _param1:     description1 blah blah blah blah blah blah blah blah blah
               blah look how I line up.
  _param2:     description2.
  _parameter3: description3.
Return:
  A boolean value corresponding to either:
  - true, if all of the following conditions were true:
    - cond1,
    - cond2,
    - cond3, or
  - false, if any of the following were true:
    - etc1,
    - etc2
*/
bool
d_some_boolean_fn(
    const void*       _a, // align (re: line-up) on the `_` character
    const void* const _b  // use spaces, not tabs
)
{
  ...
}
```

**Key points:**

-   Multi-line comment block with function name and description
-   Parameter descriptions aligned vertically
-   Return value clearly documented (use "none." for `void` functions)
-   Parameters aligned on the `_` character
-   Use spaces for alignment, not tabs

#### Void Function Requirements

Functions with a `void` return type must:

1.  Document the return as "none." in the function comment
2.  Include an explicit empty `return;` statement at the end of the function

**Example:**

```c
/*
d_some_struct_free
  Frees a d_some_struct and all associated memory.

Parameter(s):
  _some_struct: the struct to free; may be NULL.
Return:
  none.
*/
void
d_some_struct_free(
    struct some_struct* _some_struct
)
{
    if (_some_struct)
    {
        if (_some_struct->member)
        {
            free(_some_struct->member);
        }

        free(_some_struct);
    }

    return;
}
```

#### Unit Test Functions

Simplified format for unit tests (both standalone AND DTest):

in the header (declarations) \*.h file:

```c
// group of tests name (corresponding one section of the module's header
// file, if applicable)
bool d_tests_some_feature(parameters);
bool d_tests_another_feature(parameters);
```

in the source (definitions) \*.c file:

```c
/*
d_tests_some_feature
  brief description
  Tests the following:
  - summary
  - of what
  - is
  - specifically tested
  - in this function
*/
bool
d_tests_some_feature(
    parameters
)
{
  ...
}
```

### Macros

-   Macros and definitions for very specific cases should be hidden with the
    `D_INTERNAL_` prefix.

-   All `#define` statements, including both constants and macros, should
    consist of the following format (excluding blocks of iterative macros, see
    below):

```c
// D_DEFINED_OR_MACRO_NAME
//   macro: a brief, concise description of the macro, using multiple
// lines if necessary to avoid going over 80 total characters.
#define D_DEFINED_OR_MACRO_NAME 42

// D_SOME_MACRO
//   macro: brief macros should be defined on the following line,
// as shown.
#define D_SOME_MACRO(param1)     MACRO_DEFINITION

// D_SOME_OTHER_MACRO
//   macro: a macro whose parameter list would go over 80
// characters of total line width should have its parameters
// spread out:
#define D_SOME_OTHER_MACRO(param1,                                \
                           param2,                                \
                           param3,                                \
                           param4,                                \
                           param5,                                \
                           param6)                                \
    SOME_OTHER_MACRO_DEFINITION

// D_INTERNAL_SOME_STRUCT
//   macro: a macro whose definition covers multiple lines,
// ESPECIALLY with macros that define blocks of code or entire
// functions, should be broken up into smaller lines, with the
// backslash characters ('\') lined up. Function definitions in
// macros should resemble normally-defined functions in formatting.
#define D_INTERNAL_SOME_STRUCT(ret_type,                          \
                               fn_name,                           \
                               param1,                            \
                               param2,                            \
                               other_fn_name,                     \
                               call_params)                       \
    ret_type                                                      \
    d_some_struct_hypothetical_example_fn                         \
    (                                                             \
        struct d_some_struct* param1,                             \
        param2                                                    \
    )                                                             \
    {                                                             \
        if (param1)                                               \
        {                                                         \
            return other_fn_name(                                 \
                (struct d_some_struct*)param1,                    \
                param1->count,                                    \
                call_params);                                     \
        }                                                         \
                                                                  \
        return (ret_type){0}; /* or appropriate default */        \
    }
```

-   Iterative macros are macros that call one another in a chain, typically on
    adjacent lines with numerical suffixes. Frequently they are called by a
    root, or master function: e.g.

```c
#define D_EVAL(...)             D_INTERNAL_ARGS01(D_INTERNAL_ARGS01(D_INTERNAL_ARGS01(__VA_ARGS__)))

// D_INTERNAL_ARGS<01-10>
//   macro:
#define D_INTERNAL_ARGS01(...) D_INTERNAL_ARGS02(D_INTERNAL_ARGS02(D_INTERNAL_ARGS02(__VA_ARGS__)))
#define D_INTERNAL_ARGS02(...) D_INTERNAL_ARGS03(D_INTERNAL_ARGS03(D_INTERNAL_ARGS03(__VA_ARGS__)))
#define D_INTERNAL_ARGS03(...) D_INTERNAL_ARGS04(D_INTERNAL_ARGS04(D_INTERNAL_ARGS04(__VA_ARGS__)))
#define D_INTERNAL_ARGS04(...) D_INTERNAL_ARGS05(D_INTERNAL_ARGS05(D_INTERNAL_ARGS05(__VA_ARGS__)))
#define D_INTERNAL_ARGS05(...) D_INTERNAL_ARGS06(D_INTERNAL_ARGS06(D_INTERNAL_ARGS06(__VA_ARGS__)))
#define D_INTERNAL_ARGS06(...) D_INTERNAL_ARGS07(D_INTERNAL_ARGS07(D_INTERNAL_ARGS07(__VA_ARGS__)))
#define D_INTERNAL_ARGS07(...) D_INTERNAL_ARGS08(D_INTERNAL_ARGS08(D_INTERNAL_ARGS08(__VA_ARGS__)))
#define D_INTERNAL_ARGS08(...) D_INTERNAL_ARGS09(D_INTERNAL_ARGS09(D_INTERNAL_ARGS09(__VA_ARGS__)))
#define D_INTERNAL_ARGS09(...) D_INTERNAL_ARGS10(D_INTERNAL_ARGS10(D_INTERNAL_ARGS10(__VA_ARGS__)))
#define D_INTERNAL_ARGS10(...) __VA_ARGS__
```

Since all of the subfunctions called by `D_EVAL` are only called by `D_EVAL`,
and will probably only ever be used in that context, one comment for the entire
block is sufficient.

---

# Part II: C++ Only

## File Structure

### Header Files

-   Certain header files will have no extension (e.g. `djinterp`)
-   C++ module headers should be named `.hpp` and should contain only
    declarations whenever possible

### Source Files (.cpp)

-   Source definitions should be in `.cpp` files
-   Follow the same general structure as C source files (no header comment
    block, single include of corresponding header)

## Definition Comments

All C++ definitions (types, traits, classes, concepts, structs, etc.) should
have a brief 1-3 line comment immediately preceding them. The comment should
name the entity and describe its purpose.

**Format:**

```cpp
// <name>
//   <category>: <brief description of purpose>.
```

**Examples:**

```cpp
// nonesuch
//   type: placeholder type for detection idiom representing "no such type".
struct nonesuch
{
};

// detector
//   trait: primary template for SFINAE-based type detection (failure case).
template<typename _Default,
         typename _AlwaysVoid,
         template<typename...> typename _Op,
         typename...                    _Args>
struct detector
{
    using value_t = std::false_type;
    using type    = _Default;
};

// is_iterable
//   concept: constrains types that support range-based iteration.
template<typename _T>
concept is_iterable = requires(_T _t)
{
    std::begin(_t);
    std::end(_t);
};

// array_like
//   trait: determines if a type satisfies array-like interface requirements.
template<typename _T>
struct array_like
{
    static constexpr bool value =
        ( is_array_like_interface<_T>::value &&
          has_tuple_protocol<_T>::value      &&
          has_constexpr_size<_T>::value );
};
```

**Categories** (use as appropriate):

-   `type` — type aliases, placeholder types
-   `trait` — type traits, metafunctions
-   `concept` — C++20 concepts
-   `class` — class definitions
-   `struct` — struct definitions
-   `enum` — enumeration types

## Brackets

### Empty Bodies

Empty class, struct, and other bodies should have `{};` on its own line,
separate from the type declaration.

**Correct:**

```cpp
// nonesuch
//   type: placeholder type for detection idiom.
struct nonesuch
{};
```

**Incorrect:**

```cpp
struct nonesuch
{
};
```

## Templates

### Template Parameter Formatting

Template parameters should be formatted as follows:

-   One parameter per line, starting on the `template<...` line
-   Parameters should start with an underscore and line up on that underscore
-   **Always use `typename`**, never `class`, for type template parameters
-   Equal signs for default values should line up vertically
-   Type specifiers (`typename`, non-type specifiers) should align

**Example:**

```cpp
// detector
//   trait: primary template for SFINAE-based type detection (failure case).
template<typename                       _Default,
         typename                       _AlwaysVoid,
         template<typename...> typename _Op,
         typename...                    _Args>
struct detector
{
    using value_t = std::false_type;
    using type    = _Default;
};

// detector specialization (success case)
//   trait: partial specialization when _Op<_Args...> is well-formed.
template<typename                       _Default,
         template<typename...> typename _Op,
         typename...                    _Args>
struct detector<_Default, void_t<_Op<_Args...>>, _Op, _Args...>
{
    using value_t = std::true_type;
    using type    = _Op<_Args...>;
};
```

**Example with default values:**

```cpp
// fixed_array
//   class: fixed-size array container with configurable size and index types.
template<typename  _Type,
         typename  _Iterator,
         typename  _ConstIterator,
         typename  _DifferenceType = std::ptrdiff_t,
         typename  _SizeType       = std::size_t,
         _SizeType _MaxSize        = std::numeric_limits<_SizeType>::max()>
class fixed_array : public base<_Type, _DifferenceType, _SizeType, _MaxSize>
{
    // body
};
```

**Rules:**

-   Each template parameter on its own line
-   Align on the `_` character of parameter names
-   **Always use `typename`** for type parameters (never `class`)
-   Default value `=` operators should align vertically
-   `typename` and non-type parameter specifiers should align

## Boolean Expressions

All multi-condition boolean expressions, including `static constexpr bool`
member definitions, should be parenthesized with a top-level set of parentheses.

**Format:**

```cpp
static constexpr bool value =
    ( is_array_like_interface<clean_type>::value &&
      has_tuple_protocol<clean_type>::value      &&
      has_constexpr_size<clean_type>::value );
```

**Rules:**

-   Enclose entire expression in top-level parentheses
-   Each condition on its own line
-   Align boolean operators (`&&`, `||`) vertically
-   Closing `);` on the same line as the last condition

## Classes

### Member Ordering

Private aliases should be at the top, followed by protected, followed by public.
One empty line separating each section.

With the exception of type aliases, function declarations and members should be
at the bottom.

Private members and functions should start with `m_`, and should be declared at
the very end of the class.

**Format:**

```cpp
// some_class
//   class: brief description of purpose.
class some_class
{
private:
    // private type aliases

protected:
    // protected type aliases

public:
    // public type aliases

    // class body (public functions, etc.)

protected:
    // protected members

private:
    // private members (prefixed with m_)
};
```

**Example:**

```cpp
// some_class
//   class: example class demonstrating member ordering conventions.
class some_class
{
private:
    using internal_type = std::vector<int>;

protected:
    using size_type = std::size_t;

public:
    using value_type = int;
    using reference  = value_type&;

    some_class();
    ~some_class();

    void        do_something();
    value_type  get_value() const;

protected:
    size_type m_protected_count;

private:
    internal_type m_data;
    bool          m_initialized;
};
```

### Inheritance

Inheriting classes should be one per line, starting on the line with
`class ...` in it.

**Example:**

```cpp
// derived
//   class: example derived class with multiple inheritance.
class derived : public  base_one,
                public  base_two,
                private implementation_detail
{
    // body
};
```

### Constructors

Constructor definitions follow a specific formatting style that differs from
standard C-style function definitions. The opening parenthesis stays on the
same line as the constructor name, parameters are indented two levels, and the
closing parenthesis is indented one level with any `noexcept` specifier on the
same line.

**Format:**

```cpp
    constexpr chunk_view(
		const _Container& _c,
		size_type         _chunk_sz
	) noexcept
		: m_ref(_c),
		  m_chunk_sz(_chunk_sz)
	{}
```

**Rules:**

-   Opening parenthesis `(` on the same line as the constructor name
-   Parameters indented two levels from the constructor's base indentation,
    aligned on the `_` character as with standard function parameters
-   Closing parenthesis `)` on its own line, indented one level from the
    constructor's base indentation; `noexcept` (or other specifiers) on the
    same line as the closing parenthesis
-   Member initializer list colon `:` indented two levels, on the line
    following the closing parenthesis
-   Subsequent member initializers aligned with the first, with commas at the
    end of each preceding line
-   Empty body `{}` on its own line, indented one level from the constructor's
    base indentation

**Example with body:**

```cpp
    constexpr chunk_view(
		const _Container& _c,
		size_type         _chunk_sz
	) noexcept
		: m_ref(_c),
		  m_chunk_sz(_chunk_sz)
	{
		// body
	}
```

**Example with single parameter and no initializer list:**

```cpp
    explicit chunk_view(
		const _Container& _c
	) noexcept
	{}
```

## Type Traits

### Structure

1.  Helper types should be hidden within `NS_INTERNAL` (`namespace internal {`),
    with an empty line after `NS_INTERNAL` and an empty line before `NS_END`
    (`}`)
2.  The primary struct/class type should be just after the `NS_INTERNAL` block
3.  `::type` getters should be suffixed with `_t`

### SFINAE Implementation

SFINAE class/struct types may be defined in the header, but non-static body functions should be declared in the header and defined in the appropriate `.cpp` source file.

**Example:**

```cpp
// tuple_type_at
//   trait: extracts the type at a given index from a tuple or tuple-like type.
NS_INTERNAL

    // tuple_type_at_helper
    //   trait: internal helper for tuple type extraction (primary template).
    template<std::size_t _Index,
             typename    _Tuple>
    struct tuple_type_at_helper
    {};

    // tuple_type_at_helper<0, ...>
    //   trait: base case specialization for index 0.
    template<typename    _Head,
             typename... _Tail>
    struct tuple_type_at_helper<0, std::tuple<_Head, _Tail...>>
    {
        using type = _Head;

        static constexpr auto value(const std::tuple<_Head, _Tail...>& _t)
        {
            return std::get<0>(_t);
        }
    };

    // tuple_type_at_helper<_Index, ...>
    //   trait: recursive case specialization for index > 0.
    template<std::size_t _Index,
             typename    _Head,
             typename... _Tail>
    struct tuple_type_at_helper<_Index, std::tuple<_Head, _Tail...>>
    {
        using type = typename tuple_type_at_helper<_Index - 1,
                                                   std::tuple<_Tail...>>::type;
    };

NS_END  // internal

// tuple_type_at
//   trait: public interface for extracting tuple element types by index.
template<std::size_t _Index,
         typename... _Types>
struct tuple_type_at
{
private:
    using tuple_type = to_tuple_t<_Types...>;
    static_assert((_Index < std::tuple_size_v<tuple_type>),
                  "Non-type parameter `_Index` cannot be greater than or equal "
                  "to the tuple size of type parameter `_Tuple`.");

public:
    using type = internal::tuple_type_at_helper<_Index, tuple_type>;
};

// tuple_type_at_t
//   type: convenience alias for tuple_type_at<...>::type.
template<std::size_t _Index,
         typename... _Types>
using tuple_type_at_t = tuple_type_at<_Index, _Types...>::type;
```

### Type Trait Naming

-   Primary type traits: `snake_case` (e.g., `tuple_type_at`)
-   Type alias helpers: suffix with `_t` (e.g., `tuple_type_at_t`)
-   Value helpers: suffix with `_v` (e.g., `is_same_v`)
-   Internal helpers: suffix with `_helper` and place in `internal` namespace