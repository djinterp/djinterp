/*******************************************************************************
* djinterp [core]                                                    memento.hpp
*
* Memento Pattern Module:
*   Provides a comprehensive, container-agnostic, version-portable foundation
* for the memento pattern. Supports multiple snapshot strategies, undo/redo
* history management, and compile-time capability detection - all without
* coupling to any specific data structure.
*
*   DESIGN:
*   The module is organized in three layers:
*     1. TRAITS - SFINAE-based detection of memento-related capabilities
*        (save/restore, clone, serialize, diff) on arbitrary types.
*     2. CORE - abstract and CRTP base classes that define the memento
*        protocol: originator (state owner), memento (snapshot), and
*        caretaker (history manager).
*     3. POLICIES - pluggable snapshot strategies (deep copy, serialized,
*        delta/diff, external) and history policies (unlimited, bounded,
*        coalescing) that compose with the core via template parameters.
*
*   PORTABILITY:
*   - C++11  : core memento protocol, snapshot strategies, history stack,
*              SFINAE capability traits, caretaker, type-erased memento
*              (via re_std::any - RTTI-free, constexpr-capable)
*   - C++14  : generic lambda support in for_each_memento, make_caretaker
*   - C++17  : std::optional integration, string_view tags, if constexpr
*              dispatch, structured bindings
*   - C++20  : concept-constrained originator/memento/caretaker,
*              std::span for external buffer snapshots
*
*
* path:      /inc/djinterp/core/paradigm/momento/memento.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.09
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CONFIGURATION & FEATURE GATES
      -----------------------------
      i.    D_MEMENTO_HAS_OPTIONAL
      ii.   D_MEMENTO_HAS_CONCEPTS
      iii.  D_MEMENTO_HAS_SPAN
      iv.   D_MEMENTO_DEFAULT_HISTORY_CAPACITY

II.   CAPABILITY TRAITS
      -----------------
      i.    has_save_state_method
      ii.   has_restore_state_method
      iii.  has_clone_method
      iv.   has_serialize_method
      v.    has_deserialize_method
      vi.   has_diff_method
      vii.  has_apply_diff_method
      viii. has_equality_operator
      ix.   is_memento_capable
      x.    is_serializable_memento_capable
      xi.   is_diff_memento_capable
      xii.  memento_capability (aggregate)

III.  SNAPSHOT STRATEGIES (POLICIES)
      ------------------------------
      i.    deep_copy_snapshot
      ii.   clone_snapshot
      iii.  serialized_snapshot
      iv.   delta_snapshot (internal: delta_record)
      v.    external_snapshot

IV.   HISTORY POLICIES
      ----------------
      i.    unlimited_history
      ii.   bounded_history
      iii.  coalescing_history

V.    MEMENTO CORE
      ------------
      i.    memento (snapshot wrapper)
      ii.   memento_metadata
      iii.  memento_originator (CRTP)
      iv.   memento_caretaker

VI.   UNDO / REDO STACK
      -----------------
      i.    undo_redo_stack

VII.  TYPE-ERASED MEMENTO (C++11+, via re_std::any)
      --------------------------------------------
      i.    any_memento
      ii.   any_memento_caretaker

VIII. CONVENIENCE FACTORIES (C++14+)
      ------------------------------
      i.    make_memento
      ii.   make_caretaker

IX.   CONCEPT-CONSTRAINED INTERFACES (C++20+)
      ---------------------------------------
      i.    memento_source (concept)
      ii.   memento_target (concept)
      iii.  snapshot_strategy (concept)
      iv.   history_policy (concept)
*/

#ifndef DJINTERP_PARADIGM_MOMENTO_MEMENTO_HPP
#define DJINTERP_PARADIGM_MOMENTO_MEMENTO_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_utility.hpp"  // void_t
#include "../../meta/type_traits.hpp"
// re_std
#include "../../../../re_std/any/any.hpp"  // include
                                           // "../../../../re_std/any/any.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // std
    #include <chrono>
    #include <functional>
    #include <memory>
    #include <utility>
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // std
    #include <optional>
    #include <string_view>
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // std
    #include <concepts>
    #include <span>
#endif


///////////////////////////////////////////////////////////////////////////////
///          I.    CONFIGURATION & FEATURE GATES                            ///
///////////////////////////////////////////////////////////////////////////////

// D_MEMENTO_HAS_OPTIONAL
//   macro: 1 if std::optional is available (C++17+).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_MEMENTO_HAS_OPTIONAL 1
#else
    #define D_MEMENTO_HAS_OPTIONAL 0
#endif

// D_MEMENTO_HAS_CONCEPTS
//   macro: 1 if concepts are available (C++20+).
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #define D_MEMENTO_HAS_CONCEPTS 1
#else
    #define D_MEMENTO_HAS_CONCEPTS 0
#endif

// D_MEMENTO_HAS_SPAN
//   macro: 1 if std::span is available (C++20+).
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #define D_MEMENTO_HAS_SPAN 1
#else
    #define D_MEMENTO_HAS_SPAN 0
#endif

// D_MEMENTO_DEFAULT_HISTORY_CAPACITY
//   macro: default maximum number of snapshots retained by bounded
// history policies. Users may define this before including memento.hpp.
#ifndef D_MEMENTO_DEFAULT_HISTORY_CAPACITY
    #define D_MEMENTO_DEFAULT_HISTORY_CAPACITY 64
