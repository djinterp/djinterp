#include "../../../../../inc/djinterp/c/util/pdf/pdf_metrics.h"


/*
   THE TABLES BELOW WERE EXTRACTED FROM pdf_metrics.hpp MECHANICALLY.

   2,560 advances across ten faces, retyped by hand, is a transcription surface
   with no natural detector: one wrong digit in one cell produces widths that
   are correct for every string not containing that glyph, and wrong -- by a
   fraction of a millimetre -- for every string that does.  Nothing in a
   fixture built from ASCII samples would ever see it.

   So they were pulled by a script that asserts 256 values per face, and the
   differential compares ALL 2,560 cells against the C++ side rather than
   sampling.  A cell is either identical or the differential says which one.

   WinAnsiEncoding for the Helvetica and Times families, matching the
   /Encoding the foundation emits.  Symbol and ZapfDingbats carry their own
   built-in encodings.  Courier is uniform and computed -- see courier_width.
*/

static const short g_helvetica_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     278,  278,  355,  556,  556,  889,  667,  191,  333,  333,  389,  584,  278,  333,  278,  278,
     556,  556,  556,  556,  556,  556,  556,  556,  556,  556,  278,  278,  584,  584,  584,  556,
    1015,  667,  667,  722,  722,  667,  611,  778,  722,  278,  500,  667,  556,  833,  722,  778,
     667,  778,  722,  667,  611,  722,  667,  944,  667,  667,  611,  278,  278,  278,  469,  556,
     333,  556,  556,  500,  556,  556,  278,  556,  556,  222,  222,  500,  222,  833,  556,  556,
     556,  556,  333,  500,  278,  556,  500,  722,  500,  500,  500,  334,  260,  334,  584,  350,
     556,  350,  222,  556,  333, 1000,  556,  556,  333, 1000,  667,  333, 1000,  350,  611,  350,
     350,  222,  222,  333,  333,  350,  556, 1000,  333, 1000,  500,  333,  944,  350,  500,  667,
     278,  333,  556,  556,  556,  556,  260,  556,  333,  737,  370,  556,  584,  333,  737,  333,
     400,  584,  333,  333,  333,  556,  537,  278,  333,  333,  365,  556,  834,  834,  834,  611,
     667,  667,  667,  667,  667,  667, 1000,  722,  667,  667,  667,  667,  278,  278,  278,  278,
     722,  722,  778,  778,  778,  778,  778,  584,  778,  722,  722,  722,  722,  667,  667,  611,
     556,  556,  556,  556,  556,  556,  889,  500,  556,  556,  556,  556,  278,  278,  278,  278,
     556,  556,  556,  556,  556,  556,  556,  584,  611,  556,  556,  556,  556,  500,  556,  500
};

static const short g_helvetica_bold_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     278,  333,  474,  556,  556,  889,  722,  238,  333,  333,  389,  584,  278,  333,  278,  278,
     556,  556,  556,  556,  556,  556,  556,  556,  556,  556,  333,  333,  584,  584,  584,  611,
     975,  722,  722,  722,  722,  667,  611,  778,  722,  278,  556,  722,  611,  833,  722,  778,
     667,  778,  722,  667,  611,  722,  667,  944,  667,  667,  611,  333,  278,  333,  584,  556,
     333,  556,  611,  556,  611,  556,  333,  611,  611,  278,  278,  556,  278,  889,  611,  611,
     611,  611,  389,  556,  333,  611,  556,  778,  556,  556,  500,  389,  280,  389,  584,  350,
     556,  350,  278,  556,  500, 1000,  556,  556,  333, 1000,  667,  333, 1000,  350,  611,  350,
     350,  278,  278,  500,  500,  350,  556, 1000,  333, 1000,  556,  333,  944,  350,  500,  667,
     278,  333,  556,  556,  556,  556,  280,  556,  333,  737,  370,  556,  584,  333,  737,  333,
     400,  584,  333,  333,  333,  611,  556,  278,  333,  333,  365,  556,  834,  834,  834,  611,
     722,  722,  722,  722,  722,  722, 1000,  722,  667,  667,  667,  667,  278,  278,  278,  278,
     722,  722,  778,  778,  778,  778,  778,  584,  778,  722,  722,  722,  722,  667,  667,  611,
     556,  556,  556,  556,  556,  556,  889,  556,  556,  556,  556,  556,  278,  278,  278,  278,
     611,  611,  611,  611,  611,  611,  611,  584,  611,  611,  611,  611,  611,  556,  611,  556
};

