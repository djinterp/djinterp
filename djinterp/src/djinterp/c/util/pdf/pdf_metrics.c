/*******************************************************************************
* djinterp [c]                                                     pdf_metrics.c
*
* Glyph and string widths for the standard-14 faces.
*   The ten tabulated faces' advances were extracted from pdf_metrics.hpp by
* a script that asserts 256 values per face; the four Courier faces are
* computed. Strings are summed in integer per-mille-em units and scaled once,
* as pdf_metrics.h specifies.
*
*
* path:      /src/djinterp/c/util/pdf/pdf_metrics.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/pdf/pdf_metrics.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // memcpy, strlen
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int32_t


// the advance tables, per 1000 em, indexed by byte (WinAnsi)
static const short d_internal_pdf_helvetica_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x00
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x08
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x10
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x18
     278,  278,  355,  556,  556,  889,  667,  191,  // 0x20
     333,  333,  389,  584,  278,  333,  278,  278,  // 0x28
     556,  556,  556,  556,  556,  556,  556,  556,  // 0x30
     556,  556,  278,  278,  584,  584,  584,  556,  // 0x38
    1015,  667,  667,  722,  722,  667,  611,  778,  // 0x40
     722,  278,  500,  667,  556,  833,  722,  778,  // 0x48
     667,  778,  722,  667,  611,  722,  667,  944,  // 0x50
     667,  667,  611,  278,  278,  278,  469,  556,  // 0x58
     333,  556,  556,  500,  556,  556,  278,  556,  // 0x60
     556,  222,  222,  500,  222,  833,  556,  556,  // 0x68
     556,  556,  333,  500,  278,  556,  500,  722,  // 0x70
     500,  500,  500,  334,  260,  334,  584,  350,  // 0x78
     556,  350,  222,  556,  333, 1000,  556,  556,  // 0x80
     333, 1000,  667,  333, 1000,  350,  611,  350,  // 0x88
     350,  222,  222,  333,  333,  350,  556, 1000,  // 0x90
     333, 1000,  500,  333,  944,  350,  500,  667,  // 0x98
     278,  333,  556,  556,  556,  556,  260,  556,  // 0xA0
     333,  737,  370,  556,  584,  333,  737,  333,  // 0xA8
     400,  584,  333,  333,  333,  556,  537,  278,  // 0xB0
     333,  333,  365,  556,  834,  834,  834,  611,  // 0xB8
     667,  667,  667,  667,  667,  667, 1000,  722,  // 0xC0
     667,  667,  667,  667,  278,  278,  278,  278,  // 0xC8
     722,  722,  778,  778,  778,  778,  778,  584,  // 0xD0
     778,  722,  722,  722,  722,  667,  667,  611,  // 0xD8
     556,  556,  556,  556,  556,  556,  889,  500,  // 0xE0
     556,  556,  556,  556,  278,  278,  278,  278,  // 0xE8
     556,  556,  556,  556,  556,  556,  556,  584,  // 0xF0
     611,  556,  556,  556,  556,  500,  556,  500   // 0xF8
};

static const short d_internal_pdf_helvetica_bold_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x00
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x08
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x10
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x18
     278,  333,  474,  556,  556,  889,  722,  238,  // 0x20
     333,  333,  389,  584,  278,  333,  278,  278,  // 0x28
     556,  556,  556,  556,  556,  556,  556,  556,  // 0x30
     556,  556,  333,  333,  584,  584,  584,  611,  // 0x38
     975,  722,  722,  722,  722,  667,  611,  778,  // 0x40
     722,  278,  556,  722,  611,  833,  722,  778,  // 0x48
     667,  778,  722,  667,  611,  722,  667,  944,  // 0x50
     667,  667,  611,  333,  278,  333,  584,  556,  // 0x58
     333,  556,  611,  556,  611,  556,  333,  611,  // 0x60
     611,  278,  278,  556,  278,  889,  611,  611,  // 0x68
     611,  611,  389,  556,  333,  611,  556,  778,  // 0x70
     556,  556,  500,  389,  280,  389,  584,  350,  // 0x78
     556,  350,  278,  556,  500, 1000,  556,  556,  // 0x80
     333, 1000,  667,  333, 1000,  350,  611,  350,  // 0x88
     350,  278,  278,  500,  500,  350,  556, 1000,  // 0x90
     333, 1000,  556,  333,  944,  350,  500,  667,  // 0x98
     278,  333,  556,  556,  556,  556,  280,  556,  // 0xA0
     333,  737,  370,  556,  584,  333,  737,  333,  // 0xA8
     400,  584,  333,  333,  333,  611,  556,  278,  // 0xB0
     333,  333,  365,  556,  834,  834,  834,  611,  // 0xB8
     722,  722,  722,  722,  722,  722, 1000,  722,  // 0xC0
     667,  667,  667,  667,  278,  278,  278,  278,  // 0xC8
     722,  722,  778,  778,  778,  778,  778,  584,  // 0xD0
     778,  722,  722,  722,  722,  667,  667,  611,  // 0xD8
     556,  556,  556,  556,  556,  556,  889,  556,  // 0xE0
     556,  556,  556,  556,  278,  278,  278,  278,  // 0xE8
     611,  611,  611,  611,  611,  611,  611,  584,  // 0xF0
     611,  611,  611,  611,  611,  556,  611,  556   // 0xF8
};

static const short d_internal_pdf_helvetica_oblique_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x00
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x08
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x10
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x18
     278,  278,  355,  556,  556,  889,  667,  191,  // 0x20
     333,  333,  389,  584,  278,  333,  278,  278,  // 0x28
     556,  556,  556,  556,  556,  556,  556,  556,  // 0x30
     556,  556,  278,  278,  584,  584,  584,  556,  // 0x38
    1015,  667,  667,  722,  722,  667,  611,  778,  // 0x40
     722,  278,  500,  667,  556,  833,  722,  778,  // 0x48
     667,  778,  722,  667,  611,  722,  667,  944,  // 0x50
     667,  667,  611,  278,  278,  278,  469,  556,  // 0x58
     333,  556,  556,  500,  556,  556,  278,  556,  // 0x60
     556,  222,  222,  500,  222,  833,  556,  556,  // 0x68
     556,  556,  333,  500,  278,  556,  500,  722,  // 0x70
     500,  500,  500,  334,  260,  334,  584,  350,  // 0x78
     556,  350,  222,  556,  333, 1000,  556,  556,  // 0x80
     333, 1000,  667,  333, 1000,  350,  611,  350,  // 0x88
     350,  222,  222,  333,  333,  350,  556, 1000,  // 0x90
     333, 1000,  500,  333,  944,  350,  500,  667,  // 0x98
     278,  333,  556,  556,  556,  556,  260,  556,  // 0xA0
     333,  737,  370,  556,  584,  333,  737,  333,  // 0xA8
     400,  584,  333,  333,  333,  556,  537,  278,  // 0xB0
     333,  333,  365,  556,  834,  834,  834,  611,  // 0xB8
     667,  667,  667,  667,  667,  667, 1000,  722,  // 0xC0
     667,  667,  667,  667,  278,  278,  278,  278,  // 0xC8
     722,  722,  778,  778,  778,  778,  778,  584,  // 0xD0
     778,  722,  722,  722,  722,  667,  667,  611,  // 0xD8
     556,  556,  556,  556,  556,  556,  889,  500,  // 0xE0
     556,  556,  556,  556,  278,  278,  278,  278,  // 0xE8
     556,  556,  556,  556,  556,  556,  556,  584,  // 0xF0
     611,  556,  556,  556,  556,  500,  556,  500   // 0xF8
};

static const short d_internal_pdf_helvetica_bold_oblique_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x00
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x08
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x10
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x18
     278,  333,  474,  556,  556,  889,  722,  238,  // 0x20
     333,  333,  389,  584,  278,  333,  278,  278,  // 0x28
     556,  556,  556,  556,  556,  556,  556,  556,  // 0x30
     556,  556,  333,  333,  584,  584,  584,  611,  // 0x38
     975,  722,  722,  722,  722,  667,  611,  778,  // 0x40
     722,  278,  556,  722,  611,  833,  722,  778,  // 0x48
     667,  778,  722,  667,  611,  722,  667,  944,  // 0x50
     667,  667,  611,  333,  278,  333,  584,  556,  // 0x58
     333,  556,  611,  556,  611,  556,  333,  611,  // 0x60
     611,  278,  278,  556,  278,  889,  611,  611,  // 0x68
     611,  611,  389,  556,  333,  611,  556,  778,  // 0x70
     556,  556,  500,  389,  280,  389,  584,  350,  // 0x78
     556,  350,  278,  556,  500, 1000,  556,  556,  // 0x80
     333, 1000,  667,  333, 1000,  350,  611,  350,  // 0x88
     350,  278,  278,  500,  500,  350,  556, 1000,  // 0x90
     333, 1000,  556,  333,  944,  350,  500,  667,  // 0x98
     278,  333,  556,  556,  556,  556,  280,  556,  // 0xA0
     333,  737,  370,  556,  584,  333,  737,  333,  // 0xA8
     400,  584,  333,  333,  333,  611,  556,  278,  // 0xB0
     333,  333,  365,  556,  834,  834,  834,  611,  // 0xB8
     722,  722,  722,  722,  722,  722, 1000,  722,  // 0xC0
     667,  667,  667,  667,  278,  278,  278,  278,  // 0xC8
     722,  722,  778,  778,  778,  778,  778,  584,  // 0xD0
     778,  722,  722,  722,  722,  667,  667,  611,  // 0xD8
     556,  556,  556,  556,  556,  556,  889,  556,  // 0xE0
     556,  556,  556,  556,  278,  278,  278,  278,  // 0xE8
     611,  611,  611,  611,  611,  611,  611,  584,  // 0xF0
     611,  611,  611,  611,  611,  556,  611,  556   // 0xF8
};

static const short d_internal_pdf_times_roman_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x00
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x08
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x10
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x18
     250,  333,  408,  500,  500,  833,  778,  180,  // 0x20
     333,  333,  500,  564,  250,  333,  250,  278,  // 0x28
     500,  500,  500,  500,  500,  500,  500,  500,  // 0x30
     500,  500,  278,  278,  564,  564,  564,  444,  // 0x38
     921,  722,  667,  667,  722,  611,  556,  722,  // 0x40
     722,  333,  389,  722,  611,  889,  722,  722,  // 0x48
     556,  722,  667,  556,  611,  722,  722,  944,  // 0x50
     722,  722,  611,  333,  278,  333,  469,  500,  // 0x58
     333,  444,  500,  444,  500,  444,  333,  500,  // 0x60
     500,  278,  278,  500,  278,  778,  500,  500,  // 0x68
     500,  500,  333,  389,  278,  500,  500,  722,  // 0x70
     500,  500,  444,  480,  200,  480,  541,  350,  // 0x78
     500,  350,  333,  500,  444, 1000,  500,  500,  // 0x80
     333, 1000,  556,  333,  889,  350,  611,  350,  // 0x88
     350,  333,  333,  444,  444,  350,  500, 1000,  // 0x90
     333,  980,  389,  333,  722,  350,  444,  722,  // 0x98
     250,  333,  500,  500,  500,  500,  200,  500,  // 0xA0
     333,  760,  276,  500,  564,  333,  760,  333,  // 0xA8
     400,  564,  300,  300,  333,  500,  453,  250,  // 0xB0
     333,  300,  310,  500,  750,  750,  750,  444,  // 0xB8
     722,  722,  722,  722,  722,  722,  889,  667,  // 0xC0
     611,  611,  611,  611,  333,  333,  333,  333,  // 0xC8
     722,  722,  722,  722,  722,  722,  722,  564,  // 0xD0
     722,  722,  722,  722,  722,  722,  556,  500,  // 0xD8
     444,  444,  444,  444,  444,  444,  667,  444,  // 0xE0
     444,  444,  444,  444,  278,  278,  278,  278,  // 0xE8
     500,  500,  500,  500,  500,  500,  500,  564,  // 0xF0
     500,  500,  500,  500,  500,  500,  500,  500   // 0xF8
};

static const short d_internal_pdf_times_bold_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x00
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x08
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x10
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x18
     250,  333,  555,  500,  500, 1000,  833,  278,  // 0x20
     333,  333,  500,  570,  250,  333,  250,  278,  // 0x28
     500,  500,  500,  500,  500,  500,  500,  500,  // 0x30
     500,  500,  333,  333,  570,  570,  570,  500,  // 0x38
     930,  722,  667,  722,  722,  667,  611,  778,  // 0x40
     778,  389,  500,  778,  667,  944,  722,  778,  // 0x48
     611,  778,  722,  556,  667,  722,  722, 1000,  // 0x50
     722,  722,  667,  333,  278,  333,  581,  500,  // 0x58
     333,  500,  556,  444,  556,  444,  333,  500,  // 0x60
     556,  278,  333,  556,  278,  833,  556,  500,  // 0x68
     556,  556,  444,  389,  333,  556,  500,  722,  // 0x70
     500,  500,  444,  394,  220,  394,  520,  350,  // 0x78
     500,  350,  333,  500,  500, 1000,  500,  500,  // 0x80
     333, 1000,  556,  333, 1000,  350,  667,  350,  // 0x88
     350,  333,  333,  500,  500,  350,  500, 1000,  // 0x90
     333, 1000,  389,  333,  722,  350,  444,  722,  // 0x98
     250,  333,  500,  500,  500,  500,  220,  500,  // 0xA0
     333,  747,  300,  500,  570,  333,  747,  333,  // 0xA8
     400,  570,  300,  300,  333,  556,  540,  250,  // 0xB0
     333,  300,  330,  500,  750,  750,  750,  500,  // 0xB8
     722,  722,  722,  722,  722,  722, 1000,  722,  // 0xC0
     667,  667,  667,  667,  389,  389,  389,  389,  // 0xC8
     722,  722,  778,  778,  778,  778,  778,  570,  // 0xD0
     778,  722,  722,  722,  722,  722,  611,  556,  // 0xD8
     500,  500,  500,  500,  500,  500,  722,  444,  // 0xE0
     444,  444,  444,  444,  278,  278,  278,  278,  // 0xE8
     500,  556,  500,  500,  500,  500,  500,  570,  // 0xF0
     500,  556,  556,  556,  556,  500,  556,  500   // 0xF8
};

static const short d_internal_pdf_times_italic_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x00
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x08
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x10
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x18
     250,  333,  420,  500,  500,  833,  778,  214,  // 0x20
     333,  333,  500,  675,  250,  333,  250,  278,  // 0x28
     500,  500,  500,  500,  500,  500,  500,  500,  // 0x30
     500,  500,  333,  333,  675,  675,  675,  500,  // 0x38
     920,  611,  611,  667,  722,  611,  611,  722,  // 0x40
     722,  333,  444,  667,  556,  833,  667,  722,  // 0x48
     611,  722,  611,  500,  556,  722,  611,  833,  // 0x50
     611,  556,  556,  389,  278,  389,  422,  500,  // 0x58
     333,  500,  500,  444,  500,  444,  278,  500,  // 0x60
     500,  278,  278,  444,  278,  722,  500,  500,  // 0x68
     500,  500,  389,  389,  278,  500,  444,  667,  // 0x70
     444,  444,  389,  400,  275,  400,  541,  350,  // 0x78
     500,  350,  333,  500,  556,  889,  500,  500,  // 0x80
     333, 1000,  500,  333,  944,  350,  556,  350,  // 0x88
     350,  333,  333,  556,  556,  350,  500,  889,  // 0x90
     333,  980,  389,  333,  667,  350,  389,  556,  // 0x98
     250,  389,  500,  500,  500,  500,  275,  500,  // 0xA0
     333,  760,  276,  500,  675,  333,  760,  333,  // 0xA8
     400,  675,  300,  300,  333,  500,  523,  250,  // 0xB0
     333,  300,  310,  500,  750,  750,  750,  500,  // 0xB8
     611,  611,  611,  611,  611,  611,  889,  667,  // 0xC0
     611,  611,  611,  611,  333,  333,  333,  333,  // 0xC8
     722,  667,  722,  722,  722,  722,  722,  675,  // 0xD0
     722,  722,  722,  722,  722,  556,  611,  500,  // 0xD8
     500,  500,  500,  500,  500,  500,  667,  444,  // 0xE0
     444,  444,  444,  444,  278,  278,  278,  278,  // 0xE8
     500,  500,  500,  500,  500,  500,  500,  675,  // 0xF0
     500,  500,  500,  500,  500,  444,  500,  444   // 0xF8
};

static const short d_internal_pdf_times_bold_italic_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x00
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x08
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x10
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x18
     250,  389,  555,  500,  500,  833,  778,  278,  // 0x20
     333,  333,  500,  570,  250,  333,  250,  278,  // 0x28
     500,  500,  500,  500,  500,  500,  500,  500,  // 0x30
     500,  500,  333,  333,  570,  570,  570,  500,  // 0x38
     832,  667,  667,  667,  722,  667,  667,  722,  // 0x40
     778,  389,  500,  667,  611,  889,  722,  722,  // 0x48
     611,  722,  667,  556,  611,  722,  667,  889,  // 0x50
     667,  611,  611,  333,  278,  333,  570,  500,  // 0x58
     333,  500,  500,  444,  500,  444,  333,  500,  // 0x60
     556,  278,  278,  500,  278,  778,  556,  500,  // 0x68
     500,  500,  389,  389,  278,  556,  444,  667,  // 0x70
     500,  444,  389,  348,  220,  348,  570,  350,  // 0x78
     500,  350,  333,  500,  500, 1000,  500,  500,  // 0x80
     333, 1000,  556,  333,  944,  350,  611,  350,  // 0x88
     350,  333,  333,  500,  500,  350,  500, 1000,  // 0x90
     333, 1000,  389,  333,  722,  350,  389,  611,  // 0x98
     250,  389,  500,  500,  500,  500,  220,  500,  // 0xA0
     333,  747,  266,  500,  606,  333,  747,  333,  // 0xA8
     400,  570,  300,  300,  333,  576,  500,  250,  // 0xB0
     333,  300,  300,  500,  750,  750,  750,  500,  // 0xB8
     667,  667,  667,  667,  667,  667,  944,  667,  // 0xC0
     667,  667,  667,  667,  389,  389,  389,  389,  // 0xC8
     722,  722,  722,  722,  722,  722,  722,  570,  // 0xD0
     722,  722,  722,  722,  722,  611,  611,  500,  // 0xD8
     500,  500,  500,  500,  500,  500,  722,  444,  // 0xE0
     444,  444,  444,  444,  278,  278,  278,  278,  // 0xE8
     500,  556,  500,  500,  500,  500,  500,  570,  // 0xF0
     500,  556,  556,  556,  556,  444,  500,  444   // 0xF8
};

static const short d_internal_pdf_symbol_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x00
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x08
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x10
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x18
     250,  333,  713,  500,  549,  833,  778,  439,  // 0x20
     333,  333,  500,  549,  250,  549,  250,  278,  // 0x28
     500,  500,  500,  500,  500,  500,  500,  500,  // 0x30
     500,  500,  278,  278,  549,  549,  549,  444,  // 0x38
     549,  722,  667,  722,  612,  611,  763,  603,  // 0x40
     722,  333,  631,  722,  686,  889,  722,  722,  // 0x48
     768,  741,  556,  592,  611,  690,  439,  768,  // 0x50
     645,  795,  611,  333,  863,  333,  658,  500,  // 0x58
     500,  631,  549,  549,  494,  439,  521,  411,  // 0x60
     603,  329,  603,  549,  549,  576,  521,  549,  // 0x68
     549,  521,  549,  603,  439,  576,  713,  686,  // 0x70
     493,  686,  494,  480,  200,  480,  549,    0,  // 0x78
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x80
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x88
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x90
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x98
     750,  620,  247,  549,  167,  713,  500,  753,  // 0xA0
     753,  753,  753, 1042,  987,  603,  987,  603,  // 0xA8
     400,  549,  411,  549,  549,  713,  494,  460,  // 0xB0
     549,  549,  549,  549, 1000,  603, 1000,  658,  // 0xB8
     823,  686,  795,  987,  768,  768,  823,  768,  // 0xC0
     768,  713,  713,  713,  713,  713,  713,  713,  // 0xC8
     768,  713,  790,  790,  890,  823,  549,  250,  // 0xD0
     713,  603,  603, 1042,  987,  603,  987,  603,  // 0xD8
     494,  329,  790,  790,  786,  713,  384,  384,  // 0xE0
     384,  384,  384,  384,  494,  494,  494,  494,  // 0xE8
       0,  329,  274,  686,  686,  686,  384,  384,  // 0xF0
     384,  384,  384,  384,  494,  494,  494,    0   // 0xF8
};

static const short d_internal_pdf_zapf_dingbats_w[256] =
{
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x00
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x08
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x10
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x18
     278,  974,  961,  974,  980,  719,  789,  790,  // 0x20
     791,  690,  960,  939,  549,  855,  911,  933,  // 0x28
     911,  945,  974,  755,  846,  762,  761,  571,  // 0x30
     677,  763,  760,  759,  754,  494,  552,  537,  // 0x38
     577,  692,  786,  788,  788,  790,  793,  794,  // 0x40
     816,  823,  789,  841,  823,  833,  816,  831,  // 0x48
     923,  744,  723,  749,  790,  792,  695,  776,  // 0x50
     768,  792,  759,  707,  708,  682,  701,  826,  // 0x58
     815,  789,  789,  707,  687,  696,  689,  786,  // 0x60
     787,  713,  791,  785,  791,  873,  761,  762,  // 0x68
     762,  759,  759,  892,  892,  788,  784,  438,  // 0x70
     138,  277,  415,  392,  392,  668,  668,    0,  // 0x78
     390,  390,  317,  317,  276,  276,  509,  509,  // 0x80
     410,  410,  234,  234,  334,  334,    0,    0,  // 0x88
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x90
       0,    0,    0,    0,    0,    0,    0,    0,  // 0x98
       0,  732,  544,  544,  910,  667,  760,  760,  // 0xA0
     776,  595,  694,  626,  788,  788,  788,  788,  // 0xA8
     788,  788,  788,  788,  788,  788,  788,  788,  // 0xB0
     788,  788,  788,  788,  788,  788,  788,  788,  // 0xB8
     788,  788,  788,  788,  788,  788,  788,  788,  // 0xC0
     788,  788,  788,  788,  788,  788,  788,  788,  // 0xC8
     788,  788,  788,  788,  894,  838, 1016,  458,  // 0xD0
     748,  924,  748,  918,  927,  928,  928,  834,  // 0xD8
     873,  828,  924,  924,  917,  930,  931,  463,  // 0xE0
     883,  836,  836,  867,  867,  696,  696,  874,  // 0xE8
       0,  874,  760,  946,  771,  865,  771,  888,  // 0xF0
     967,  888,  831,  873,  927,  970,  918,    0   // 0xF8
};


const short*
d_pdf_width_table_for(
    int32_t _font
)
{
    switch (_font)
    {
        case D_PDF_FONT_HELVETICA:
            return d_internal_pdf_helvetica_w;
        case D_PDF_FONT_HELVETICA_BOLD:
            return d_internal_pdf_helvetica_bold_w;
        case D_PDF_FONT_HELVETICA_OBLIQUE:
            return d_internal_pdf_helvetica_oblique_w;
        case D_PDF_FONT_HELVETICA_BOLD_OBLIQUE:
            return d_internal_pdf_helvetica_bold_oblique_w;
        case D_PDF_FONT_TIMES_ROMAN:
            return d_internal_pdf_times_roman_w;
        case D_PDF_FONT_TIMES_BOLD:
            return d_internal_pdf_times_bold_w;
        case D_PDF_FONT_TIMES_ITALIC:
            return d_internal_pdf_times_italic_w;
        case D_PDF_FONT_TIMES_BOLD_ITALIC:
            return d_internal_pdf_times_bold_italic_w;
        case D_PDF_FONT_SYMBOL:
            return d_internal_pdf_symbol_w;
        case D_PDF_FONT_ZAPF_DINGBATS:
            return d_internal_pdf_zapf_dingbats_w;
        default:
            return NULL;
    }
}

/*
d_internal_pdf_advance
  One byte's advance: the table's entry, or Courier's rule -- nothing for the
control range below 0x20, D_PDF_COURIER_ADVANCE otherwise -- for Courier and
any face without a table.
*/
static long
d_internal_pdf_advance(
    const short*  _table,
    unsigned char _code
)
{
    if (_table)
    {
        return (long)_table[_code];
    }

    return (_code < 0x20u) ? 0L : (long)D_PDF_COURIER_ADVANCE;
}

