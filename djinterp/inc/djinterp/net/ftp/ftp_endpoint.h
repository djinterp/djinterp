/*******************************************************************************
* djinterp [net]                                                  ftp_endpoint.h
*
* FTP data-connection endpoints: PORT, PASV, EPRT, and EPSV.
*   Parses and formats the address-and-port encodings of RFC 959 and RFC 2428
* for active and passive transfers, over IPv4 and IPv6.
*
*
* path:      /inc/djinterp/net/ftp/ftp_endpoint.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Buffer sizes
         1.  D_FTP_ADDRESS_SIZE
         2.  D_FTP_PORT_ARGUMENT_SIZE
         3.  D_FTP_EPRT_ARGUMENT_SIZE
2.  TYPES
    -----
    1.  Data connections
         1.  d_ftp_data_mode
         2.  d_ftp_address_family
         3.  d_ftp_endpoint
3.  DATA CONNECTIONS
    ----------------
    1.  Parsing
    2.  Formatting
*/

#ifndef DJINTERP_NET_FTP_FTP_ENDPOINT_H
#define DJINTERP_NET_FTP_FTP_ENDPOINT_H 1

// std
#include <stddef.h>  // size_t
#include <stdint.h>  // uint16_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ftp_common.h"      // d_ftp_error, d_ftp_buffer


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftp_endpoint.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONSTANTS
//==============================================================================


// 1.1    Buffer sizes
//------------------------------------------------------------------------------
// 1.1.1
// D_FTP_ADDRESS_SIZE
//   constant: bytes for a textual IPv4 or IPv6 address and its terminator;
// the same as INET6_ADDRSTRLEN.
#define D_FTP_ADDRESS_SIZE       46

// 1.1.2
// D_FTP_PORT_ARGUMENT_SIZE
//   constant: bytes for the longest PORT argument, "255,255,255,255,255,255",
// and its terminator.
#define D_FTP_PORT_ARGUMENT_SIZE 24

// 1.1.3
// D_FTP_EPRT_ARGUMENT_SIZE
//   constant: bytes for the longest EPRT argument -- a 45-character IPv6
// address, a five-digit port, the protocol digit, and four delimiters -- and
// its terminator.
#define D_FTP_EPRT_ARGUMENT_SIZE 56


//==============================================================================
// 2.  TYPES
//==============================================================================


// 2.1    Data connections
//------------------------------------------------------------------------------
// 2.1.1
// d_ftp_data_mode
//   enum: how the data connection is established. Passive modes have the
// client connect to the server and survive client-side NAT and firewalls;
// active modes have the server connect back to the client.
enum d_ftp_data_mode
{
    D_FTP_DATA_PASSIVE_AUTO = 0,  // EPSV, falling back to PASV
    D_FTP_DATA_PASSIVE,           // PASV only (IPv4)
    D_FTP_DATA_EXTENDED_PASSIVE,  // EPSV only
    D_FTP_DATA_ACTIVE_AUTO,       // EPRT, falling back to PORT
    D_FTP_DATA_ACTIVE,            // PORT only (IPv4)
    D_FTP_DATA_EXTENDED_ACTIVE    // EPRT only
};

// 2.1.2
// d_ftp_address_family
//   enum: an endpoint's address family; the values are RFC 2428's network
// protocol numbers.
enum d_ftp_address_family
{
    D_FTP_FAMILY_NONE = 0,  // no address: use the control connection's peer
    D_FTP_FAMILY_IPV4 = 1,  // an IPv4 address
    D_FTP_FAMILY_IPV6 = 2   // an IPv6 address
};

// 2.1.3
// d_ftp_endpoint
//   struct: the address and port of a data connection, as carried by PASV,
// EPSV, PORT, and EPRT. The address stays textual, ready for getaddrinfo()
// or inet_pton() in whichever backend opens the connection.
struct d_ftp_endpoint
{
    enum d_ftp_address_family family;                       // see above
    uint16_t                  port;                         // host order
    char                      address[D_FTP_ADDRESS_SIZE];  // text or ""
};


