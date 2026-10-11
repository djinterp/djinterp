/* levels.hpp: the level is env's to say */

#if __cplusplus >= 201103L
int d_modern;
#elif __cplusplus > 199711L
int d_between;
#endif

#ifdef __cplusplus
extern "C" {
#endif

int d_plain;

#if defined(__cplusplus)
}
#endif

#if defined __cplusplus
int d_spaced;
#endif

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
int d_tested;
#endif