/*
d_pdf_glyph_advance_em
  A code outside 0..255 names no byte, and has no advance.
*/
int32_t
d_pdf_glyph_advance_em(
    int32_t _font,
    int32_t _code
)
{
    if ( (_code < 0) ||
         (_code > 255) )
    {
        return 0;
    }

    return (int32_t)d_internal_pdf_advance(d_pdf_width_table_for(_font),
                                           (unsigned char)_code);
}

double
d_pdf_glyph_width(
    int32_t _font,
    int32_t _code,
    double  _size
)
{
    return (double)d_pdf_glyph_advance_em(_font, _code) * _size / 1000.0;
}

double
d_pdf_text_width(
    int32_t     _font,
    const char* _text,
    size_t      _length,
    double      _size
)
{
    const short* const table = d_pdf_width_table_for(_font);
    long               total = 0L;

    // no text, no width
    if (!_text)
    {
        return 0.0;
    }

    // sum in per-mille-em, scale once
    for (size_t i = 0u; i < _length; ++i)
    {
        total += d_internal_pdf_advance(table, (unsigned char)_text[i]);
    }

    return (double)total * _size / 1000.0;
}

double
d_pdf_text_width_z(
    int32_t     _font,
    const char* _text,
    double      _size
)
{
    return (_text != NULL)
        ? d_pdf_text_width(_font, _text, strlen(_text), _size)
        : 0.0;
}

