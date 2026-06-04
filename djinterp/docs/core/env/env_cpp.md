# C++ Modules — `env_cpp98.h`, `env_cpp_features.h`

Two complementary headers describe C++ capabilities. `env_cpp98.h` covers the
pre-feature-test-macro era (the C++98 standard library headers); `env_cpp_features.h`
covers everything from C++11 onward using the `__cpp_*` / `__cpp_lib_*` feature-test
macros. Both require `env.h` first, and every macro is pre-definable to override
detection.

---

## `env_cpp98.h` — C++98 standard-library headers

C++98 predates feature-test macros, so these flags assume availability under any
conforming C++ implementation and mainly exist to (a) document the dependency and
(b) provide override hooks for non-conforming/embedded toolchains. Naming:
`D_ENV_CPP98_HAS_<HEADER>` — `1` if available, `0` otherwise. A few have real
detection logic (locale, RTTI, exceptions).

### A. Containers

`D_ENV_CPP98_HAS_VECTOR`, `_LIST`, `_DEQUE`, `_QUEUE`, `_STACK`, `_MAP`, `_SET`,
`_BITSET`.

### B. Algorithms and iterators

`D_ENV_CPP98_HAS_ALGORITHM`, `_ITERATOR`, `_FUNCTIONAL`, `_NUMERIC`.

### C. Strings and localization

`D_ENV_CPP98_HAS_STRING`, `_LOCALE` (the latter is `0` on `__AVR__`/`__ANDROID__`).

### D. I/O streams

`D_ENV_CPP98_HAS_IOSTREAM`, `_ISTREAM`, `_OSTREAM`, `_FSTREAM`, `_SSTREAM`,
`_IOMANIP`, `_IOS`, `_IOSFWD`, `_STREAMBUF`.

### E. Utilities

`D_ENV_CPP98_HAS_UTILITY`, `_MEMORY`, `_NEW`, `_LIMITS`, and the conditionally
detected `_TYPEINFO` (RTTI), `_EXCEPTION`, `_STDEXCEPT` (exceptions). The latter
three resolve to `0` when RTTI/exceptions are disabled by compiler flags.

### F. Numerics

`D_ENV_CPP98_HAS_COMPLEX`, `_VALARRAY`.

### Aggregate checks

| Symbol | Meaning |
| --- | --- |
| `D_ENV_CPP98_HAS_ALL_CONTAINERS` | All container headers present |
| `D_ENV_CPP98_HAS_ALL_ALGORITHMS` | All algorithm/iterator headers present |
| `D_ENV_CPP98_HAS_ALL_IOSTREAMS` | All I/O stream headers present |
| `D_ENV_CPP98_HAS_ALL_UTILITIES` | All utility headers present |
| `D_ENV_CPP98_HAS_ALL_NUMERICS` | All numeric headers present |
| `D_ENV_CPP98_HAS_FULL_STL` | The complete C++98 STL is present |

> Embedded note: bare-metal/embedded C++ toolchains commonly omit `<locale>`,
> `<exception>`/`<stdexcept>` (exceptions off), `<typeinfo>` (RTTI off), and the
> I/O streams. Override the relevant macro to `0` to model that.

---

## `env_cpp_features.h` — C++11–C++26 feature detection

Wraps the standard feature-test macros in a uniform interface. **Two families:**

- `D_ENV_CPP_FEATURE_LANG_*` — language features (`__cpp_*`)
- `D_ENV_CPP_FEATURE_STL_*` — standard-library features (`__cpp_lib_*`)

**Each feature gets five macros:**

| Suffix | Meaning |
| --- | --- |
| *(none)* | `1` if enabled, `0` if not |
| `_NAME` | The underlying `__cpp*` macro name, as a string |
| `_DESC` | Human-readable description |
| `_VAL` | The feature-test macro value (or `0L` if undefined) |
| `_VERS` | C++ version string, e.g. `"(C++17)"` |

So, for example, `if constexpr` exposes `D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR`,
`…_IF_CONSTEXPR_NAME` (`"__cpp_if_constexpr"`), `…_DESC`, `…_VAL`, and `…_VERS`.

### Language features by standard

- **C++11:** `ALIAS_TEMPLATES`, `ATTRIBUTES`, `CONSTEXPR`, `DECLTYPE`,
  `DELEGATING_CONSTRUCTORS`, `INHERITING_CONSTRUCTORS`, `INITIALIZER_LISTS`,
  `LAMBDAS`, `NSDMI`, `RANGE_BASED_FOR`, `RAW_STRINGS`, `REF_QUALIFIERS`,
  `RVALUE_REFERENCES`, `STATIC_ASSERT`, `THREADSAFE_STATIC_INIT`,
  `UNICODE_CHARACTERS`, `UNICODE_LITERALS`, `USER_DEFINED_LITERALS`,
  `VARIADIC_TEMPLATES`.
- **C++14:** `AGGREGATE_NSDMI`, `BINARY_LITERALS`, `DECLTYPE_AUTO`,
  `ENUMERATOR_ATTRIBUTES`, `GENERIC_LAMBDAS`, `INIT_CAPTURES`,
  `NAMESPACE_ATTRIBUTES`, `NONTYPE_TEMPLATE_ARGS`, `RETURN_TYPE_DEDUCTION`,
  `SIZED_DEALLOCATION`, `VARIABLE_TEMPLATES`.
