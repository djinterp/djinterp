/******************************************************************************
* djinterp [meta]                                                   lookup.hpp
*
* The C++ face of lookup.h: finding a record by a key whose position the record
* declares, expressible as a typed compile-time scan or as a `d_lookup_view`.
*
*
* WHAT C++ ADDS, AND WHY IT IS NOT JUST A WRAPPER
* ===============================================
*   lookup.h has to compare keys through a widened carrier, because a runtime
* descriptor cannot name a type. That is why it has four find entry points --
* unsigned, signed, string, bytes -- and why a key that is none of those has
* nowhere to go.
*
*   THE COMPILE-TIME FORM HAS NO SUCH LIMIT. `typed_view` compares keys with
* `operator<` and `operator==` on the member's own type, so a std::string key,
* a user comparator, a fixed_string, or anything else with an ordering works
* without a fifth entry point and without a widening step. The C form remains
* for the case it is for: bytes whose record type is not visible where the
* search happens.
*
*   BOTH PRODUCE THE SAME ANSWER SHAPE. `lookup_result` is the C struct's
* contract -- found, index, and ON A MISS THE INSERTION POINT rather than a
* sentinel -- so a caller written against one reads the same from the other.
*
*
* THE COMPARATOR IS A TYPE, NOT A FLAG
* ====================================
*   D_LOOKUP_FLAG_CASE_INSENSITIVE exists in C because a comparator cannot be
* passed at zero cost through a struct. Here it is a template parameter with a
* default, so `ascii_fold_less` is one comparator among any number and the
* framework's own is not privileged. `ci_less` is provided because the ASCII
* fold is the one comparison the C side had to get right for determinacy --
* std::tolower is locale-dependent, so a registry keyed on it answers
* differently under different locales, and that reasoning is unchanged here.
*
*
* SORTEDNESS IS STILL A CLAIM
* ===========================
*   `sorted_tag` and `unsorted_tag` pick the scan at compile time by overload
* rather than by branch, so the bisection and the walk are separate functions
* and neither pays for the other. What they do NOT do is verify the claim:
* asserting a view is ordered costs the scan the bisection was avoiding. A
* wrong tag misses keys that are present, exactly as a wrong flag does in C.
*
*
* path:      /inc/djinterp/core/meta/lookup.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.05
******************************************************************************/

#ifndef DJINTERP_META_LOOKUP_
#define DJINTERP_META_LOOKUP_ 1

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../c/util/lookup.h"
#include "./kv.hpp"


NS_DJINTERP


// I.     traits

NS_INTERNAL

    // lookup_less_t
    //   type: the result of `a < b`, if the expression is well-formed. An
    // alias template so the failure lands in the immediate context; see the
    // note in kv.hpp on why a member typedef would not.
    template<typename _Type>
    using lookup_less_t = decltype(std::declval<const _Type&>() <
                                   std::declval<const _Type&>());

    // lookup_equal_t
    //   type: the result of `a == b`, under the same rule.
    template<typename _Type>
    using lookup_equal_t = decltype(std::declval<const _Type&>() ==
                                    std::declval<const _Type&>());

NS_END  // internal

// is_ordered
//   trait: whether `_Type` has both an ordering and an equality (failure
// case). This is the whole requirement a key must meet, stated once.
template<typename _Type,
         typename _Enable = void>
struct is_ordered : std::false_type
{};

// is_ordered
//   trait: partial specialization for a type supporting `<` and `==`.
template<typename _Type>
struct is_ordered<_Type,
                  kv_void_t<internal::lookup_less_t<_Type>,
                            internal::lookup_equal_t<_Type>>>
    : std::true_type
{};

// is_lookup_key
//   trait: whether `_Type` may serve as a key. Ordering is the requirement;
// being a scalar is not, which is the difference from the C side.
template<typename _Type>
struct is_lookup_key
{
    static D_CONSTEXPR bool value = is_ordered<_Type>::value;
};

