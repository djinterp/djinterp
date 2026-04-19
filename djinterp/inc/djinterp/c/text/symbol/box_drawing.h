/******************************************************************************
* djinterp [text]                                                box_drawing.h
*
* 
*
* path:      \inc\text\box_drawing.h        
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.07
******************************************************************************/

#ifndef DJINTERP_TEXT_BOX_DRAWING_
#define DJINTERP_TEXT_BOX_DRAWING_


#ifndef D_BOX_ASCII
#  define D_BOX_ASCII 0
#endif  // D_BOX_ASCII

// Canonical names: codepoint-based (unique across Unicode)
#if defined(D_BOX_ASCII)
  // ASCII fallbacks
  #define D_UTF8_U2500 "-"  
  #define D_UTF8_U2502 "|"  
  #define D_UTF8_U250C "+"  
  #define D_UTF8_U2510 "+"  
  #define D_UTF8_U2514 "+"  
  #define D_UTF8_U2518 "+"  
  #define D_UTF8_U251C "+"  
  #define D_UTF8_U2524 "+"  
  #define D_UTF8_U252C "+"  
  #define D_UTF8_U2534 "+"  
  #define D_UTF8_U253C "+"  
  #define D_UTF8_U2550 "="  
  #define D_UTF8_U2551 "||" 
  #define D_UTF8_U2554 "+"  
  #define D_UTF8_U2557 "+"  
  #define D_UTF8_U255A "+"  
  #define D_UTF8_U255D "+"  
  #define D_UTF8_U2560 "+"  
  #define D_UTF8_U2563 "+"  
  #define D_UTF8_U2566 "+"  
  #define D_UTF8_U2569 "+"  
  #define D_UTF8_U256C "+"  
#else
  // UTF-8 byte escapes (source stays ASCII)
  #define D_UTF8_U2500 u8"\xE2\x94\x80"
  #define D_UTF8_U2502 u8"\xE2\x94\x82"
  #define D_UTF8_U250C u8"\xE2\x94\x8C"
  #define D_UTF8_U2510 u8"\xE2\x94\x90"
  #define D_UTF8_U2514 u8"\xE2\x94\x94"
  #define D_UTF8_U2518 u8"\xE2\x94\x98"
  #define D_UTF8_U251C u8"\xE2\x94\x9C"
  #define D_UTF8_U2524 u8"\xE2\x94\xA4"
  #define D_UTF8_U252C u8"\xE2\x94\xAC"
  #define D_UTF8_U2534 u8"\xE2\x94\xB4"
  #define D_UTF8_U253C u8"\xE2\x94\xBC"

  #define D_UTF8_U2550 u8"\xE2\x95\x90"
  #define D_UTF8_U2551 u8"\xE2\x95\x91"
  #define D_UTF8_U2554 u8"\xE2\x95\x94"
  #define D_UTF8_U2557 u8"\xE2\x95\x97"
  #define D_UTF8_U255A u8"\xE2\x95\x9A"
  #define D_UTF8_U255D u8"\xE2\x95\x9D"
  #define D_UTF8_U2560 u8"\xE2\x95\xA0"
  #define D_UTF8_U2563 u8"\xE2\x95\xA3"
  #define D_UTF8_U2566 u8"\xE2\x95\xA6"
  #define D_UTF8_U2569 u8"\xE2\x95\xA9"
  #define D_UTF8_U256C u8"\xE2\x95\xAC"
#endif  // defined(D_BOX_ASCII)

/* ---- Friendly aliases (point to the canonical names) ---- */
#define D_CHAR_BOX_LIGHT_HORIZONTAL   D_UTF8_U2500
#define D_CHAR_BOX_LIGHT_VERTICAL     D_UTF8_U2502
#define D_CHAR_BOX_CORNER_UL          D_UTF8_U250C
#define D_CHAR_BOX_CORNER_UR          D_UTF8_U2510
#define D_CHAR_BOX_CORNER_LL          D_UTF8_U2514
#define D_CHAR_BOX_CORNER_LR          D_UTF8_U2518
#define D_CHAR_BOX_T_RIGHT            D_UTF8_U251C  // connects rightwards
#define D_CHAR_BOX_T_LEFT             D_UTF8_U2524  // connects leftwards 
#define D_CHAR_BOX_T_DOWN             D_UTF8_U252C  // connects downward  
#define D_CHAR_BOX_T_UP               D_UTF8_U2534  // connects upward    
#define D_CHAR_BOX_CROSS              D_UTF8_U253C
			
#define D_CHAR_BOX_DOUBLE_HORIZONTAL  D_UTF8_U2550
#define D_CHAR_BOX_DOUBLE_VERTICAL    D_UTF8_U2551
#define D_CHAR_BOX_DOUBLE_UL          D_UTF8_U2554
#define D_CHAR_BOX_DOUBLE_UR          D_UTF8_U2557
#define D_CHAR_BOX_DOUBLE_LL          D_UTF8_U255A
#define D_CHAR_BOX_DOUBLE_LR          D_UTF8_U255D
#define D_CHAR_BOX_DOUBLE_TR          D_UTF8_U2560
#define D_CHAR_BOX_DOUBLE_TL          D_UTF8_U2563
#define D_CHAR_BOX_DOUBLE_TD          D_UTF8_U2566
#define D_CHAR_BOX_DOUBLE_TU          D_UTF8_U2569
#define D_CHAR_BOX_DOUBLE_CROSS       D_UTF8_U256C


#endif	// DJINTERP_TEXT_BOX_DRAWING_