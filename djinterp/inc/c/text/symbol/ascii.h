/******************************************************************************
* djinterp [text]                                                      ascii.h
*
* 
* 
*
* path:      \inc\text\ascii.h             
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.08
******************************************************************************/

#ifndef DJINTERP_TEXT_SYMBOLS_ASCII_
#define DJINTERP_TEXT_SYMBOLS_ASCII_

#include "../../djinterp.h"


// D_ASCII_IS_NUMERIC
//   macro: evaluates to true if character is a numeric digit (0-9).
#define D_ASCII_IS_NUMERIC(c)        \
	( ( (c) >= '0') &&               \
      ( (c) <= '9') )

// D_ASCII_IS_ALPHA_UPPER
//   macro: evaluates to true if character is an uppercase letter (A-Z).
#define D_ASCII_IS_ALPHA_UPPER(c)    \
	( ( (c) >= 'A') &&               \
      ( (c) <= 'Z') )

// D_ASCII_IS_ALPHA_LOWER
//   macro: evaluates to true if character is a lowercase letter (a-z).
#define D_ASCII_IS_ALPHA_LOWER(c)    \
	( ( (c) >= 'a') &&               \
      ( (c) <= 'z') )

// D_ASCII_IS_ALPHABETICAL
//   macro: evaluates to true if character is an alphabetical letter (A-Z or
// a-z).
#define D_ASCII_IS_ALPHABETICAL(c)   \
	( D_ASCII_IS_ALPHA_UPPER(c) ||   \
      D_ASCII_IS_ALPHA_LOWER(c) )

// D_ASCII_IS_ALPHANUMERIC
//   macro: evaluates to true if character is alphanumeric (A-Z, a-z, or 0-9).
#define D_ASCII_IS_ALPHANUMERIC(c)   \
	( D_ASCII_IS_ALPHABETICAL(c) ||  \
      D_ASCII_IS_NUMERIC(c) )

// D_ASCII_IS_WHITESPACE
//   macro: evaluates to true if character is a whitespace character (space,
// tab, newline, carriage return, vertical tab, or form feed).
#define D_ASCII_IS_WHITESPACE(c)     \
	( ( (c) == ' ' ) ||              \
	  ( (c) == '\t') ||              \
	  ( (c) == '\n') ||              \
	  ( (c) == '\r') ||              \
	  ( (c) == '\v') ||              \
	  ( (c) == '\f') )

// D_ASCII_IS_NULL
//   macro: evaluates to true if character is the null terminator.
#define D_ASCII_IS_NULL(c)           \
	( (c) == '\0' )

// D_ASCII_IS_CONTROL
//   macro: evaluates to true if character is a control character (0x00-0x1F or
// 0x7F). These are non-printable characters used for device control.
#define D_ASCII_IS_CONTROL(c)        \
	( ( ( (c) >= 0x00)   &&          \ 
        ( (c) <= 0x1F) ) ||          \ 
	  ( (c) == 0x7F) )			     

// D_ASCII_IS_PRINTABLE
//   macro: evaluates to true if character is a printable character (0x20-0x7E).
#define D_ASCII_IS_PRINTABLE(c)      \
	( ((c) >= 0x20) && ((c) <= 0x7E) )

// D_ASCII_IS_PUNCTUATION
//   macro: evaluates to true if character is a punctuation or symbol character
// (printable but not alphanumeric or whitespace).
#define D_ASCII_IS_PUNCTUATION(c)        \
	( D_ASCII_IS_PRINTABLE(c)       &&   \
	  (!D_ASCII_IS_ALPHANUMERIC(c)) &&   \
	  (!D_ASCII_IS_WHITESPACE(c)) )

// D_ASCII_IS_HEX_DIGIT
//   macro: evaluates to true if character is a hexadecimal digit (0-9, A-F, or
// a-f).
#define D_ASCII_IS_HEX_DIGIT(c)            \
	( D_ASCII_IS_NUMERIC(c)            ||  \
	  (( (c) >= 'A') && ( (c) <= 'F')) ||  \
	  (( (c) >= 'a') && ( (c) <= 'f')) )

// D_ASCII_TO_UPPER
//   macro: converts a lowercase letter to uppercase; leaves other characters
// unchanged.
#define D_ASCII_TO_UPPER(c)              \
	( D_ASCII_IS_ALPHA_LOWER(c) ? ((c) - 32) : (c) )