#endif


NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///             II.   CAPABILITY TRAITS                                     ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // =====================================================================
    // Individual method detection
    // =====================================================================

    // has_save_state_method
    //   trait: detects Type::save_state() returning a snapshot object.
    template<typename Type,
             typename = void>
    struct has_save_state_method : std::false_type
    {};

    template<typename Type>
    struct has_save_state_method<Type, void_t<
        decltype(std::declval<const Type>().save_state())
    >> : std::true_type
    {};

    // has_restore_state_method
    //   trait: detects Type::restore_state(snapshot) accepting the type
    // returned by save_state().
    template<typename Type,
             typename = void>
    struct has_restore_state_method : std::false_type
    {};

    template<typename Type>
    struct has_restore_state_method<Type, void_t<
        decltype(std::declval<Type>().restore_state(
            std::declval<const Type>().save_state()))
    >> : std::true_type
    {};

    // has_clone_method
    //   trait: detects Type::clone() returning a copy of the object.
    template<typename Type,
             typename = void>
    struct has_clone_method : std::false_type
    {};

    template<typename Type>
    struct has_clone_method<Type, void_t<
        decltype(std::declval<const Type>().clone())
    >> : std::true_type
    {};

    // has_serialize_method
    //   trait: detects Type::serialize() returning a byte-like sequence.
    template<typename Type,
             typename = void>
    struct has_serialize_method : std::false_type
    {};

    template<typename Type>
    struct has_serialize_method<Type, void_t<
        decltype(std::declval<const Type>().serialize())
    >> : std::true_type
    {};

    // has_deserialize_method
    //   trait: detects a static Type::deserialize(...) factory or a member
    // deserialize() accepting the output of serialize().
    template<typename Type,
             typename = void>
    struct has_deserialize_method : std::false_type
    {};

    template<typename Type>
    struct has_deserialize_method<Type, void_t<
        decltype(std::declval<Type>().deserialize(
            std::declval<const Type>().serialize()))
    >> : std::true_type
    {};

    // has_diff_method
    //   trait: detects Type::diff(other) producing a delta between two
    // states.
    template<typename Type,
             typename = void>
    struct has_diff_method : std::false_type
    {};

    template<typename Type>
    struct has_diff_method<Type, void_t<
        decltype(std::declval<const Type>().diff(
            std::declval<const Type>()))
    >> : std::true_type
    {};

    // has_apply_diff_method
    //   trait: detects Type::apply_diff(delta) to reconstruct state from
    // a delta.
    template<typename Type,
             typename = void>
    struct has_apply_diff_method : std::false_type
    {};

    template<typename Type>
    struct has_apply_diff_method<Type, void_t<
        decltype(std::declval<Type>().apply_diff(
            std::declval<const Type>().diff(std::declval<const Type>())))
    >> : std::true_type
    {};

    // has_equality_operator
    //   trait: detects operator==(const Type&, const Type&).
    template<typename Type,
             typename = void>
    struct has_equality_operator : std::false_type
    {};

    template<typename Type>
    struct has_equality_operator<Type, void_t<
        decltype(std::declval<const Type>() == std::declval<const Type>())
    >> : std::true_type
    {};

NS_END  // internal

// =========================================================================
// Public trait accessors
// =========================================================================

// is_memento_capable
//   trait: true if Type has both save_state() and restore_state().
template<typename Type>
struct is_memento_capable
{
    static constexpr bool value =
        ( internal::has_save_state_method<Type>::value &&
          internal::has_restore_state_method<Type>::value );
};

// is_copyable_memento_capable
//   trait: true if Type is copy-constructible (enabling deep-copy snapshots).
template<typename Type>
struct is_copyable_memento_capable
{
    static constexpr bool value = std::is_copy_constructible<Type>::value;
};

// is_clonable_memento_capable
//   trait: true if Type provides clone().
template<typename Type>
struct is_clonable_memento_capable
{
    static constexpr bool value = internal::has_clone_method<Type>::value;
};

// is_serializable_memento_capable
//   trait: true if Type provides serialize() and deserialize().
template<typename Type>
struct is_serializable_memento_capable
{
    static constexpr bool value =
        ( internal::has_serialize_method<Type>::value &&
          internal::has_deserialize_method<Type>::value );
};

