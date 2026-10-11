/* sibling_in_a_conditional_left_behind: the case inside the #ifdef was not
   measured, and stayed where the others had stood */

int
d_sibling_left_behind(
    int _kind
)
{
    int result = 0;

    switch (_kind)
    {
    case 1:
        result = 1;

        break;

#ifdef D_SIBLING_SECOND
    case 2:
        result = 2;

        break;
#endif

    default:
        break;
    }

    return result;
}