//==============================================================================
// 3.  DATA CONNECTIONS
//==============================================================================
// PASV and PORT carry an IPv4 address and a port as six decimal bytes; EPSV
// carries a port alone, and EPRT an address of either family (RFC 2428).
// Clients should connect to the control connection's peer rather than to a
// PASV address, which is often private or spoofed; see ignore_pasv_address
// in d_ftp_options.


// 3.1    Parsing
//------------------------------------------------------------------------------
/**
 * @brief Extracts the endpoint from the text of a 227 reply.
 *
 * The tuple's position in the text is not standardized (RFC 1123 4.1.2.6),
 * so every run of digits is tried until one begins a valid tuple.
 *
 * @param[in]  _text   the reply text.
 * @param[in]  _length its length in bytes.
 * @param[out] _out    the IPv4 endpoint.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, or D_FTP_ERROR_MALFORMED.
 */
enum d_ftp_error d_ftp_parse_pasv(const char*            _text,
                                  size_t                 _length,
                                  struct d_ftp_endpoint* _out);
/**
 * @brief Extracts the port from the text of a 229 reply, "(|||port|)".
 *
 * @param[in]  _text   the reply text.
 * @param[in]  _length its length in bytes.
 * @param[out] _out    an endpoint of family D_FTP_FAMILY_NONE: connect to
 *                     the control connection's peer on that port.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, or D_FTP_ERROR_MALFORMED.
 */
enum d_ftp_error d_ftp_parse_epsv(const char*            _text,
                                  size_t                 _length,
                                  struct d_ftp_endpoint* _out);
/**
 * @brief Parses a PORT argument, "h1,h2,h3,h4,p1,p2", for a server.
 *
 * @param[in]  _text   the argument.
 * @param[in]  _length its length in bytes.
 * @param[out] _out    the IPv4 endpoint.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, or D_FTP_ERROR_MALFORMED.
 */
enum d_ftp_error d_ftp_parse_port(const char*            _text,
                                  size_t                 _length,
                                  struct d_ftp_endpoint* _out);
/**
 * @brief Parses an EPRT argument, "|proto|address|port|", for a server.
 *
 * @param[in]  _text   the argument.
 * @param[in]  _length its length in bytes.
 * @param[out] _out    the endpoint.
 * @return D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT, D_FTP_ERROR_MALFORMED, or
 *         D_FTP_ERROR_PROTOCOL_UNSUPPORTED for a network protocol other
 *         than 1 or 2 (reply 522).
 */
enum d_ftp_error d_ftp_parse_eprt(const char*            _text,
                                  size_t                 _length,
                                  struct d_ftp_endpoint* _out);

// 3.2    Formatting
//------------------------------------------------------------------------------
// Each appends to `_out` and returns D_FTP_OK, D_FTP_ERROR_INVALID_ARGUMENT
// for an endpoint the encoding cannot carry (a zero port always; any family
// but IPv4 for PORT and PASV), or D_FTP_ERROR_BUFFER_TOO_SMALL. The PORT and
// EPRT forms are command arguments; the PASV and EPSV forms are reply texts,
// "Entering Passive Mode (h1,h2,h3,h4,p1,p2)." and
// "Entering Extended Passive Mode (|||port|)".
enum d_ftp_error d_ftp_format_port(const struct d_ftp_endpoint* _endpoint,
                                   struct d_ftp_buffer*         _out);
enum d_ftp_error d_ftp_format_eprt(const struct d_ftp_endpoint* _endpoint,
                                   struct d_ftp_buffer*         _out);
enum d_ftp_error d_ftp_format_pasv(const struct d_ftp_endpoint* _endpoint,
                                   struct d_ftp_buffer*         _out);
enum d_ftp_error d_ftp_format_epsv(const struct d_ftp_endpoint* _endpoint,
                                   struct d_ftp_buffer*         _out);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_ENDPOINT_H
