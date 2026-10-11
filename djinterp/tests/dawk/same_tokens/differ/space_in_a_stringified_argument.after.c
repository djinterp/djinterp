#define D_DIFFER_TEXT(x) #x
const char* d_differ_sum(void)
{
    return D_DIFFER_TEXT(1 + 2);
}