// D_ASCII_TO_LOWER
//   macro: converts an uppercase letter to lowercase; leaves other characters
// unchanged.
#define D_ASCII_TO_LOWER(c)              \
	( D_ASCII_IS_ALPHA_UPPER(c) ? ((c) + 32) : (c) )


// D_ASCII_ENCLOSE_PARENS
//   macro: encloses a string literal in parentheses.
#define D_ASCII_ENCLOSE_PARENS(str)      \
	"(" str ")"

// D_ASCII_ENCLOSE_BRACKETS
//   macro: encloses a string literal in square brackets.
#define D_ASCII_ENCLOSE_BRACKETS(str)    \
	"[" str "]"

// D_ASCII_ENCLOSE_BRACES
//   macro: encloses a string literal in curly braces.
#define D_ASCII_ENCLOSE_BRACES(str)      \
	"{" str "}"

// D_ASCII_ENCLOSE_ANGLES
//   macro: encloses a string literal in angle brackets.
#define D_ASCII_ENCLOSE_ANGLES(str)      \
	"<" str ">"

// D_ASCII_ENCLOSE_QUOTES_DOUBLE
//   macro: encloses a string literal in double quotes.
#define D_ASCII_ENCLOSE_QUOTES_DOUBLE(str)  \
	"\"" str "\""

// D_ASCII_ENCLOSE_QUOTES_SINGLE
//   macro: encloses a string literal in single quotes.
#define D_ASCII_ENCLOSE_QUOTES_SINGLE(str)  \
	"'" str "'"

// D_ASCII_ENCLOSE_BACKTICKS
//   macro: encloses a string literal in backticks.
#define D_ASCII_ENCLOSE_BACKTICKS(str)   \
	"`" str "`"


