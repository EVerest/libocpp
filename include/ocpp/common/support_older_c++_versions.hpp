/*
 * support_older_c++_versions.hpp
 *
 */

#ifndef LIB_LIBOCPPMYFORK_INCLUDE_OCPP_COMMON_SUPPORT_OLDER_C___VERSIONS_HPP_
#define LIB_LIBOCPPMYFORK_INCLUDE_OCPP_COMMON_SUPPORT_OLDER_C___VERSIONS_HPP_

#include <openssl/asn1.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#ifndef BOOSTFILESYSTEM
#include <filesystem>
#else
namespace fs = boost::filesystem;
#endif

#ifndef OPENSSL1_1_1F
//const unsigned char* cnStr = ASN1_STRING_get0_data(cnAsn1);
extern const unsigned char *(*ansi_string_data_selected)(ASN1_STRING *);
extern int (*ssl_ctx_set_cipher_list_selected)(SSL_CTX *ctx, const char *str);
extern ASN1_TIME *(*x509_get_not_after_selected)(const X509 *x);
//const unsigned char* cnStr = ASN1_STRING_get0_data(cnAsn1);
#else
extern unsigned char *(*ansi_string_data_selected)(ASN1_STRING *);
xtern int (*ssl_ctx_set_cipher_list_selected)(SSL_CTX *ctx, const char *str);
#define  x509_get_not_after_selected(x) X509_get_notAfter(x);

#endif


#endif /* LIB_LIBOCPPMYFORK_INCLUDE_OCPP_COMMON_SUPPORT_OLDER_C___VERSIONS_HPP_ */