static const short g_helvetica_oblique_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     278,  278,  355,  556,  556,  889,  667,  191,  333,  333,  389,  584,  278,  333,  278,  278,
     556,  556,  556,  556,  556,  556,  556,  556,  556,  556,  278,  278,  584,  584,  584,  556,
    1015,  667,  667,  722,  722,  667,  611,  778,  722,  278,  500,  667,  556,  833,  722,  778,
     667,  778,  722,  667,  611,  722,  667,  944,  667,  667,  611,  278,  278,  278,  469,  556,
     333,  556,  556,  500,  556,  556,  278,  556,  556,  222,  222,  500,  222,  833,  556,  556,
     556,  556,  333,  500,  278,  556,  500,  722,  500,  500,  500,  334,  260,  334,  584,  350,
     556,  350,  222,  556,  333, 1000,  556,  556,  333, 1000,  667,  333, 1000,  350,  611,  350,
     350,  222,  222,  333,  333,  350,  556, 1000,  333, 1000,  500,  333,  944,  350,  500,  667,
     278,  333,  556,  556,  556,  556,  260,  556,  333,  737,  370,  556,  584,  333,  737,  333,
     400,  584,  333,  333,  333,  556,  537,  278,  333,  333,  365,  556,  834,  834,  834,  611,
     667,  667,  667,  667,  667,  667, 1000,  722,  667,  667,  667,  667,  278,  278,  278,  278,
     722,  722,  778,  778,  778,  778,  778,  584,  778,  722,  722,  722,  722,  667,  667,  611,
     556,  556,  556,  556,  556,  556,  889,  500,  556,  556,  556,  556,  278,  278,  278,  278,
     556,  556,  556,  556,  556,  556,  556,  584,  611,  556,  556,  556,  556,  500,  556,  500
};

static const short g_helvetica_bold_oblique_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     278,  333,  474,  556,  556,  889,  722,  238,  333,  333,  389,  584,  278,  333,  278,  278,
     556,  556,  556,  556,  556,  556,  556,  556,  556,  556,  333,  333,  584,  584,  584,  611,
     975,  722,  722,  722,  722,  667,  611,  778,  722,  278,  556,  722,  611,  833,  722,  778,
     667,  778,  722,  667,  611,  722,  667,  944,  667,  667,  611,  333,  278,  333,  584,  556,
     333,  556,  611,  556,  611,  556,  333,  611,  611,  278,  278,  556,  278,  889,  611,  611,
     611,  611,  389,  556,  333,  611,  556,  778,  556,  556,  500,  389,  280,  389,  584,  350,
     556,  350,  278,  556,  500, 1000,  556,  556,  333, 1000,  667,  333, 1000,  350,  611,  350,
     350,  278,  278,  500,  500,  350,  556, 1000,  333, 1000,  556,  333,  944,  350,  500,  667,
     278,  333,  556,  556,  556,  556,  280,  556,  333,  737,  370,  556,  584,  333,  737,  333,
     400,  584,  333,  333,  333,  611,  556,  278,  333,  333,  365,  556,  834,  834,  834,  611,
     722,  722,  722,  722,  722,  722, 1000,  722,  667,  667,  667,  667,  278,  278,  278,  278,
     722,  722,  778,  778,  778,  778,  778,  584,  778,  722,  722,  722,  722,  667,  667,  611,
     556,  556,  556,  556,  556,  556,  889,  556,  556,  556,  556,  556,  278,  278,  278,  278,
     611,  611,  611,  611,  611,  611,  611,  584,  611,  611,  611,  611,  611,  556,  611,  556
};

static const short g_times_roman_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     250,  333,  408,  500,  500,  833,  778,  180,  333,  333,  500,  564,  250,  333,  250,  278,
     500,  500,  500,  500,  500,  500,  500,  500,  500,  500,  278,  278,  564,  564,  564,  444,
     921,  722,  667,  667,  722,  611,  556,  722,  722,  333,  389,  722,  611,  889,  722,  722,
     556,  722,  667,  556,  611,  722,  722,  944,  722,  722,  611,  333,  278,  333,  469,  500,
     333,  444,  500,  444,  500,  444,  333,  500,  500,  278,  278,  500,  278,  778,  500,  500,
     500,  500,  333,  389,  278,  500,  500,  722,  500,  500,  444,  480,  200,  480,  541,  350,
     500,  350,  333,  500,  444, 1000,  500,  500,  333, 1000,  556,  333,  889,  350,  611,  350,
     350,  333,  333,  444,  444,  350,  500, 1000,  333,  980,  389,  333,  722,  350,  444,  722,
     250,  333,  500,  500,  500,  500,  200,  500,  333,  760,  276,  500,  564,  333,  760,  333,
     400,  564,  300,  300,  333,  500,  453,  250,  333,  300,  310,  500,  750,  750,  750,  444,
     722,  722,  722,  722,  722,  722,  889,  667,  611,  611,  611,  611,  333,  333,  333,  333,
     722,  722,  722,  722,  722,  722,  722,  564,  722,  722,  722,  722,  722,  722,  556,  500,
     444,  444,  444,  444,  444,  444,  667,  444,  444,  444,  444,  444,  278,  278,  278,  278,
     500,  500,  500,  500,  500,  500,  500,  564,  500,  500,  500,  500,  500,  500,  500,  500
};

