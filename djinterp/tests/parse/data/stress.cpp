/*******************************************************************************
* djinterp [test]                                                     stress.cpp
*
* Exercises `local.hpp`.
*   Data, not a unit: input for parse's C++ lexer, given by name to the
* lexer tools beside it (`dump_lex_cpp`, `corpus_lex --cpp`), holding
* digraphs, raw strings, C++23 literals and a line splice. Nothing compiles
* it, and `local.hpp` exists nowhere; the checks leave `tests/**/data/` out
* (lane A, 11-12).
*
*
* path:      /tests/parse/data/stress.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
*******************************************************************************/
#include <string>
#include "local.hpp"
namespace n <% using namespace std;
template <class T> struct S <%
    auto f() -> T*; auto g() const -> decltype(auto);
    friend bool operator<=>(S const&, S const&) = default;
%>;
constexpr auto a = 1'000'000uz, b = 0b1010'1010, c = 0x1p-3;
auto r0 = R"(unescaped " and \ and )not-the-end)";
auto r1 = R"xy(nested )xz( inside)xy";
auto r2 = u8R"(utf-8 raw)";
auto r3 = LR"(wide raw)";
auto s0 = "plain"sv; auto s1 = u8"utf8"s; auto s2 = 1_km; auto s3 = 2.5_deg;
auto v = A<::B>::value;   auto w = arr<:0:>;
bool t = x not_eq y and not z or w bitand v;
int spl\
iced = 1;
auto lam = [&]<typename U>(U&& u) noexcept -> void { (void)u; };
auto co = [] -> int { co_return 1; };
%>
