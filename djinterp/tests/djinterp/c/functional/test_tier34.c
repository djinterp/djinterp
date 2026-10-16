/*******************************************************************************
* djinterp [functional]                                            test_tier34.c
*
*
* path:      /tests/djinterp/c/functional/test_tier34.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <stdint.h>
#include <stdio.h>
// djinterp
#include "../../../../inc/djinterp/c/functional/functor.h"
#include "../../../../inc/djinterp/c/functional/free.h"
#include "../../../../inc/djinterp/c/functional/monoid.h"
D_MAYBE_DECLARE(tm, intmax_t);
D_RESULT_DECLARE(tr, intmax_t, intmax_t);
static size_t g_ck=0,g_ok=0;
static void ck(bool c,const char*l){g_ck++;if(c)g_ok++;else printf("    FAIL  %s\n",l);}
static bool idt(const void*i,void*o,void*x){(void)x;*(intmax_t*)o=*(const intmax_t*)i;return true;}
static bool inc(const void*i,void*o,void*x){(void)x;*(intmax_t*)o=*(const intmax_t*)i+1;return true;}
static bool dbl(const void*i,void*o,void*x){(void)x;*(intmax_t*)o=*(const intmax_t*)i*2;return true;}
static bool evenonly(const void*i,void*o,void*x){(void)x;if(*(const intmax_t*)i%2)return false;
  *(intmax_t*)o=*(const intmax_t*)i;return true;}
static bool addz(const void*a,const void*b,void*o,void*x){(void)x;
  *(intmax_t*)o=*(const intmax_t*)a+*(const intmax_t*)b;return true;}
static void k_inc(const void*v,void*oc,void*x){(void)x;intmax_t n=*(const intmax_t*)v+1;
  d_maybe_just((struct d_maybe*)oc,&n);}
static void k_unit(const void*v,void*oc,void*x){(void)x;d_maybe_just((struct d_maybe*)oc,v);}
static void k_res(const void*v,void*oc,void*x){(void)x;intmax_t n=*(const intmax_t*)v*3;
  d_result_ok((struct d_result*)oc,&n);}
/* free: catamorphism arms summing hole values */
static bool oh(const void*v,void*o,void*x){(void)x;*(intmax_t*)o=*(const intmax_t*)v;return true;}
static bool on(const void*l,const void*cr,size_t n,size_t rs,void*o,void*x){(void)l;(void)x;(void)rs;
  intmax_t t=0;size_t i;for(i=0;i<n;i++)t+=((const intmax_t*)cr)[i];*(intmax_t*)o=t;return true;}
static struct d_free* graft_pair(const void*v,struct d_free_arena*ar,void*x){
  struct d_free**slots=(struct d_free**)x;struct d_free*kids=slots[0];
  (void)v; return d_free_node(ar,NULL,kids,2);}