/*
d_pdf_fit_char_count
  The longest prefix whose width does not exceed _max_width. The running sum
stays in per-mille-em and is scaled at each comparison, so the answer agrees
with d_pdf_text_width on the prefix it returns. The C++ accumulates each
glyph's scaled width instead, and so can land on the other side of a width
that a prefix meets exactly: a differential over 5,600 random strings found
4 such boundary cases (and 1 truncation that followed from one), and no other
difference in widths, advances or wrapping. This is the accurate side of the
divergence pdf_metrics.h records for the width model.
*/
size_t
d_pdf_fit_char_count(
    int32_t     _font,
    const char* _text,
    size_t      _length,
    double      _size,
    double      _max_width
)
{
    const short* const table = d_pdf_width_table_for(_font);
    long               total = 0L;

    // nothing to fit
    if (!_text)
    {
        return 0u;
    }

    // add bytes while the prefix still fits
    for (size_t i = 0u; i < _length; ++i)
    {
        const long next = total +
            d_internal_pdf_advance(table, (unsigned char)_text[i]);

        if (((double)next * _size / 1000.0) > _max_width)
        {
            return i;
        }

        total = next;
    }

    return _length;
}

/*
d_pdf_truncate_ellipsis
  The C++ result: the text itself when it fits; otherwise the longest prefix
that fits beside "...", followed by it; and empty when even "..." does not
fit. The return is the result's length. It is written, NUL-terminated, only
when `_out` has room for all of it (`_out_capacity` greater than the length);
otherwise nothing is written and the length says how much room to give.
*/
size_t
d_pdf_truncate_ellipsis(
    int32_t     _font,
    const char* _text,
    size_t      _length,
    double      _size,
    double      _max_width,
    char*       _out,
    size_t      _out_capacity
)
{
    static const char ellipsis[] = "...";
    const size_t      dots       = sizeof(ellipsis) - 1u;
    size_t            keep       = 0u;
    size_t            tail       = 0u;

    // the text, when it fits whole
    if ( (_text != NULL) &&
         (d_pdf_text_width(_font, _text, _length, _size) <= _max_width) )
    {
        keep = _length;
    }
    // a prefix and the ellipsis, when the ellipsis itself fits
    else if (d_pdf_text_width(_font, ellipsis, dots, _size) <= _max_width)
    {
        keep = d_pdf_fit_char_count(_font,
                                    _text,
                                    _length,
                                    _size,
                                    _max_width -
                                        d_pdf_text_width(_font,
                                                         ellipsis,
                                                         dots,
                                                         _size));
        tail = dots;
    }

    // write it all, or nothing
    if ( (_out != NULL) &&
         (_out_capacity > (keep + tail)) )
    {
        if (keep > 0u)
        {
            memcpy(_out, _text, keep);
        }

        memcpy(_out + keep, ellipsis, tail);
        _out[keep + tail] = '\0';
    }

    return keep + tail;
}

