"""gen_c_math.log_d - the text of log_d.h (S439).

One module of scripts/gen_c_math_float.py, which runs them all; see it."""

from .power import (power)
from .exp_d import (exp_d_digits_literal, exp_d_hex_literal, exp_d_literal,
                    exp_d_round)


# -----------------------------------------------------------------------------
# exponential.h: the double family's natural logarithm, correctly rounded on
# doubles and pairs of doubles (chunk 52, S424)
# -----------------------------------------------------------------------------
# The constants are computed here with integers alone, each rounding checked
# to be the same at both ends of an interval that holds the exact value; the
# polynomials' coefficients are Sollya's (chunk 52's record,
# proofs/log_poly.sollya), and their error bounds and the three phases' are
# proven there (proofs/).

LOG_D_BITS = 520


def log_d_ln_fixed(num, den):
    """ln(num/den) times 2^LOG_D_BITS, num and den positive and num/den in
    [1/2, 2], as (value, error): 2 atanh(s), s = (num - den)/(num + den), its
    series in integers, each term truncated; the value within error units of
    the exact one."""
    a, d = abs(num - den), num + den
    one = 1 << LOG_D_BITS
    s = (a << LOG_D_BITS) // d          # below s 2^B by less than 1
    s2 = (a * a << LOG_D_BITS) // (d * d)
    total, power, k, cuts = 0, s, 1, 1
    while power:
        total += power // k
        power = (power * s2) >> LOG_D_BITS
        k += 2
        cuts += 2
    value = 2 * total
    # each power below its exact value by at most the number of steps so far,
    # each quotient by 1 more, the dropped tail below 1, all doubled
    error = 2 * (cuts * cuts + 2)
    return (value if num >= den else -value), error


def log_d_round_interval(lo, hi, scale, bits=53):
    """the value in [lo, hi] 2^-scale rounded to bits bits, as (m, e), checked
    to be the same at both ends."""
    a = exp_d_round(lo, 1 << scale, bits)
    b = exp_d_round(hi, 1 << scale, bits)
    assert a == b, "a rounding the interval leaves undecided"
    return a


# the inverses' significant bits: at most 26, so that m inv is exact by
# Dekker's product with m alone split (S432)
LOG_D_INV_BITS = 26


def log_d_constants():
    """ln2 in parts, and the table: for i from 0 to 128 the reciprocal inv_i
    of c_i = 1 + i/128 (of c_i/2 from i = 53) rounded to LOG_D_INV_BITS
    bits, 1 at 0 and 128, stored halved from 53 (so that m need not be),
    and -ln(inv_i) as a pair; also |-ln(inv_i)| 2^224 rounded down, as
    hexadecimal digits."""
    from fractions import Fraction
    B = LOG_D_BITS
    ln2, e2 = log_d_ln_fixed(2, 1)
    consts = {}
    parts = []
    rest_lo, rest_hi = ln2 - e2, ln2 + e2
    for name, bits in (("LN2H", 42), ("LN2M", 42), ("LN2L", 53)):
        m, e = log_d_round_interval(rest_lo, rest_hi, B, bits)
        v = Fraction(m) * Fraction(2) ** e
        consts[name] = exp_d_literal(m, e)
        parts.append(v)
        rest_lo -= int(v * (1 << B))
        rest_hi -= int(v * (1 << B))
    # phase 1's: ln2 - LN2H rounded to 53 bits
    vh = int(parts[0] * (1 << B))
    consts["LN2L1"] = exp_d_literal(*log_d_round_interval(ln2 - e2 - vh,
                                                          ln2 + e2 - vh, B))
    rows, digits = [], []
    for i in range(129):
        if i in (0, 128):
            inv = Fraction(1)
        else:
            num = 128 if i < 53 else 256
            m, e = exp_d_round(num, 128 + i, LOG_D_INV_BITS)
            inv = Fraction(m) * Fraction(2) ** e
        stored = inv / 2 if i >= 53 else inv
        if inv == 1:
            rows.append(("1.0" if i == 0 else exp_d_literal(1, -1), "0.0",
                         "0.0"))
            digits.append("0" * 56)
            continue
        # -ln(inv) = ln(1/inv)
        L, err = log_d_ln_fixed(inv.denominator, inv.numerator)
        mh, eh = log_d_round_interval(L - err, L + err, B)
        vh = int(Fraction(mh) * Fraction(2) ** eh * (1 << B))
        ml, el = log_d_round_interval(L - err - vh, L + err - vh, B)
        rows.append((exp_d_literal(*exp_d_round(stored.numerator,
                                                stored.denominator)),
                     exp_d_literal(mh, eh), exp_d_literal(ml, el)))
        lo_d = (abs(L) - err) >> (B - 224)
        hi_d = (abs(L) + err) >> (B - 224)
        assert lo_d == hi_d, "a digit the interval leaves undecided"
        digits.append("%056x" % lo_d)
    return consts, rows, "".join(digits)


# log_fast's table (S500): x = 2^k z, z in [0x1.6p-1, 0x1.6p0), as the
# bits of x less those of 0x1.6p-1 give them; row i, the bits of z's
# significand from the 46th, holds z in [0x1.6p-1 + i 2^-8, + 2^-8) for i
# below 80 and [1 + (i - 80) 2^-7, + 2^-7) from 80, with c the row's middle;
# inv a double near 1/c, chosen so that 1/inv = chi + clo within 2^-66, chi
# a double and clo a multiple of 2^-60, which makes z - chi - clo exact; L =
# -log(inv) as L1, a multiple of 2^-42 so that k LN2H + L1 is exact, and L2,
# the rest rounded.
LOG_D_FAST_CLO = 60
LOG_D_FAST_RES = 66


def log_d_fast_interval(i):
    """row i's z, as [lo, hi), exact."""
    from fractions import Fraction
    if i < 80:
        lo = Fraction(11, 16) + Fraction(i, 256)
        return lo, lo + Fraction(1, 256)
    lo = 1 + Fraction(i - 80, 128)
    return lo, lo + Fraction(1, 128)


