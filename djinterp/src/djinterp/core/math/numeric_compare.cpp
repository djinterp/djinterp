/*******************************************************************************
* djinterp [core]                                            numeric_compare.cpp
*
*
* path:      /src/djinterp/core/math/numeric_compare.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp


constexpr D_INLINE std::string
normalize
(
    std::string_view _s
)
{
    std::size_t start;

    start = 0;

    // skip leading zeros
    while ( (start < _s.size() - 1) &&
            (_s[start] == '0') )
    {
        ++start;
    }

    if (start == 0)
    {
        return std::string(_s);
    }

    return std::string(_s.substr(start));
}

constexpr D_INLINE std::string
power_of_10
(
    std::size_t _n
)
{
    std::string result;

    result.reserve(_n + 1);
    result.push_back('1');
    result.append(_n, '0');

    return result;
}

constexpr D_INLINE std::string
pad_right
(
    std::string_view _s,
    std::size_t      _len
)
{
    std::string result;

    result = std::string(_s);

    if (result.size() < _len)
    {
        result.append(_len - result.size(), '0');
    }

    return result;
}

constexpr D_INLINE int
big_cmp
(
    std::string_view _a,
    std::string_view _b
)
{
    // compare lengths first
    if (_a.size() != _b.size())
    {
        return (_a.size() < _b.size()) ? -1 : 1;
    }

    // same length: lexicographic comparison works for digits
    for (std::size_t i = 0; i < _a.size(); ++i)
    {
        if (_a[i] != _b[i])
        {
            return (_a[i] < _b[i]) ? -1 : 1;
        }
    }

    return 0;
}

constexpr D_INLINE std::string
big_add
(
    std::string_view _a,
    std::string_view _b
)
{
    std::string result;
    std::size_t max_len;
    int         carry;
    std::size_t i;

    max_len = std::max(_a.size(), _b.size());
    result.reserve(max_len + 1);
    carry = 0;

    for (i = 0; i < max_len || carry; ++i)
    {
        int digit_a;
        int digit_b;
        int sum;

        digit_a = (i < _a.size()) ? (_a[_a.size() - 1 - i] - '0') : 0;
        digit_b = (i < _b.size()) ? (_b[_b.size() - 1 - i] - '0') : 0;
        sum     = digit_a + digit_b + carry;
        carry   = sum / 10;

        result.push_back(static_cast<char>('0' + (sum % 10)));
    }

    // reverse to get correct order
    std::reverse(result.begin(), result.end());

    return result;
}

constexpr D_INLINE std::string
big_sub
(
    std::string_view _a,
    std::string_view _b
)
{
    std::string result;
    int         borrow;
    std::size_t i;

    // precondition: _a >= _b
    result.reserve(_a.size());
    borrow = 0;

    for (i = 0; i < _a.size(); ++i)
    {
        int digit_a;
        int digit_b;
        int diff;

        digit_a = _a[_a.size() - 1 - i] - '0';
        digit_b = (i < _b.size()) ? (_b[_b.size() - 1 - i] - '0') : 0;
        diff    = digit_a - digit_b - borrow;

        if (diff < 0)
        {
            diff   += 10;
            borrow  = 1;
        }
        else
        {
            borrow = 0;
        }

        result.push_back(static_cast<char>('0' + diff));
    }

    // reverse to get correct order
    std::reverse(result.begin(), result.end());

    return normalize(result);
}

constexpr D_INLINE std::string
big_mul
(
    std::string_view _a,
    std::string_view _b
)
{
    std::size_t len_a;
    std::size_t len_b;
    std::string result;

    len_a = _a.size();
    len_b = _b.size();

    // handle zero cases
    if ( (len_a == 1 && _a[0] == '0') ||
         (len_b == 1 && _b[0] == '0') )
    {
        return "0";
    }

    result.assign(len_a + len_b, '0');

    // grade-school multiplication
    for (std::size_t i = 0; i < len_a; ++i)
    {
        int carry;

        carry = 0;

        for (std::size_t j = 0; j < len_b || carry; ++j)
        {
            int digit_a;
            int digit_b;
            int current;
            int product;
            std::size_t pos;

            pos     = (len_a - 1 - i) + (len_b - 1 - j) + 1;
            digit_a = _a[len_a - 1 - i] - '0';
            digit_b = (j < len_b) ? (_b[len_b - 1 - j] - '0') : 0;
            current = result[pos] - '0';
            product = current + (digit_a * digit_b) + carry;
            carry   = product / 10;

            result[pos] = static_cast<char>('0' + (product % 10));
        }
    }

    return normalize(result);
}

constexpr D_INLINE std::string
big_mul_10
(
    std::string_view _a
)
{
    // simple case: append '0'
    if ( (_a.size() == 1) &&
         (_a[0] == '0') )
    {
        return "0";
    }

    std::string result;

    result = std::string(_a);
    result.push_back('0');

    return result;
}

}  // namespace internal

// ============================================================================
// integer_constant implementation
// ============================================================================

constexpr D_INLINE
integer_constant::integer_constant()
    : m_magnitude("0")
    , m_negative(false)
{
}

constexpr D_INLINE
integer_constant::integer_constant
(
    std::string_view _value
)
    : m_magnitude()
    , m_negative(false)
{
    std::size_t pos;

    pos = 0;

    // skip whitespace
    while ( (pos < _value.size()) &&
            (_value[pos] == ' ') )
    {
        ++pos;
    }

    // parse sign
    if (pos < _value.size())
    {
        if (_value[pos] == '-')
        {
            m_negative = true;
            ++pos;
        }
        else if (_value[pos] == '+')
        {
            ++pos;
        }
    }

    // extract digits
    std::size_t start;

    start = pos;

    while ( (pos < _value.size()) &&
            (_value[pos] >= '0')  &&
            (_value[pos] <= '9') )
    {
        ++pos;
    }

    if (pos == start)
    {
        m_magnitude = "0";
        m_negative  = false;
    }
    else
    {
        m_magnitude = internal::normalize(_value.substr(start, pos - start));

        // zero is never negative
        if ( (m_magnitude.size() == 1) &&
             (m_magnitude[0] == '0') )
        {
            m_negative = false;
        }
    }
}

constexpr D_INLINE
integer_constant::integer_constant
(
    std::int64_t _value
)
    : m_magnitude()
    , m_negative(_value < 0)
{
    std::uint64_t abs_value;

    if (_value < 0)
    {
        // handle INT64_MIN carefully
        abs_value = static_cast<std::uint64_t>(-(_value + 1)) + 1;
    }
    else
    {
        abs_value = static_cast<std::uint64_t>(_value);
    }

    if (abs_value == 0)
    {
        m_magnitude = "0";
        m_negative  = false;

        return;
    }

    // extract digits
    while (abs_value > 0)
    {
        m_magnitude.push_back(static_cast<char>('0' + (abs_value % 10)));
        abs_value /= 10;
    }

    std::reverse(m_magnitude.begin(), m_magnitude.end());
}

constexpr D_INLINE bool
integer_constant::is_negative() const
{
    return m_negative;
}

constexpr D_INLINE bool
integer_constant::is_zero() const
{
    return ( (m_magnitude.size() == 1) &&
             (m_magnitude[0] == '0') );
}

constexpr D_INLINE std::string_view
integer_constant::magnitude() const
{
    return m_magnitude;
}

constexpr D_INLINE std::string
integer_constant::to_string() const
{
    if (m_negative)
    {
        return "-" + m_magnitude;
    }

    return m_magnitude;
}

constexpr D_INLINE std::strong_ordering
integer_constant::operator<=>
(
    const integer_constant& _other
) const
{
    int cmp;

    cmp = compare(*this, _other);

    if (cmp < 0)
    {
        return std::strong_ordering::less;
    }

    if (cmp > 0)
    {
        return std::strong_ordering::greater;
    }

    return std::strong_ordering::equal;
}

constexpr D_INLINE bool
integer_constant::operator==
(
    const integer_constant& _other
) const
{
    return compare(*this, _other) == 0;
}

constexpr D_INLINE std::strong_ordering
integer_constant::operator<=>
(
    const decimal_constant& _other
) const
{
    int cmp;

    cmp = compare(*this, _other);

    if (cmp < 0)
    {
        return std::strong_ordering::less;
    }

    if (cmp > 0)
    {
        return std::strong_ordering::greater;
    }

    return std::strong_ordering::equal;
}

constexpr D_INLINE std::strong_ordering
integer_constant::operator<=>
(
    const rational_constant& _other
) const
{
    int cmp;

    cmp = compare(*this, _other);

    if (cmp < 0)
    {
        return std::strong_ordering::less;
    }

    if (cmp > 0)
    {
        return std::strong_ordering::greater;
    }

    return std::strong_ordering::equal;
}

constexpr D_INLINE bool
integer_constant::operator==
(
    const decimal_constant& _other
) const
{
    return compare(*this, _other) == 0;
}

constexpr D_INLINE bool
integer_constant::operator==
(
    const rational_constant& _other
) const
{
    return compare(*this, _other) == 0;
}

// ============================================================================
// decimal_constant implementation
// ============================================================================

constexpr D_INLINE
decimal_constant::decimal_constant()
    : m_integer_part("0")
    , m_fractional_part("")
    , m_negative(false)
{
}

constexpr D_INLINE
decimal_constant::decimal_constant
(
    std::string_view _value
)
    : m_integer_part()
    , m_fractional_part()
    , m_negative(false)
{
    std::size_t pos;

    pos = 0;

    // skip whitespace
    while ( (pos < _value.size()) &&
            (_value[pos] == ' ') )
    {
        ++pos;
    }

    // parse sign
    if (pos < _value.size())
    {
        if (_value[pos] == '-')
        {
            m_negative = true;
            ++pos;
        }
        else if (_value[pos] == '+')
        {
            ++pos;
        }
    }

    // parse integer part
    std::size_t int_start;

    int_start = pos;

    while ( (pos < _value.size()) &&
            (_value[pos] >= '0')  &&
            (_value[pos] <= '9') )
    {
        ++pos;
    }

    if (pos > int_start)
    {
        m_integer_part = internal::normalize(_value.substr(int_start,
                                                           pos - int_start));
    }
    else
    {
        m_integer_part = "0";
    }

    // parse fractional part
    if ( (pos < _value.size()) &&
         (_value[pos] == '.') )
    {
        ++pos;

        std::size_t frac_start;

        frac_start = pos;

        while ( (pos < _value.size()) &&
                (_value[pos] >= '0')  &&
                (_value[pos] <= '9') )
        {
            ++pos;
        }

        if (pos > frac_start)
        {
            m_fractional_part = std::string(_value.substr(frac_start,
                                                          pos - frac_start));

            // remove trailing zeros from fractional part
            while ( (!m_fractional_part.empty()) &&
                    (m_fractional_part.back() == '0') )
            {
                m_fractional_part.pop_back();
            }
        }
    }

    // zero is never negative
    if ( is_zero() )
    {
        m_negative = false;
    }
}

constexpr D_INLINE bool
decimal_constant::is_negative() const
{
    return m_negative;
}

constexpr D_INLINE bool
decimal_constant::is_zero() const
{
    return ( (m_integer_part.size() == 1)   &&
             (m_integer_part[0] == '0')     &&
             (m_fractional_part.empty()) );
}

constexpr D_INLINE std::string_view
decimal_constant::integer_part() const
{
    return m_integer_part;
}

constexpr D_INLINE std::string_view
decimal_constant::fractional_part() const
{
    return m_fractional_part;
}

constexpr D_INLINE std::size_t
decimal_constant::fractional_digits() const
{
    return m_fractional_part.size();
}

constexpr D_INLINE std::string
decimal_constant::to_string() const
{
    std::string result;

    if (m_negative)
    {
        result = "-";
    }

    result += m_integer_part;

    if (!m_fractional_part.empty())
    {
        result += ".";
        result += m_fractional_part;
    }

    return result;
}

constexpr D_INLINE std::strong_ordering
decimal_constant::operator<=>
(
    const decimal_constant& _other
) const
{
    int cmp;

    cmp = compare(*this, _other);

    if (cmp < 0)
    {
        return std::strong_ordering::less;
    }

    if (cmp > 0)
    {
        return std::strong_ordering::greater;
    }

    return std::strong_ordering::equal;
}

constexpr D_INLINE bool
decimal_constant::operator==
(
    const decimal_constant& _other
) const
{
    return compare(*this, _other) == 0;
}

constexpr D_INLINE std::strong_ordering
decimal_constant::operator<=>
(
    const integer_constant& _other
) const
{
    int cmp;

    cmp = compare(*this, _other);

    if (cmp < 0)
    {
        return std::strong_ordering::less;
    }

    if (cmp > 0)
    {
        return std::strong_ordering::greater;
    }

    return std::strong_ordering::equal;
}

constexpr D_INLINE std::strong_ordering
decimal_constant::operator<=>
(
    const rational_constant& _other
) const
{
    int cmp;

    cmp = compare(*this, _other);

    if (cmp < 0)
    {
        return std::strong_ordering::less;
    }

    if (cmp > 0)
    {
        return std::strong_ordering::greater;
    }

    return std::strong_ordering::equal;
}

constexpr D_INLINE bool
decimal_constant::operator==
(
    const integer_constant& _other
) const
{
    return compare(*this, _other) == 0;
}

constexpr D_INLINE bool
decimal_constant::operator==
(
    const rational_constant& _other
) const
{
    return compare(*this, _other) == 0;
}

// ============================================================================
// rational_constant implementation
// ============================================================================

constexpr D_INLINE
rational_constant::rational_constant()
    : m_numerator("0")
    , m_denominator("1")
    , m_negative(false)
{
}

constexpr D_INLINE
rational_constant::rational_constant
(
    std::string_view _numerator,
    std::string_view _denominator,
    bool             _negative
)
    : m_numerator(internal::normalize(_numerator))
    , m_denominator(internal::normalize(_denominator))
    , m_negative(_negative)
{
    // zero is never negative
    if ( (m_numerator.size() == 1) &&
         (m_numerator[0] == '0') )
    {
        m_negative = false;
    }
}

constexpr D_INLINE
rational_constant::rational_constant
(
    std::string_view _value
)
    : m_numerator()
    , m_denominator("1")
    , m_negative(false)
{
    std::size_t pos;

    pos = 0;

    // skip whitespace
    while ( (pos < _value.size()) &&
            (_value[pos] == ' ') )
    {
        ++pos;
    }

    // parse sign
    if (pos < _value.size())
    {
        if (_value[pos] == '-')
        {
            m_negative = true;
            ++pos;
        }
        else if (_value[pos] == '+')
        {
            ++pos;
        }
    }

    // parse numerator
    std::size_t num_start;

    num_start = pos;

    while ( (pos < _value.size()) &&
            (_value[pos] >= '0')  &&
            (_value[pos] <= '9') )
    {
        ++pos;
    }

    if (pos > num_start)
    {
        m_numerator = internal::normalize(_value.substr(num_start,
                                                        pos - num_start));
    }
    else
    {
        m_numerator = "0";
    }

    // check for fraction slash
    if ( (pos < _value.size()) &&
         (_value[pos] == '/') )
    {
        ++pos;

        std::size_t denom_start;

        denom_start = pos;

        while ( (pos < _value.size()) &&
                (_value[pos] >= '0')  &&
                (_value[pos] <= '9') )
        {
            ++pos;
        }

        if (pos > denom_start)
        {
            m_denominator = internal::normalize(_value.substr(denom_start,
                                                              pos - denom_start));
        }
    }

    // zero is never negative
    if ( (m_numerator.size() == 1) &&
         (m_numerator[0] == '0') )
    {
        m_negative = false;
    }
}

constexpr D_INLINE bool
rational_constant::is_negative() const
{
    return m_negative;
}

constexpr D_INLINE bool
rational_constant::is_zero() const
{
    return ( (m_numerator.size() == 1) &&
             (m_numerator[0] == '0') );
}

constexpr D_INLINE std::string_view
rational_constant::numerator() const
{
    return m_numerator;
}

constexpr D_INLINE std::string_view
rational_constant::denominator() const
{
    return m_denominator;
}

constexpr D_INLINE std::string
rational_constant::to_string() const
{
    std::string result;

    if (m_negative)
    {
        result = "-";
    }

    result += m_numerator;
    result += "/";
    result += m_denominator;

    return result;
}

constexpr D_INLINE std::strong_ordering
rational_constant::operator<=>
(
    const rational_constant& _other
) const
{
    int cmp;

    cmp = compare(*this, _other);

    if (cmp < 0)
    {
        return std::strong_ordering::less;
    }

    if (cmp > 0)
    {
        return std::strong_ordering::greater;
    }

    return std::strong_ordering::equal;
}

constexpr D_INLINE bool
rational_constant::operator==
(
    const rational_constant& _other
) const
{
    return compare(*this, _other) == 0;
}

constexpr D_INLINE std::strong_ordering
rational_constant::operator<=>
(
    const integer_constant& _other
) const
{
    int cmp;

    cmp = compare(*this, _other);

    if (cmp < 0)
    {
        return std::strong_ordering::less;
    }

    if (cmp > 0)
    {
        return std::strong_ordering::greater;
    }

    return std::strong_ordering::equal;
}

constexpr D_INLINE std::strong_ordering
rational_constant::operator<=>
(
    const decimal_constant& _other
) const
{
    int cmp;

    cmp = compare(*this, _other);

    if (cmp < 0)
    {
        return std::strong_ordering::less;
    }

    if (cmp > 0)
    {
        return std::strong_ordering::greater;
    }

    return std::strong_ordering::equal;
}

constexpr D_INLINE bool
rational_constant::operator==
(
    const integer_constant& _other
) const
{
    return compare(*this, _other) == 0;
}

constexpr D_INLINE bool
rational_constant::operator==
(
    const decimal_constant& _other
) const
{
    return compare(*this, _other) == 0;
}

// ============================================================================
// Free comparison function implementations
// ============================================================================

// compare (integer, integer)
constexpr D_INLINE int
compare
(
    const integer_constant& _a,
    const integer_constant& _b
)
{
    // handle zero cases
    if ( _a.is_zero() &&
         _b.is_zero() )
    {
        return 0;
    }

    if (_a.is_zero())
    {
        return _b.is_negative() ? 1 : -1;
    }

    if (_b.is_zero())
    {
        return _a.is_negative() ? -1 : 1;
    }

    // handle sign differences
    if ( _a.is_negative() &&
         !_b.is_negative() )
    {
        return -1;
    }

    if ( !_a.is_negative() &&
         _b.is_negative() )
    {
        return 1;
    }

    // same sign: compare magnitudes
    int mag_cmp;

    mag_cmp = internal::big_cmp(_a.magnitude(), _b.magnitude());

    // if both negative, flip the result
    if (_a.is_negative())
    {
        return -mag_cmp;
    }

    return mag_cmp;
}

// compare (decimal, decimal)
constexpr D_INLINE int
compare
(
    const decimal_constant& _a,
    const decimal_constant& _b
)
{
    // handle zero cases
    if ( _a.is_zero() &&
         _b.is_zero() )
    {
        return 0;
    }

    if (_a.is_zero())
    {
        return _b.is_negative() ? 1 : -1;
    }

    if (_b.is_zero())
    {
        return _a.is_negative() ? -1 : 1;
    }

    // handle sign differences
    if ( _a.is_negative() &&
         !_b.is_negative() )
    {
        return -1;
    }

    if ( !_a.is_negative() &&
         _b.is_negative() )
    {
        return 1;
    }

    // same sign: compare integer parts first
    int int_cmp;

    int_cmp = internal::big_cmp(_a.integer_part(), _b.integer_part());

    if (int_cmp != 0)
    {
        return _a.is_negative() ? -int_cmp : int_cmp;
    }

    // integer parts equal: compare fractional parts
    // pad to same length for comparison
    std::size_t max_frac_len;
    std::string frac_a;
    std::string frac_b;
    int         frac_cmp;

    max_frac_len = std::max(_a.fractional_digits(), _b.fractional_digits());
    frac_a       = internal::pad_right(_a.fractional_part(), max_frac_len);
    frac_b       = internal::pad_right(_b.fractional_part(), max_frac_len);

    // handle empty fractional parts
    if (frac_a.empty())
    {
        frac_a = "0";
    }

    if (frac_b.empty())
    {
        frac_b = "0";
    }

    frac_cmp = internal::big_cmp(frac_a, frac_b);

    return _a.is_negative() ? -frac_cmp : frac_cmp;
}

// compare (rational, rational)
//   Uses cross-multiplication: a/b vs c/d => a*d vs b*c
constexpr D_INLINE int
compare
(
    const rational_constant& _a,
    const rational_constant& _b
)
{
    // handle zero cases
    if ( _a.is_zero() &&
         _b.is_zero() )
    {
        return 0;
    }

    if (_a.is_zero())
    {
        return _b.is_negative() ? 1 : -1;
    }

    if (_b.is_zero())
    {
        return _a.is_negative() ? -1 : 1;
    }

    // handle sign differences
    if ( _a.is_negative() &&
         !_b.is_negative() )
    {
        return -1;
    }

    if ( !_a.is_negative() &&
         _b.is_negative() )
    {
        return 1;
    }

    // same sign: cross-multiply
    // a/b vs c/d => a*d vs b*c
    std::string left_product;
    std::string right_product;
    int         mag_cmp;

    left_product  = internal::big_mul(_a.numerator(),   _b.denominator());
    right_product = internal::big_mul(_a.denominator(), _b.numerator());
    mag_cmp       = internal::big_cmp(left_product, right_product);

    // if both negative, flip the result
    if (_a.is_negative())
    {
        return -mag_cmp;
    }

    return mag_cmp;
}

// compare (integer, decimal)
constexpr D_INLINE int
compare
(
    const integer_constant& _a,
    const decimal_constant& _b
)
{
    // convert integer to decimal (integer part only, no fractional)
    decimal_constant a_as_decimal(_a.to_string());

    return compare(a_as_decimal, _b);
}

// compare (decimal, integer)
constexpr D_INLINE int
compare
(
    const decimal_constant& _a,
    const integer_constant& _b
)
{
    return -compare(_b, _a);
}

// compare (integer, rational)
//   integer n compared to a/b => n*b vs a
constexpr D_INLINE int
compare
(
    const integer_constant&  _a,
    const rational_constant& _b
)
{
    // convert integer to rational: n/1
    rational_constant a_as_rational(_a.magnitude(), "1", _a.is_negative());

    return compare(a_as_rational, _b);
}

// compare (rational, integer)
constexpr D_INLINE int
compare
(
    const rational_constant& _a,
    const integer_constant&  _b
)
{
    return -compare(_b, _a);
}

// compare (decimal, rational)
//   Convert decimal to rational: 123.456 = 123456/1000
constexpr D_INLINE int
compare
(
    const decimal_constant&  _a,
    const rational_constant& _b
)
{
    // build rational from decimal
    std::string numerator;
    std::string denominator;

    numerator = std::string(_a.integer_part());

    if (_a.fractional_digits() > 0)
    {
        numerator  += std::string(_a.fractional_part());
        denominator = internal::power_of_10(_a.fractional_digits());
    }
    else
    {
        denominator = "1";
    }

    numerator = internal::normalize(numerator);

    rational_constant a_as_rational(numerator, denominator, _a.is_negative());

    return compare(a_as_rational, _b);
}

// compare (rational, decimal)
constexpr D_INLINE int
compare
(
    const rational_constant& _a,
    const decimal_constant&  _b
)
{
    return -compare(_b, _a);
}
