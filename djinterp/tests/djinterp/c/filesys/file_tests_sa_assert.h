/*******************************************************************************
* djinterp [test]                                         file_tests_sa_assert.h
*
* The standalone framework's object model and assertion macros, as the
* filesys suites use them.
*   test_standalone.h comments out (2026.09.30) the four object-model
* declarations and the D_ASSERT_* macros built on them, as declared but never
* defined. test_standalone.c defines the four functions, and so does the
* standalone driver, test_standalone_min.c, so the suites declare them here,
* with the macros, until the framework's own come back; each macro steps
* aside for a definition already made.
*
* path:      /tests/djinterp/c/filesys/file_tests_sa_assert.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_FILESYS_FILE_TESTS_SA_ASSERT_H
#define DJINTERP_TESTS_C_FILESYS_FILE_TESTS_SA_ASSERT_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // NULL, size_t
// djinterp
// struct d_test_object, d_test_str_equal
#include "../../../../inc/djinterp/test/c/test_standalone.h"

// the object model, defined by test_standalone.c and test_standalone_min.c
struct d_test_object* d_test_object_new_leaf(const char* _name,
                                             const char* _message,
                                             bool        _result);
struct d_test_object* d_test_object_new_interior(const char* _name,
                                                 size_t      _child_count);
void                  d_test_object_free(struct d_test_object* _obj);
void                  d_test_object_add_child(struct d_test_object* _parent,
                                              struct d_test_object* _child,
                                              size_t                _index);

// D_ASSERT_TRUE
//   macro: a leaf asserting that a condition holds.
#ifndef D_ASSERT_TRUE
    #define D_ASSERT_TRUE(_name, _condition, _message)                        \
        d_test_object_new_leaf((_name), (_message), ((_condition) ? 1 : 0))
#endif  // D_ASSERT_TRUE

// D_ASSERT_FALSE
//   macro: a leaf asserting that a condition does not hold.
#ifndef D_ASSERT_FALSE
    #define D_ASSERT_FALSE(_name, _condition, _message)                       \
        d_test_object_new_leaf((_name), (_message), ((_condition) ? 0 : 1))
#endif  // D_ASSERT_FALSE

// D_ASSERT_NULL
//   macro: a leaf asserting that a pointer is NULL.
#ifndef D_ASSERT_NULL
    #define D_ASSERT_NULL(_name, _ptr, _message)                              \
        d_test_object_new_leaf((_name), (_message), ((_ptr) == NULL))
#endif  // D_ASSERT_NULL

// D_ASSERT_NOT_NULL
//   macro: a leaf asserting that a pointer is not NULL.
#ifndef D_ASSERT_NOT_NULL
    #define D_ASSERT_NOT_NULL(_name, _ptr, _message)                          \
        d_test_object_new_leaf((_name), (_message), ((_ptr) != NULL))
#endif  // D_ASSERT_NOT_NULL

// D_ASSERT_EQUAL
//   macro: a leaf asserting that two values compare equal.
#ifndef D_ASSERT_EQUAL
    #define D_ASSERT_EQUAL(_name, _val1, _val2, _message)                     \
        d_test_object_new_leaf((_name), (_message), ((_val1) == (_val2)))
#endif  // D_ASSERT_EQUAL

// D_ASSERT_STR_EQUAL
//   macro: a leaf asserting that two strings are equal, NULL equal to NULL.
#ifndef D_ASSERT_STR_EQUAL
    #define D_ASSERT_STR_EQUAL(_name, _str1, _str2, _message)                 \
        d_test_object_new_leaf((_name),                                       \
                               (_message),                                    \
                               d_test_str_equal((_str1), (_str2)))
#endif  // D_ASSERT_STR_EQUAL


#endif  // DJINTERP_TESTS_C_FILESYS_FILE_TESTS_SA_ASSERT_H