int main(void){
  struct tm ca,cb,co; struct tr ra,rb,ro;
  struct d_maybe ma,mb,mo; struct d_result rra,rrb,rro;
  struct d_functor fm,fr; struct d_applicative am,ar_; struct d_monad mm,mr;
  intmax_t v,w;
  printf("djinterp [functional] tier 3/4 conformance\n");
  ma=D_MAYBE_VIEW(ca); mb=D_MAYBE_VIEW(cb); mo=D_MAYBE_VIEW(co);
  rra=D_RESULT_VIEW(ra); rrb=D_RESULT_VIEW(rb); rro=D_RESULT_VIEW(ro);
  fm=d_functor_maybe(); fr=d_functor_result();
  am=d_applicative_maybe(); ar_=d_applicative_result();
  mm=d_monad_maybe(); mr=d_monad_result();
  /* Functor laws through the dictionary */
  v=5; d_maybe_just(&ma,&v);
  d_functor_map(&fm,&ma,&mo,idt,NULL); ck(co.value==5,"Functor identity (maybe)");
  d_functor_map(&fm,&ma,&mb,inc,NULL); d_functor_map(&fm,&mb,&mo,dbl,NULL);
  ck(co.value==12,"Functor composition (maybe)");
  d_maybe_nothing(&ma); d_functor_map(&fm,&ma,&mo,inc,NULL);
  ck(d_maybe_is_nothing(&mo),"Functor preserves absence");
  d_result_ok(&rra,&v); d_functor_map(&fr,&rra,&rro,dbl,NULL);
  ck(ro.payload.value==10,"Functor over result");
  w=-9; d_result_err(&rra,&w); d_functor_map(&fr,&rra,&rro,dbl,NULL);
  ck(d_result_is_err(&rro),"Functor passes an error through");
  /* Applicative */
  v=3; d_maybe_just(&ma,&v); w=4; d_maybe_just(&mb,&w);
  ck(d_applicative_map2(&am,&ma,&mb,&mo,addz,NULL)&&co.value==7,"map2 combines");
  d_maybe_nothing(&mb);
  ck(!d_applicative_map2(&am,&ma,&mb,&mo,addz,NULL)&&d_maybe_is_nothing(&mo),
     "map2 propagates absence");
  v=1; d_applicative_pure(&am,&mo,&v); ck(co.value==1&&d_maybe_is_just(&mo),"pure embeds");
  v=2; d_result_ok(&rra,&v); w=5; d_result_ok(&rrb,&w);
  ck(d_applicative_map2(&ar_,&rra,&rrb,&rro,addz,NULL)&&ro.payload.value==7,"map2 over result");
  w=-1; d_result_err(&rrb,&w);
  ck(!d_applicative_map2(&ar_,&rra,&rrb,&rro,addz,NULL)&&d_result_is_err(&rro),
     "map2 takes the first error");
  /* both arms failing: the FIRST error must win, not merely some error */
  {intmax_t e1=-11,e2=-22,got;
   d_result_err(&rra,&e1); d_result_err(&rrb,&e2);
   d_applicative_map2(&ar_,&rra,&rrb,&rro,addz,NULL);
   ck(d_result_error(&rro,&got)&&got==-11,"map2 error selection is left-biased");
   d_applicative_map2(&ar_,&rrb,&rra,&rro,addz,NULL);
   ck(d_result_error(&rro,&got)&&got==-22,"map2 error selection follows the order");}
  /* Monad laws */
  v=7; d_monad_unit(&mm,&ma,&v);
  ck(d_maybe_is_just(&ma)&&ca.value==7,"unit embeds");
  d_monad_bind(&mm,&ma,&mo,k_inc,NULL); ck(co.value==8,"Monad left identity");
  d_monad_bind(&mm,&ma,&mo,k_unit,NULL); ck(co.value==7,"Monad right identity");
  d_maybe_nothing(&ma); d_monad_bind(&mm,&ma,&mo,k_inc,NULL);
  ck(d_maybe_is_nothing(&mo),"bind short-circuits on absence");
  v=4; d_result_ok(&rra,&v); d_monad_bind(&mr,&rra,&rro,k_res,NULL);
  ck(ro.payload.value==12,"bind over result");
  w=-2; d_result_err(&rra,&w); d_monad_bind(&mr,&rra,&rro,k_res,NULL);
  ck(d_result_is_err(&rro),"bind short-circuits on error");
  /* free */
  {struct d_free nodes[64],kids[4],*t,*m2,*b2; struct d_free_arena ar2;
   struct d_value_arena va; intmax_t vals[16],scr[16],hv[4]={1,2,3,4},out;
   struct d_reducer red; struct d_reducing_state st; struct d_monoid sum;
   d_free_arena_init(&ar2,nodes,64);
   kids[0]=*d_free_hole(&ar2,&hv[0]); kids[1]=*d_free_hole(&ar2,&hv[1]);
   kids[2]=*d_free_hole(&ar2,&hv[2]);
   t=d_free_node(&ar2,NULL,kids,3);
   ck(d_free_size(t)==4,"free size counts nodes and holes");
   ck(d_free_depth(t)==2,"free depth");
   ck(d_free_hole_count(t)==3,"free hole count");
   ck(!d_free_is_hole(t)&&d_free_is_hole(&kids[0]),"hole and node discriminate");
   ck(d_free_fold(t,oh,on,NULL,&out,sizeof(intmax_t),scr,sizeof(scr))&&out==6,
      "catamorphism sums the holes");
   /* Foldable: the tier 1 reducer folds a free tree */
   sum=d_monoid_sum_intmax(); red=d_monoid_reducer(&sum); out=0;
   d_reducing_state_init(&st,&out);
   ck(d_free_drive_holes(t,&red,&st,sizeof(intmax_t))==3&&out==6,
      "a monoid reducer folds a free tree");
   /* Functor over the hole type */
   d_free_arena_init(&ar2,nodes,64); d_value_arena_init(&va,vals,sizeof(intmax_t),16);
   kids[0]=*d_free_hole(&ar2,&hv[0]); kids[1]=*d_free_hole(&ar2,&hv[1]);
   t=d_free_node(&ar2,NULL,kids,2);
   m2=d_free_map(t,&ar2,dbl,NULL,&va);
   ck(m2!=NULL,"free map rebuilds");
   out=0; d_reducing_state_init(&st,&out); d_free_drive_holes(m2,&red,&st,sizeof(intmax_t));
   ck(out==6,"free map doubled every hole");
   ck(d_free_size(m2)==3,"free map preserves the shape");
   /* Monad: substitution at the holes */
   {struct d_free sub[2]; struct d_free* slots[1];
    sub[0]=*d_free_hole(&ar2,&hv[0]); sub[1]=*d_free_hole(&ar2,&hv[1]);
    slots[0]=sub;
    b2=d_free_bind(t,&ar2,graft_pair,slots);
    ck(b2!=NULL&&d_free_hole_count(b2)==4,"bind substitutes a subtree per hole");}
   /* traverse: the narrow slice */
   d_free_arena_init(&ar2,nodes,64); d_value_arena_init(&va,vals,sizeof(intmax_t),16);
   {intmax_t ev[2]={2,4}; struct d_free k2[2],*tt,*res=NULL;
    k2[0]=*d_free_hole(&ar2,&ev[0]); k2[1]=*d_free_hole(&ar2,&ev[1]);
    tt=d_free_node(&ar2,NULL,k2,2);
    ck(d_free_traverse(tt,evenonly,NULL,&ar2,&va,&res)&&res!=NULL,
       "traverse succeeds when every visit does");
    d_free_arena_init(&ar2,nodes,64); d_value_arena_init(&va,vals,sizeof(intmax_t),16);
    k2[0]=*d_free_hole(&ar2,&hv[0]); k2[1]=*d_free_hole(&ar2,&ev[0]);
    tt=d_free_node(&ar2,NULL,k2,2);
    ck(!d_free_traverse(tt,evenonly,NULL,&ar2,&va,&res),
       "traverse fails if any visit does");}
   /* scratch too small must be refused, not overrun */
   d_free_arena_init(&ar2,nodes,64);
   kids[0]=*d_free_hole(&ar2,&hv[0]); kids[1]=*d_free_hole(&ar2,&hv[1]);
   kids[2]=*d_free_hole(&ar2,&hv[2]); kids[3]=*d_free_hole(&ar2,&hv[3]);
   t=d_free_node(&ar2,NULL,kids,4);
   ck(!d_free_fold(t,oh,on,NULL,&out,sizeof(intmax_t),scr,sizeof(intmax_t)*2),
      "fold refuses a scratch that is too small");
   /* a two-level tree: 3 inner nodes of 3 holes each. Unwinding the scratch
      needs 6 slots concurrently; never unwinding would need 12. A scratch of 8
      distinguishes the two, which a shallow tree cannot. */
   {struct d_free deep[32],inner[3],leaf[9]; struct d_free_arena da;
    intmax_t dv[9],sc8[8],dout; size_t i;
    for(i=0;i<9;i++)dv[i]=(intmax_t)(i+1);
    d_free_arena_init(&da,deep,32);
    for(i=0;i<9;i++)leaf[i]=*d_free_hole(&da,&dv[i]);
    for(i=0;i<3;i++)inner[i]=*d_free_node(&da,NULL,&leaf[i*3],3);
    t=d_free_node(&da,NULL,inner,3);
    ck(d_free_hole_count(t)==9,"deep tree has nine holes");
    ck(d_free_fold(t,oh,on,NULL,&dout,sizeof(intmax_t),sc8,sizeof(sc8))&&dout==45,
       "fold unwinds the scratch between siblings");}
  }
  /* cofree */
  {struct d_cofree cn[8],ck2[2],*c; struct d_cofree_arena ca2;
   intmax_t an[3]={10,20,30},out; struct d_reducer red; struct d_reducing_state st;
   struct d_monoid sum=d_monoid_sum_intmax();
   d_cofree_arena_init(&ca2,cn,8);
   ck2[0]=*d_cofree_node(&ca2,&an[1],NULL,NULL,0);
   ck2[1]=*d_cofree_node(&ca2,&an[2],NULL,NULL,0);
   c=d_cofree_node(&ca2,&an[0],NULL,ck2,2);
   ck(*(const intmax_t*)d_cofree_extract(c)==10,"extract is total at the root");
   ck(d_cofree_size(c)==3,"cofree size");
   ck(d_cofree_depth(c)==2,"cofree depth");
   red=d_monoid_reducer(&sum); out=0; d_reducing_state_init(&st,&out);
   ck(d_cofree_drive(c,&red,&st,sizeof(intmax_t))==3&&out==60,
      "a monoid reducer folds a cofree tree");}
  printf("\n  %lu / %lu assertions passed\n",(unsigned long)g_ok,(unsigned long)g_ck);
  return (g_ok==g_ck)?0:1;
}
