/*
 * support_older_c++_versions.cpp
 *
 */
#include <openssl/asn1.h>
#include <openssl/ssl.h>
 #include <openssl/x509.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ocsp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#ifndef OPENSSL1_1_1F
//const unsigned char* cnStr = ASN1_STRING_get0_data(cnAsn1);
const unsigned char *(*ansi_string_data_selected)(ASN1_STRING *)=ASN1_STRING_get0_data;
int (*ssl_ctx_set_cipher_list_selected)(SSL_CTX *ctx, const char *str)=SSL_CTX_set_ciphersuites;
ASN1_TIME *(*x509_get_not_after_selected)(const X509 *x)=X509_get0_notAfter;
//const unsigned char* cnStr = ASN1_STRING_get0_data(cnAsn1);
#else
unsigned char *(*ansi_string_data_selected)(ASN1_STRING *)=ASN1_STRING_data;
int (*ssl_ctx_set_cipher_list_selected)(SSL_CTX *ctx, const char *str)=SSL_CTX_set_cipher_list;
#define  x509_get_not_after_selected(x) X509_get_notAfter(x);

#endif