// is_lowerable_key
//   trait: whether a key of this type can also be searched for through the C
// entry points -- an integral or enumeration type the widened carrier can
// hold. A key that is ordered but not this is C++-only, and `lower_view` is
// gated on it rather than silently reinterpreting the bytes.
template<typename _Type>
struct is_lowerable_key
{
    static D_CONSTEXPR bool value = is_kv_scalar<_Type>::value;
};


// II.    ordering tags and comparators

// sorted_tag
//   type: asserts the records are in ascending key order, selecting the
// bisecting scan. A CLAIM, not a check -- verifying it costs the scan the
// bisection exists to avoid.
struct sorted_tag
{};

// unsorted_tag
//   type: makes no ordering claim, selecting the linear scan.
struct unsorted_tag
{};

// natural_less
//   struct: the default comparator -- `operator<` on the key's own type.
template<typename _Key>
struct natural_less
{
    D_CONSTEXPR bool
    operator()(
        const _Key& _lhs,
        const _Key& _rhs
    ) const D_NOEXCEPT
    {
        return (_lhs < _rhs);
    }
};

// ci_less
//   struct: case-insensitive ordering over NUL-terminated strings, folding
// ASCII only.
//   THE FOLD IS LOCALE-FREE AND DELIBERATELY ASCII-ONLY, for the reason
// lookup.h gives: std::tolower answers differently under different locales, so
// a registry keyed on it is not deterministic across builds. It defers to the
// C fold so there is one table and not two.
struct ci_less
{
    bool
    operator()(
        const char* _lhs,
        const char* _rhs
    ) const D_NOEXCEPT
    {
        return (::d_lookup_compare_string(_lhs, _rhs, true) < 0);
    }
};


// III.   the result

// lookup_result
//   struct: the outcome of a search. On a miss `index` is the INSERTION POINT
// rather than a sentinel, so a caller that failed to find a key already knows
// where to put it -- the same contract d_lookup_result carries.
template<typename _Record>
struct lookup_result
{
    const _Record* record;  // borrowed; null when not found
    std::size_t    index;   // position when found; insertion point when not
    bool           found;   // the verdict

    // operator bool
    //   function: the verdict, so a result may be tested directly.
    D_CONSTEXPR explicit operator bool() const D_NOEXCEPT
    {
        return found;
    }
};


// IV.    the compile-time view

// typed_view
//   class: a contiguous array of records and the field their key sits in,
// both known at compile time. The key is compared with its OWN TYPE, so no
// widening happens and any ordered type serves.
template<typename _Field,
         typename _Compare =
             natural_less<typename _Field::member_type>>
class typed_view
{
public:
    using record_type = typename _Field::record_type;
    using key_type    = typename _Field::member_type;
    using field_type  = _Field;
    using compare_type = _Compare;
    using result_type = lookup_result<record_type>;

    static_assert(is_lookup_key<key_type>::value,
                  "lookup: a key type must support both `<` and `==`.");

    typed_view(
        const record_type* _records,
        std::size_t        _count
    ) D_NOEXCEPT
        : m_records(_records),
          m_count(_count),
          m_compare(_Compare())
    {}

    typed_view(
        const record_type* _records,
        std::size_t        _count,
        const _Compare&    _compare
    ) D_NOEXCEPT
        : m_records(_records),
          m_count(_count),
          m_compare(_compare)
    {}

    const record_type* data() const D_NOEXCEPT
    {
        return m_records;
    }

    std::size_t size() const D_NOEXCEPT
    {
        return m_count;
    }

    bool empty() const D_NOEXCEPT
    {
        return ( (!m_records) ||
                 (m_count == 0u) );
    }

    // at
    //   function: the record at an index, or null past the end.
    const record_type* at(std::size_t _index) const D_NOEXCEPT
    {
        // an index past the end names no record
        if ( (empty()) ||
             (_index >= m_count) )
        {
            return NULL;
        }

        return (m_records + _index);
    }

