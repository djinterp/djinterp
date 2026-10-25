/*******************************************************************************
* djinterp [net]                                       tls_test_certificate.hpp
*
* The certificate the TLS tests serve: P-256, self-signed for localhost and
* 127.0.0.1, valid until 2126, and its private key, both in PEM. The TLS
* suite and the server suite's https test share it.
*
*
* path:      /tests/djinterp/net/tls/tls_test_certificate.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.05
*                                                            revised: 2026.10.05
*******************************************************************************/

#ifndef DJINTERP_NET_TLS_TLS_TEST_CERTIFICATE_HPP
#define DJINTERP_NET_TLS_TLS_TEST_CERTIFICATE_HPP 1

// djinterp
#include "../../../../inc/djinterp/djinterp.hpp"  // framework root


NS_DJINTERP
NS_TESTING
NS_INTERNAL

    // tls_certificate_pem, tls_key_pem
    //   function: the certificate and its key, as PEM text.
    inline const char*
    tls_certificate_pem()
    {
        return
            "-----BEGIN CERTIFICATE-----\n"
            "MIIBmTCCAUCgAwIBAgITPG+vIAwqV95A6x7Bi7b359VfwzAKBggqhkjOPQQDAjAU\n"
            "MRIwEAYDVQQDDAlsb2NhbGhvc3QwIBcNMjYxMDA0MTU1OTI5WhgPMjEyNjA5MTAx\n"
            "NTU5MjlaMBQxEjAQBgNVBAMMCWxvY2FsaG9zdDBZMBMGByqGSM49AgEGCCqGSM49\n"
            "AwEHA0IABGzWwSj7UnqPl1cbVv4DbUX9UEQne0NzsXYoCFTtOanGCLB3tKmJmJig\n"
            "77rciaXozPf+zXYVBngqXzjtbRohjvajbzBtMB0GA1UdDgQWBBQwH8R11e6ZbXem\n"
            "wZOtymq2VGw1vjAfBgNVHSMEGDAWgBQwH8R11e6ZbXemwZOtymq2VGw1vjAPBgNV\n"
            "HRMBAf8EBTADAQH/MBoGA1UdEQQTMBGCCWxvY2FsaG9zdIcEfwAAATAKBggqhkjO\n"
            "PQQDAgNHADBEAiAiHwD2TvQh1NUqF974ZW1wITjEafNlhqpqt+SyNVQ3FAIgNnWi\n"
            "5wgBuyHFxh5DqyqocwDXenFr8ysnZoc3Fq2/pHk=\n"
            "-----END CERTIFICATE-----\n";
    }

    inline const char*
    tls_key_pem()
    {
        return
            "-----BEGIN PRIVATE KEY-----\n"
            "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQg09JwCZEJzgY6Fphx\n"
            "ZNC5CHRXhwQPKABAefoTYW85rW6hRANCAARs1sEo+1J6j5dXG1b+A21F/VBEJ3tD\n"
            "c7F2KAhU7Tmpxgiwd7SpiZiYoO+63Iml6Mz3/s12FQZ4Kl847W0aIY72\n"
            "-----END PRIVATE KEY-----\n";
    }

NS_END  // internal
NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_NET_TLS_TLS_TEST_CERTIFICATE_HPP
