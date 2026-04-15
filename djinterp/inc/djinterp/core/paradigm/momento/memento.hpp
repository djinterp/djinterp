/******************************************************************************
* djinterp [core]                                                  memento.hpp
*
* Memento Pattern Module:
*   Provides a comprehensive, container-agnostic, version-portable foundation
* for the memento pattern. Supports multiple snapshot strategies, undo/redo
* history management, and compile-time capability detection — all without
* coupling to any specific data structure.
*
*   DESIGN:
*   The module is organized in three layers:
*     1. TRAITS — SFINAE-based detection of memento-related capabilities
*        (save/restore, clone, serialize, diff) on arbitrary types.
*     2. CORE — abstract and CRTP base classes that define the memento
*        protocol: originator (state owner), memento (snapshot), and
*        caretaker (history manager).
*     3. POLICIES — pluggable snapshot strategies (deep copy, serialized,
*        delta/diff, external) and history policies (unlimited, bounded,
*        coalescing) that compose with the core via template parameters.
*
*   PORTABILITY:
*   - C++11  : core memento protocol, snapshot strategies, history stack,
*              SFINAE capability traits, caretaker, type-erased memento
*              (via restd::any — RTTI-free, constexpr-capable)
*   - C++14  : generic lambda support in for_each_memento, make_caretaker
*   - C++17  : std::optional integration, string_view tags, if constexpr
*              dispatch, structured bindings
*   - C++20  : concept-constrained originator/memento/caretaker,
*              std::span for external buffer snapshots
*
*
* path:      /inc/djinterp/paradigm/momento/memento.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.09
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CONFIGURATION & FEATURE GATES
      --------------------------------
      i.    D_MEMENTO_HAS_OPTIONAL
      ii.   D_MEMENTO_HAS_CONCEPTS
      iii.  D_MEMENTO_HAS_SPAN
      iv.   D_MEMENTO_DEFAULT_HISTORY_CAPACITY

II.   CAPABILITY TRAITS
      -------------------
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
      --------------------------------
      i.    deep_copy_snapshot
      ii.   clone_snapshot
      iii.  serialized_snapshot
      iv.   delta_snapshot (internal: delta_record)
      v.    external_snapshot

IV.   HISTORY POLICIES
      ------------------
      i.    unlimited_history
      ii.   bounded_history
      iii.  coalescing_history

V.    MEMENTO CORE
      ---------------
      i.    memento (snapshot wrapper)
      ii.   memento_metadata
      iii.  memento_originator (CRTP)
      iv.   memento_caretaker

VI.   UNDO / REDO STACK
      --------------------
      i.    undo_redo_stack

VII.  TYPE-ERASED MEMENTO (C++11+, via restd::any)
      ------------------------------------------------
      i.    any_memento
      ii.   any_memento_caretaker

VIII. CONVENIENCE FACTORIES (C++14+)
      ---------------------------------
      i.    make_memento
      ii.   make_caretaker

IX.   CONCEPT-CONSTRAINED INTERFACES (C++20+)
      ------------------------------------------
      i.    memento_source (concept)
      ii.   memento_target (concept)
      iii.  snapshot_strategy (concept)
      iv.   history_policy (concept)
*/

#ifndef DJINTERP_PARADIGM_MEMENTO_
#define DJINTERP_PARADIGM_MEMENTO_ 1

