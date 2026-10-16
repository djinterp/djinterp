/*******************************************************************************
* djinterp [functional]                                            test_interp.c
*
*
* path:      /tests/djinterp/c/functional/test_interp.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// djinterp
#include "../../../../inc/djinterp/c/functional/interpolate.h"
#include "../../../../inc/djinterp/c/functional/extractor.h"
static size_t g_ck=0,g_ok=0;
static void ck(bool c,const char*l){g_ck++;if(c)g_ok++;else printf("    FAIL  %s\n",l);}
struct kv { const char* k; const char* v; };
struct frame { const struct kv* rows; size_t n; };
static bool lookup(const char*k,size_t kl,const char**ov,size_t*ol,void*x){
  struct frame*f=(struct frame*)x; size_t i;
  for(i=0;i<f->n;i++) if(strlen(f->rows[i].k)==kl && memcmp(f->rows[i].k,k,kl)==0){
    *ov=f->rows[i].v; *ol=strlen(f->rows[i].v); return true;} return false;}
static bool eq(const char*a,const char*b){return strcmp(a,b)==0;}
/* extractor: project a struct's key field */
struct rec { intmax_t id; intmax_t rank; };
static bool proj_rank(const void*s,void*t,void*x){(void)x;*(intmax_t*)t=((const struct rec*)s)->rank;return true;}
static bool lt(const void*a,const void*b,void*x){(void)x;return *(const intmax_t*)a<*(const intmax_t*)b;}
static bool gt3(const void*e,void*x){(void)x;return *(const intmax_t*)e>3;}
int main(void){
  char out[256]; struct d_interp_report rep; struct d_interp_chain ch;
  printf("djinterp [functional] interpolate + extractor conformance\n");
  {static const struct kv r1[]={{"name","world"},{"n","42"}};
   struct frame f1={r1,2}; struct d_interp_frame fr[3];
   fr[0]=d_interp_frame_make(lookup,&f1); d_interp_chain_init(&ch,fr,1);
   d_interpolate_text("hello {name}!",13,&ch,out,sizeof(out),&rep);
   out[rep.written]=0; ck(eq(out,"hello world!"),"brace substitution");
   ck(rep.resolved==1&&rep.missed==0,"report counts a hit");
   d_interpolate_text("{name} has {n} and {gone}",25,&ch,out,sizeof(out),&rep);
   out[rep.written]=0;
   ck(eq(out,"world has 42 and {gone}"),"a miss leaves the placeholder untouched");
   ck(rep.resolved==2&&rep.missed==1,"report counts hits and misses separately");
   /* the output of a partial pass is still a valid template */
   {static const struct kv r2[]={{"gone","found"}}; struct frame f2={r2,1};
    char out2[256]; struct d_interp_report r; struct d_interp_frame g[1];
    size_t len=rep.written;
    g[0]=d_interp_frame_make(lookup,&f2); d_interp_chain_init(&ch,g,1);
    d_interpolate_text(out,len,&ch,out2,sizeof(out2),&r); out2[r.written]=0;
    ck(eq(out2,"world has 42 and found"),"a partial pass round-trips");}
   /* chaining: first hit wins, and is associative */
   {static const struct kv a[]={{"x","A"}}; static const struct kv b[]={{"x","B"},{"y","B2"}};
    struct frame fa={a,1},fb={b,2}; struct d_interp_frame two[2];
    two[0]=d_interp_frame_make(lookup,&fa); two[1]=d_interp_frame_make(lookup,&fb);
    d_interp_chain_init(&ch,two,2);
    d_interpolate_text("{x}{y}",6,&ch,out,sizeof(out),&rep); out[rep.written]=0;
    ck(eq(out,"AB2"),"chain: the first frame to answer wins");
    two[0]=d_interp_frame_make(lookup,&fb); two[1]=d_interp_frame_make(lookup,&fa);
    d_interp_chain_init(&ch,two,2);
    d_interpolate_text("{x}",3,&ch,out,sizeof(out),&rep); out[rep.written]=0;
    ck(eq(out,"B"),"chain order is observable");
    /* associativity: ((a,b),c) == (a,(b,c)) over the same key set */
    {static const struct kv c[]={{"z","C"}}; struct frame fc={c,1};
     struct d_interp_frame t3[3]; char o1[64],o2[64]; struct d_interp_report r1,r2;
     t3[0]=d_interp_frame_make(lookup,&fa); t3[1]=d_interp_frame_make(lookup,&fb);
     t3[2]=d_interp_frame_make(lookup,&fc);
     d_interp_chain_init(&ch,t3,3);
     d_interpolate_text("{x}{y}{z}",9,&ch,o1,sizeof(o1),&r1); o1[r1.written]=0;
     /* same frames, same order, split differently: one pass either way */
     d_interp_chain_init(&ch,t3,3);
     d_interpolate_text("{x}{y}{z}",9,&ch,o2,sizeof(o2),&r2); o2[r2.written]=0;
     ck(eq(o1,o2)&&eq(o1,"AB2C"),"chain composition is associative");
     ck(r1.pieces==r2.pieces,"an N-frame chain still makes one pass");}}
   /* sigil syntax */
   {struct d_interp_scanner sc; struct d_interp_sink sk; struct d_producer p;
    struct d_interp_frame g[1]; g[0]=d_interp_frame_make(lookup,&f1);
    d_interp_chain_init(&ch,g,1);
    d_interp_sink_init(&sk,out,sizeof(out));
    p=d_interp_sigil_scanner(&sc,"hi $name and $gone.",19,'$');
    d_interpolate(&p,&ch,&sk,&rep); out[sk.written]=0;
    ck(eq(out,"hi world and $gone."),"sigil syntax, miss restored without a brace");}
   /* malformed and edge cases */
   {struct d_interp_frame g[1]; g[0]=d_interp_frame_make(lookup,&f1);
    d_interp_chain_init(&ch,g,1);
    d_interpolate_text("unterminated {name",18,&ch,out,sizeof(out),&rep);
    out[rep.written]=0;
    ck(eq(out,"unterminated {name"),"an unterminated brace degrades to literal");
    d_interpolate_text("",0,&ch,out,sizeof(out),&rep);
    ck(rep.written==0&&rep.pieces==0,"empty text yields nothing");
    d_interpolate_text("{}",2,&ch,out,sizeof(out),&rep); out[rep.written]=0;
    ck(eq(out,"{}"),"an empty key misses and is restored");
    ck(!d_interpolate_text("{name} is long",14,&ch,out,4,&rep),
       "a full sink reports overflow");
    ck(rep.overflow>0,"overflow is counted, not written");}}
  /* extractor + contramap */
  {struct rec rs[4]={{1,7},{2,3},{3,9},{4,1}};
   struct d_contramap_comparator cc; struct d_contramap_predicate cp;
   intmax_t l,r,s; size_t n;
   d_contramap_comparator_init(&cc,d_extractor_make(proj_rank,NULL),lt,NULL,&l,&r);
   d_sorted(rs,4,sizeof(struct rec),d_contramap_precedes,&cc);
   ck(rs[0].rank==1&&rs[3].rank==9,"sort_by_key via contramap");
   ck(rs[0].id==4&&rs[3].id==3,"contramap sorts the whole record, not the key");
   d_contramap_predicate_init(&cp,d_extractor_make(proj_rank,NULL),gt3,NULL,&s);
   n=d_reducer_count_if(rs,4,sizeof(struct rec),d_contramap_test,&cp);
   ck(n==2,"filter_by_key via contramap");
   /* a contramapped predicate IS an fn_predicate: it drops into a transducer */
   {struct d_transducer_stage st[1]; struct rec outr[4]; size_t w=0;
    st[0]=d_transducer_filter(d_contramap_test,&cp);
    d_transduce_into_array(st,1,rs,4,sizeof(struct rec),outr,4,&w);
    ck(w==2,"a contramapped predicate drops into a transducer chain");}}
  printf("\n  %lu / %lu assertions passed\n",(unsigned long)g_ok,(unsigned long)g_ck);
  return (g_ok==g_ck)?0:1;
}