- **C++17:** `AGGREGATE_BASES`, `ALIGNED_NEW`, `CAPTURE_STAR_THIS`,
  `CONSTEXPR_IN_DECLTYPE`, `DEDUCTION_GUIDES`, `FOLD_EXPRESSIONS`,
  `GUARANTEED_COPY_ELISION`, `HEX_FLOAT`, `IF_CONSTEXPR`, `INLINE_VARIABLES`,
  `NOEXCEPT_FUNCTION_TYPE`, `NONTYPE_TEMPLATE_PARAMETER_AUTO`,
  `STRUCTURED_BINDINGS`, `TEMPLATE_TEMPLATE_ARGS`, `VARIADIC_USING`.
- **C++20:** `AGGREGATE_PAREN_INIT`, `CHAR8_T`, `CONCEPTS` (plus `_CONCEPTS_TS`
  and `_CONCEPTS_CPP20` value-discriminated sub-flags), `CONDITIONAL_EXPLICIT`,
  `CONSTEVAL`, `CONSTEXPR_DYNAMIC_ALLOC`, `CONSTINIT`, `DESIGNATED_INITIALIZERS`,
  `IMPL_COROUTINE`, `IMPL_DESTROYING_DELETE`, `IMPL_THREE_WAY_COMPARISON`,
  `MODULES`, `USING_ENUM`.
- **C++23:** `AUTO_CAST`, `EXPLICIT_THIS_PARAMETER`, `IF_CONSTEVAL`,
  `IMPLICIT_MOVE`, `MULTIDIMENSIONAL_SUBSCRIPT`, `NAMED_CHARACTER_ESCAPES`,
  `SIZE_T_SUFFIX`, `STATIC_CALL_OPERATOR`.
- **C++26:** `CONSTEXPR_EXCEPTIONS`, `CONTRACTS`, `DELETED_FUNCTION`,
  `PACK_INDEXING`, `PLACEHOLDER_VARIABLES`, `PP_EMBED`, `TEMPLATE_PARAMETERS`,
  `TRIVIAL_RELOCATABILITY`, `TRIVIAL_UNION`, `VARIADIC_FRIEND`.

### Standard-library features by standard

- **C++14:** `CHRONO_UDLS`, `COMPLEX_UDLS`.
- **C++17:** `ADDRESSOF_CONSTEXPR`, `ANY`, `APPLY`, `ARRAY_CONSTEXPR`, `AS_CONST`,
  `BOOL_CONSTANT`, `BOYER_MOORE_SEARCHER`, `BYTE`, `CLAMP`, `FILESYSTEM`,
  `OPTIONAL`, `VARIANT`.
- **C++20:** `ASSUME_ALIGNED`, `ATOMIC_FLAG_TEST`, `ATOMIC_FLOAT`, `ATOMIC_REF`,
  `ATOMIC_WAIT`, `BARRIER`, `BIND_FRONT`, `BIT_CAST`, `BITOPS`,
  `BOUNDED_ARRAY_TRAITS`, `CHAR8_T`, `CONCEPTS`, `CONSTEXPR_ALGORITHMS`,
  `CONSTEXPR_COMPLEX`, `CONSTEXPR_DYNAMIC_ALLOC`, `CONSTEXPR_STRING`,
  `CONSTEXPR_VECTOR`, `COROUTINE`, `ENDIAN`, `FORMAT`, `JTHREAD`, `LATCH`,
  `MATH_CONSTANTS`, `RANGES`, `SEMAPHORE`, `SOURCE_LOCATION`, `SPAN`,
  `THREE_WAY_COMPARISON`, `TO_ARRAY`.
- **C++23:** `ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR`,
  `ASSOCIATIVE_HETEROGENEOUS_ERASURE`, `BIND_BACK`, `BYTESWAP`,
  `CONSTEXPR_BITSET`, `CONSTEXPR_CHARCONV`, `CONSTEXPR_CMATH`, `EXPECTED`,
  `FLAT_MAP`, `FLAT_SET`, `GENERATOR`, `MDSPAN`, `MOVE_ONLY_FUNCTION`, `PRINT`,
  `RANGES_TO_CONTAINER`, `SPANSTREAM`, `STACKTRACE`, `STDATOMIC_H`,
  `STRING_CONTAINS`, `STRING_RESIZE_AND_OVERWRITE`, `UNREACHABLE`.
- **C++26:** `ALGORITHM_DEFAULT_VALUE_TYPE`,
  `ASSOCIATIVE_HETEROGENEOUS_INSERTION`, `ATOMIC_MIN_MAX`, `CONSTEXPR_ATOMIC`,
  `CONSTEXPR_DEQUE`, `CONTRACTS`, `COPYABLE_FUNCTION`, `DEBUGGING`, `FORMAT_PATH`,
  `FUNCTION_REF`, `HAZARD_POINTER`, `HIVE`, `INPLACE_VECTOR`, `LINALG`,
  `POLYMORPHIC`, `RCU`, `SATURATION_ARITHMETIC`, `SENDERS`, `SIMD`,
  `TEXT_ENCODING`.

(Each name above is the suffix after `D_ENV_CPP_FEATURE_LANG_` or
`D_ENV_CPP_FEATURE_STL_`.)

### Aggregate feature checks

For each standard `XX` ∈ {11, 14, 17, 20, 23, 26}:

| Symbol | Meaning |
| --- | --- |
| `D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPPxx` | `1` if **all** language features of C++xx are present |
| `D_ENV_CPP_FEATURE_HAS_ALL_STL_CPPxx` | `1` if **all** library features of C++xx are present |
| `D_ENV_CPP_FEATURE_HAS_ALL_CPPxx` | `1` if both of the above are present |

(C++11 has only the `_LANG_` and combined `_CPP11` aggregates, since no `__cpp_lib_*`
features are tracked for C++11 here.)