static const short g_times_bold_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     250,  333,  555,  500,  500, 1000,  833,  278,  333,  333,  500,  570,  250,  333,  250,  278,
     500,  500,  500,  500,  500,  500,  500,  500,  500,  500,  333,  333,  570,  570,  570,  500,
     930,  722,  667,  722,  722,  667,  611,  778,  778,  389,  500,  778,  667,  944,  722,  778,
     611,  778,  722,  556,  667,  722,  722, 1000,  722,  722,  667,  333,  278,  333,  581,  500,
     333,  500,  556,  444,  556,  444,  333,  500,  556,  278,  333,  556,  278,  833,  556,  500,
     556,  556,  444,  389,  333,  556,  500,  722,  500,  500,  444,  394,  220,  394,  520,  350,
     500,  350,  333,  500,  500, 1000,  500,  500,  333, 1000,  556,  333, 1000,  350,  667,  350,
     350,  333,  333,  500,  500,  350,  500, 1000,  333, 1000,  389,  333,  722,  350,  444,  722,
     250,  333,  500,  500,  500,  500,  220,  500,  333,  747,  300,  500,  570,  333,  747,  333,
     400,  570,  300,  300,  333,  556,  540,  250,  333,  300,  330,  500,  750,  750,  750,  500,
     722,  722,  722,  722,  722,  722, 1000,  722,  667,  667,  667,  667,  389,  389,  389,  389,
     722,  722,  778,  778,  778,  778,  778,  570,  778,  722,  722,  722,  722,  722,  611,  556,
     500,  500,  500,  500,  500,  500,  722,  444,  444,  444,  444,  444,  278,  278,  278,  278,
     500,  556,  500,  500,  500,  500,  500,  570,  500,  556,  556,  556,  556,  500,  556,  500
};

static const short g_times_italic_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     250,  333,  420,  500,  500,  833,  778,  214,  333,  333,  500,  675,  250,  333,  250,  278,
     500,  500,  500,  500,  500,  500,  500,  500,  500,  500,  333,  333,  675,  675,  675,  500,
     920,  611,  611,  667,  722,  611,  611,  722,  722,  333,  444,  667,  556,  833,  667,  722,
     611,  722,  611,  500,  556,  722,  611,  833,  611,  556,  556,  389,  278,  389,  422,  500,
     333,  500,  500,  444,  500,  444,  278,  500,  500,  278,  278,  444,  278,  722,  500,  500,
     500,  500,  389,  389,  278,  500,  444,  667,  444,  444,  389,  400,  275,  400,  541,  350,
     500,  350,  333,  500,  556,  889,  500,  500,  333, 1000,  500,  333,  944,  350,  556,  350,
     350,  333,  333,  556,  556,  350,  500,  889,  333,  980,  389,  333,  667,  350,  389,  556,
     250,  389,  500,  500,  500,  500,  275,  500,  333,  760,  276,  500,  675,  333,  760,  333,
     400,  675,  300,  300,  333,  500,  523,  250,  333,  300,  310,  500,  750,  750,  750,  500,
     611,  611,  611,  611,  611,  611,  889,  667,  611,  611,  611,  611,  333,  333,  333,  333,
     722,  667,  722,  722,  722,  722,  722,  675,  722,  722,  722,  722,  722,  556,  611,  500,
     500,  500,  500,  500,  500,  500,  667,  444,  444,  444,  444,  444,  278,  278,  278,  278,
     500,  500,  500,  500,  500,  500,  500,  675,  500,  500,  500,  500,  500,  444,  500,  444
};