#include <cstddef>
#include <type_traits>
#include <vector>
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../../../re_std/any/any.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <utility>
    #include <functional>
    #include <chrono>
    #include <memory>
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <optional>
    #include <string_view>
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #include <span>
    #include <concepts>
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
    //   trait: detects _Type::save_state() returning a snapshot object.
    template<typename _Type,
             typename = void>
    struct has_save_state_method : std::false_type
    {};

    template<typename _Type>
    struct has_save_state_method<_Type, void_t<
        decltype(std::declval<const _Type>().save_state())
    >> : std::true_type
    {};

    // has_restore_state_method
    //   trait: detects _Type::restore_state(snapshot) accepting the type
    // returned by save_state().
    template<typename _Type,
             typename = void>
    struct has_restore_state_method : std::false_type
    {};

    template<typename _Type>
    struct has_restore_state_method<_Type, void_t<
        decltype(std::declval<_Type>().restore_state(
            std::declval<const _Type>().save_state()))
    >> : std::true_type
    {};

    // has_clone_method
    //   trait: detects _Type::clone() returning a copy of the object.
    template<typename _Type,
             typename = void>
    struct has_clone_method : std::false_type
    {};

    template<typename _Type>
    struct has_clone_method<_Type, void_t<
        decltype(std::declval<const _Type>().clone())
    >> : std::true_type
    {};

    // has_serialize_method
    //   trait: detects _Type::serialize() returning a byte-like sequence.
    template<typename _Type,
             typename = void>
    struct has_serialize_method : std::false_type
    {};

    template<typename _Type>
    struct has_serialize_method<_Type, void_t<
        decltype(std::declval<const _Type>().serialize())
    >> : std::true_type
    {};

    // has_deserialize_method
    //   trait: detects a static _Type::deserialize(...) factory or a member
    // deserialize() accepting the output of serialize().
    template<typename _Type,
             typename = void>
    struct has_deserialize_method : std::false_type
    {};

    template<typename _Type>
    struct has_deserialize_method<_Type, void_t<
        decltype(std::declval<_Type>().deserialize(
            std::declval<const _Type>().serialize()))
    >> : std::true_type
    {};

    // has_diff_method
    //   trait: detects _Type::diff(other) producing a delta between two
    // states.
    template<typename _Type,
             typename = void>
    struct has_diff_method : std::false_type
    {};

    template<typename _Type>
    struct has_diff_method<_Type, void_t<
        decltype(std::declval<const _Type>().diff(
            std::declval<const _Type>()))
    >> : std::true_type
    {};

    // has_apply_diff_method
    //   trait: detects _Type::apply_diff(delta) to reconstruct state from
    // a delta.
    template<typename _Type,
             typename = void>
    struct has_apply_diff_method : std::false_type
    {};

    template<typename _Type>
    struct has_apply_diff_method<_Type, void_t<
        decltype(std::declval<_Type>().apply_diff(
            std::declval<const _Type>().diff(std::declval<const _Type>())))
    >> : std::true_type
    {};

    // has_equality_operator
    //   trait: detects operator==(const _Type&, const _Type&).
    template<typename _Type,
             typename = void>
    struct has_equality_operator : std::false_type
    {};

    template<typename _Type>
    struct has_equality_operator<_Type, void_t<
        decltype(std::declval<const _Type>() == std::declval<const _Type>())
    >> : std::true_type
    {};

NS_END  // internal

// =========================================================================
// Public trait accessors
// =========================================================================

// is_memento_capable
//   trait: true if _Type has both save_state() and restore_state().
template<typename _Type>
struct is_memento_capable
{
    static constexpr bool value =
        ( internal::has_save_state_method<_Type>::value &&
          internal::has_restore_state_method<_Type>::value );
};

// is_copyable_memento_capable
//   trait: true if _Type is copy-constructible (enabling deep-copy snapshots).
template<typename _Type>
struct is_copyable_memento_capable
{
    static constexpr bool value = std::is_copy_constructible<_Type>::value;
};

// is_clonable_memento_capable
//   trait: true if _Type provides clone().
template<typename _Type>
struct is_clonable_memento_capable
{
    static constexpr bool value = internal::has_clone_method<_Type>::value;
};

// is_serializable_memento_capable
//   trait: true if _Type provides serialize() and deserialize().
template<typename _Type>
struct is_serializable_memento_capable
{
    static constexpr bool value =
        ( internal::has_serialize_method<_Type>::value &&
          internal::has_deserialize_method<_Type>::value );
};