    // key_at
    //   function: the key of the record at an index, by its own type.
    const key_type& key_at(std::size_t _index) const D_NOEXCEPT
    {
        return _Field::get(m_records[_index]);
    }

    // find
    //   function: the linear scan, selected by tag rather than by branch, so
    // the two scans are separate functions and neither pays for the other.
    result_type
    find(
        const key_type& _key,
        unsorted_tag
    ) const D_NOEXCEPT
    {
        result_type result = { NULL, 0u, false };
        std::size_t index;

        // an empty view misses at insertion point zero
        if (empty())
        {
            return result;
        }

        // walk every record; an unordered view appends on a miss
        for (index = 0u; index < m_count; ++index)
        {
            if ( (!m_compare(key_at(index), _key)) &&
                 (!m_compare(_key, key_at(index))) )
            {
                result.record = (m_records + index);
                result.index  = index;
                result.found  = true;

                return result;
            }
        }

        result.index = m_count;

        return result;
    }

    // find
    //   function: the bisecting scan, narrowing to the insertion point when
    // the key is absent.
    //   THE MIDPOINT IS low + (high - low) / 2, for the reason lookup.h gives:
    // the obvious form overflows and nothing would report it.
    result_type
    find(
        const key_type& _key,
        sorted_tag
    ) const D_NOEXCEPT
    {
        result_type result = { NULL, 0u, false };
        std::size_t low;
        std::size_t high;
        std::size_t mid;

        // an empty view misses at insertion point zero
        if (empty())
        {
            return result;
        }

        low  = 0u;
        high = m_count;

        // bisect, narrowing to the insertion point when the key is absent
        while (low < high)
        {
            mid = low + ((high - low) / 2u);

            // the record sorts before the key, so the key is to the right
            if (m_compare(key_at(mid), _key))
            {
                low = mid + 1u;
            }
            else if (m_compare(_key, key_at(mid)))
            {
                high = mid;
            }
            else
            {
                result.record = (m_records + mid);
                result.index  = mid;
                result.found  = true;

                return result;
            }
        }

        result.index = low;

        return result;
    }

    // find
    //   function: the unsorted scan, for a caller that names no tag. THE SAFE
    // DEFAULT: a wrong sortedness claim misses keys that are present, and the
    // linear scan is never wrong, only slower.
    result_type find(const key_type& _key) const D_NOEXCEPT
    {
        return find(_key, unsorted_tag());
    }

    // contains
    //   function: the verdict, for a caller that wants no record.
    bool contains(const key_type& _key) const D_NOEXCEPT
    {
        return find(_key).found;
    }

    const record_type* begin() const D_NOEXCEPT
    {
        return m_records;
    }

    const record_type* end() const D_NOEXCEPT
    {
        return (m_records + m_count);
    }

private:
    const record_type* m_records;
    std::size_t        m_count;
    _Compare           m_compare;
};

// make_typed_view
//   function: deduce a view from an array, so the record count is not written
// out and cannot disagree with the array.
template<typename _Field,
         typename _Record,
         std::size_t _Count>
D_INLINE typed_view<_Field>
make_typed_view(
    const _Record (&_records)[_Count]
) D_NOEXCEPT
{
    static_assert(std::is_same<_Record,
                               typename _Field::record_type>::value,
                  "lookup: the field's record type must match the array's.");

    return typed_view<_Field>(_records, _Count);
}


// V.     the runtime view

// lookup_view
//   class: the runtime form. PRIVATELY INHERITS THE C STRUCT AND ADDS NO DATA
// MEMBER, so its layout is the C struct's and the address of one is the
// address of the other.
class lookup_view : private ::d_lookup_view
{
public:
    lookup_view() D_NOEXCEPT
        : ::d_lookup_view()
    {}