static const short g_times_bold_italic_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     250,  389,  555,  500,  500,  833,  778,  278,  333,  333,  500,  570,  250,  333,  250,  278,
     500,  500,  500,  500,  500,  500,  500,  500,  500,  500,  333,  333,  570,  570,  570,  500,
     832,  667,  667,  667,  722,  667,  667,  722,  778,  389,  500,  667,  611,  889,  722,  722,
     611,  722,  667,  556,  611,  722,  667,  889,  667,  611,  611,  333,  278,  333,  570,  500,
     333,  500,  500,  444,  500,  444,  333,  500,  556,  278,  278,  500,  278,  778,  556,  500,
     500,  500,  389,  389,  278,  556,  444,  667,  500,  444,  389,  348,  220,  348,  570,  350,
     500,  350,  333,  500,  500, 1000,  500,  500,  333, 1000,  556,  333,  944,  350,  611,  350,
     350,  333,  333,  500,  500,  350,  500, 1000,  333, 1000,  389,  333,  722,  350,  389,  611,
     250,  389,  500,  500,  500,  500,  220,  500,  333,  747,  266,  500,  606,  333,  747,  333,
     400,  570,  300,  300,  333,  576,  500,  250,  333,  300,  300,  500,  750,  750,  750,  500,
     667,  667,  667,  667,  667,  667,  944,  667,  667,  667,  667,  667,  389,  389,  389,  389,
     722,  722,  722,  722,  722,  722,  722,  570,  722,  722,  722,  722,  722,  611,  611,  500,
     500,  500,  500,  500,  500,  500,  722,  444,  444,  444,  444,  444,  278,  278,  278,  278,
     500,  556,  500,  500,  500,  500,  500,  570,  500,  556,  556,  556,  556,  444,  500,  444
};

static const short g_symbol_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     250,  333,  713,  500,  549,  833,  778,  439,  333,  333,  500,  549,  250,  549,  250,  278,
     500,  500,  500,  500,  500,  500,  500,  500,  500,  500,  278,  278,  549,  549,  549,  444,
     549,  722,  667,  722,  612,  611,  763,  603,  722,  333,  631,  722,  686,  889,  722,  722,
     768,  741,  556,  592,  611,  690,  439,  768,  645,  795,  611,  333,  863,  333,  658,  500,
     500,  631,  549,  549,  494,  439,  521,  411,  603,  329,  603,  549,  549,  576,  521,  549,
     549,  521,  549,  603,  439,  576,  713,  686,  493,  686,  494,  480,  200,  480,  549,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     750,  620,  247,  549,  167,  713,  500,  753,  753,  753,  753, 1042,  987,  603,  987,  603,
     400,  549,  411,  549,  549,  713,  494,  460,  549,  549,  549,  549, 1000,  603, 1000,  658,
     823,  686,  795,  987,  768,  768,  823,  768,  768,  713,  713,  713,  713,  713,  713,  713,
     768,  713,  790,  790,  890,  823,  549,  250,  713,  603,  603, 1042,  987,  603,  987,  603,
     494,  329,  790,  790,  786,  713,  384,  384,  384,  384,  384,  384,  494,  494,  494,  494,
       0,  329,  274,  686,  686,  686,  384,  384,  384,  384,  384,  384,  494,  494,  494,    0
};

static const short g_zapf_dingbats_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     278,  974,  961,  974,  980,  719,  789,  790,  791,  690,  960,  939,  549,  855,  911,  933,
     911,  945,  974,  755,  846,  762,  761,  571,  677,  763,  760,  759,  754,  494,  552,  537,
     577,  692,  786,  788,  788,  790,  793,  794,  816,  823,  789,  841,  823,  833,  816,  831,
     923,  744,  723,  749,  790,  792,  695,  776,  768,  792,  759,  707,  708,  682,  701,  826,
     815,  789,  789,  707,  687,  696,  689,  786,  787,  713,  791,  785,  791,  873,  761,  762,
     762,  759,  759,  892,  892,  788,  784,  438,  138,  277,  415,  392,  392,  668,  668,    0,
     390,  390,  317,  317,  276,  276,  509,  509,  410,  410,  234,  234,  334,  334,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,  732,  544,  544,  910,  667,  760,  760,  776,  595,  694,  626,  788,  788,  788,  788,
     788,  788,  788,  788,  788,  788,  788,  788,  788,  788,  788,  788,  788,  788,  788,  788,
     788,  788,  788,  788,  788,  788,  788,  788,  788,  788,  788,  788,  788,  788,  788,  788,
     788,  788,  788,  788,  894,  838, 1016,  458,  748,  924,  748,  918,  927,  928,  928,  834,
     873,  828,  924,  924,  917,  930,  931,  463,  883,  836,  836,  867,  867,  696,  696,  874,
       0,  874,  760,  946,  771,  865,  771,  888,  967,  888,  831,  873,  927,  970,  918,    0
};