def log_d_fast_rows():
    """log_fast's table: 128 rows of (inv, chi, clo, L1, L2), as literals,
    each value checked exact as written."""
    from fractions import Fraction
    B = LOG_D_BITS
    unit = Fraction(1, 1 << LOG_D_FAST_CLO)
    rows = []
    for i in range(128):
        lo, hi = log_d_fast_interval(i)
        c = (lo + hi) / 2
        m0, e0 = exp_d_round(c.denominator, c.numerator)
        ulp = Fraction(2) ** e0
        found = None
        for k in range(0, 4096):
            for step in ((k,) if k == 0 else (k, -k)):
                inv = (m0 + step) * ulp
                cp = 1 / inv
                mh, eh = exp_d_round(cp.numerator, cp.denominator)
                rem = cp - Fraction(mh) * Fraction(2) ** eh
                n = round(rem / unit)
                if abs(rem - n * unit) <= Fraction(1, 1 << LOG_D_FAST_RES):
                    found = (inv, mh, eh, n)
                    break
            if found:
                break
        assert found, "no inverse found for row %d" % i
        inv, mh, eh, n = found
        # -log(inv) = log(1/inv), 1/inv in [1/2, 2]
        cp = 1 / inv
        L, err = log_d_ln_fixed(cp.numerator, cp.denominator)
        lo_r = ((L - err) + (1 << (B - 43))) >> (B - 42)
        hi_r = ((L + err) + (1 << (B - 43))) >> (B - 42)
        assert lo_r == hi_r, "L1 undecided"
        l1 = lo_r
        ml, el = log_d_round_interval((L - err) - (l1 << (B - 42)),
                                      (L + err) - (l1 << (B - 42)), B)
        rows.append((exp_d_literal(*exp_d_round(inv.numerator,
                                                 inv.denominator)),
                     exp_d_literal(mh, eh),
                     exp_d_literal(*exp_d_round(n, 1 << LOG_D_FAST_CLO))
                     if n else "0.0",
                     exp_d_literal(*exp_d_round(l1, 1 << 42)) if l1 else
                     "0.0",
                     exp_d_literal(ml, el) if ml else "0.0"))
    return rows


# Sollya's coefficients (chunk 67's record, proofs/log_fast_poly.out):
# log_fast's A, degree 4, with log1p(r) = r + r^2 A(r) on the table's path,
# and B, degree 9, with log1p(r) = r - r^2/2 + r^3 B(r) near 1 (S500)
LOG_D_FAST_A = ["-0x1p-1", "0x1.55555555279e5p-2", "-0x1.ffffffff6fff5p-3",
                "0x1.999b07518cd5dp-3", "-0x1.5556e22399966p-3"]
LOG_D_FAST_B = ["0x1.5555555555558p-2", "-0x1.fffffffffff96p-3",
                "0x1.99999999886a5p-3", "-0x1.55555555b868cp-3",
                "0x1.249249ab35c54p-3", "-0x1.fffffd26a75acp-4",
                "0x1.c71994128709fp-4", "-0x1.999cc56cd59b5p-4",
                "0x1.778b51962f2e5p-4", "-0x1.55647c7f8e79p-4"]


# Sollya's coefficients (proofs/log_poly.out): phase 1's q, degree 6, with
# log1p(r) = r + r^2 q(r) (S432); phase 2's P, degree 10, its first five
# coefficients as pairs
LOG_D_Q = ["-0x1p-1", "0x1.5555555555556p-2", "-0x1.0000000000002p-2",
           "0x1.99999998dddd4p-3", "-0x1.5555555420254p-3",
           "0x1.2493cf3e41b7fp-3", "-0x1.0001c201fc422p-3"]
LOG_D_P = [("0x1.5555555555555p-2", "0x1.5555555555555p-56"),
           ("-0x1p-2", "-0x1.92438p-91"),
           ("0x1.999999999999ap-3", "-0x1.9999999760c7p-57"),
           ("-0x1.5555555555555p-3", "-0x1.554d79f1fd72cp-57"),
           ("0x1.2492492492492p-3", "0x1.247c7c5ea07ap-57"),
           ("-0x1.000000000000bp-3", None), ("0x1.c71c71c71c745p-4", None),
           ("-0x1.999999967504fp-4", None), ("0x1.745d174176c9ap-4", None),
           ("-0x1.555879eab4efap-4", None), ("0x1.3b17310bb3497p-4", None)]

# log_fast's bounds, in ulps, proven in chunk 67's record (proofs/)
LOG_D_FAST_BOUNDS = {"all": "0.53", "near1": "0.52", "table": "0.53"}

# the bounds proven in chunks 52's and 54's records (proofs/)
LOG_D_BOUNDS = {"p1a": "2^-66.4", "p1": "2^-59.9", "qerr": "2^-65.05",
                "ka": 90, "kappa": (11, 1 << 55),
                "kappa_text": "11 2^-55",
                "p2": "2^-101.4", "p3": "2^-165.8", "p3u": "36.1"}


def log_d_table_macro(rows):
    """the table as a macro's initializer, one value a line."""
    out = ["#define D_INTERNAL_MATH_D_LN_TABLE" + " " * 45 + "\\",
           "    {" + " " * 74 + "\\"]
    values = [v for row in rows for v in row]
    for k, v in enumerate(values):
        text = "        " + v + ("" if k + 1 == len(values) else ",")
        out.append(text + " " * (79 - len(text)) + "\\")
    out.append("    }")
    return "\n".join(out)

