/* extern_c.h: the linkage form the converted env headers use */
#ifndef D_EXTERN_C_FIXTURE
#define D_EXTERN_C_FIXTURE 1

#if D_ENV_LANG_USING_CPP
extern "C" {
#endif

int d_extern_c_fixture(int _a);

#if D_ENV_LANG_USING_CPP
}
#endif


#endif  // D_EXTERN_C_FIXTURE