/*
courier_width_helper
  The advance of a Courier glyph, per 1000 em.

  COMPUTED RATHER THAN TABULATED, because all four Courier faces are uniform --
  but NOT unconditional.

  THE CONTROL RANGE HAS NO GLYPH AND CONTRIBUTES 0.  This was got wrong on the
  first attempt: the port returned 600 for every code, on the reading that
  "Courier is uniformly 600/1000 em" -- which is what the C++ banner says and
  is true of every PRINTABLE code.  The tabulated faces all carry 0 through
  0x1F, so a Courier face returning 600 there is the one face that measures
  control bytes as occupying space.
    The differential caught it as 128 divergent cells -- four Courier faces by
  thirty-two control codes -- on its first run.  Nothing in a fixture built
  from printable samples would have seen it, and the effect in production is a
  line containing a tab or a stray \r measuring wider in Courier than in
  Helvetica, which reads as a font quirk rather than as a bug.

Parameter(s):
  _code:  the byte, already masked to 0..255 by the caller.
Return:
  0 below 0x20, 600 otherwise.
*/
static int32_t
courier_width_helper
(
    int32_t _code
)
{
    if (_code < 0x20)
    {
        return 0;
    }

    return D_PDF_COURIER_ADVANCE;
}


/* =========================================================================
   I.     faces
   ========================================================================= */

/*
d_pdf_width_table_for
  The width table for a face.

Parameter(s):
  _font:  enum d_pdf_base_font.
Return:
  a borrowed static array of 256 advances, or NULL for the Courier faces and
  for any unrecognised value.  NULL is not an error -- see the header's note on
  why an unknown face measures as Courier rather than trapping.
*/
const short*
d_pdf_width_table_for
(
    int32_t _font
)
{
    switch (_font)
    {
        case D_PDF_FONT_HELVETICA:              return g_helvetica_w;
        case D_PDF_FONT_HELVETICA_BOLD:         return g_helvetica_bold_w;
        case D_PDF_FONT_HELVETICA_OBLIQUE:      return g_helvetica_oblique_w;
        case D_PDF_FONT_HELVETICA_BOLD_OBLIQUE: return g_helvetica_bold_oblique_w;
        case D_PDF_FONT_TIMES_ROMAN:            return g_times_roman_w;
        case D_PDF_FONT_TIMES_BOLD:             return g_times_bold_w;
        case D_PDF_FONT_TIMES_ITALIC:           return g_times_italic_w;
        case D_PDF_FONT_TIMES_BOLD_ITALIC:      return g_times_bold_italic_w;
        case D_PDF_FONT_SYMBOL:                 return g_symbol_w;
        case D_PDF_FONT_ZAPF_DINGBATS:          return g_zapf_dingbats_w;
        default:                                break;
    }

    return 0;
}

/*
   d_pdf_font_is_monospaced MOVED to d_pdf_primitives.c.

   Its declaration was removed from this header and its DEFINITION left here --
   so both translation units defined it and the LINKER caught what neither
   compiler could: "multiple definition of d_pdf_font_is_monospaced".
   Recorded because it is the exact failure mode this split exists to prevent,
   arriving one step later than expected: removing a duplicate declaration
   without removing the duplicate definition leaves the duplicate.
*/


/* =========================================================================
   II.    glyph advance
   ========================================================================= */

/*
d_pdf_glyph_advance_em
  The advance of one byte in a face, per 1000 em.

Parameter(s):
  _font:  enum d_pdf_base_font.
  _code:  the byte.  MASKED to 0..255 -- see below.
Return:
  the advance; 0 for a code the face does not map.
*/
int32_t
d_pdf_glyph_advance_em
(
    int32_t _font,
    int32_t _code
)
{
    const short* _table = d_pdf_width_table_for(_font);

    /*   MASKED, NOT ASSUMED IN RANGE.  A caller passing a plain `char` on a
       target where char is signed hands us -23 for WinAnsi 0xE9 (e-acute), and
       an unmasked index reads 23 shorts BEFORE the table -- returning whatever
       is in the preceding object.  That is a silent wrong width on precisely
       the accented text the Adobe tables exist to measure, so the mask is not
       defensive padding but the thing that makes non-ASCII correct. */
    _code &= 0xFF;

    if (!_table)
    {
        return courier_width_helper(_code);
    }

    return (int32_t)_table[_code];
}