LOG_D_SECTION = r"""// 3.3    Natural logarithm
//------------------------------------------------------------------------------
// log_c is the natural logarithm correctly rounded, in double arithmetic
// alone - no long double and no C library - and a constant expression from
// C++14 (S424). x = m 2^e, m in [1, 2), a subnormal first lifted by 2^54; i,
// from 0 to 128, picks the c = 1 + i/128 nearest m, and from c > sqrt(2), i
// from 53, c is halved and e raised by 1; then log(x) = e ln2 + L +
// log1p(r), with inv a number of 26 bits within 2^-26 of 1/c, relatively,
// L = -log(inv) from a table of 129 triples, the table holding inv halved
// from 53 so that m is never halved, and r = m inv - 1 exactly as a pair,
// m inv taken by Dekker's product with m alone split, |r| < 2^-8 (S432).
// Three phases follow, each with a bound proven in chunks 52's and 54's
// records (proofs/), each giving its result where the interval its bound
// allows rounds one way, as Ziv's method has it: phase 1, in doubles, within
// 2^-@KA@ of the result plus @KAPPA_TEXT@ rh^2, @B1@ of the logarithm at
// worst, answers all but some 0.4% of arguments within 2^-7 of 1 and nearly
// every other; phase 2, in pairs of doubles (dd.h), within @B2@; phase 3, in
// fixed point of 256 bits, within @B3@, far closer than a double's logarithm
// is known to come to a rounding boundary - the hardest case known, Lefevre
// and Muller's x = 0x1.62a88613629b6p+678, lies 2^-118.0 of its logarithm
// from one. The bounds hold whether or not the compiler fuses a multiply
// and an add (math_common.h, section 5). NaN and +inf are their own
// logarithms, log(+-0) is -inf, a negative x gives NaN, and log(1) is +0.
// Where double is evaluated wider than double, a 32-bit x87, log_c refuses
// to compile: build with -msse2 -mfpmath=sse (math_common.h,
// D_MATH_DBL_REFUSE). The plain log is log_c, as ruling 12 has a plain
// double function once its correctly rounded form is within about twice the
// C library's time (S496), but where double is evaluated wider than double,
// where it is the C library's (S497).

// D_INTERNAL_MATH_D_LN_TABLE
//   for i from 0 to 128, at 3 i: inv, 1 at 0 and 128, otherwise 1/c (below
// 53) or 2/c (from 53) rounded to 26 significant bits, halved from 53 (S432);
// at 3 i + 1 and 3 i + 2 -log(inv), inv unhalved, as a pair, within 2^-106 of
// it, relatively; each exact as written (chunk 54's record,
// proofs/log_consts.py).
@TABLE@

#ifdef __cplusplus
// dj_internal_math_d_ln_table
//   struct: the table as a template's static member, as exp_c's is (S420).
template <int Unused>
struct dj_internal_math_d_ln_table
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr double value[387] = D_INTERNAL_MATH_D_LN_TABLE;
#else
    static const double value[387];
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
};

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
template <int Unused>
const double dj_internal_math_d_ln_table<Unused>::value[387] =
    D_INTERNAL_MATH_D_LN_TABLE;
#elif !D_ENV_LANG_IS_CPP17_OR_HIGHER
template <int Unused>
constexpr double dj_internal_math_d_ln_table<Unused>::value[387];
#endif
#endif  // __cplusplus

// d_internal_math_d_ln_t
//   the table's _index-th value. In C a function's own table, so that a
// translation unit that never reaches it holds none.
D_MATH_FN double
d_internal_math_d_ln_t(
    int _index
)
{
#ifdef __cplusplus
    return dj_internal_math_d_ln_table<0>::value[_index];
#else
    static const double table[387] = D_INTERNAL_MATH_D_LN_TABLE;

    return table[_index];
#endif  // __cplusplus
}

// dj_internal_math_d_ln_split
//   struct: a positive normal double as m 2^e, m in [1, 2); i, the integer
// nearest 128 (m - 1); and mh, m's high part, by which m inv is taken
// exactly (d_internal_math_d_ln_product).
struct dj_internal_math_d_ln_split
{
    double m;
    double mh;
    int    e;
    int    i;
};

#if ( ( !defined(__cplusplus) && defined(UINT64_MAX) ) ||                     \
      ( defined(__cplusplus) && defined(UINT64_MAX) &&                         \
        defined(D_INTERNAL_MATH_HAS_BIT_CAST) ) )
    // D_INTERNAL_MATH_D_LN_BITS
    //   1 where log_c reads a double's bits: in C with a 64-bit integer, by a
    // union, and in C++ by __builtin_bit_cast, a constant expression. A test
    // may define it 0 first, to take the other path where it is 1.
    #ifndef D_INTERNAL_MATH_D_LN_BITS
        #define D_INTERNAL_MATH_D_LN_BITS 1
    #endif
#else
    #undef D_INTERNAL_MATH_D_LN_BITS
    #define D_INTERNAL_MATH_D_LN_BITS 0
#endif

#if D_INTERNAL_MATH_D_LN_BITS
// d_internal_math_d_ln_bits
//   _v's bits.
D_MATH_FN uint64_t
d_internal_math_d_ln_bits(
    double _v
)
{
#ifdef __cplusplus
    return __builtin_bit_cast(uint64_t, _v);
#else
    union
    {
        uint64_t bits;
        double   value;
    } v = { 0 };

    v.value = _v;

    return v.bits;
#endif  // __cplusplus
}

// d_internal_math_d_ln_double
//   the double whose bits are _bits.
D_MATH_FN double
d_internal_math_d_ln_double(
    uint64_t _bits
)
{
#ifdef __cplusplus
    return __builtin_bit_cast(double, _bits);
#else
    union
    {
        uint64_t bits;
        double   value;
    } v = { 0 };

    v.bits = _bits;

    return v.value;
#endif  // __cplusplus
}
#endif  // D_INTERNAL_MATH_D_LN_BITS

// d_internal_math_d_ln_split
//   a positive normal _v as m 2^e, m in [1, 2), exactly, with i and mh.
// Where the language reads a double's bits, from them alone: m's 52 bits
// below its leading one are f, so that 128 (m - 1) + 1/2 is (f + 2^44)
// 2^-45 and i that shifted by 45, and mh is m with f's last 26 bits cleared,
// of 27 significant bits, m - mh of at most 26. Elsewhere by float_order.h's
// binade search, i from 128 (m - 1) + 1/2, a multiple of 2^-45 below 128.5,
// and mh by Veltkamp's split, of 26 bits (S495).
D_MATH_FN struct dj_internal_math_d_ln_split
d_internal_math_d_ln_split(
    double _v
)
{
    struct dj_internal_math_d_ln_split result = { 1.0, 1.0, 0, 0 };
#if D_INTERNAL_MATH_D_LN_BITS
    const uint64_t bits = d_internal_math_d_ln_bits(_v);
    const uint64_t f    = bits & ((((uint64_t)1) << 52) - 1);
    const uint64_t one  = ((uint64_t)1023) << 52;

    result.e  = (int)((bits >> 52) & 0x7FFu) - 1023;
    result.i  = (int)((f + (((uint64_t)1) << 44)) >> 45);
    result.m  = d_internal_math_d_ln_double(f | one);
    result.mh = d_internal_math_d_ln_double(
        (f & ~((((uint64_t)1) << 26) - 1)) | one);
#else
    const struct dj_internal_math_d_binade b =
        d_internal_math_d_binade_of(_v);
    const double m = _v / b.power;
    const double t = d_internal_math_d_veltkamp() * m;

    result.e  = b.exponent;
    result.i  = (int)(((m - 1.0) * 128.0) + 0.5);
    result.m  = m;
    result.mh = t - (t - m);
#endif  // D_INTERNAL_MATH_D_LN_BITS

    return result;
}

// d_internal_math_d_ln_normal
//   whether _v is a positive normal finite double, every double but the
// rest: from its bits by one unsigned comparison where the language reads
// them, by two comparisons elsewhere (S495).
D_MATH_FN bool
d_internal_math_d_ln_normal(
    double _v
)
{
#if D_INTERNAL_MATH_D_LN_BITS
    return ( (d_internal_math_d_ln_bits(_v) - (((uint64_t)1) << 52)) <
             (((uint64_t)0x7FE) << 52) );
#else
    return ( (_v >= DBL_MIN) &&
             (_v <= DBL_MAX) );
#endif  // D_INTERNAL_MATH_D_LN_BITS
}

// dj_internal_math_d_ln_arg
//   struct: x reduced: log(x) = e ln2 + L_i + log1p(rh + rl), exactly, L_i =
// -log(inv_i); |rh + rl| < 2^-8, rl within half of rh's last place.
struct dj_internal_math_d_ln_arg
{
    int    e;
    int    i;   // 0 to 128
    double rh;
    double rl;
};

// d_internal_math_d_ln_product
//   _m _inv exactly, as high + low, high its rounding, _inv of at most 26
// significant bits: the low part by a fused multiply-add where the target
// has one, and otherwise by Dekker's product with _m alone split, at _mh,
// so that both give the same bits as d_math_dd_two_product (S432). _mh of
// at most 27 significant bits and _m - _mh of at most 26, each product by
// _inv is exact; mh inv less high is a multiple of 2^-53 below 2^-25, and
// exact; that plus (m - mh) inv is m inv - high, a multiple of 2^-79 at most
// 2^-53, exact too (S495; chunk 66's record, proofs/).
D_MATH_FN struct dj_math_dd
d_internal_math_d_ln_product(
    double _m,
    double _mh,
    double _inv
)
{
    struct dj_math_dd result = { 0.0, 0.0 };

#if D_MATH_DBL_FMA
    if (D_MATH_RUN_TIME())
    {
        result.high = _m * _inv;
        result.low  = D_MATH_DBL_FMA_OF(_m, _inv, -result.high);

        return result;
    }
#endif  // D_MATH_DBL_FMA

    {
        const double high = _m * _inv;

        result.high = high;
        result.low  = ((_mh * _inv) - high) + ((_m - _mh) * _inv);

        return result;
    }
}

// d_internal_math_d_ln_arg_of
//   _v 2^_lift, _v positive, normal and finite, reduced. i is the integer
// nearest (m - 1) 128, so that |m - c| <= 1/256. From i = 53 the table holds
// inv halved, so that m times it is m/2 times inv, exactly, and e is raised
// by 1 (S432). m inv is exact as a pair; its high part lies within 2^-7 of
// 1, so that it minus 1 is exact (Sterbenz's lemma), and at least the low
// part's place, or 0, so that Fast2Sum is exact: r = m inv - 1, a multiple
// of 2^-106, of which rh and rl are each a multiple too.
D_MATH_FN struct dj_internal_math_d_ln_arg
d_internal_math_d_ln_arg_of(
    double _v,
    int    _lift
)
{
    const struct dj_internal_math_d_ln_split s =
        d_internal_math_d_ln_split(_v);
    const int    i = s.i;
    const struct dj_math_dd p = d_internal_math_d_ln_product(
        s.m, s.mh, d_internal_math_d_ln_t(3 * i));
    const struct dj_math_dd r = d_math_dd_fast_two_sum(p.high - 1.0, p.low);
    struct dj_internal_math_d_ln_arg result = { 0, 0, 0.0, 0.0 };

    result.e  = (s.e + _lift) + ((i >= 53) ? 1 : 0);
    result.i  = i;
    result.rh = r.high;
    result.rl = r.low;

    return result;
}

// d_internal_math_d_ln_decide
//   _yh + _yl rounded once, where the exact value is within _err of it less
// the rounding of _yl -+ _err, which the bounds leave room for: decided
// where both ends of the interval round alike. The logarithm of a double is
// neither subnormal nor past the largest double.
D_MATH_FN struct dj_internal_math_d_exp_result
d_internal_math_d_ln_decide(
    double _yh,
    double _yl,
    double _err
)
{
    struct dj_internal_math_d_exp_result result = { false, 0.0 };
    const double low  = _yh + (_yl - _err);
    const double high = _yh + (_yl + _err);

    if (low == high)
    {
        result.decided = true;
        result.value   = low;
    }

    return result;
}

// d_internal_math_d_ln_phase1
//   log(x) from _a in doubles: E = e LN2H, exact (LN2H of 42 bits, |e| <
// 1100), and L_i's high part, then rh, taken exactly by Fast2Sum, |E| >=
// |L_i| where e is not 0 and |L_i| >= 2^-8 > |rh| where i is neither 0 nor
// 128, either 0 otherwise; log1p(rh) = rh + rh^2 q(rh), q of degree 6
// (Sollya's), within @QERR@ of (log1p(rh) - rh)/rh^2; rl adds rl (1 - rh);
// the low parts summed (S432). The computed pair lies within 2^-@KA@ of the
// result plus @KAPPA_TEXT@ rh^2 of log(x), less the rounding of lo -+ err,
// in each of three cases - e not 0; e 0 and i neither 0 nor 128; e 0 and
// i 0 or 128, where r = x - 1 - (proofs/log_phase1_*.g): within @B1A@ of
// it, relatively, where e is not 0, and @B1@ in the worst case.
D_MATH_FN struct dj_internal_math_d_exp_result
d_internal_math_d_ln_phase1(
    struct dj_internal_math_d_ln_arg _a
)
{
    // ln2's first 42 bits, and the rest rounded
    const double ln2h  = @LN2H@;
    const double ln2l1 = @LN2L1@;
    const double q0    = @Q0@;
    const double q1    = @Q1@;
    const double q2    = @Q2@;
    const double q3    = @Q3@;
    const double q4    = @Q4@;
    const double q5    = @Q5@;
    const double q6    = @Q6@;
    const double e     = (double)_a.e;
    const double rh    = _a.rh;
    const double rl    = _a.rl;
    const struct dj_math_dd a = d_math_dd_fast_two_sum(
        e * ln2h, d_internal_math_d_ln_t((3 * _a.i) + 1));
    const struct dj_math_dd b = d_math_dd_fast_two_sum(a.high, rh);
    const double r2    = rh * rh;
    const double q     =
        q0 + (rh * (q1 + (rh * (q2 + (rh * (q3 + (rh * (q4 + (rh * (q5 +
        (rh * q6)))))))))));
    const double lo    =
        ( ( (a.low + b.low) + (d_internal_math_d_ln_t((3 * _a.i) + 2) +
                               (e * ln2l1)) ) +
          (rl * (1.0 - rh)) ) +
        (r2 * q);
    // the test's room: 2^-@KA@ of the result, and @KAPPA_TEXT@ of rh^2, the
    // part of the error that grows with it (S432)
    const double room1 = @EA@;
    const double room2 = @KAPPA@;
    // |b.high| as the greater of it and its negation, a negation and a
    // maximum where a comparison took five instructions (S495)
    const double err   = (((b.high > -b.high) ? b.high : -b.high) * room1) +
                         (r2 * room2);

    return d_internal_math_d_ln_decide(b.high, lo, err);
}

// d_internal_math_d_ln_phase2
//   log(x) from _a in pairs of doubles: log1p(rh) = rh - rh^2/2 + rh^3
// P(rh), P of degree 10 (Sollya's), within 2^-101.8, its tail in doubles and
// its first coefficients as pairs; rl adds rl/(1 + rh); e ln2 from ln2 in
// three parts, the first two of 42 bits, so that their products by e are
// exact; then e ln2 + L_i, and the logarithm of 1 + r added. Within
// @B2@ of log(x), relatively, in each of phase 1's three cases (proofs/
// log_phase2_*.g): tested against 2^-100 times the result's high part.
D_MATH_FN struct dj_internal_math_d_exp_result
d_internal_math_d_ln_phase2(
    struct dj_internal_math_d_ln_arg _a
)
{
    const double ln2h = @LN2H@;
    const double ln2m = @LN2M@;
    const double ln2l =
        @LN2L@;
    const struct dj_math_dd p0 = {
        @P0H@,
        @P0L@
    };
    const struct dj_math_dd p1 = {
        @P1H@,
        @P1L@
    };
    const struct dj_math_dd p2 = {
        @P2H@,
        @P2L@
    };
    const struct dj_math_dd p3 = {
        @P3H@,
        @P3L@
    };
    const struct dj_math_dd p4 = {
        @P4H@,
        @P4L@
    };
    const double p5 = @P5@;
    const double p6 = @P6@;
    const double p7 = @P7@;
    const double p8 = @P8@;
    const double p9 = @P9@;
    const double p10 = @P10@;
    const double e  = (double)_a.e;
    const double rh = _a.rh;
    const double t  =
        p5 + (rh * (p6 + (rh * (p7 + (rh * (p8 + (rh * (p9 + (rh * p10)))))))));
    // P = p0 + rh (p1 + rh (p2 + rh (p3 + rh (p4 + rh t))))
    const struct dj_math_dd w4 =
        d_math_dd_add(p4, d_math_dd_mul_d(d_math_dd_of(t), rh));
    const struct dj_math_dd w3 = d_math_dd_add(p3, d_math_dd_mul_d(w4, rh));
    const struct dj_math_dd w2 = d_math_dd_add(p2, d_math_dd_mul_d(w3, rh));
    const struct dj_math_dd w1 = d_math_dd_add(p1, d_math_dd_mul_d(w2, rh));
    const struct dj_math_dd w0 = d_math_dd_add(p0, d_math_dd_mul_d(w1, rh));
    const struct dj_math_dd r2 = d_math_dd_two_product(rh, rh);
    const struct dj_math_dd r3 = d_math_dd_mul_d(r2, rh);
    // -rh^2/2, exactly
    const struct dj_math_dd h  = { r2.high * -0.5, r2.low * -0.5 };
    // rh - rh^2/2 + rh^3 P, then + rl/(1 + rh)
    const struct dj_math_dd l1 = d_math_dd_add_d(
        d_math_dd_add(d_math_dd_add_d(h, rh), d_math_dd_mul(r3, w0)),
        _a.rl / (1.0 + rh));
    // e ln2, then + L_i
    const struct dj_math_dd el = d_math_dd_add_d(
        d_math_dd_two_sum(e * ln2h, e * ln2m), e * ln2l);
    const struct dj_math_dd tl = { d_internal_math_d_ln_t((3 * _a.i) + 1),
                                   d_internal_math_d_ln_t((3 * _a.i) + 2) };
    const struct dj_math_dd y  = d_math_dd_add(d_math_dd_add(el, tl), l1);

    return d_internal_math_d_ln_decide(
        y.high, y.low,
        ((y.high < 0.0) ? -y.high : y.high) *
            (1.0 / 1267650600228229401496703205376.0));
}
"""

