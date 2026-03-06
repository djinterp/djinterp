#include "../../../inc/c/test/debug.h"


/*
d_debug_print_addr
    Prints a pointer address as a string of bytes on one line.

Parameter(s):
	address: the pointer being printed.
Return:
	none
*/
void 
d_debug_print_addr
(
	const void * const address
)
{
	size_t          index;
	unsigned char * addr;

	if (address)
	{
		addr = (unsigned char *) address;
		index = sizeof(void *) - 1;

		// print first element without space
		printf("%02x", addr[index--]);

		// print the bytes of the function pointer's address
		for (; index != 0; index--)
		{
			printf(" %02x", addr[index]);
		}
	}
	else
	{
		printf(D_DEBUG_NULL_STR);
	}

	return;
}

/*
d_debug_print_bytes


Parameter(s):
	log: the DLogEntry being freed.
Return:
	none
*/
void 
d_debug_print_bytes
(
	const void * const address,
	size_t             size
)
{
	unsigned char * addr;

	if (address)
	{
		addr = (unsigned char *) address;

		// print first element without space
		printf("%02x", addr[size--]);

		// print the bytes of the function pointer's address
		for (; size != 0; size--)
		{
			printf(" %02x", addr[size]);
		}
	}
	else
	{
		printf(D_DEBUG_NULL_STR);
	}

	return;
}

/*
d_debug_print_array


Parameter(s):
	log: the DLogEntry being freed.
Return:
	none
*/
void 
d_debug_print_array
(
	const void ** const arr,
	size_t              count,
	size_t              size_per,
	const char  *       prefix,
	size_t              prefix_length
)
{
	char * element_prefix;
	size_t index;

	if ( (arr) &&
		 (count > 0) )
	{
		if (prefix)
		{
			element_prefix = d_str_copy_append_s(prefix, prefix_length, D_DEBUG_DEFAULT_INDENT_STR, 2);
		}
		else
		{
			prefix = "";
			prefix_length = 0;
			element_prefix = D_DEBUG_DEFAULT_INDENT_STR;
		}

		D_DEBUG_PRINT_LINE(prefix, "[", "");

		for (index = 0; index < count; index++)
		{
			printf("%s", element_prefix);
			d_debug_print_bytes(arr[index], size_per);
		}

		D_DEBUG_PRINT_LINE(prefix, "]", "");

		free(element_prefix);
	}

	return;
}

/*
d_debug_print_array_fn


Parameter(s):
	log: the DLogEntry being freed.
Return:
	none
*/
void 
d_debug_print_array_addr
(
	const void ** const arr,
	size_t              count,
	const char *        key_name,
	const char *        prefix,
	size_t              prefix_length
)
{
	char * element_prefix;
	size_t index;
	
	if ( (arr) &&
		 (count > 0) )
	{
		// set key to "elements" by default if NULL
		key_name = (key_name) ? (key_name) :
			                    ("elements");

		if (prefix)
		{
			element_prefix = d_str_copy_append_s(prefix, prefix_length, D_DEBUG_DEFAULT_INDENT_STR, 2);
		}
		else
		{
			prefix = "";
			prefix_length = 0;
			element_prefix = D_DEBUG_DEFAULT_INDENT_STR;
		}

		D_DEBUG_PRINT_LINE(prefix, "elements:", "");
		D_DEBUG_PRINT_LINE(prefix, "[", "");

		for (index = 0; index < count; )
		{
			D_DEBUG_PRINT(element_prefix, "addr: ", "");
			d_debug_print_addr(arr);
			printf("\n");

			if (++index != count)
			{
				printf(",");
			}
		}

		D_DEBUG_PRINT_LINE(prefix, "]", "");

		free(element_prefix);
	}

	return;
}

/*
d_debug_print_array_fn


Parameter(s):
	log: the DLogEntry being freed.
Return:
	none
*/
void 
d_debug_print_array_fn
(
	const void ** const arr,
	size_t              count,
	const char *        key_name,
	const char *        prefix,
	size_t              prefix_length,
	fn_debug_print      debug_print_fn
)
{
	char * element_prefix;
	size_t element_prefix_length,
	       index;

	if ( (arr) &&
		 (count > 0) )
	{
		// set key to "elements" by default if NULL
		key_name = (key_name) ? (key_name) :
			                    ("elements");

		// if prefix is NULL, set 'prefix' to empty string "" and 'length' to 0
		if (prefix)
		{
			element_prefix = d_str_copy_append_s(prefix, prefix_length, D_DEBUG_DEFAULT_INDENT_STR, 2);
			element_prefix_length = prefix_length + 2;
		}
		else
		{
			prefix = "";
			prefix_length = 0;
			element_prefix = D_DEBUG_DEFAULT_INDENT_STR;
			element_prefix_length = 2;
		}

		D_DEBUG_PRINT_LINE_ARGS(prefix, "%s:", "", key_name);
		D_DEBUG_PRINT_LINE(prefix, "[", "");

		for (index = 0; index < count; index++)
		{
			debug_print_fn( (void *)arr[index], element_prefix, element_prefix_length );
			
			if (index != (count - 1))
			{
				printf(",\n");
			}
			else
			{
				printf("\n");
			}
		}

		D_DEBUG_PRINT_LINE(prefix, "]", "");

		free(element_prefix);
	}

	return;
}



/*
TO DO: delete?
/*
d_debug_print_array


Parameter(s):
	log: the DLogEntry being freed.
Return:
	none

void
d_debug_print_array
(
	const void** const arr,
	size_t              count,
	const char* prefix,
	size_t              prefix_length
)
{
	char* element_prefix;
	size_t index;

	if ((arr) &&
		(count > 0))
	{
		if (prefix)
		{
			element_prefix = d_str_copy_append_s(prefix, prefix_length, D_DEBUG_DEFAULT_INDENT_STR, 2);
		}
		else
		{
			prefix = "";
			prefix_length = 0;
			element_prefix = D_DEBUG_DEFAULT_INDENT_STR;
		}

		D_DEBUG_PRINT_LINE(prefix, "[", "");

		for (index = 0; index < count; index++)
		{
			printf("%s", element_prefix);
			d_debug_print_addr(arr[index]);
		}

		D_DEBUG_PRINT_LINE(prefix, "]", "");

		free(element_prefix);
	}

	return;
}
*/