/*
d_pdf_glyph_width
  The rendered width of one byte at a point size.

Parameter(s):
  _font:  enum d_pdf_base_font.
  _code:  the byte.
  _size:  point size.
Return:
  advance * size / 1000.
*/
double
d_pdf_glyph_width
(
    int32_t _font,
    int32_t _code,
    double  _size
)
{
    return ((double)d_pdf_glyph_advance_em(_font, _code) * _size) / 1000.0;
}


/* =========================================================================
   III.   string width
   ========================================================================= */

/*
d_pdf_text_width
  The rendered width of a byte range at a point size.

  SUMS ADVANCES IN INTEGER, SCALES ONCE.  Scaling each glyph and summing the
  reals accumulates a rounding error per character; summing the exact integer
  advances and scaling the total once does not.  The C++ side sums reals, so
  the two can differ in the last bits on long strings -- which the differential
  checks with a tolerance rather than pretending it does not happen.  Doing it
  the accurate way here and recording the difference is better than reproducing
  a rounding artefact for the sake of a bit-identical answer.

Parameter(s):
  _font:    enum d_pdf_base_font.
  _text:    the bytes; may be NULL only when _length is 0.
  _length:  how many.
  _size:    point size.
Return:
  the width; 0 for an empty or NULL range.
*/
double
d_pdf_text_width
(
    int32_t     _font,
    const char* _text,
    size_t      _length,
    double      _size
)
{
    int64_t _sum = 0;
    size_t  _i   = 0;

    if ( (!_text) || (_length == 0u) )
    {
        return 0.0;
    }

    for (_i = 0; _i < _length; ++_i)
    {
        _sum += (int64_t)d_pdf_glyph_advance_em(_font, (int32_t)
                    (unsigned char)_text[_i]);
    }

    return ((double)_sum * _size) / 1000.0;
}

/*
d_pdf_text_width_z
  The rendered width of a NUL-terminated string.

Parameter(s):
  _font:  enum d_pdf_base_font.
  _text:  the string; NULL measures 0.
  _size:  point size.
Return:
  the width.
*/
double
d_pdf_text_width_z
(
    int32_t     _font,
    const char* _text,
    double      _size
)
{
    size_t _n = 0;

    if (!_text)
    {
        return 0.0;
    }

    while (_text[_n] != '\0')
    {
        ++_n;
    }

    return d_pdf_text_width(_font, _text, _n, _size);
}


/* =========================================================================
   IV.    fitting & truncation
   ========================================================================= */

/*
d_pdf_fit_char_count
  How many leading bytes fit within a width.

Parameter(s):
  _font:       enum d_pdf_base_font.
  _text:       the bytes.
  _length:     how many.
  _size:       point size.
  _max_width:  the budget.
Return:
  the count; 0 when even the first glyph does not fit, _length when all do.
*/
size_t
d_pdf_fit_char_count
(
    int32_t     _font,
    const char* _text,
    size_t      _length,
    double      _size,
    double      _max_width
)
{
    int64_t _sum   = 0;
    size_t  _i     = 0;
    double  _scale = _size / 1000.0;

    if ( (!_text) || (_length == 0u) || (_max_width <= 0.0) )
    {
        return 0u;
    }

    for (_i = 0; _i < _length; ++_i)
    {
        _sum += (int64_t)d_pdf_glyph_advance_em(_font, (int32_t)
                    (unsigned char)_text[_i]);

        /*   STRICTLY GREATER, so a glyph landing EXACTLY on the budget fits.
           The alternative drops a character from every string measured to the
           width it was measured at, which is the common case in a fitting
           routine rather than an edge one. */
        if (((double)_sum * _scale) > _max_width)
        {
            return _i;
        }
    }

    return _length;
}

