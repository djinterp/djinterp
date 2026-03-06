#include "../../../inc/c/test/dtest"


/******************************************************************************
 * `d_test_type` FUNCTIONS
 *****************************************************************************/

/*
d_test_type_new
  Creates a new d_test_type node with the specified type and element.

Parameter(s):
  _type:    the type discriminator for the node
  _element: pointer to the element to store in the union (must match _type)
Return:
  A pointer to the newly allocated d_test_type, or NULL if allocation failed.
*/
struct d_test_type*
d_test_type_new
(
    enum DTestTypeFlag _type,
    void*              _element
)
{
    struct d_test_type* result;

    result = (struct d_test_type*)malloc(sizeof(struct d_test_type));

    if (!result)
    {
        return NULL;
    }

    result->type   = _type;
    result->config = NULL;

    // assign element to appropriate union member based on type
    switch (_type)
    {
        case D_TEST_TYPE_TEST_FN:
            result->D_KEYWORD_TEST_TEST_FN = (struct d_test_fn*)_element;
            break;

        case D_TEST_TYPE_ASSERT:
            result->D_KEYWORD_TEST_ASSERTION = (struct d_assertion*)_element;
            break;

        case D_TEST_TYPE_TEST:
            result->D_KEYWORD_TEST_TEST = (struct d_test*)_element;
            break;

        case D_TEST_TYPE_TEST_BLOCK:
            result->D_KEYWORD_TEST_BLOCK = (struct d_test_block*)_element;
            break;

        case D_TEST_TYPE_MODULE:
            result->D_KEYWORD_TEST_MODULE = (struct d_test_module*)_element;
            break;

        case D_TEST_TYPE_UNKNOWN:
        default:
            result->D_KEYWORD_TEST_TEST_FN = NULL;
            break;
    }

    return result;
}

/*
d_test_type_new_with_config
  Creates a new d_test_type node with the specified type, element, and config.

Parameter(s):
  _type:    the type discriminator for the node
  _element: pointer to the element to store in the union (must match _type)
  _config:  configuration for the node
Return:
  A pointer to the newly allocated d_test_type, or NULL if allocation failed.
*/
struct d_test_type*
d_test_type_new_with_config
(
    enum DTestTypeFlag    _type,
    void*                 _element,
    struct d_test_config* _config
)
{
    struct d_test_type* result;

    result = d_test_type_new(_type, _element);

    if (result)
    {
        result->config = _config;
    }

    return result;
}

/*
d_test_type_free
  Frees the memory associated with a d_test_type node.
Note: this does NOT free the contained element; only the wrapper itself.

Parameter(s):
  _test_type: the d_test_type to free
Return:
  none
*/
void
d_test_type_free
(
    struct d_test_type* _test_type
)
{
    if (_test_type)
    {
        free(_test_type);
    }

    return;
}


/******************************************************************************
 * TYPE CONVERSION FUNCTIONS
 *****************************************************************************/

/*
d_test_type_to_string
  Converts a test tree node type enum to its string representation.

Parameter(s):
  _type: the type to convert
Return:
  String representation of the type.
*/
const char*
d_test_type_to_string
(
    enum DTestTypeFlag _type
)
{
    switch (_type)
    {
        case D_TEST_TYPE_TEST_FN:
            return "TEST_FN";

        case D_TEST_TYPE_ASSERT:
            return "ASSERTION";

        case D_TEST_TYPE_TEST:
            return "TEST";

        case D_TEST_TYPE_TEST_BLOCK:
            return "TEST_BLOCK";

        case D_TEST_TYPE_MODULE:
            return "MODULE";

        case D_TEST_TYPE_UNKNOWN:
        default:
            return "UNKNOWN";
    }
}