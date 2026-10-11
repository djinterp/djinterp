/* contracts.h: Function Comments -- the contract block */

/**
 * @brief Negative: every parameter, each with its direction.
 *
 * @param[in]  _name   the name.
 * @param[out] _value  the value found.
 * @return `true` if the name was found.
 */
bool   d_contract_find(const char* _name,
                       int*        _value);
/**
 * @brief Positive: `_value` has no @param.
 *
 * @param[in] _name  the name.
 * @return `true` if the name was found.
 */
bool   d_contract_missing(const char* _name,
                          int*        _value);
/**
 * @brief Positive: a @param with no direction.
 *
 * @param _name  the name.
 * @return its length.
 */
size_t d_contract_undirected(const char* _name);
/**
 * @brief Positive twice: `_nmae` is no parameter, and `_name` has none.
 *
 * @param[in] _nmae  the name.
 * @return its length.
 */
size_t d_contract_stranger(const char* _name);
/**
 * @brief Positive: a value is returned and never described.
 *
 * @param[in] _name  the name.
 */
size_t d_contract_silent(const char* _name);
/**
 * @brief Negative: a void function has no return to describe.
 *
 * @param[in] _name  the name; may be `NULL`.
 */
void   d_contract_free(char* _name);
/**
 * @brief Positive: a void function that describes one all the same.
 *
 * @param[in] _name  the name; may be `NULL`.
 * @return none.
 */
void   d_contract_release(char* _name);
/**
 * @brief Negative: a function-pointer parameter answers to its own name,
 *        and a variadic one has none to document.
 *
 * @param[in] _compare  the ordering.
 * @return 0 on success.
 */
int    d_contract_sort(int (*_compare)(const void* _a, const void* _b),
                       ...);

// a group comment -- negative: no contract, so nothing is asked of it
int    d_contract_plain(int _a);