/*
d_internal_pdf_emit
  Records one line if there is room, and counts it either way.
*/
static void
d_internal_pdf_emit(
    struct d_pdf_wrap_line* _out,
    size_t                  _out_capacity,
    size_t*                 _count,
    const char*             _begin,
    size_t                  _length
)
{
    if ( (_out != NULL) &&
         (*_count < _out_capacity) )
    {
        _out[*_count].begin  = _begin;
        _out[*_count].length = _length;
    }

    ++*_count;

    return;
}

/*
d_pdf_wrap_to_width
  The C++ wrap, producing spans instead of strings. Words are split at each
space and at newlines; a word is added to the current line when the line
still fits with it, a newline ends the line, and a word wider than the
whole width is broken into pieces of at most the width (at least one byte
each). Because the C++ rejoins words with the single spaces that separated
them, each of its lines is a contiguous run of the input, which is what
makes a span the same answer.
  As in the C++, a line holds no leading separator: a word that starts a
line starts it, so the spaces before it go with the line before.
  The return is the number of lines, at least one (an empty text is one
empty line); the first `_out_capacity` of them are written to `_out`, which
may be NULL to count.
*/
size_t
d_pdf_wrap_to_width(
    int32_t                 _font,
    const char*             _text,
    size_t                  _length,
    double                  _size,
    double                  _max_width,
    struct d_pdf_wrap_line* _out,
    size_t                  _out_capacity
)
{
    const char* const text       = (_text != NULL) ? _text : "";
    const size_t      length     = (_text != NULL) ? _length : 0u;
    size_t            count      = 0u;
    size_t            pos        = 0u;
    size_t            line_begin = 0u;
    size_t            line_end   = 0u;
    int               has_line   = 0;

    // one word per pass: text[pos, end), ended by a space, newline or end
    for (;;)
    {
        size_t end = pos;

        while ( (end < length) &&
                (text[end] != ' ') &&
                (text[end] != '\n') )
        {
            ++end;
        }

        const int at_newline = ( (end < length) && (text[end] == '\n') );

        // a word too wide for any line is broken into pieces of its own
        if (d_pdf_text_width(_font, text + pos, end - pos, _size) > _max_width)
        {
            if (has_line)
            {
                d_internal_pdf_emit(_out, _out_capacity, &count,
                                    text + line_begin, line_end - line_begin);
                has_line = 0;
            }

            for (size_t at = pos; at < end;)
            {
                size_t fit = d_pdf_fit_char_count(_font, text + at, end - at,
                                                  _size, _max_width);

                if (fit == 0u)
                {
                    fit = 1u;
                }

                d_internal_pdf_emit(_out, _out_capacity, &count,
                                    text + at, fit);
                at += fit;
            }
        }
        // otherwise it starts the line, or joins it while the line still fits
        else if (!has_line)
        {
            line_begin = pos;
            line_end   = end;
            has_line   = (end > pos);
        }
        else if (d_pdf_text_width(_font, text + line_begin, end - line_begin,
                                  _size) > _max_width)
        {
            d_internal_pdf_emit(_out, _out_capacity, &count,
                                text + line_begin, line_end - line_begin);
            line_begin = pos;
            line_end   = end;
            has_line   = (end > pos);
        }
        else
        {
            line_end = end;
        }

        // a newline ends the line, even an empty one
        if (at_newline)
        {
            d_internal_pdf_emit(_out, _out_capacity, &count,
                                text + (has_line ? line_begin : pos),
                                has_line ? (line_end - line_begin) : 0u);
            has_line = 0;
        }

        if (end >= length)
        {
            break;
        }

        pos = end + 1u;
    }

    // what remains is the last line
    if (has_line)
    {
        d_internal_pdf_emit(_out, _out_capacity, &count,
                            text + line_begin, line_end - line_begin);
    }

    // an empty text is one empty line
    if (count == 0u)
    {
        d_internal_pdf_emit(_out, _out_capacity, &count, text, 0u);
    }

    return count;
}