// is_diff_memento_capable
//   trait: true if Type provides diff() and apply_diff().
template<typename Type>
struct is_diff_memento_capable
{
    static constexpr bool value =
        ( internal::has_diff_method<Type>::value &&
          internal::has_apply_diff_method<Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename Type>
    constexpr bool is_memento_capable_v =
        is_memento_capable<Type>::value;

    template<typename Type>
    constexpr bool is_copyable_memento_capable_v =
        is_copyable_memento_capable<Type>::value;

    template<typename Type>
    constexpr bool is_clonable_memento_capable_v =
        is_clonable_memento_capable<Type>::value;

    template<typename Type>
    constexpr bool is_serializable_memento_capable_v =
        is_serializable_memento_capable<Type>::value;

    template<typename Type>
    constexpr bool is_diff_memento_capable_v =
        is_diff_memento_capable<Type>::value;

#endif

// memento_capability
//   struct: aggregate classification of a type's memento support.
template<typename Type>
struct memento_capability
{
    static constexpr bool has_save_restore =
        is_memento_capable<Type>::value;

    static constexpr bool has_copy =
        is_copyable_memento_capable<Type>::value;

    static constexpr bool has_clone =
        is_clonable_memento_capable<Type>::value;

    static constexpr bool has_serialization =
        is_serializable_memento_capable<Type>::value;

    static constexpr bool has_diff =
        is_diff_memento_capable<Type>::value;

    static constexpr bool has_equality =
        internal::has_equality_operator<Type>::value;

    // true if any snapshot strategy is viable
    static constexpr bool is_snapshottable =
        ( has_save_restore ||
          has_copy         ||
          has_clone        ||
          has_serialization );
};


///////////////////////////////////////////////////////////////////////////////
///          III.  SNAPSHOT STRATEGIES (POLICIES)                           ///
///////////////////////////////////////////////////////////////////////////////

// Each snapshot strategy is a stateless policy class with two static
// methods:
//   static auto capture(const State& s) -> snapshot_type;
//   static void restore(State& s, const snapshot_type& snap);
//
// The caretaker and undo_redo_stack are parameterized on these policies.

// =========================================================================
// deep_copy_snapshot
// =========================================================================

// deep_copy_snapshot
//   policy: captures state by copy construction and restores by copy
// assignment. The simplest strategy; requires State to be copyable.
struct deep_copy_snapshot
{
    // snapshot_type_for
    //   type: for a given state type, the snapshot is a plain copy.
    template<typename State>
    using snapshot_type_for = State;

    template<typename State>
    static State
    capture(
        const State& _state
    )
    {
        return _state;
    }

    template<typename State>
    static void
    restore(
        State&       _state,
        const State& _snapshot
    )
    {
        _state = _snapshot;

        return;
    }
};


// =========================================================================
// clone_snapshot
// =========================================================================

// clone_snapshot
//   policy: captures state via State::clone() and restores by copy
// assignment. For types where copy construction is disabled but a
// virtual or explicit clone is provided.
struct clone_snapshot
{
    template<typename State>
    using snapshot_type_for = decltype(std::declval<const State>().clone());

    template<typename State>
    static auto
    capture(
        const State& _state
    ) -> decltype(_state.clone())
    {
        return _state.clone();
    }

    template<typename State>
    static void
    restore(
        State&                                               _state,
        const decltype(std::declval<const State>().clone())& _snapshot
    )
    {
        _state = _snapshot;

        return;
    }
};


// =========================================================================
// serialized_snapshot
// =========================================================================

// serialized_snapshot
//   policy: captures state via State::serialize() and restores via
// State::deserialize(). Snapshots are stored in the serialized form
// (typically std::vector<char> or std::string), enabling compact
// storage and potential persistence.
struct serialized_snapshot
{
    template<typename State>
    using snapshot_type_for =
        decltype(std::declval<const State>().serialize());

    template<typename State>
    static auto
    capture(
        const State& _state
    ) -> decltype(_state.serialize())
    {
        return _state.serialize();
    }

    template<typename State>
    static void
    restore(
        State& _state,
        const decltype(std::declval<const State>().serialize())& _snapshot
    )
    {
        _state.deserialize(_snapshot);

        return;
    }
};


// =========================================================================
// delta_snapshot
// =========================================================================

NS_INTERNAL

    // delta_record
    //   struct: stores a delta (diff) between two states and holds a
    // reference baseline for reconstruction.
    template<typename DiffType>
    struct delta_record
    {
        DiffType diff;
        bool      is_baseline;

        delta_record()
            : diff(),
              is_baseline(false)
        {}

        explicit delta_record(
            DiffType _d,
            bool      _baseline = false
        )
            : diff(std::move(_d)),
                is_baseline(_baseline)
        {}
    };

NS_END  // internal

// delta_snapshot
//   policy: captures state as a diff from the previous state. The first
// capture is always a full baseline. Subsequent captures store only the
// delta (via State::diff()). Restoration walks deltas back to the
// nearest baseline.
//
// Note: this policy is stateful at the strategy level - the caretaker
// must store the previous state for computing diffs. The policy itself
// provides the diff/apply_diff wrappers.
struct delta_snapshot
{
    template<typename State>
    using diff_type = decltype(
        std::declval<const State>().diff(std::declval<const State>()));

    template<typename State>
    using snapshot_type_for = internal::delta_record<diff_type<State>>;

    // capture_baseline
    //   function: captures a full-state diff acting as a baseline.
    template<typename State>
    static snapshot_type_for<State>
    capture_baseline(
        const State& _state
    )
    {
        // baseline: diff against a default-constructed state
        return snapshot_type_for<State>(
            _state.diff(State{}),
            true);
    }

    // capture_delta
    //   function: captures the diff between _previous and _current.
    template<typename State>
    static snapshot_type_for<State>
    capture_delta(
        const State& _previous,
        const State& _current
    )
    {
        return snapshot_type_for<State>(
            _current.diff(_previous),
            false);
    }

    // restore_from_baseline
    //   function: restores state from a baseline delta.
    template<typename State>
    static void
    restore_from_baseline(
        State&                          _state,
        const snapshot_type_for<State>& _record
    )
    {
        State base{};
        base.apply_diff(_record.diff);
        _state = std::move(base);

        return;
    }

    // apply_delta
    //   function: applies a non-baseline delta to the current state.
    template<typename State>
    static void
    apply_delta(
        State&                          _state,
        const snapshot_type_for<State>& _record
    )
    {
        _state.apply_diff(_record.diff);

        return;
    }
};


// =========================================================================
// external_snapshot
// =========================================================================

// external_snapshot
//   policy: delegates capture and restore to user-supplied callables.
// Useful when the snapshot mechanism lives outside the state object
// (e.g., a database transaction, a file checkpoint, or a third-party
// serialization library).
template<typename CaptureCallable,
         typename RestoreCallable>
struct external_snapshot
{
    CaptureCallable capture_fn;
    RestoreCallable restore_fn;

    template<typename State>
    using snapshot_type_for =
        decltype(std::declval<CaptureCallable>()(
            std::declval<const State&>()));

    template<typename State>
    auto
    capture(
        const State& _state
    ) const -> snapshot_type_for<State>
    {
        return capture_fn(_state);
    }

    template<typename State,
             typename Snapshot>
    void
    restore(
        State&          _state,
        const Snapshot& _snapshot
    ) const
    {
        restore_fn(_state, _snapshot);

        return;
    }
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

    // make_external_snapshot
    //   function: factory for external_snapshot policies.
    template<typename CaptureFn,
             typename RestoreFn>
    D_CONSTEXPR_INLINE auto
    make_external_snapshot(
        CaptureFn&& _capture,
        RestoreFn&& _restore
    )
        -> external_snapshot<typename std::decay<CaptureFn>::type,
                             typename std::decay<RestoreFn>::type>
    {
        return { std::forward<CaptureFn>(_capture),
                 std::forward<RestoreFn>(_restore) };
    }

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///              IV.   HISTORY POLICIES                                     ///
///////////////////////////////////////////////////////////////////////////////

// History policies control how the caretaker manages its stack of
// mementos. Each policy provides:
//   static bool should_push(const container& history, const snapshot& s);
//   static void after_push(container& history);

// unlimited_history
//   policy: retains all snapshots with no eviction.
struct unlimited_history
{
    template<typename Container,
             typename Snapshot>
    static bool
    should_push(
        const Container& /* _history */,
        const Snapshot&  /* _snapshot */
    )
    {
        return true;
    }

    template<typename Container>
    static void
    after_push(
        Container& /* _history */
    )
    {
        return;
    }
};

// bounded_history
//   policy: retains at most MaxSize snapshots, evicting the oldest
// when the limit is reached.
template<std::size_t MaxSize = D_MEMENTO_DEFAULT_HISTORY_CAPACITY>
struct bounded_history
{
    static constexpr std::size_t max_size = MaxSize;

    template<typename Container,
             typename Snapshot>
    static bool
    should_push(
        const Container& /* _history */,
        const Snapshot&  /* _snapshot */
    )
    {
        return true;
    }

    template<typename Container>
    static void
    after_push(
        Container& _history
    )
    {
        while (_history.size() > MaxSize)
        {
            _history.erase(_history.begin());
        }

        return;
    }
};

// coalescing_history
//   policy: suppresses duplicate consecutive snapshots. A new snapshot
// is only pushed if it differs from the most recent one (requires
// operator== on the snapshot type).
struct coalescing_history
{
    template<typename Container,
             typename Snapshot>
    static bool
    should_push(
        const Container& _history,
        const Snapshot&  _snapshot
    )
    {
        if (_history.empty())
        {
            return true;
        }

        return !(_history.back().state == _snapshot);
    }

    template<typename Container>
    static void
    after_push(
        Container& /* _history */
    )
    {
        return;
    }
};


///////////////////////////////////////////////////////////////////////////////
///               V.    MEMENTO CORE                                        ///
///////////////////////////////////////////////////////////////////////////////

// =========================================================================
// memento_metadata
// =========================================================================

// memento_metadata
//   struct: optional metadata attached to each snapshot. Stores a
// monotonic sequence number and a user-provided description tag.
struct memento_metadata
{
    std::size_t sequence;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    std::string_view tag;
#endif

    memento_metadata()
        : sequence(0)
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
        , tag()
#endif
    {}

    explicit memento_metadata(
            std::size_t _seq
        )
            : sequence(_seq)
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
            , tag()
#endif
        {}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    memento_metadata(
            std::size_t      _seq,
            std::string_view _tag
        )
            : sequence(_seq),
              tag(_tag)
        {}
#endif
};


// =========================================================================
// memento
// =========================================================================

// memento
//   struct: a single snapshot entry pairing captured state with metadata.
template<typename Snapshot>
struct memento
{
    using snapshot_type = Snapshot;

    Snapshot         state;
    memento_metadata meta;

    memento()
        : state(),
          meta()
    {}

    explicit memento(
        Snapshot         _s,
        memento_metadata _m = memento_metadata()
    )
        : state(std::move(_s)),
          meta(_m)
    {}
};


// =========================================================================
// memento_originator (CRTP)
// =========================================================================

// memento_originator
//   class: CRTP base that injects save/restore protocol into a state-
// owning class. Derived is the concrete type; SnapshotPolicy is the
// strategy used to capture and restore snapshots.
//
// Derived must be accessible via static_cast from this base; it
// represents the complete state object.
//
// Usage:
//   class editor_state
//       : public memento_originator<editor_state, deep_copy_snapshot>
//   {
//       std::string text;
//   public:
//       // deep_copy_snapshot uses copy ctor/assignment - nothing
//       // extra needed.
//   };
//
//   editor_state state;
//   auto snap = state.create_memento();
//   // ... mutate state ...
//   state.restore_memento(snap);
template<typename Derived,
         typename SnapshotPolicy = deep_copy_snapshot>
class memento_originator
{
public:
    using snapshot_policy = SnapshotPolicy;
    using snapshot_type   = typename SnapshotPolicy::template
                                snapshot_type_for<Derived>;
    using memento_type    = memento<snapshot_type>;

    // create_memento
    //   function: captures the current state as a memento.
    memento_type
    create_memento() const
    {
        const Derived& self = static_cast<const Derived&>(*this);
        snapshot_type snap    = SnapshotPolicy::capture(self);

        memento_type m(std::move(snap),
                       memento_metadata(m_sequence++));

        return m;
    }

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // create_memento (tagged)
    //   function: captures the current state with a descriptive tag.
    memento_type
    create_memento(
        std::string_view _tag
    ) const
    {
        const Derived& self = static_cast<const Derived&>(*this);
        snapshot_type snap    = SnapshotPolicy::capture(self);

        memento_type m(std::move(snap),
                       memento_metadata(m_sequence++, _tag));

        return m;
    }
#endif

    // restore_memento
    //   function: restores state from a previously captured memento.
    void
    restore_memento(
        const memento_type& _memento
    )
    {
        Derived& self = static_cast<Derived&>(*this);

        // preserve the originator's own bookkeeping across the restore: a
        // whole-object snapshot policy (e.g. deep_copy_snapshot) round-trips
        // m_sequence through the snapshotted state, which would otherwise
        // rewind the monotonic memento counter.
        const std::size_t saved_sequence = m_sequence;
        SnapshotPolicy::restore(self,
                                 _memento.state);
        m_sequence = saved_sequence;

        return;
    }

    // current_sequence
    //   function: returns the sequence number that will be assigned to
    // the next memento.
    std::size_t
    current_sequence() const noexcept
    {
        return m_sequence;
    }

private:
    mutable std::size_t m_sequence = 0;
};


// =========================================================================
// memento_caretaker
// =========================================================================

// memento_caretaker
//   class: manages a history of mementos for a single originator.
// Parameterized on the snapshot type and the history eviction policy.
template<typename Snapshot,
         typename HistoryPolicy = unlimited_history>
class memento_caretaker
{
public:
    using snapshot_type  = Snapshot;
    using memento_type   = memento<Snapshot>;
    using history_policy = HistoryPolicy;
    using container_type = std::vector<memento_type>;
    using size_type      = std::size_t;

    // push
    //   function: stores a memento into the history. The history policy
    // may suppress or evict entries.
    void
    push(
        memento_type _m
    )
    {
        if (HistoryPolicy::should_push(m_history, _m.state))
        {
            m_history.push_back(std::move(_m));
            HistoryPolicy::after_push(m_history);
        }

        return;
    }

    // pop
    //   function: removes and returns the most recent memento.
    // Undefined behaviour if history is empty; call empty() first.
    memento_type
    pop()
    {
        memento_type m = std::move(m_history.back());
        m_history.pop_back();

        return m;
    }

#if D_MEMENTO_HAS_OPTIONAL

    // try_pop
    //   function: removes and returns the most recent memento, or
    // std::nullopt if the history is empty.
    std::optional<memento_type>
    try_pop()
    {
        if (m_history.empty())
        {
            return std::nullopt;
        }

        return pop();
    }

#endif  // D_MEMENTO_HAS_OPTIONAL

    // peek
    //   function: returns a const reference to the most recent memento
    // without removing it.
    const memento_type&
    peek() const
    {
        return m_history.back();
    }

    // at
    //   function: indexed access into the history (0 = oldest).
    const memento_type&
    at(
        size_type _index
    ) const
    {
        return m_history[_index];
    }

    // size
    //   function: number of mementos currently held.
    D_CONSTEXPR size_type
    size() const noexcept
    {
        return m_history.size();
    }

    // empty
    //   function: true if no mementos are stored.
    D_CONSTEXPR bool
    empty() const noexcept
    {
        return m_history.empty();
    }

    // clear
    //   function: discards all stored mementos.
    void
    clear()
    {
        m_history.clear();

        return;
    }

    // for_each
    //   function: iterates over all mementos oldest-to-newest, invoking
    // _fn(const memento_type&) for each.
    template<typename Fn>
    void
    for_each(
        Fn&& _fn
    ) const
    {
        for (const auto& m : m_history)
        {
            _fn(m);
        }

        return;
    }

    // for_each_reverse
    //   function: iterates newest-to-oldest.
    template<typename Fn>
    void
    for_each_reverse(
        Fn&& _fn
    ) const
    {
        for (auto it = m_history.rbegin(); it != m_history.rend(); ++it)
        {
            _fn(*it);
        }

        return;
    }

private:
    container_type m_history;
};


///////////////////////////////////////////////////////////////////////////////
///            VI.   UNDO / REDO STACK                                      ///
///////////////////////////////////////////////////////////////////////////////

// undo_redo_stack
//   class: manages a dual-stack undo/redo history for an originator.
// Capturing a new checkpoint pushes to the undo stack and clears the
// redo stack (branching invalidates the forward history). Undo pops
// the undo stack and pushes to redo; redo does the reverse.
//
// Usage:
//   undo_redo_stack<editor_state> history;
//   history.checkpoint(state);    // save current
//   // ... mutate state ...
//   history.undo(state);          // restore previous, push current to redo
//   history.redo(state);          // restore forward, push current to undo
template<typename State,
         typename SnapshotPolicy = deep_copy_snapshot,
         typename HistoryPolicy   = unlimited_history>
class undo_redo_stack
{
public:
    using snapshot_type = typename SnapshotPolicy::template
                              snapshot_type_for<State>;
    using memento_type  = memento<snapshot_type>;

    // checkpoint
    //   function: captures the current state as an undo point. Clears
    // the redo stack (forward history is invalidated by mutation).
    void
    checkpoint(
        const State& _state
    )
    {
        memento_type m(
            SnapshotPolicy::capture(_state),
            memento_metadata(m_sequence++));

        m_undo.push(std::move(m));
        m_redo.clear();

        return;
    }

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // checkpoint (tagged)
    //   function: captures with a descriptive tag.
    void
    checkpoint(
        const State&    _state,
        std::string_view _tag
    )
    {
        memento_type m(
            SnapshotPolicy::capture(_state),
            memento_metadata(m_sequence++, _tag));

        m_undo.push(std::move(m));
        m_redo.clear();

        return;
    }
#endif

    // undo
    //   function: restores the most recent undo snapshot into _state,
    // pushing the current state onto the redo stack. Returns true if
    // an undo was performed, false if the undo stack was empty.
    bool
    undo(
        State& _state
    )
    {
        if (m_undo.empty())
        {
            return false;
        }

        // save current state to redo before restoring
        memento_type redo_point(
            SnapshotPolicy::capture(_state),
            memento_metadata(m_sequence++));
        m_redo.push(std::move(redo_point));

        // restore from undo
        memento_type prev = m_undo.pop();
        SnapshotPolicy::restore(_state,
                                 prev.state);

        return true;
    }

    // redo
    //   function: restores the most recent redo snapshot into _state,
    // pushing the current state onto the undo stack. Returns true if
    // a redo was performed, false if the redo stack was empty.
    bool
    redo(
        State& _state
    )
    {
        if (m_redo.empty())
        {
            return false;
        }

        // save current state to undo before restoring
        memento_type undo_point(
            SnapshotPolicy::capture(_state),
            memento_metadata(m_sequence++));
        m_undo.push(std::move(undo_point));

        // restore from redo
        memento_type next = m_redo.pop();
        SnapshotPolicy::restore(_state,
                                 next.state);

        return true;
    }

    // can_undo
    //   function: true if undo is available.
    bool
    can_undo() const noexcept
    {
        return !m_undo.empty();
    }

    // can_redo
    //   function: true if redo is available.
    bool
    can_redo() const noexcept
    {
        return !m_redo.empty();
    }

    // undo_depth
    //   function: number of undo steps available.
    std::size_t
    undo_depth() const noexcept
    {
        return m_undo.size();
    }

    // redo_depth
    //   function: number of redo steps available.
    std::size_t
    redo_depth() const noexcept
    {
        return m_redo.size();
    }

    // clear
    //   function: discards all undo and redo history.
    void
    clear()
    {
        m_undo.clear();
        m_redo.clear();
        m_sequence = 0;

        return;
    }

    // clear_redo
    //   function: discards forward history only.
    void
    clear_redo()
    {
        m_redo.clear();

        return;
    }

#if D_MEMENTO_HAS_OPTIONAL

    // peek_undo
    //   function: returns the snapshot at the top of the undo stack,
    // or std::nullopt if empty.
    std::optional<std::reference_wrapper<const memento_type>>
    peek_undo() const
    {
        if (m_undo.empty())
        {
            return std::nullopt;
        }

        return std::cref(m_undo.peek());
    }

    // peek_redo
    //   function: returns the snapshot at the top of the redo stack,
    // or std::nullopt if empty.
    std::optional<std::reference_wrapper<const memento_type>>
    peek_redo() const
    {
        if (m_redo.empty())
        {
            return std::nullopt;
        }

        return std::cref(m_redo.peek());
    }

#endif  // D_MEMENTO_HAS_OPTIONAL

private:
    memento_caretaker<snapshot_type, HistoryPolicy> m_undo;
    memento_caretaker<snapshot_type, HistoryPolicy> m_redo;
    std::size_t m_sequence = 0;
};


///////////////////////////////////////////////////////////////////////////////
///         VII.  TYPE-ERASED MEMENTO (C++11+, via re_std::any)             ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// any_memento
//   class: type-erased memento that can store any snapshot type via
// re_std::any. Useful when a caretaker must manage heterogeneous
// state objects (e.g., a multi-document editor where each document
// type has a different snapshot representation).
//
// Uses re_std::any rather than std::any, making this available from
// C++11 with RTTI-free type identity (holds<T>() via function-pointer
// tags) and constexpr support for SBO-eligible types.
//
// The any_type / any_id_type aliases isolate the any namespace; if
// the any header moves to a different namespace, update these two
// aliases and everything flows.
class any_memento
{
private:
    // ---- namespace isolation aliases ----
    // Change these if the any header lives in a different namespace.
    using any_type    = re_std::any;
    using any_id_type = re_std::any_type_id;

public:
    any_memento() = default;

    template<typename Snapshot>
    explicit any_memento(
            Snapshot         _snap,
            memento_metadata _meta = memento_metadata()
        )
            : m_state(std::move(_snap)),
              m_meta(_meta)
        {}

    // has_value
    //   function: true if a snapshot is stored.
    D_CONSTEXPR_CPP14 bool
    has_value() const noexcept
    {
        return m_state.has_value();
    }

    // holds
    //   function: true if the stored snapshot was originally of type
    // Snapshot. RTTI-free; uses any's function-pointer type identity.
    //
    // Note: no .template disambiguator - any_memento is not a class
    // template, so m_state's type is not dependent.
    template<typename Snapshot>
    D_CONSTEXPR bool
    holds() const noexcept
    {
        return m_state.holds<Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (const, SBO types - bool)
    // -----------------------------------------------------------------

    template<typename Snapshot,
             typename std::enable_if<
                 std::is_same<Snapshot, bool>::value,
                 int
             >::type = 0>
    D_CONSTEXPR Snapshot
    get() const noexcept
    {
        return m_state.get<Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (const, SBO types - signed integral, not bool)
    // -----------------------------------------------------------------

    template<typename Snapshot,
             typename std::enable_if<
                 ( std::is_integral<Snapshot>::value &&
                   std::is_signed<Snapshot>::value   &&
                   !std::is_same<Snapshot, bool>::value ),
                 int
             >::type = 0>
    D_CONSTEXPR Snapshot
    get() const noexcept
    {
        return m_state.get<Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (const, SBO types - unsigned integral, not bool)
    // -----------------------------------------------------------------

    template<typename Snapshot,
             typename std::enable_if<
                 ( std::is_integral<Snapshot>::value  &&
                   std::is_unsigned<Snapshot>::value  &&
                   !std::is_same<Snapshot, bool>::value ),
                 int
             >::type = 0>
    D_CONSTEXPR Snapshot
    get() const noexcept
    {
        return m_state.get<Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (const, SBO types - floating point)
    // -----------------------------------------------------------------

    template<typename Snapshot,
             typename std::enable_if<
                 std::is_floating_point<Snapshot>::value,
                 int
             >::type = 0>
    D_CONSTEXPR Snapshot
    get() const noexcept
    {
        return m_state.get<Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (const, SBO types - enum)
    // -----------------------------------------------------------------

    template<typename Snapshot,
             typename std::enable_if<
                 std::is_enum<Snapshot>::value,
                 int
             >::type = 0>
    D_CONSTEXPR Snapshot
    get() const noexcept
    {
        return m_state.get<Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (const, SBO types - pointer)
    // -----------------------------------------------------------------

    template<typename Snapshot,
             typename std::enable_if<
                 std::is_pointer<Snapshot>::value,
                 int
             >::type = 0>
    D_CONSTEXPR Snapshot
    get() const noexcept
    {
        return m_state.get<Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (const, heap types)
    // -----------------------------------------------------------------

    template<typename Snapshot,
             typename std::enable_if<
                 ( !std::is_integral<Snapshot>::value       &&
                   !std::is_floating_point<Snapshot>::value &&
                   !std::is_enum<Snapshot>::value           &&
                   !std::is_pointer<Snapshot>::value ),
                 int
             >::type = 0>
    const Snapshot&
    get() const
    {
        return m_state.get<Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (mutable, heap types only)
    // -----------------------------------------------------------------

    template<typename Snapshot,
             typename std::enable_if<
                 ( !std::is_integral<Snapshot>::value       &&
                   !std::is_floating_point<Snapshot>::value &&
                   !std::is_enum<Snapshot>::value           &&
                   !std::is_pointer<Snapshot>::value ),
                 int
             >::type = 0>
    Snapshot&
    get()
    {
        return m_state.get<Snapshot>();
    }

    // metadata
    //   function: returns the associated metadata.
    const memento_metadata&
    metadata() const noexcept
    {
        return m_meta;
    }

    // type
    //   function: returns the any_type_id of the stored snapshot
    // (a function pointer unique per type).
    D_CONSTEXPR_CPP14 any_id_type
    type() const noexcept
    {
        return m_state.type();
    }

    // reset
    //   function: clears the stored snapshot.
    void
    reset()
    {
        m_state.reset();

        return;
    }

private:
    any_type         m_state;
    memento_metadata m_meta;
};

// any_memento_caretaker
//   class: caretaker managing a history of type-erased mementos.
// Accepts any_memento directly; the caller is responsible for
// type consistency at restore time. Available from C++11.
template<typename HistoryPolicy = unlimited_history>
class any_memento_caretaker
{
public:
    using memento_type = any_memento;
    using size_type    = std::size_t;

    void
    push(
        any_memento _m
    )
    {
        m_history.push_back(std::move(_m));
        HistoryPolicy::after_push(m_history);

        return;
    }

    // pop
    //   function: removes and returns the most recent memento.
    // Undefined behaviour if empty.
    any_memento
    pop()
    {
        any_memento m = std::move(m_history.back());
        m_history.pop_back();

        return m;
    }

#if D_MEMENTO_HAS_OPTIONAL

    // try_pop
    //   function: removes and returns the most recent memento, or
    // std::nullopt if the history is empty.
    std::optional<any_memento>
    try_pop()
    {
        if (m_history.empty())
        {
            return std::nullopt;
        }

        return pop();
    }

#endif  // D_MEMENTO_HAS_OPTIONAL

    const any_memento&
    peek() const
    {
        return m_history.back();
    }

    size_type
    size() const noexcept
    {
        return m_history.size();
    }

    bool
    empty() const noexcept
    {
        return m_history.empty();
    }

    void
    clear()
    {
        m_history.clear();

        return;
    }

    // for_each
    //   function: iterates oldest-to-newest.
    template<typename Fn>
    void
    for_each(
        Fn&& _fn
    ) const
    {
        for (const auto& m : m_history)
        {
            _fn(m);
        }

        return;
    }

private:
    std::vector<any_memento> m_history;
};

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///        VIII. CONVENIENCE FACTORIES (C++14+)                             ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// make_memento
//   function: captures the current state of an object using the given
// snapshot policy and wraps it in a memento.
template<typename SnapshotPolicy = deep_copy_snapshot,
         typename State>
inline auto
make_memento(
    const State& _state
)
    -> memento<typename SnapshotPolicy::template snapshot_type_for<State>>
{
    using snap_t = typename SnapshotPolicy::template snapshot_type_for<State>;

    return memento<snap_t>(
        SnapshotPolicy::capture(_state));
}

// restore_memento
//   function: restores state from a memento using the given policy.
template<typename SnapshotPolicy = deep_copy_snapshot,
         typename State,
         typename Snapshot>
inline void
restore_memento(
    State&                   _state,
    const memento<Snapshot>& _m
)
{
    SnapshotPolicy::restore(_state,
                             _m.state);

    return;
}

// make_caretaker
//   function: factory returning a caretaker with the specified
// snapshot and history policy types.
template<typename Snapshot,
         typename HistoryPolicy = unlimited_history>
inline memento_caretaker<Snapshot, HistoryPolicy>
make_caretaker()
{
    return memento_caretaker<Snapshot, HistoryPolicy>{};
}

// make_undo_redo
//   function: factory returning an undo_redo_stack for a given state
// type and policies.
template<typename State,
         typename SnapshotPolicy = deep_copy_snapshot,
         typename HistoryPolicy   = unlimited_history>
inline undo_redo_stack<State, SnapshotPolicy, HistoryPolicy>
make_undo_redo()
{
    return undo_redo_stack<State, SnapshotPolicy, HistoryPolicy>{};
}

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///       IX.   CONCEPT-CONSTRAINED INTERFACES (C++20+)                    ///
///////////////////////////////////////////////////////////////////////////////

#if D_MEMENTO_HAS_CONCEPTS

// memento_source
//   concept: constrains types that can produce snapshots. Requires
// either the save_state()/restore_state() protocol or copy
// constructibility.
template<typename Type>
concept memento_source =
    ( (requires(const Type& _t) { _t.save_state(); }  &&
       requires(Type& _t, const Type& _o)
       {
           _t.restore_state(_o.save_state());
       }) ||
      std::copy_constructible<Type> );

// memento_target
//   concept: constrains types that can accept restored state via
// copy assignment or a restore_state() method.
template<typename Type>
concept memento_target =
    ( std::is_copy_assignable<Type>::value ||
      requires(Type& _t, const Type& _o)
      {
          _t.restore_state(_o.save_state());
      } );

// snapshot_strategy
//   concept: constrains snapshot policy types. Must provide capture()
// and restore() static methods compatible with State.
template<typename Policy,
         typename State>
concept snapshot_strategy = requires(const State& _cs, State& _s)
{
    { Policy::capture(_cs) };
    { Policy::restore(_s, Policy::capture(_cs)) };
};

// history_policy
//   concept: constrains history eviction policies.
template<typename Policy,
         typename Container,
         typename Snapshot>
concept history_policy = requires(
    const Container& _ch,
    Container&       _h,
    const Snapshot&  _snap)
{
    { Policy::should_push(_ch, _snap) } -> std::convertible_to<bool>;
    { Policy::after_push(_h) };
};

// constrained_checkpoint
//   function: concept-constrained checkpoint creation.
template<typename       Policy = deep_copy_snapshot,
         memento_source State>
requires snapshot_strategy<Policy, State>
inline auto
constrained_checkpoint(
    const State& _state
)
{
    return make_memento<Policy>(_state);
}

#endif  // D_MEMENTO_HAS_CONCEPTS


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARADIGM_MOMENTO_MEMENTO_HPP
