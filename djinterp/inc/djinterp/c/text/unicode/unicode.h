/******************************************************************************
* djinterp [text]                                                    unicode.h
*
* Unicode building blocks (utf-8).
*   This header defines commonly used unicode codepoints as utf-8 string
* constants, including regional indicator letters, variation selectors,
* joiner/format controls, and selected combining marks used in emoji/text
* sequences. These constants are intended to be composed by higher-level
* modules (e.g. emoji flags, keycaps, and zwj sequences).
* 
*
* path:      \inc\text\unicode\unicode.h             
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.08
******************************************************************************/

#ifndef DJINTERP_TEXT_UNICODE_
#define DJINTERP_TEXT_UNICODE_


// regional Indicator Letters (UTF-8) 
#define D_UNICODE_RI_A  "\xF0\x9F\x87\xA6"          // U+1F1E6
#define D_UNICODE_RI_B  "\xF0\x9F\x87\xA7"          // U+1F1E7
#define D_UNICODE_RI_C  "\xF0\x9F\x87\xA8"          // U+1F1E8
#define D_UNICODE_RI_D  "\xF0\x9F\x87\xA9"          // U+1F1E9
#define D_UNICODE_RI_E  "\xF0\x9F\x87\xAA"          // U+1F1EA
#define D_UNICODE_RI_F  "\xF0\x9F\x87\xAB"          // U+1F1EB
#define D_UNICODE_RI_G  "\xF0\x9F\x87\xAC"          // U+1F1EC
#define D_UNICODE_RI_H  "\xF0\x9F\x87\xAD"          // U+1F1ED
#define D_UNICODE_RI_I  "\xF0\x9F\x87\xAE"          // U+1F1EE
#define D_UNICODE_RI_J  "\xF0\x9F\x87\xAF"          // U+1F1EF
#define D_UNICODE_RI_K  "\xF0\x9F\x87\xB0"          // U+1F1F0
#define D_UNICODE_RI_L  "\xF0\x9F\x87\xB1"          // U+1F1F1
#define D_UNICODE_RI_M  "\xF0\x9F\x87\xB2"          // U+1F1F2
#define D_UNICODE_RI_N  "\xF0\x9F\x87\xB3"          // U+1F1F3
#define D_UNICODE_RI_O  "\xF0\x9F\x87\xB4"          // U+1F1F4
#define D_UNICODE_RI_P  "\xF0\x9F\x87\xB5"          // U+1F1F5
#define D_UNICODE_RI_Q  "\xF0\x9F\x87\xB6"          // U+1F1F6
#define D_UNICODE_RI_R  "\xF0\x9F\x87\xB7"          // U+1F1F7
#define D_UNICODE_RI_S  "\xF0\x9F\x87\xB8"          // U+1F1F8
#define D_UNICODE_RI_T  "\xF0\x9F\x87\xB9"          // U+1F1F9
#define D_UNICODE_RI_U  "\xF0\x9F\x87\xBA"          // U+1F1FA
#define D_UNICODE_RI_V  "\xF0\x9F\x87\xBB"          // U+1F1FB
#define D_UNICODE_RI_W  "\xF0\x9F\x87\xBC"          // U+1F1FC
#define D_UNICODE_RI_X  "\xF0\x9F\x87\xBD"          // U+1F1FD
#define D_UNICODE_RI_Y  "\xF0\x9F\x87\xBE"          // U+1F1FE
#define D_UNICODE_RI_Z  "\xF0\x9F\x87\xBF"          // U+1F1FF

// variation selectors

// D_UNICODE_VS15
//   constant: variation selector-15 (requests text presentation when 
// applicable).
#define D_UNICODE_VS15           "\xEF\xB8\x8E"     // U+FE0E

// D_UNICODE_VS16
//   constant: variation selector-16 (requests emoji presentation when
// applicable).
#define D_UNICODE_VS16           "\xEF\xB8\x8F"     // U+FE0F

// joiners/format controls

// D_UNICODE_ZWJ
//   constant: zero width joiner (used to form zwj sequences).
#define D_UNICODE_ZWJ            "\xE2\x80\x8D"     // U+200D

// D_UNICODE_ZWNJ
//   constant: zero width non-joiner (prevents joining in some scripts).
#define D_UNICODE_ZWNJ           "\xE2\x80\x8C"     // U+200C

// combining marks / modifiers

// D_UNICODE_KEYCAP_SUFFIX
//   constant: combining enclosing keycap.
#define D_UNICODE_KEYCAP_SUFFIX  "\xE2\x83\xA3"     // U+20E3


#endif	// DJINTERP_TEXT_UNICODE_