// is_diff_memento_capable
//   trait: true if _Type provides diff() and apply_diff().
template<typename _Type>
struct is_diff_memento_capable
{
    static constexpr bool value =
        ( internal::has_diff_method<_Type>::value &&
          internal::has_apply_diff_method<_Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    constexpr bool is_memento_capable_v =
        is_memento_capable<_Type>::value;

    template<typename _Type>
    constexpr bool is_copyable_memento_capable_v =
        is_copyable_memento_capable<_Type>::value;

    template<typename _Type>
    constexpr bool is_clonable_memento_capable_v =
        is_clonable_memento_capable<_Type>::value;

    template<typename _Type>
    constexpr bool is_serializable_memento_capable_v =
        is_serializable_memento_capable<_Type>::value;

    template<typename _Type>
    constexpr bool is_diff_memento_capable_v =
        is_diff_memento_capable<_Type>::value;

#endif

// memento_capability
//   struct: aggregate classification of a type's memento support.
template<typename _Type>
struct memento_capability
{
    static constexpr bool has_save_restore =
        is_memento_capable<_Type>::value;

    static constexpr bool has_copy =
        is_copyable_memento_capable<_Type>::value;

    static constexpr bool has_clone =
        is_clonable_memento_capable<_Type>::value;

    static constexpr bool has_serialization =
        is_serializable_memento_capable<_Type>::value;

    static constexpr bool has_diff =
        is_diff_memento_capable<_Type>::value;

    static constexpr bool has_equality =
        internal::has_equality_operator<_Type>::value;

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
//   static auto capture(const _State& s) -> snapshot_type;
//   static void restore(_State& s, const snapshot_type& snap);
//
// The caretaker and undo_redo_stack are parameterized on these policies.

// =========================================================================
// deep_copy_snapshot
// =========================================================================

// deep_copy_snapshot
//   policy: captures state by copy construction and restores by copy
// assignment. The simplest strategy; requires _State to be copyable.
struct deep_copy_snapshot
{
    // snapshot_type_for
    //   type: for a given state type, the snapshot is a plain copy.
    template<typename _State>
    using snapshot_type_for = _State;

    template<typename _State>
    static _State
    capture(
        const _State& _state
    )
    {
        return _state;
    }

    template<typename _State>
    static void
    restore(
        _State&       _state,
        const _State& _snapshot
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
//   policy: captures state via _State::clone() and restores by copy
// assignment. For types where copy construction is disabled but a
// virtual or explicit clone is provided.
struct clone_snapshot
{
    template<typename _State>
    using snapshot_type_for = decltype(std::declval<const _State>().clone());

    template<typename _State>
    static auto
    capture(
        const _State& _state
    ) -> decltype(_state.clone())
    {
        return _state.clone();
    }

    template<typename _State>
    static void
    restore(
        _State&                                               _state,
        const decltype(std::declval<const _State>().clone())& _snapshot
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
//   policy: captures state via _State::serialize() and restores via
// _State::deserialize(). Snapshots are stored in the serialized form
// (typically std::vector<char> or std::string), enabling compact
// storage and potential persistence.
struct serialized_snapshot
{
    template<typename _State>
    using snapshot_type_for =
        decltype(std::declval<const _State>().serialize());

    template<typename _State>
    static auto
    capture(
        const _State& _state
    ) -> decltype(_state.serialize())
    {
        return _state.serialize();
    }

    template<typename _State>
    static void
    restore(
        _State& _state,
        const decltype(std::declval<const _State>().serialize())& _snapshot
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
    template<typename _DiffType>
    struct delta_record
    {
        _DiffType diff;
        bool      is_baseline;

        delta_record()
            : diff(),
              is_baseline(false)
        {}

        explicit delta_record(
            _DiffType _d,
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
// delta (via _State::diff()). Restoration walks deltas back to the
// nearest baseline.
//
// Note: this policy is stateful at the strategy level — the caretaker
// must store the previous state for computing diffs. The policy itself
// provides the diff/apply_diff wrappers.
struct delta_snapshot
{
    template<typename _State>
    using diff_type = decltype(
        std::declval<const _State>().diff(std::declval<const _State>()));

    template<typename _State>
    using snapshot_type_for = internal::delta_record<diff_type<_State>>;

    // capture_baseline
    //   function: captures a full-state diff acting as a baseline.
    template<typename _State>
    static snapshot_type_for<_State>
    capture_baseline(
        const _State& _state
    )
    {
        // baseline: diff against a default-constructed state
        return snapshot_type_for<_State>(
            _state.diff(_State{}),
            true);
    }

    // capture_delta
    //   function: captures the diff between _previous and _current.
    template<typename _State>
    static snapshot_type_for<_State>
    capture_delta(
        const _State& _previous,
        const _State& _current
    )
    {
        return snapshot_type_for<_State>(
            _current.diff(_previous),
            false);
    }

    // restore_from_baseline
    //   function: restores state from a baseline delta.
    template<typename _State>
    static void
    restore_from_baseline(
        _State&                          _state,
        const snapshot_type_for<_State>& _record
    )
    {
        _State base{};
        base.apply_diff(_record.diff);
        _state = std::move(base);

        return;
    }

    // apply_delta
    //   function: applies a non-baseline delta to the current state.
    template<typename _State>
    static void
    apply_delta(
        _State&                          _state,
        const snapshot_type_for<_State>& _record
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
template<typename _CaptureCallable,
         typename _RestoreCallable>
struct external_snapshot
{
    _CaptureCallable capture_fn;
    _RestoreCallable restore_fn;

    template<typename _State>
    using snapshot_type_for =
        decltype(std::declval<_CaptureCallable>()(
            std::declval<const _State&>()));

    template<typename _State>
    auto
    capture(
        const _State& _state
    ) const -> snapshot_type_for<_State>
    {
        return capture_fn(_state);
    }

    template<typename _State,
             typename _Snapshot>
    void
    restore(
        _State&          _state,
        const _Snapshot& _snapshot
    ) const
    {
        restore_fn(_state, _snapshot);

        return;
    }
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

    // make_external_snapshot
    //   function: factory for external_snapshot policies.
    template<typename _CaptureFn,
             typename _RestoreFn>
    D_CONSTEXPR_INLINE auto
    make_external_snapshot(
        _CaptureFn&& _capture,
        _RestoreFn&& _restore
    )
        -> external_snapshot<std::decay_t<_CaptureFn>,
                             std::decay_t<_RestoreFn>>
    {
        return { std::forward<_CaptureFn>(_capture),
                 std::forward<_RestoreFn>(_restore) };
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
    template<typename _Container,
             typename _Snapshot>
    static bool
    should_push(
        const _Container& /* _history */,
        const _Snapshot&  /* _snapshot */
    )
    {
        return true;
    }

    template<typename _Container>
    static void
    after_push(
        _Container& /* _history */
    )
    {
        return;
    }
};

// bounded_history
//   policy: retains at most _MaxSize snapshots, evicting the oldest
// when the limit is reached.
template<std::size_t _MaxSize = D_MEMENTO_DEFAULT_HISTORY_CAPACITY>
struct bounded_history
{
    static constexpr std::size_t max_size = _MaxSize;

    template<typename _Container,
             typename _Snapshot>
    static bool
    should_push(
        const _Container& /* _history */,
        const _Snapshot&  /* _snapshot */
    )
    {
        return true;
    }

    template<typename _Container>
    static void
    after_push(
        _Container& _history
    )
    {
        while (_history.size() > _MaxSize)
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
    template<typename _Container,
             typename _Snapshot>
    static bool
    should_push(
        const _Container& _history,
        const _Snapshot&  _snapshot
    )
    {
        if (_history.empty())
        {
            return true;
        }

        return !(_history.back().state == _snapshot);
    }

    template<typename _Container>
    static void
    after_push(
        _Container& /* _history */
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
template<typename _Snapshot>
struct memento
{
    using snapshot_type = _Snapshot;

    _Snapshot        state;
    memento_metadata meta;

    memento()
        : state(),
          meta()
    {}

    explicit memento(
        _Snapshot        _s,
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
// owning class. _Derived is the concrete type; _SnapshotPolicy is the
// strategy used to capture and restore snapshots.
//
// _Derived must be accessible via static_cast from this base; it
// represents the complete state object.
//
// Usage:
//   class editor_state
//       : public memento_originator<editor_state, deep_copy_snapshot>
//   {
//       std::string text;
//   public:
//       // deep_copy_snapshot uses copy ctor/assignment — nothing
//       // extra needed.
//   };
//
//   editor_state state;
//   auto snap = state.create_memento();
//   // ... mutate state ...
//   state.restore_memento(snap);
template<typename _Derived,
         typename _SnapshotPolicy = deep_copy_snapshot>
class memento_originator
{
public:
    using snapshot_policy = _SnapshotPolicy;
    using snapshot_type   = typename _SnapshotPolicy::template
                                snapshot_type_for<_Derived>;
    using memento_type    = memento<snapshot_type>;

    // create_memento
    //   function: captures the current state as a memento.
    memento_type
    create_memento() const
    {
        const _Derived& self = static_cast<const _Derived&>(*this);
        snapshot_type snap    = _SnapshotPolicy::capture(self);

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
        const _Derived& self = static_cast<const _Derived&>(*this);
        snapshot_type snap    = _SnapshotPolicy::capture(self);

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
        _Derived& self = static_cast<_Derived&>(*this);
        _SnapshotPolicy::restore(self,
                                 _memento.state);

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
template<typename _Snapshot,
         typename _HistoryPolicy = unlimited_history>
class memento_caretaker
{
public:
    using snapshot_type  = _Snapshot;
    using memento_type   = memento<_Snapshot>;
    using history_policy = _HistoryPolicy;
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
        if (_HistoryPolicy::should_push(m_history, _m.state))
        {
            m_history.push_back(std::move(_m));
            _HistoryPolicy::after_push(m_history);
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
    template<typename _Fn>
    void
    for_each(
        _Fn&& _fn
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
    template<typename _Fn>
    void
    for_each_reverse(
        _Fn&& _fn
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
template<typename _State,
         typename _SnapshotPolicy = deep_copy_snapshot,
         typename _HistoryPolicy  = unlimited_history>
class undo_redo_stack
{
public:
    using snapshot_type = typename _SnapshotPolicy::template
                              snapshot_type_for<_State>;
    using memento_type  = memento<snapshot_type>;

    // checkpoint
    //   function: captures the current state as an undo point. Clears
    // the redo stack (forward history is invalidated by mutation).
    void
    checkpoint(
        const _State& _state
    )
    {
        memento_type m(
            _SnapshotPolicy::capture(_state),
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
        const _State&    _state,
        std::string_view _tag
    )
    {
        memento_type m(
            _SnapshotPolicy::capture(_state),
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
        _State& _state
    )
    {
        if (m_undo.empty())
        {
            return false;
        }

        // save current state to redo before restoring
        memento_type redo_point(
            _SnapshotPolicy::capture(_state),
            memento_metadata(m_sequence++));
        m_redo.push(std::move(redo_point));

        // restore from undo
        memento_type prev = m_undo.pop();
        _SnapshotPolicy::restore(_state,
                                 prev.state);

        return true;
    }

    // redo
    //   function: restores the most recent redo snapshot into _state,
    // pushing the current state onto the undo stack. Returns true if
    // a redo was performed, false if the redo stack was empty.
    bool
    redo(
        _State& _state
    )
    {
        if (m_redo.empty())
        {
            return false;
        }

        // save current state to undo before restoring
        memento_type undo_point(
            _SnapshotPolicy::capture(_state),
            memento_metadata(m_sequence++));
        m_undo.push(std::move(undo_point));

        // restore from redo
        memento_type next = m_redo.pop();
        _SnapshotPolicy::restore(_state,
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
    memento_caretaker<snapshot_type, _HistoryPolicy> m_undo;
    memento_caretaker<snapshot_type, _HistoryPolicy> m_redo;
    std::size_t m_sequence = 0;
};


///////////////////////////////////////////////////////////////////////////////
///         VII.  TYPE-ERASED MEMENTO (C++11+, via restd::any)             ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// any_memento
//   class: type-erased memento that can store any snapshot type via
// restd::any. Useful when a caretaker must manage heterogeneous
// state objects (e.g., a multi-document editor where each document
// type has a different snapshot representation).
//
// Uses restd::any rather than std::any, making this available from
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
    using any_type    = restd::any;
    using any_id_type = restd::any_type_id;

public:
    any_memento() = default;

    template<typename _Snapshot>
    explicit any_memento(
            _Snapshot        _snap,
            memento_metadata _meta = memento_metadata()
        )
            : m_state(std::move(_snap)),
              m_meta(_meta)
        {}

    // has_value
    //   function: true if a snapshot is stored.
    D_CONSTEXPR bool
    has_value() const noexcept
    {
        return m_state.has_value();
    }

    // holds
    //   function: true if the stored snapshot was originally of type
    // _Snapshot. RTTI-free; uses any's function-pointer type identity.
    //
    // Note: no .template disambiguator — any_memento is not a class
    // template, so m_state's type is not dependent.
    template<typename _Snapshot>
    D_CONSTEXPR bool
    holds() const noexcept
    {
        return m_state.holds<_Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (const, SBO types — bool)
    // -----------------------------------------------------------------

    template<typename _Snapshot,
             typename std::enable_if<
                 std::is_same<_Snapshot, bool>::value,
                 int
             >::type = 0>
    D_CONSTEXPR _Snapshot
    get() const noexcept
    {
        return m_state.get<_Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (const, SBO types — signed integral, not bool)
    // -----------------------------------------------------------------

    template<typename _Snapshot,
             typename std::enable_if<
                 ( std::is_integral<_Snapshot>::value &&
                   std::is_signed<_Snapshot>::value   &&
                   !std::is_same<_Snapshot, bool>::value ),
                 int
             >::type = 0>
    D_CONSTEXPR _Snapshot
    get() const noexcept
    {
        return m_state.get<_Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (const, SBO types — unsigned integral, not bool)
    // -----------------------------------------------------------------

    template<typename _Snapshot,
             typename std::enable_if<
                 ( std::is_integral<_Snapshot>::value  &&
                   std::is_unsigned<_Snapshot>::value  &&
                   !std::is_same<_Snapshot, bool>::value ),
                 int
             >::type = 0>
    D_CONSTEXPR _Snapshot
    get() const noexcept
    {
        return m_state.get<_Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (const, SBO types — floating point)
    // -----------------------------------------------------------------

    template<typename _Snapshot,
             typename std::enable_if<
                 std::is_floating_point<_Snapshot>::value,
                 int
             >::type = 0>
    D_CONSTEXPR _Snapshot
    get() const noexcept
    {
        return m_state.get<_Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (const, SBO types — enum)
    // -----------------------------------------------------------------

    template<typename _Snapshot,
             typename std::enable_if<
                 std::is_enum<_Snapshot>::value,
                 int
             >::type = 0>
    D_CONSTEXPR _Snapshot
    get() const noexcept
    {
        return m_state.get<_Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (const, SBO types — pointer)
    // -----------------------------------------------------------------

    template<typename _Snapshot,
             typename std::enable_if<
                 std::is_pointer<_Snapshot>::value,
                 int
             >::type = 0>
    D_CONSTEXPR _Snapshot
    get() const noexcept
    {
        return m_state.get<_Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (const, heap types)
    // -----------------------------------------------------------------

    template<typename _Snapshot,
             typename std::enable_if<
                 ( !std::is_integral<_Snapshot>::value       &&
                   !std::is_floating_point<_Snapshot>::value &&
                   !std::is_enum<_Snapshot>::value           &&
                   !std::is_pointer<_Snapshot>::value ),
                 int
             >::type = 0>
    const _Snapshot&
    get() const
    {
        return m_state.get<_Snapshot>();
    }

    // -----------------------------------------------------------------
    // get (mutable, heap types only)
    // -----------------------------------------------------------------

    template<typename _Snapshot,
             typename std::enable_if<
                 ( !std::is_integral<_Snapshot>::value       &&
                   !std::is_floating_point<_Snapshot>::value &&
                   !std::is_enum<_Snapshot>::value           &&
                   !std::is_pointer<_Snapshot>::value ),
                 int
             >::type = 0>
    _Snapshot&
    get()
    {
        return m_state.get<_Snapshot>();
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
    D_CONSTEXPR any_id_type
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
template<typename _HistoryPolicy = unlimited_history>
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
        _HistoryPolicy::after_push(m_history);

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
    template<typename _Fn>
    void
    for_each(
        _Fn&& _fn
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
template<typename _SnapshotPolicy = deep_copy_snapshot,
         typename _State>
inline auto
make_memento(
    const _State& _state
)
    -> memento<typename _SnapshotPolicy::template snapshot_type_for<_State>>
{
    using snap_t = typename _SnapshotPolicy::template snapshot_type_for<_State>;

    return memento<snap_t>(
        _SnapshotPolicy::capture(_state));
}

// restore_memento
//   function: restores state from a memento using the given policy.
template<typename _SnapshotPolicy = deep_copy_snapshot,
         typename _State,
         typename _Snapshot>
inline void
restore_memento(
    _State&                   _state,
    const memento<_Snapshot>& _m
)
{
    _SnapshotPolicy::restore(_state,
                             _m.state);

    return;
}

// make_caretaker
//   function: factory returning a caretaker with the specified
// snapshot and history policy types.
template<typename _Snapshot,
         typename _HistoryPolicy = unlimited_history>
inline memento_caretaker<_Snapshot, _HistoryPolicy>
make_caretaker()
{
    return memento_caretaker<_Snapshot, _HistoryPolicy>{};
}

// make_undo_redo
//   function: factory returning an undo_redo_stack for a given state
// type and policies.
template<typename _State,
         typename _SnapshotPolicy = deep_copy_snapshot,
         typename _HistoryPolicy  = unlimited_history>
inline undo_redo_stack<_State, _SnapshotPolicy, _HistoryPolicy>
make_undo_redo()
{
    return undo_redo_stack<_State, _SnapshotPolicy, _HistoryPolicy>{};
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
template<typename _Type>
concept memento_source =
    ( (requires(const _Type& _t) { _t.save_state(); }  &&
       requires(_Type& _t, const _Type& _o)
       {
           _t.restore_state(_o.save_state());
       }) ||
      std::copy_constructible<_Type> );

// memento_target
//   concept: constrains types that can accept restored state via
// copy assignment or a restore_state() method.
template<typename _Type>
concept memento_target =
    ( std::is_copy_assignable_v<_Type> ||
      requires(_Type& _t, const _Type& _o)
      {
          _t.restore_state(_o.save_state());
      } );

// snapshot_strategy
//   concept: constrains snapshot policy types. Must provide capture()
// and restore() static methods compatible with _State.
template<typename _Policy,
         typename _State>
concept snapshot_strategy = requires(const _State& _cs, _State& _s)
{
    { _Policy::capture(_cs) };
    { _Policy::restore(_s, _Policy::capture(_cs)) };
};

// history_policy
//   concept: constrains history eviction policies.
template<typename _Policy,
         typename _Container,
         typename _Snapshot>
concept history_policy = requires(
    const _Container& _ch,
    _Container&       _h,
    const _Snapshot&  _snap)
{
    { _Policy::should_push(_ch, _snap) } -> std::convertible_to<bool>;
    { _Policy::after_push(_h) };
};

// constrained_checkpoint
//   function: concept-constrained checkpoint creation.
template<typename       _Policy = deep_copy_snapshot,
         memento_source _State>
requires snapshot_strategy<_Policy, _State>
inline auto
constrained_checkpoint(
    const _State& _state
)
{
    return make_memento<_Policy>(_state);
}

#endif  // D_MEMENTO_HAS_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_PARADIGM_MEMENTO_