/*
d_pdf_truncate_ellipsis
  Truncates to fit, appending "..." when it does not.

  THE ELLIPSIS IS MEASURED IN THE SAME FACE, and the budget for the text is the
  width remaining after it.  Truncating first and appending afterwards produces
  a result WIDER than the budget -- which is the whole failure the function
  exists to prevent, and is what "truncate then decorate" always gets wrong.

Parameter(s):
  _font:          enum d_pdf_base_font.
  _text:          the bytes.
  _length:        how many.
  _size:          point size.
  _max_width:     the budget.
  _out:           caller storage.
  _out_capacity:  its size; _length + 4 is always enough.
Return:
  the length written, not counting the terminator, or (size_t)-1 when the
  buffer is too small.
*/
size_t
d_pdf_truncate_ellipsis
(
    int32_t     _font,
    const char* _text,
    size_t      _length,
    double      _size,
    double      _max_width,
    char*       _out,
    size_t      _out_capacity
)
{
    static const char _dots[3] = { '.', '.', '.' };

    double _dots_width = 0.0;
    size_t _fit        = 0;
    size_t _i          = 0;

    if ( (!_out) || (_out_capacity == 0u) )
    {
        return (size_t)-1;
    }

    if (!_text)
    {
        _out[0] = '\0';
        return 0u;
    }

    /* fits whole: copy and done */
    if (d_pdf_text_width(_font, _text, _length, _size) <= _max_width)
    {
        if (_out_capacity < (_length + 1u))
        {
            return (size_t)-1;
        }

        for (_i = 0; _i < _length; ++_i)
        {
            _out[_i] = _text[_i];
        }

        _out[_length] = '\0';

        return _length;
    }

    _dots_width = d_pdf_text_width(_font, _dots, 3u, _size);

    /*   No room even for the ellipsis: emit nothing rather than emit dots that
       themselves overflow.  A caller with a budget this small wants an empty
       cell, not a cell whose only content is an overflowing marker. */
    if (_dots_width > _max_width)
    {
        _out[0] = '\0';
        return 0u;
    }

    _fit = d_pdf_fit_char_count(_font, _text, _length, _size,
                                _max_width - _dots_width);

    if (_out_capacity < (_fit + 4u))
    {
        return (size_t)-1;
    }

    for (_i = 0; _i < _fit; ++_i)
    {
        _out[_i] = _text[_i];
    }

    _out[_fit + 0u] = '.';
    _out[_fit + 1u] = '.';
    _out[_fit + 2u] = '.';
    _out[_fit + 3u] = '\0';

    return _fit + 3u;
}


/* =========================================================================
   V.     word wrapping
   ========================================================================= */

/*
emit_line_helper
  Records one produced line, counting it whether or not there is room.

Parameter(s):
  _out:           caller storage; may be NULL when only the count is wanted.
  _out_capacity:  its size.
  _count:         how many lines have been produced so far.
  _begin:         the span's first byte.
  _length:        its length.
Return:
  nothing.
*/
static void
emit_line_helper
(
    struct d_pdf_wrap_line* _out,
    size_t                  _out_capacity,
    size_t                  _count,
    const char*             _begin,
    size_t                  _length
)
{
    if (_out && (_count < _out_capacity))
    {
        _out[_count].begin  = _begin;
        _out[_count].length = _length;
    }

    return;
}

