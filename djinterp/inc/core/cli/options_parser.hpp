/*******************************************************************************
* djinterp [meta]                                                tuple_util.hpp
*
*
* 
* author(s): Samuel 'teer' Neal-Blim
* link:   TBA
* file:   \inc\meta\tuple_util.hpp                             date: 2024.04.25
*******************************************************************************/
#ifndef DJINTERP_TUPLE_UTIL_
#define	DJINTERP_TUPLE_UTIL_ 1

#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include "..\djinterp"


NS_DJINTERP

    // forward declarations
    //template <typename _Key, typename _Value> struct option;
    //template <typename... _Options>           struct option_set;

    // =============================================================================
    // Core Option Types
    // =============================================================================


    template <typename _Key, typename _Value>
    struct option
    { };


    // option
    //   type: 
    template <typename _Key, 
              typename _Value>
    struct option 
    {
        using key_type   = _Key;
        using value_type = _Value;
    
        static constexpr _Key   key{};
        static constexpr _Value value{};
    
        constexpr option() = default;
        constexpr option(key_type _key, _Value _value) : _Key(_key), _Value(_value) 
        {}
    };

    // compile-time string literal for keys (common case)
    template <size_t N>
    struct string_literal 
    {
        constexpr StringLiteral(const char (&str)[N]) 
        {
            std::copy_n(str, N, _Value);
        }
    
        char _Value[N];
        static constexpr size_t size = N - 1; // exclude null terminator
    
        constexpr operator std::string_view() const 
        {
            return std::string_view(_Value, size);
        }
    
        constexpr bool operator==(const StringLiteral& other) const 
        {
            if constexpr (N != other.size + 1) 
            {
                return false;
            }

            for (size_t i = 0; i < size; ++i) 
            {
                if (_Value[i] != other._Value[i]) 
                {
                    return false;
                }
            }

            return true;
        }
    };

    // Helper to create string literal options easily
    template <string_literal _Key, auto _Value>
    using string_option = option<decltype(_Key), decltype(_Value)>;

    // =============================================================================
    // Option Set - Container for multiple options
    // =============================================================================

    template <typename... Options>
    struct option_set 
    {
        static constexpr size_t size = sizeof...(Options);
    
        // Check if an option with given _Key exists
        template <typename _Key>
        static constexpr bool has_option() 
        {
            return (std::is_same_v<_Key, typename Options::key_type> || ...);
        }
    
        // Get _Value for a given _Key (returns default if not found)
        template <typename _Key, 
                  typename _Default = void>
        static constexpr auto get_value() 
        {
            return get_value_impl<_Key, Default, Options...>();
        }
    
    private:
        template <typename _Key, typename _Default, typename First, typename... Rest>
        static constexpr auto get_value_impl() 
        {
            if constexpr (std::is_same_v<_Key, typename First::key_type>) 
            {
                return First::_Value;
            } 
            else if constexpr (sizeof...(Rest) > 0) 
            {
                return get_value_impl<_Key, _Default, Rest...>();
            } 
            else 
            {
                if constexpr (!std::is_same_v<_Default, void>) 
                {
                    return _Default{};
                } 
                else 
                {
                    static_assert(std::is_same_v<_Key, void>, "option not found and no default provided");
                }
            }
        }
    };

    // =============================================================================
    // Tweakable Class Infrastructure
    // =============================================================================

    // Base template for tweakable classes
    template <typename Derived, typename OptionsT = option_set<>>
    struct Tweakable 
    {
        using options_type = OptionsT;
    
        // Allow derived classes to access options
        template <typename _Key, typename Default = void>
        static constexpr auto get_option() 
        {
            return OptionsT::template get_value<_Key, Default>();
        }
    
        template <typename _Key>
        static constexpr bool has_option() {
            return OptionsT::template has_option<_Key>();
        }
    };

    // Macro to help define tweakable classes
    #define DECLARE_TWEAKABLE_CLASS(ClassName, DefaultOptions...) \
        template <typename Options = option_set<DefaultOptions>> \
        class ClassName : public Tweakable<ClassName<Options>, Options>

    // =============================================================================
    // Option Parsing Utilities (getopt-like)
    // =============================================================================

    // Parse options from parameter pack
    template <typename... Args>
    constexpr auto parse_options(Args... args) {
        return option_set<Args...>{};
    }

    // Helper macros for common option patterns
    #define OPT(_Key, _Value) option<decltype(_Key), decltype(_Value)>{_Key, _Value}
    #define STR_OPT(_Key, _Value) StrOption<_Key, _Value>{}

    // =============================================================================
    // Example Usage and Specializations
    // =============================================================================

    // Example tweakable class - a simple container
    DECLARE_TWEAKABLE_CLASS(TweakableVector) {
    private:
        static constexpr size_t DEFAULT_CAPACITY = 16;
        static constexpr bool DEFAULT_BOUNDS_CHECK = true;
    
    public:
        using value_type = int; // For simplicity
    
        // Use options to configure behavior
        static constexpr size_t capacity = 
            Options::template has_option<StringLiteral{"capacity"}>() ? 
            Options::template get_value<StringLiteral{"capacity"}>() : 
            DEFAULT_CAPACITY;
    
        static constexpr bool bounds_check = 
            Options::template has_option<StringLiteral{"bounds_check"}>() ? 
            Options::template get_value<StringLiteral{"bounds_check"}>() : 
            DEFAULT_BOUNDS_CHECK;
    
        // Storage
        value_type data[capacity];
        size_t size_ = 0;
    
        // Methods that use the options
        void push_back(const value_type& val) {
            if constexpr (bounds_check) {
                if (size_ >= capacity) {
                    throw std::out_of_range("TweakableVector capacity exceeded");
                }
            }
            data[size_++] = val;
        }
    
        value_type& operator[](size_t idx) {
            if constexpr (bounds_check) {
                if (idx >= size_) {
                    throw std::out_of_range("TweakableVector index out of bounds");
                }
            }
            return data[idx];
        }
    
        size_t size() const { return size_; }
        constexpr size_t max_size() const { return capacity; }
    };

    // =============================================================================
    // Advanced: Template Specialization Support
    // =============================================================================

    // Helper to detect if a type is an option_set
    template <typename T>
    struct is_option_set : std::false_type {};

    template <typename... Options>
    struct is_option_set<option_set<Options...>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_option_set_v = is_option_set<T>::_Value;

    // Specialization helper - allows classes to provide specialized behavior
    // based on specific option combinations
    template <typename Class, typename Options, typename = void>
    struct ClassSpecializer {
        using type = Class;
    };

    // Example specialization: when capacity is set to a specific _Value
    template <typename Options>
    struct ClassSpecializer<
        TweakableVector<Options>, 
        Options,
        std::enable_if_t<
            Options::template has_option<StringLiteral{"capacity"}>() &&
            Options::template get_value<StringLiteral{"capacity"}>() == 1000
        >
    > {
        // Could define a specialized version here
        using type = TweakableVector<Options>; // For now, same type
    
        // Could add specialized static methods, etc.
        static constexpr bool is_specialized = true;
    };

NS_END	// djinterp

#endif	// DJINTERP_TUPLE_UTIL_