#define D_SYM_ASCII_NUL					"\x00"	// 0x00
#define D_SYM_ASCII_SOH					"\x01"	// 0x01 start of Heading
#define D_SYM_ASCII_STX					"\x02"	// 0x02 start of Text
#define D_SYM_ASCII_TRX					"\x03"	// 0x03 end of Text (ETX) legacy name TRX avoided; using ETX below
#define D_SYM_ASCII_ETX					"\x03"	// 0x03 end of Text
#define D_SYM_ASCII_EOT					"\x04"	// 0x04 end of Transmission
#define D_SYM_ASCII_ENQ					"\x05"	// 0x05 enquiry
#define D_SYM_ASCII_ACK					"\x06"	// 0x06 acknowledge
#define D_SYM_ASCII_BEL					"\x07"	// 0x07 bell
#define D_SYM_ASCII_BS					"\x08"	// 0x08 backspace
#define D_SYM_ASCII_HT					"\x09"	// 0x09 horizontal Tab
#define D_SYM_ASCII_LF					"\x0A"	// 0x0A line Feed
#define D_SYM_ASCII_VT					"\x0B"	// 0x0B vertical Tab
#define D_SYM_ASCII_FF					"\x0C"	// 0x0C form Feed
#define D_SYM_ASCII_CR					"\x0D"	// 0x0D carriage Return
#define D_SYM_ASCII_SO					"\x0E"	// 0x0E shift Out
#define D_SYM_ASCII_SI					"\x0F"	// 0x0F shift In
#define D_SYM_ASCII_DLE					"\x10"	// 0x10 data Link Escape
#define D_SYM_ASCII_DC1					"\x11"	// 0x11 device Control 1 (XON)
#define D_SYM_ASCII_DC2					"\x12"	// 0x12 device Control 2
#define D_SYM_ASCII_DC3					"\x13"	// 0x13 device Control 3 (XOFF)
#define D_SYM_ASCII_DC4					"\x14"	// 0x14 device Control 4
#define D_SYM_ASCII_NAK					"\x15"	// 0x15 negative Acknowledge
#define D_SYM_ASCII_SYN					"\x16"	// 0x16 synchronous Idle
#define D_SYM_ASCII_ETB					"\x17"	// 0x17 end of Transmission Block
#define D_SYM_ASCII_CAN					"\x18"	// 0x18 cancel
#define D_SYM_ASCII_EM					"\x19"	// 0x19 end of Medium
#define D_SYM_ASCII_SUB					"\x1A"	// 0x1A substitute
#define D_SYM_ASCII_ESC					"\x1B"	// 0x1B escape
#define D_SYM_ASCII_FS					"\x1C"	// 0x1C file Separator
#define D_SYM_ASCII_GS					"\x1D"	// 0x1D group Separator
#define D_SYM_ASCII_RS					"\x1E"	// 0x1E record Separator
#define D_SYM_ASCII_US					"\x1F"	// 0x1F unit Separator
// 0x20-0x2F: symbols
#define D_SYM_ASCII_SPACE				"\x20"  // 
#define D_SYM_ASCII_EXCLAMATION			"\x21"  // !
#define D_SYM_ASCII_DOUBLE_QUOTE		"\x22"  // "
#define D_SYM_ASCII_HASH				"\x23"  // #
#define D_SYM_ASCII_DOLLAR				"\x24"  // $
#define D_SYM_ASCII_PERCENT				"\x25"  // %
#define D_SYM_ASCII_AMPERSAND			"\x26"  // &
#define D_SYM_ASCII_APOSTROPHE			"\x27"  // '
#define D_SYM_ASCII_LEFT_PAREN			"\x28"  // (
#define D_SYM_ASCII_RIGHT_PAREN			"\x29"  // )
#define D_SYM_ASCII_ASTERISK			"\x2A"  // *
#define D_SYM_ASCII_PLUS				"\x2B"  // +
#define D_SYM_ASCII_COMMA				"\x2C"  // ,
#define D_SYM_ASCII_MINUS				"\x2D"  // -
#define D_SYM_ASCII_DOT					"\x2E"  // .
#define D_SYM_ASCII_SLASH				"\x2F"  // /
// 0x30-0x39: digits
#define D_SYM_ASCII_0					"\x30"	// 0
#define D_SYM_ASCII_1					"\x31"	// 1
#define D_SYM_ASCII_2					"\x32"	// 2
#define D_SYM_ASCII_3					"\x33"	// 3
#define D_SYM_ASCII_4					"\x34"	// 4
#define D_SYM_ASCII_5					"\x35"	// 5
#define D_SYM_ASCII_6					"\x36"	// 6
#define D_SYM_ASCII_7					"\x37"	// 7
#define D_SYM_ASCII_8					"\x38"	// 8
#define D_SYM_ASCII_9					"\x39"	// 9
// 0x3A-0x40: symbols
#define D_SYM_ASCII_COLON				"\x3A"  // :
#define D_SYM_ASCII_SEMICOLON			"\x3B"  // ;
#define D_SYM_ASCII_LESS_THAN			"\x3C"  // <
#define D_SYM_ASCII_EQUALS				"\x3D"  // =
#define D_SYM_ASCII_GREATER_THAN		"\x3E"  // >
#define D_SYM_ASCII_QUESTION			"\x3F"  // ?
#define D_SYM_ASCII_AT					"\x40"  // @
// 0x41-0x5A: uppercase A-Z
#define D_SYM_ASCII_A_UPPER				"\x41"	// A
#define D_SYM_ASCII_B_UPPER				"\x42"	// B
#define D_SYM_ASCII_C_UPPER				"\x43"	// C 
#define D_SYM_ASCII_D_UPPER				"\x44"	// D
#define D_SYM_ASCII_E_UPPER				"\x45"	// E
#define D_SYM_ASCII_F_UPPER				"\x46"	// F
#define D_SYM_ASCII_G_UPPER				"\x47"	// G
#define D_SYM_ASCII_H_UPPER				"\x48"	// H
#define D_SYM_ASCII_I_UPPER				"\x49"	// I
#define D_SYM_ASCII_J_UPPER				"\x4A"	// J
#define D_SYM_ASCII_K_UPPER				"\x4B"	// K
#define D_SYM_ASCII_L_UPPER				"\x4C"	// L
#define D_SYM_ASCII_M_UPPER				"\x4D"	// M
#define D_SYM_ASCII_N_UPPER				"\x4E"	// N
#define D_SYM_ASCII_O_UPPER				"\x4F"	// O
#define D_SYM_ASCII_P_UPPER				"\x50"	// P
#define D_SYM_ASCII_Q_UPPER				"\x51"	// Q
#define D_SYM_ASCII_R_UPPER				"\x52"	// R
#define D_SYM_ASCII_S_UPPER				"\x53"	// S
#define D_SYM_ASCII_T_UPPER				"\x54"	// T
#define D_SYM_ASCII_U_UPPER				"\x55"	// U
#define D_SYM_ASCII_V_UPPER				"\x56"	// V
#define D_SYM_ASCII_W_UPPER				"\x57"	// W
#define D_SYM_ASCII_X_UPPER				"\x58"	// X
#define D_SYM_ASCII_Y_UPPER				"\x59"	// Y
#define D_SYM_ASCII_Z_UPPER				"\x5A"	// Z
// 0x5B-0x60: 
#define D_SYM_ASCII_LEFT_BRACKET		"\x5B"	// [
#define D_SYM_ASCII_BACKSLASH			"\x5C"	// 
#define D_SYM_ASCII_RIGHT_BRACKET		"\x5D"	// ]
#define D_SYM_ASCII_CARET				"\x5E"	// ^
#define D_SYM_ASCII_UNDERSCORE			"\x5F"	// _
#define D_SYM_ASCII_GRAVE				"\x60"	// `
// 0x61-0x7A: lowercase a-z
#define D_SYM_ASCII_A_LOWER				"\x61"	// a
#define D_SYM_ASCII_B_LOWER				"\x62"	// b
#define D_SYM_ASCII_C_LOWER				"\x63"	// c
#define D_SYM_ASCII_D_LOWER				"\x64"	// d
#define D_SYM_ASCII_E_LOWER				"\x65"	// e
#define D_SYM_ASCII_F_LOWER				"\x66"	// f
#define D_SYM_ASCII_G_LOWER				"\x67"	// g
#define D_SYM_ASCII_H_LOWER				"\x68"	// h
#define D_SYM_ASCII_I_LOWER				"\x69"	// i
#define D_SYM_ASCII_J_LOWER				"\x6A"	// j
#define D_SYM_ASCII_K_LOWER				"\x6B"	// k
#define D_SYM_ASCII_L_LOWER				"\x6C"	// l
#define D_SYM_ASCII_M_LOWER				"\x6D"	// m
#define D_SYM_ASCII_N_LOWER				"\x6E"	// n
#define D_SYM_ASCII_O_LOWER				"\x6F"	// o
#define D_SYM_ASCII_P_LOWER				"\x70"	// p
#define D_SYM_ASCII_Q_LOWER				"\x71"	// q
#define D_SYM_ASCII_R_LOWER				"\x72"	// r
#define D_SYM_ASCII_S_LOWER				"\x73"	// s
#define D_SYM_ASCII_T_LOWER				"\x74"	// t
#define D_SYM_ASCII_U_LOWER				"\x75"	// u
#define D_SYM_ASCII_V_LOWER				"\x76"	// v
#define D_SYM_ASCII_W_LOWER				"\x77"	// w
#define D_SYM_ASCII_X_LOWER				"\x78"	// x
#define D_SYM_ASCII_Y_LOWER				"\x79"	// y
#define D_SYM_ASCII_Z_LOWER				"\x7A"	// z
// 0x7B-0x7E: curly brackets, pipe, tilde
#define D_SYM_ASCII_LEFT_BRACE			"\x7B"  // {
#define D_SYM_ASCII_VERTICAL_BAR		"\x7C"  // |
#define D_SYM_ASCII_RIGHT_BRACE			"\x7D"  // }
#define D_SYM_ASCII_TILDE				"\x7E"  // ~


// I.    character classification
bool d_ascii_char_is_numeric(char _c);
bool d_ascii_char_is_alphabetical(char _c);
bool d_ascii_char_is_alphanumeric(char _c);
bool d_ascii_char_is_whitespace(char _c);
bool d_ascii_char_is_null(char _c);
bool d_ascii_char_is_control(char _c);
bool d_ascii_char_is_printable(char _c);
bool d_ascii_char_is_punctuation(char _c);
bool d_ascii_char_is_hex_digit(char _c);

// II.   character conversion
char d_ascii_char_to_upper(char _c);
char d_ascii_char_to_lower(char _c);


#endif	// DJINTERP_TEXT_SYMBOLS_ASCII_