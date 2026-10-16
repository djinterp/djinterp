/*******************************************************************************
* djinterp [functional]                                          test_sequence.c
*
*
* path:      /tests/djinterp/c/functional/test_sequence.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// djinterp
static size_t g_ck=0,g_ok=0;
static void ck(bool c,const char*l){g_ck++;if(c)g_ok++;else printf("    FAIL  %s\n",l);}
static bool lt(const void*a,const void*b,void*x){(void)x;return *(const intmax_t*)a < *(const intmax_t*)b;}
static bool eq(const void*a,const void*b,void*x){(void)x;return *(const intmax_t*)a == *(const intmax_t*)b;}
static bool even(const void*e,void*x){(void)x;return (*(const intmax_t*)e%2)==0;}
static int cmp3(const void*a,const void*b,void*x){(void)x;intmax_t l=*(const intmax_t*)a,r=*(const intmax_t*)b;return (l<r)?-1:((l>r)?1:0);}
static bool add(const void*a,const void*b,void*o,void*x){(void)x;*(intmax_t*)o=*(const intmax_t*)a+*(const intmax_t*)b;return true;}
static size_t dup2(const void*e,void*out,size_t cap,void*x){(void)x;if(cap<2)return 0;
  ((intmax_t*)out)[0]=*(const intmax_t*)e;((intmax_t*)out)[1]=*(const intmax_t*)e;return 2;}
int main(void){
  intmax_t a[8],b[8],o[32],seen[16],scr[8];size_t n,i;
  printf("djinterp [functional] sequence conformance (tier 1 completion)\n");
  /* reverse */
  for(i=0;i<5;i++)a[i]=(intmax_t)i+1;
  d_reverse(a,5,sizeof(intmax_t));
  ck(a[0]==5&&a[4]==1,"reverse in place");
  d_reverse_into(a,5,sizeof(intmax_t),o);
  ck(o[0]==1&&o[4]==5,"reverse_into leaves the source alone");
  ck(a[0]==5,"reverse_into does not disturb the source");
  ck(d_reverse(a,0,sizeof(intmax_t)),"reverse of empty succeeds");
  /* even length: an odd-length array hides an off-by-one at the midpoint */
  for(i=0;i<4;i++)a[i]=(intmax_t)i+1;
  d_reverse(a,4,sizeof(intmax_t));
  ck(a[0]==4&&a[1]==3&&a[2]==2&&a[3]==1,"reverse of an even-length array");
  for(i=0;i<2;i++)a[i]=(intmax_t)i+1;
  d_reverse(a,2,sizeof(intmax_t));
  ck(a[0]==2&&a[1]==1,"reverse of an adjacent pair");
  /* sorted */
  a[0]=5;a[1]=1;a[2]=4;a[3]=1;a[4]=3;a[5]=9;a[6]=2;a[7]=6;
  ck(!d_is_sorted(a,8,sizeof(intmax_t),lt,NULL),"unsorted reads unsorted");
  d_sorted(a,8,sizeof(intmax_t),lt,NULL);
  ck(d_is_sorted(a,8,sizeof(intmax_t),lt,NULL),"heapsort orders the array");
  ck(a[0]==1&&a[1]==1&&a[7]==9,"heapsort keeps every element");
  ck(d_is_sorted(a,0,sizeof(intmax_t),lt,NULL),"empty is vacuously sorted");
  ck(d_is_sorted(a,1,sizeof(intmax_t),lt,NULL),"one element is vacuously sorted");
  /* comparator bridge */
  {struct d_comparator_binding cb; cb.compare=cmp3; cb.context=NULL;
   a[0]=3;a[1]=1;a[2]=2;
   d_sorted(a,3,sizeof(intmax_t),d_precedes_from_comparator,&cb);
   ck(a[0]==1&&a[2]==3,"three-way comparator bridges to the bool ordering");}
  /* zip */
  for(i=0;i<4;i++){a[i]=(intmax_t)i;b[i]=(intmax_t)i*10;}
  n=d_zip(a,sizeof(intmax_t),b,sizeof(intmax_t),4,o,32);
  ck(n==4&&o[0]==0&&o[1]==0&&o[2]==1&&o[3]==10,"zip concatenates the arms");
  n=d_zip(a,sizeof(intmax_t),b,sizeof(intmax_t),4,o,2);
  ck(n==2,"zip stops at the capacity");
  n=d_zip_with(a,sizeof(intmax_t),b,sizeof(intmax_t),4,o,sizeof(intmax_t),32,add,NULL);
  ck(n==4&&o[0]==0&&o[3]==33,"zip_with combines without materialising pairs");
  /* flat_map */
  for(i=0;i<3;i++)a[i]=(intmax_t)i+1;
  n=d_flat_map(a,3,sizeof(intmax_t),dup2,NULL,o,sizeof(intmax_t),32);
  ck(n==6&&o[0]==1&&o[1]==1&&o[5]==3,"flat_map expands n to m");
  n=d_flat_map(a,3,sizeof(intmax_t),dup2,NULL,o,sizeof(intmax_t),3);
  ck(n<=3,"flat_map respects the capacity");
  /* slice */
  for(i=0;i<8;i++)a[i]=(intmax_t)i;
  n=d_slice(a,8,sizeof(intmax_t),2,5,o,32);
  ck(n==3&&o[0]==2&&o[2]==4,"slice copies the half-open range");
  ck(d_slice(a,8,sizeof(intmax_t),5,2,o,32)==0,"an inverted slice yields nothing");
  ck(d_slice(a,8,sizeof(intmax_t),2,99,o,32)==6,"slice clips to the array");
  /* distinct */
  {struct d_distinct_state st;
   a[0]=1;a[1]=2;a[2]=1;a[3]=3;a[4]=2;a[5]=3;
   d_distinct_state_init(&st,seen,16,sizeof(intmax_t),eq,NULL);
   n=d_distinct(a,6,&st,o,32);
   ck(n==3&&o[0]==1&&o[1]==2&&o[2]==3,"distinct keeps first occurrences");
   ck(n<=6,"distinct never grows the stream");}
  /* searches */
  for(i=0;i<6;i++)a[i]=(intmax_t)i;
  {size_t idx;
   ck(d_index_of(a,6,sizeof(intmax_t),even,NULL,&idx)&&idx==0,"index_of finds the first");
   ck(d_last_index_of(a,6,sizeof(intmax_t),even,NULL,&idx)&&idx==4,"last_index_of finds the last");
   ck(*(intmax_t*)d_find_last(a,6,sizeof(intmax_t),even,NULL)==4,"find_last returns the last match");
   a[0]=1;a[1]=3;a[2]=5;
   ck(!d_index_of(a,3,sizeof(intmax_t),even,NULL,&idx),"index_of reports absence");
   ck(d_find_last(a,3,sizeof(intmax_t),even,NULL)==NULL,"find_last reports absence");}
  /* transducer stages */
  {struct d_transducer_stage st[2];struct d_flat_map_state fm;struct d_distinct_state ds;size_t w=0;
   for(i=0;i<3;i++)a[i]=(intmax_t)i+1;
   d_flat_map_state_init(&fm,dup2,NULL,scr,8,sizeof(intmax_t));
   st[0]=d_transducer_flat_map(&fm);
   d_transduce_into_array(st,1,a,3,sizeof(intmax_t),o,32,&w);
   ck(w==6,"flat_map stage expands in a chain");
   st[0]=d_transducer_flat_map(&fm); st[1]=d_transducer_take(4); w=0;
   d_transduce_into_array(st,2,a,3,sizeof(intmax_t),o,32,&w);
   ck(w==4,"take bounds an expanding chain");
   d_distinct_state_init(&ds,seen,16,sizeof(intmax_t),eq,NULL);
   st[0]=d_transducer_flat_map(&fm); st[1]=d_transducer_distinct(&ds); w=0;
   d_transduce_into_array(st,2,a,3,sizeof(intmax_t),o,32,&w);
   ck(w==3,"distinct stage collapses the duplicates flat_map made");}
  printf("\n  %lu / %lu assertions passed\n",(unsigned long)g_ok,(unsigned long)g_ck);
  return (g_ok==g_ck)?0:1;
}