/*
d_pdf_wrap_to_width
  Greedy word wrap against a measured width.

  THE WHITESPACE POLICY IS STATED HERE BECAUSE BOTH TIERS IMPLEMENT IT.  The
two halves used to disagree on 361 of 1008 corpus cells, and almost none of
that was arithmetic: it was two different unstated answers to "what happens to
a space at a line break". Neither side had written the answer down, so neither
could be called the defect. The policy is now this, and pdf_metrics.hpp
implements the same one:

    P1  A LINE NEVER BEGINS WITH A SPACE.  Spaces at a break are the
        separator; they belong to neither line. This applies to the first
        line too, so leading indentation is dropped rather than rendered --
        PDF indents by positioning text, not by padding it.

    P2  A SPACE NEVER FORMS A LINE OF ITS OWN.  Follows from P1, and it is
        what a narrow budget used to produce here: a 12pt budget broke
        "a  b" into four lines, two of which were a single space.

    P3  TRAILING SPACES ARE NOT PART OF A LINE'S SPAN.  They render as
        nothing at the end of a line, so a span that includes them reports a
        width the reader will not see.

    P4  THE SEPARATOR THAT ENDS A LINE IS NOT CHARGED TO THE BUDGET.  This
        was a real defect and the only one here that cost layout quality.
        The old loop added each byte's advance and broke when the running
        sum exceeded the budget, so the space AFTER a word had to fit before
        that word could be kept -- and the space after the last word on a
        line is never drawn. At Helvetica 12 on a 120pt line:

              width("The quick brown fox")   = 116.69  fits
              width("The quick brown fox ")  = 120.02  does not

        so "fox" was pushed to the next line to make room for a space that
        would not have been rendered. Every line in the tree was up to one
        space-width narrower than it should have been.

    P5  A NEWLINE FORCES A BREAK, and can produce an empty line, because a
        blank line between paragraphs is content rather than an artefact.

    P6  EMPTY INPUT PRODUCES ZERO LINES, not one empty line. A caller that
        wants "at least one line" can ask for it; a caller counting rendered
        lines cannot subtract one it did not ask for.

    P7  A WORD LONGER THAN THE LINE IS HARD-BROKEN at the widest prefix that
        fits, and at one glyph minimum. A word with no break point leaves
        only an infinite loop, an overflowing line, or a split; the split is
        the one that terminates and stays inside the budget.

  RETURNS THE COUNT THAT WOULD BE PRODUCED, even when the buffer is smaller.
A caller can therefore size and retry, and the overflow is distinguishable
from success by comparing the return against _out_capacity -- rather than by
a truncated result that looks like a short paragraph.

Parameter(s):
  _font:          enum d_pdf_base_font.
  _text:          the bytes.
  _length:        how many.
  _size:          point size.
  _max_width:     the line budget.
  _out:           caller storage; may be NULL to count only.
  _out_capacity:  its size.
Return:
  the number of lines the text wraps to.
*/
size_t
d_pdf_wrap_to_width
(
    int32_t                 _font,
    const char*             _text,
    size_t                  _length,
    double                  _size,
    double                  _max_width,
    struct d_pdf_wrap_line* _out,
    size_t                  _out_capacity
)
{
    size_t _count = 0;
    size_t _i     = 0;
    double _scale = _size / 1000.0;

    if ( (!_text) || (_length == 0u) )
    {
        return 0u;               /* P6 */
    }

    while (_i < _length)
    {
        size_t  _line_start;
        size_t  _j;
        int64_t _content = 0;    /* width of committed content, no trailing sep */
        int64_t _pending = 0;    /* width of the space run after the content    */
        size_t  _content_end;
        size_t  _best_end = (size_t)-1;   /* content end of the last whole word */
        int     _hit_newline = 0;

        /*   P1: a line never begins with a space.  Advancing here is what
           keeps a run of spaces from becoming lines of its own (P2). */
        while ( (_i < _length) && (_text[_i] == ' ') )
        {
            ++_i;
        }

        if (_i >= _length)
        {
            break;               /* trailing spaces produce no line */
        }

        _line_start  = _i;
        _content_end = _i;
        _j           = _i;

        while (_j < _length)
        {
            unsigned char _c = (unsigned char)_text[_j];

            if (_c == '\n')
            {
                _hit_newline = 1;
                break;
            }

            if (_c == ' ')
            {
                /*   A WORD JUST ENDED AND IT FIT, so this is a legal break
                   point.  The space's own advance goes to _pending and is
                   charged only if another word follows on this line -- P4. */
                if (_content_end > _line_start)
                {
                    _best_end = _content_end;
                }

                _pending += (int64_t)d_pdf_glyph_advance_em(_font, (int32_t)_c);
                ++_j;

                continue;
            }

            {
                int64_t _trial = _content + _pending +
                                 (int64_t)d_pdf_glyph_advance_em(_font,
                                                                 (int32_t)_c);

                if (((double)_trial * _scale) > _max_width)
                {
                    break;       /* this glyph does not fit */
                }

                _content     = _trial;
                _pending     = 0;
                _content_end = _j + 1u;
            }

            ++_j;
        }

        if (_hit_newline)
        {
            /*   P3: the span stops at the content, so spaces sitting between
               the last word and the newline are not carried into the line. */
            emit_line_helper(_out, _out_capacity, _count,
                             _text + _line_start, _content_end - _line_start);
            ++_count;
            _i = _j + 1u;        /* consume the newline -- P5 */

            continue;
        }

        if (_j >= _length)
        {
            emit_line_helper(_out, _out_capacity, _count,
                             _text + _line_start, _content_end - _line_start);
            ++_count;
            _i = _length;

            continue;
        }

        /*   Overflow.  Break at the last whole word when there is one, and
           otherwise hard-break the word itself -- P7. */
        if (_best_end != (size_t)-1)
        {
            emit_line_helper(_out, _out_capacity, _count,
                             _text + _line_start, _best_end - _line_start);
            _i = _best_end;
        }
        else
        {
            /*   `_content_end == _line_start` means even the first glyph
               overflowed; taking one anyway is what stops the loop spinning
               on a budget narrower than a single character. */
            size_t _break = (_content_end > _line_start)
                            ? _content_end
                            : (_line_start + 1u);

            emit_line_helper(_out, _out_capacity, _count,
                             _text + _line_start, _break - _line_start);
            _i = _break;
        }

        ++_count;
    }

    return _count;
}
