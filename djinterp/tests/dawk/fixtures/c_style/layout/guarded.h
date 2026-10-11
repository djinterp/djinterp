/* guarded.h: layout under an include guard, and under a conditional */
#ifndef D_LAYOUT_GUARDED_FIXTURE
#define D_LAYOUT_GUARDED_FIXTURE 1

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// d_guarded
//   struct: positive -- a member out of its column, under the guard alone.
struct d_guarded
{
      int member;
};

#if D_LAYOUT_SOMETHING
// d_conditional
//   struct: negative -- the same member, inside a conditional.
struct d_conditional
{
      int member;
};
#endif

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // D_LAYOUT_GUARDED_FIXTURE