LOG_D_SECTION_3 = r"""
// The third phase computes in fixed point, as exp_c's does (section 3.2),
// with its values and their operations: e ln2 is |128 e| ln2/128 from ln2/128
// at 240 bits, truncated; |L_i| is held to 224 bits, as digits (below); r =
// rh + rl exactly, a multiple of 2^-106; log1p(r) by its series, summed
// until its terms vanish, each power and each quotient truncated; the
// positive parts and the negative ones apart, then taken from each other:
// within @B3U@ units of 2^-224 of log(x), on either side, which is less
// than @B3@ of it, |log(x)| being at least 2^-53 (proofs/log_phase3.py).
// That is far below the 2^-118.0 by which the hardest known argument's
// logarithm misses a rounding boundary, relatively, so the computed value
// rounds as the exact one does.

// D_INTERNAL_MATH_D_LN_DIGITS_A, D_INTERNAL_MATH_D_LN_DIGITS_B
//   the third phase's digits, in hexadecimal: for i from 0 to 128,
// |-log(inv_i)| times 2^224, rounded down, in 56 digits each; i from 0 to 64
// in the first literal, 3,640 digits, the rest in the second, each within
// C99's 4,095 characters.
#define D_INTERNAL_MATH_D_LN_DIGITS_A                                          \
@DIGITS_A@

#define D_INTERNAL_MATH_D_LN_DIGITS_B                                          \
@DIGITS_B@

// d_internal_math_d_ln_digits
//   |L_i| times 2^224 from its 56 digits: limbs from the last digit up, each
// of 4 digits.
D_MATH_FN struct dj_internal_math_d_exp_fixed
d_internal_math_d_ln_digits(
    int _i
)
{
    struct dj_internal_math_d_exp_fixed result = { { 0 } };
    int k = 0;

    for (k = 0; k < 56; ++k)
    {
        const int  at    = (56 * _i) + k;
        const char c     = (at < 3640)
            ? D_INTERNAL_MATH_D_LN_DIGITS_A[at]
            : D_INTERNAL_MATH_D_LN_DIGITS_B[at - 3640];
        const int  place = 55 - k;
        const unsigned long digit = (c <= '9')
            ? (unsigned long)(c - '0')
            : (unsigned long)((c - 'a') + 10);

        result.limb[place / 4] |= digit << (4 * (place % 4));
    }

    return result;
}

// d_internal_math_d_ln_phase3
//   log(x) from _a in fixed point, correctly rounded: the positive parts in
// up and the negative ones in down - e ln2 of e's sign, L_i positive below
// 53 and negative from 53, and log1p(r)'s terms, r^k/k, each of the sign
// (-1)^(k+1) r^k has; up - down then rounded at 53 bits, as the exact
// value is: no rounding boundary is known to lie so close (the note above).
// rh and rl, multiples of 2^-106 below 2^-8, are each exact in fixed point.
D_MATH_FN double
d_internal_math_d_ln_phase3(
    struct dj_internal_math_d_ln_arg _a
)
{
    struct dj_internal_math_d_exp_fixed up   = { { 0 } };
    struct dj_internal_math_d_exp_fixed down = { { 0 } };
    const struct dj_internal_math_d_exp_fixed l =
        d_internal_math_d_ln_digits(_a.i);
    const struct dj_internal_math_d_exp_fixed h =
        d_internal_math_d_exp_fixed_of(_a.rh);
    const struct dj_internal_math_d_exp_fixed g =
        d_internal_math_d_exp_fixed_of(_a.rl);
    // |r|: |rl| below |rh| where rh is not 0, and 0 where it is
    const struct dj_internal_math_d_exp_fixed r =
        ((_a.rh < 0.0) == (_a.rl < 0.0))
            ? d_internal_math_d_exp_fixed_add(h, g)
            : d_internal_math_d_exp_fixed_sub(h, g);
    const bool negative = (_a.rh < 0.0);
    struct dj_internal_math_d_exp_fixed power = r;
    unsigned long k = 1;

    if (_a.e > 0)
    {
        up = d_internal_math_d_exp_ln2_times(128.0 * (double)_a.e);
    }
    else if (_a.e < 0)
    {
        down = d_internal_math_d_exp_ln2_times(128.0 * (double)_a.e);
    }

    if (_a.i < 53)
    {
        up = d_internal_math_d_exp_fixed_add(up, l);
    }
    else
    {
        down = d_internal_math_d_exp_fixed_add(down, l);
    }

    for (k = 1; k < 64; ++k)
    {
        const struct dj_internal_math_d_exp_fixed term =
            d_internal_math_d_exp_fixed_div(power, k);

        if (d_internal_math_d_exp_fixed_bits(term) == 0)
        {
            break;
        }

        if (negative || ((k % 2) == 0))
        {
            down = d_internal_math_d_exp_fixed_add(down, term);
        }
        else
        {
            up = d_internal_math_d_exp_fixed_add(up, term);
        }

        power = d_internal_math_d_exp_fixed_mul(power, r);
    }

    {
        const bool below = d_internal_math_d_exp_fixed_less(up, down);
        const struct dj_internal_math_d_exp_fixed d =
            below ? d_internal_math_d_exp_fixed_sub(down, up)
                  : d_internal_math_d_exp_fixed_sub(up, down);
        const int    place = d_internal_math_d_exp_fixed_bits(d) - 53;
        const double m     = d_internal_math_d_exp_fixed_round(d, place);
        // m 2^(place - 224), place - 224 between -107 and -43
        const double value =
            d_internal_math_d_exp_times_pow2(m, place - 224);

        return below ? -value : value;
    }
}

// d_internal_math_d_ln_slow
//   log(_x 2^_lift), _x positive, normal and finite, by the second phase
// and, where it does not decide, the third: the arguments the first leaves.
// Out of line, so that log_c's common path, inlined into its callers, keeps
// the small frame the first phase needs.
D_MATH_FN_COLD double
d_internal_math_d_ln_slow(
    double _x,
    int    _lift
)
{
    const struct dj_internal_math_d_ln_arg a =
        d_internal_math_d_ln_arg_of(_x, _lift);
    const struct dj_internal_math_d_exp_result two =
        d_internal_math_d_ln_phase2(a);

    return two.decided ? two.value : d_internal_math_d_ln_phase3(a);
}

// d_internal_math_d_ln_of
//   log(_x 2^_lift), _x positive, normal and finite: phase 1's result,
// where it decides the rounding; else the slow phases'.
D_MATH_FN double
d_internal_math_d_ln_of(
    double _x,
    int    _lift
)
{
    const struct dj_internal_math_d_exp_result one =
        d_internal_math_d_ln_phase1(d_internal_math_d_ln_arg_of(_x, _lift));

    return one.decided ? one.value : d_internal_math_d_ln_slow(_x, _lift);
}

// d_math_d_log_c
//   log(_x), correctly rounded; see above. A positive normal _x, every
// double but the rest, is taken in one test (S432); log(1) is +0 as the
// phases give it.
D_MATH_FN double
d_math_d_log_c(
    double _x
)
{
    // 2^54, by which a subnormal is lifted into the normal range
    const double lift = 18014398509481984.0;

    D_MATH_DBL_REFUSE(_x)

    if (d_internal_math_d_ln_normal(_x))
    {
        return d_internal_math_d_ln_of(_x, 0);
    }

    // NaN and +inf are their own logarithms
    if ( (_x != _x) ||
         (_x > DBL_MAX) )
    {
        return _x;
    }

    // zero, of either sign, has -inf
    if (_x == 0.0)
    {
        return -d_internal_math_d_infinity();
    }

    // a negative value has none
    if (_x < 0.0)
    {
        return d_internal_math_d_nan();
    }

    // a positive subnormal, lifted
    return d_internal_math_d_ln_of(_x * lift, -54);
}

// d_math_d_log
//   log(_x), correctly rounded: log_c; the C library's where double is
// evaluated wider than double; see above.
D_MATH_FN_RT double
d_math_d_log(
    double _x
)
{
#if !D_MATH_DBL_EVAL_WIDE
    return d_math_d_log_c(_x);
#elif defined(__cplusplus)
    return std::log(_x);
#else
    return log(_x);
#endif  // !D_MATH_DBL_EVAL_WIDE
}

// log_fast is the natural logarithm within @LFBOUND@ of an ulp, at the C
// library's time, for a caller who wants that time rather than the correct
// rounding (S500), as exp_fast is for e^x: glibc's method, in this file's
// arithmetic, with its own table and polynomials. A positive normal x within
// [1 - 2^-4, 1 + 0x1.09p-4) takes r = x - 1, exact, and log1p(r) as r -
// r^2/2 + r^3 B(r), B of degree 9, r^2/2 split so that r + -r^2/2 is a pair
// before the last sum; every other positive normal x, x = 2^k z, z in
// [0x1.6p-1, 0x1.6p0) and row i from its bits less those of 0x1.6p-1, then r
// = (z - chi - clo) inv, z - chi - clo exact, inv a double near 1/c with
// 1/inv = chi + clo within 2^-66, so that r is within 2^-61.8 of z inv - 1,
// |r| < 2^-8, and log(x) = (k LN2H + L1) + r + (k LN2L + L2) + r^2 A(r), A
// of degree 4, k LN2H + L1 exact, the result at least 2^-4 there.
// The bounds hold whether or not the compiler fuses a multiply and an add
// (chunk 67's record, proofs/). A subnormal, zero, negative, infinite or NaN
// x, log_c's answer, and log_c's too where the language cannot read a
// double's bits (D_INTERNAL_MATH_D_LN_BITS); where double is evaluated wider
// than double, the C library's log. Double alone: no float, long double or
// C++ overload, and never a constant expression (S501). It is the C
// library's log too where cfg_exponential.h's D_CFG_MATH_LOG_FAST says so,
// for the build; d_math_d_log_fast_with takes either form call by call, at
// run time, as its argument names it (S505, S506).

// D_INTERNAL_MATH_D_LN_FAST_TABLE
//   log_fast's: for i from 0 to 127, row i holding z in [0x1.6p-1 + i 2^-8,
// + 2^-8) below 80 and [1 + (i - 80) 2^-7, + 2^-7) from 80, c its middle: at
// 5 i inv, a double near 1/c whose reciprocal is chi + clo within 2^-66; at
// 5 i + 1 chi, a double, and at 5 i + 2 clo, a multiple of 2^-60; at 5 i + 3
// and 5 i + 4 L1 and L2, -log(inv) as L1, a multiple of 2^-42, and the rest
// rounded, within 2^-96 of it; each exact as written (chunk 67's record,
// proofs/log_fast_consts.py).
@FAST_TABLE@

#ifdef __cplusplus
// dj_internal_math_d_ln_fast_table
//   struct: log_fast's table as a template's static member, as log_c's is.
template <int Unused>
struct dj_internal_math_d_ln_fast_table
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr double value[640] = D_INTERNAL_MATH_D_LN_FAST_TABLE;
#else
    static const double value[640];
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
};

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
template <int Unused>
const double dj_internal_math_d_ln_fast_table<Unused>::value[640] =
    D_INTERNAL_MATH_D_LN_FAST_TABLE;
#elif !D_ENV_LANG_IS_CPP17_OR_HIGHER
template <int Unused>
constexpr double dj_internal_math_d_ln_fast_table<Unused>::value[640];
#endif
#endif  // __cplusplus

// d_internal_math_d_ln_fast_row
//   log_fast's table's row _i, its five values from the pointer returned;
// in C a function's own table, as log_c's.
D_MATH_FN_RT const double*
d_internal_math_d_ln_fast_row(
    int _i
)
{
#ifdef __cplusplus
    return dj_internal_math_d_ln_fast_table<0>::value + (5 * _i);
#else
    static const double table[640] = D_INTERNAL_MATH_D_LN_FAST_TABLE;

    return table + (5 * _i);
#endif  // __cplusplus
}

// d_internal_math_d_ln_fast_near1
//   log(_x) within @LFBOUNDN@ of an ulp, _x in [1 - 2^-4, 1 + 0x1.09p-4): r = x -
// 1, exact; rhi, r to 26 bits, so that rhi^2 / 2 is exact, and r^2/2 =
// rhi^2/2 + rlo (rhi + r)/2; hi + lo = r - rhi^2/2 exactly; the rest, r^3
// B(r) and -rlo (rhi + r)/2, added to lo before hi.
D_MATH_FN_RT double
d_internal_math_d_ln_fast_near1(
    double _x
)
{
    const double b0  = @LFB0@;
    const double b1  = @LFB1@;
    const double b2  = @LFB2@;
    const double b3  = @LFB3@;
    const double b4  = @LFB4@;
    const double b5  = @LFB5@;
    const double b6  = @LFB6@;
    const double b7  = @LFB7@;
    const double b8  = @LFB8@;
    const double b9  = @LFB9@;
    const double r   = _x - 1.0;
    const double r2  = r * r;
    const double r3  = r * r2;
    const double p   = r3 * ( ((b0 + (r * b1)) + (r2 * b2)) +
                              (r3 * ( ((b3 + (r * b4)) + (r2 * b5)) +
                                      (r3 * ( ((b6 + (r * b7)) +
                                               (r2 * b8)) +
                                              (r3 * b9) )) )) );
    const double t   = r * 134217728.0;
    const double rhi = (r + t) - t;
    const double rlo = r - rhi;
    const double w   = (rhi * rhi) * -0.5;
    const double hi  = r + w;
    const double lo  = ((r - hi) + w) + ((-0.5 * rlo) * (rhi + r));

    return (p + lo) + hi;
}

#if D_INTERNAL_MATH_D_LN_BITS
// d_internal_math_d_ln_fast_table_path
//   log(_x) within @LFBOUNDT@ of an ulp, _x positive, normal and finite,
// outside [1 - 2^-4, 1 + 0x1.09p-4): t, x's bits less 0x1.6p-1's; i its bits
// from the 46th, 7 of them; k its top 12 as a signed integer, so that z = x
// 2^-k, x's bits less k's in the exponent, lies in [0x1.6p-1, 0x1.6p0); k
// as a double by 2^52 + 2^20 + k, built from its bits, less 2^52 + 2^20,
// both exact, no conversion from an integer, which some compilers let wait
// on an earlier result; z - chi - clo exact; w = k LN2H + L1 exact; hi + lo
// = w + r exactly, and k LN2L + L2 and r^2 A(r) added to lo before hi.
D_MATH_FN_RT double
d_internal_math_d_ln_fast_table_path(
    double _x
)
{
    const double   ln2h = @LN2H@;
    const double   ln2l = @LN2L1@;
    const double   a0   = @LFA0@;
    const double   a1   = @LFA1@;
    const double   a2   = @LFA2@;
    const double   a3   = @LFA3@;
    const double   a4   = @LFA4@;
    const uint64_t ix   = d_internal_math_d_ln_bits(_x);
    const uint64_t t    = ix - (((uint64_t)0x3FE6) << 48);
    const int      top  = (int)(t >> 52);
    const int      k    = (top ^ 2048) - 2048;
    const double*  row  =
        d_internal_math_d_ln_fast_row((int)((t >> 45) & 127u));
    const double   z    =
        d_internal_math_d_ln_double(ix - (t & (((uint64_t)0xFFF) << 52)));
    const double   e    =
        d_internal_math_d_ln_double( (((uint64_t)0x433) << 52) |
                                     (uint64_t)(k + 1048576) ) -
        4503599628419072.0;
    const double   r    = ((z - row[1]) - row[2]) * row[0];
    const double   w    = (e * ln2h) + row[3];
    const double   hi   = w + r;
    const double   lo   = ((w - hi) + r) + ((e * ln2l) + row[4]);
    const double   r2   = r * r;

    return ( (lo + (r2 * a0)) +
             ((r * r2) * ((a1 + (r * a2)) + (r2 * (a3 + (r * a4))))) ) + hi;
}
#endif  // D_INTERNAL_MATH_D_LN_BITS

// d_internal_math_d_ln_fast_near
//   whether _v is within [1 - 2^-4, 1 + 0x1.09p-4): from its bits by one
// unsigned comparison where the language reads them, by two elsewhere.
D_MATH_FN_RT bool
d_internal_math_d_ln_fast_near(
    double _v
)
{
#if D_INTERNAL_MATH_D_LN_BITS
    return ( (d_internal_math_d_ln_bits(_v) - (((uint64_t)0x3FEE) << 48)) <
             ((((uint64_t)0x3FF109) << 40) - (((uint64_t)0x3FEE) << 48)) );
#else
    return ( (_v >= 0.9375) &&
             (_v < 1.064697265625) );
#endif  // D_INTERNAL_MATH_D_LN_BITS
}

#if !D_MATH_DBL_EVAL_WIDE
// d_internal_math_d_ln_fast
//   log(_x) within @LFBOUND@ of an ulp, at the C library's time: log_fast's
// own formula; see above.
D_MATH_FN_RT double
d_internal_math_d_ln_fast(
    double _x
)
{
    if (d_internal_math_d_ln_fast_near(_x))
    {
        return d_internal_math_d_ln_fast_near1(_x);
    }

#if D_INTERNAL_MATH_D_LN_BITS
    if (d_internal_math_d_ln_normal(_x))
    {
        return d_internal_math_d_ln_fast_table_path(_x);
    }
#endif  // D_INTERNAL_MATH_D_LN_BITS

    return d_math_d_log_c(_x);
}
#endif  // !D_MATH_DBL_EVAL_WIDE

// d_internal_math_d_ln_library
//   log(_x) by the C library (std::log in C++).
D_MATH_FN_RT double
d_internal_math_d_ln_library(
    double _x
)
{
#ifdef __cplusplus
    return std::log(_x);
#else
    return log(_x);
#endif  // __cplusplus
}

// d_math_d_log_fast
//   log(_x) at the C library's time: the own formula, within @LFBOUND@ of an
// ulp, or the C library's log, as D_CFG_MATH_LOG_FAST says
// (cfg_exponential.h); the C library's where double is evaluated wider than
// double; see above.
D_MATH_FN_RT double
d_math_d_log_fast(
    double _x
)
{
#if ( (!D_MATH_DBL_EVAL_WIDE) &&                                              \
      (D_CFG_MATH_LOG_FAST == D_CFG_MATH_FAST_OWN) )
    return d_internal_math_d_ln_fast(_x);
#else
    return d_internal_math_d_ln_library(_x);
#endif  // !D_MATH_DBL_EVAL_WIDE && D_CFG_MATH_LOG_FAST is the own form
}

// d_math_d_log_fast_with
//   log(_x) by the form _form names, chosen at run time: the C library's log
// where it is D_CFG_MATH_FAST_LIBRARY, the own formula for any other value,
// D_CFG_MATH_FAST_OWN among them, whatever D_CFG_MATH_LOG_FAST says; the C
// library's where double is evaluated wider than double.
D_MATH_FN_RT double
d_math_d_log_fast_with(
    double _x,
    int    _form
)
{
#if !D_MATH_DBL_EVAL_WIDE
    return (_form == D_CFG_MATH_FAST_LIBRARY)
        ? d_internal_math_d_ln_library(_x)
        : d_internal_math_d_ln_fast(_x);
#else
    (void)_form;

    return d_internal_math_d_ln_library(_x);
#endif  // !D_MATH_DBL_EVAL_WIDE
}
"""


