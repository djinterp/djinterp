/*******************************************************************************
* djinterp [cpp/parsegen]                                             corpus.hpp
*
* Inputs made from a grammar, and two programs run over the same inputs and
* compared.
*   generate() walks a grammar's rules at random from its start rule, each
* choice and each repetition drawn from a seeded generator, and, past a depth
* or a length, takes each rule's shortest way out, so every walk ends. Each
* derivation is then copied with one character deleted, inserted or replaced:
* a near-miss, for the inputs a grammar should refuse. Derived is not
* accepted: read as a PEG, ordered choice and greedy repetition can refuse a
* string the rules derive, which is one of the things a corpus is for.
*   compare() runs two programs over a corpus and names every input they say
* different things of: one matches and the other does not, one reads further,
* or one finishes where the other runs out of steps. Two programs "agree" over
* a corpus when it names none.
*   The generator is splitmix64 and its draws are reduced here, so one seed
* gives one corpus on every platform and at every language level.
*
*
* path:      /inc/djinterp/cpp/modules/parsegen/vparse/corpus.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.17
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_VPARSE_CORPUS_HPP
#define DJINTERP_PARSEGEN_VPARSE_CORPUS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error, as
// the vparse headers it stands on are.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "./ebnf.hpp"     // ebnf::rules, the tree a walk follows
#include "./peg.hpp"      // peg::program
#include "./ruleset.hpp"  // ruleset
// re_std
#include "../../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint64_t


NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

// corpus
//   namespace: inputs made from a grammar, and differential runs over them.
namespace corpus
{

// settings
//   struct: how a corpus is made: how many walks, how deep a walk goes in
// rules before it takes the shortest way out, how long a walk's text grows
// before it does the same, how many times a repetition repeats at most, how
// many near-misses each walk is copied to, and the seed.
struct settings
{
    std::size_t      walks   = 50u;
    std::size_t      depth   = 8u;
    std::size_t      length  = 256u;
    std::size_t      repeats = 3u;
    std::size_t      misses  = 1u;
    re_std::uint64_t seed    = 1u;
};

// sample
//   struct: one input of a corpus: its text, and whether a walk derived it
// or it is a near-miss copied from one.
struct sample
{
    std::string text;
    bool        derived = true;
};

// verdict
//   struct: what one program said of one input: whether it matched, how far
// it read, how many steps it took, and whether it finished within the step
// limit.
struct verdict
{
    bool        ok       = false;
    std::size_t consumed = 0u;
    long        steps    = 0;
    bool        finished = true;
};

// difference
//   struct: an input two programs said different things of, by its place in
// the corpus, and what each said.
struct difference
{
    std::size_t index = 0u;
    verdict     first;
    verdict     second;
};

// comparison
//   struct: two programs over one corpus: how many inputs, how many both
// matched, the inputs they said different things of, in corpus order, and
// each program's steps over the whole corpus.
struct comparison
{
    std::size_t             inputs       = 0u;
    std::size_t             matched      = 0u;
    std::vector<difference> differences;
    long                    first_steps  = 0;
    long                    second_steps = 0;
};

/**
 * @brief Makes a corpus from a grammar EBNF's frontend read.
 *
 * @note A rule with no way out -- every alternative reaches it again before
 *       any text -- cannot be walked, and is left out of each walk's choices;
 *       a start rule with none gives no walks at all.
 *
 * @param[in] _rules     the grammar's rules, the first the start rule.
 * @param[in] _settings  how to make it.
 * @return the walks' texts and their near-misses, each text once, in the
 *         order first made.
 */
std::vector<sample> generate(
    const ebnf::rules& _rules,
    const settings&    _settings
);

/**
 * @brief Makes a corpus from a grammar in the metacircular notation.
 *
 * @param[in] _grammar   the grammar, the first rule the start rule.
 * @param[in] _settings  how to make it.
 * @return the corpus generate() makes from the same rules, as EBNF's tree
 *         spells them.
 */
std::vector<sample> generate(
    const ruleset&  _grammar,
    const settings& _settings
);

/**
 * @brief Runs two programs over one corpus, and names the inputs they say
 *        different things of.
 *
 * @param[in] _first       a program.
 * @param[in] _second      another, of the same language or not.
 * @param[in] _inputs      the corpus.
 * @param[in] _step_limit  the steps either may take over one input.
 * @return each difference, and each program's steps.
 */
comparison compare(
    const peg::program&             _first,
    const peg::program&             _second,
    const std::vector<std::string>& _inputs,
    long                            _step_limit
);

/**
 * @brief Runs one program over one input.
 *
 * @param[in] _program     the program.
 * @param[in] _input       the input.
 * @param[in] _step_limit  the steps it may take.
 * @return what it said.
 */
verdict judge(
    const peg::program& _program,
    const std::string&  _input,
    long                _step_limit
);

}  // namespace corpus

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_PARSEGEN_VPARSE_CORPUS_HPP
