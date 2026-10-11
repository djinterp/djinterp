/* contracts.hpp: the contract rules and the assertion's message, read as C++ */

template<typename Type>
class d_holder
{
public:
    /**
     * @brief Negative: a constructor returns nothing to describe.
     *
     * @param[in] _value  the value held.
     */
    explicit d_holder(const Type& _value);
    /**
     * @brief Negative: nor does a destructor.
     */
    ~d_holder();
    /**
     * @brief Positive: a value is returned and never described.
     *
     * @param[in] _other  the holder compared with.
     */
    bool same(const d_holder<Type>& _other) const;
    /**
     * @brief Negative: a result is a value, whatever it holds.
     *
     * @return the outcome.
     */
    result<void> run();
    /**
     * @brief Positive: a void function that describes a return.
     *
     * @return none.
     */
    void reset();
    /**
     * @brief Positive: `_second` has no @param, and the comma in the first
     *        parameter's type ends no parameter.
     *
     * @param[in] _first  the pair.
     */
    void take(const std::pair<int, Type>& _first,
              int                         _second);

    D_STATIC_ASSERT(sizeof(Type) > 0, "Type is complete");
    D_STATIC_ASSERT(sizeof(Type) > 0);

private:
    Type m_value;
};

/**
 * @brief Negative: a constructor defined outside its class.
 *
 * @param[in] _value  the value held.
 */
template<typename Type>
d_holder<Type>::d_holder(const Type& _value)
    : m_value(_value)
{}