def log_d_section():
    """the double family's section 3.3, its constants filled in."""
    consts, rows, digits = log_d_constants()
    text = LOG_D_SECTION.replace("@TABLE@", log_d_table_macro(rows))
    for key in ("LN2H", "LN2M", "LN2L", "LN2L1"):
        text = text.replace("@" + key + "@", consts[key])
    for i, h in enumerate(LOG_D_Q):
        text = text.replace("@Q%d@" % i, exp_d_hex_literal(h))
    for i, (h, l) in enumerate(LOG_D_P):
        if l is None:
            text = text.replace("@P%d@" % i, exp_d_hex_literal(h))
        else:
            text = text.replace("@P%dH@" % i, exp_d_hex_literal(h))
            text = text.replace("@P%dL@" % i, exp_d_hex_literal(l))
    third = LOG_D_SECTION_3
    for key, part in (("@DIGITS_A@", digits[:3640]),
                      ("@DIGITS_B@", digits[3640:])):
        lines = exp_d_digits_literal("", part, 56)
        out = []
        for i, line in enumerate(lines):
            if i + 1 < len(lines):
                out.append(line + " " * (79 - len(line)) + "\\")
            else:
                out.append(line)
        third = third.replace(key, "\n".join(out))
    text += third
    text = text.replace("@FAST_TABLE@", log_d_table_macro(
        log_d_fast_rows()).replace("D_INTERNAL_MATH_D_LN_TABLE ",
                                   "D_INTERNAL_MATH_D_LN_FAST_TABLE"))
    for i, h in enumerate(LOG_D_FAST_A):
        text = text.replace("@LFA%d@" % i, exp_d_hex_literal(h))
    for i, h in enumerate(LOG_D_FAST_B):
        text = text.replace("@LFB%d@" % i, exp_d_hex_literal(h))
    for key, value in (("@LFBOUND@", LOG_D_FAST_BOUNDS["all"]),
                       ("@LFBOUNDN@", LOG_D_FAST_BOUNDS["near1"]),
                       ("@LFBOUNDT@", LOG_D_FAST_BOUNDS["table"])):
        text = text.replace(key, value)
    for key in ("LN2H", "LN2L1"):
        text = text.replace("@" + key + "@", consts[key])
    for key, value in (("@B1A@", LOG_D_BOUNDS["p1a"]),
                       ("@B1@", LOG_D_BOUNDS["p1"]),
                       ("@QERR@", LOG_D_BOUNDS["qerr"]),
                       ("@KA@", str(LOG_D_BOUNDS["ka"])),
                       ("@KAPPA_TEXT@", LOG_D_BOUNDS["kappa_text"]),
                       ("@KAPPA@", "(%d.0 / %d.0)" % LOG_D_BOUNDS["kappa"]),
                       ("@EA@", "(1.0 / %d.0)" % (1 << LOG_D_BOUNDS["ka"])),
                       ("@B2@", LOG_D_BOUNDS["p2"]),
                       ("@B3U@", LOG_D_BOUNDS["p3u"]),
                       ("@B3@", LOG_D_BOUNDS["p3"])):
        text = text.replace(key, value)
    assert "@" not in text.replace("@param", "")
    return text


def log_d_swap(text):
    """the double family's text with its natural logarithm replaced."""
    a = text.index("// 3.3    Natural logarithm\n")
    b = text.index("// 3.4    Base-2 exponential\n")
    return text[:a] + log_d_section() + "\n" + text[b:]