    lookup_view(
        const void*     _records,
        std::size_t     _stride,
        std::size_t     _count,
        const kv_field& _key,
        std::uint16_t   _flags = D_LOOKUP_FLAG_NONE
    ) D_NOEXCEPT
    {
        static_cast< ::d_lookup_view&>(*this) =
            ::d_lookup_view_make(_records,
                                 _stride,
                                 _count,
                                 &_key.c_ref(),
                                 _flags);
    }

    bool empty() const D_NOEXCEPT
    {
        return ::d_lookup_view_is_empty(this);
    }

    std::size_t size() const D_NOEXCEPT
    {
        return ::d_lookup_view::count;
    }

    const void* at(std::size_t _index) const D_NOEXCEPT
    {
        return ::d_lookup_at(this, _index);
    }

    // find
    //   function: search by a scalar key, routed to the signed or unsigned C
    // entry point by the key's type rather than by the caller.
    template<typename _Key>
    typename std::enable_if<is_lowerable_key<_Key>::value,
                            lookup_result<void>>::type
    find(_Key _key) const D_NOEXCEPT
    {
        ::d_lookup_result   raw;
        lookup_result<void> result;

        // a signed key must not be widened as though it were unsigned
        if (std::is_signed<_Key>::value)
        {
            raw = ::d_lookup_find_signed(this,
                                         static_cast<std::int64_t>(_key));
        }
        else
        {
            raw = ::d_lookup_find_unsigned(this,
                                           static_cast<std::uint64_t>(_key));
        }

        result.record = raw.record;
        result.index  = raw.index;
        result.found  = raw.found;

        return result;
    }

    // find
    //   function: search by a string key, honouring the view's case flag.
    lookup_result<void> find(const char* _key) const D_NOEXCEPT
    {
        ::d_lookup_result   raw;
        lookup_result<void> result;

        raw = ::d_lookup_find_string(this, _key);

        result.record = raw.record;
        result.index  = raw.index;
        result.found  = raw.found;

        return result;
    }

    const ::d_lookup_view& c_ref() const D_NOEXCEPT
    {
        return *this;
    }
};


// VI.    lowering

// lower_view
//   function: a compile-time view as a runtime one, for handing to C.
//   ENABLED ONLY WHEN THE KEY CAN CROSS. A key that is ordered but not scalar
// -- a std::string, a fixed_string, a user type -- has no widened form, and
// reinterpreting its bytes as an integer is the class of mistake this whole
// module exists to remove. The substitution failure says so at the call site.
template<typename _View>
D_INLINE
typename std::enable_if<
    ( is_lowerable_key<typename _View::key_type>::value &&
      _View::field_type::has_offset ),
    lookup_view>::type
lower_view(
    const _View&  _view,
    std::uint16_t _flags = D_LOOKUP_FLAG_NONE
) D_NOEXCEPT
{
    static_assert(is_kv_addressable<typename _View::record_type>::value,
                  "lookup: a record whose view crosses to C must be "
                  "standard-layout and trivially copyable.");

    return lookup_view(_view.data(),
                       sizeof(typename _View::record_type),
                       _view.size(),
                       lower_field<typename _View::field_type>(),
                       _flags);
}


// VII.   layout assertions

static_assert((sizeof(lookup_view) == sizeof(::d_lookup_view)),
              "lookup_view must add no storage to d_lookup_view.");
static_assert(std::is_standard_layout<lookup_view>::value,
              "lookup_view must be standard-layout for the base to sit at "
              "offset zero.");

//   THE RESULT SHAPES MUST AGREE. A caller reading `index` off one and off the
// other must get the same meaning, so the member set is checked rather than
// trusted to stay parallel.
static_assert((sizeof(lookup_result<void>::index) ==
               sizeof(((::d_lookup_result*)0)->index)),
              "lookup_result::index must match d_lookup_result::index.");


NS_END  // djinterp


#endif  // DJINTERP_META_LOOKUP_
