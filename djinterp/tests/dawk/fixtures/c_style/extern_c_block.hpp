/* extern_c_block.hpp: an extern "C" block adds a level when it is opened
   outside any conditional, as any braced body does, and none when it is a
   guard opened inside one (H30) */

extern "C"
{
    int d_block_indented(int _a);
    int d_block_indented_too;
}

extern "C"
{
int d_block_flush(int _a);
}

#if D_ENV_LANG_USING_CPP
extern "C" {
#endif

#if defined(D_GUARD_FIXTURE_FEATURE)
int d_guard_featured;
#endif

int d_guard_plain;

#if D_ENV_LANG_USING_CPP
}
#endif
