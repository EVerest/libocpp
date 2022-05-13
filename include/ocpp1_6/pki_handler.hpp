// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef OCPP1_6_PKI_HANDLER
#define OCPP1_6_PKI_HANDLER

#include <boost/filesystem.hpp>
#include <memory>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <string>

#include <ocpp1_6/messages/GetInstalledCertificateIds.hpp>

namespace ocpp1_6 {
using BN_ptr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;
using RSA_ptr = std::unique_ptr<RSA, decltype(&::RSA_free)>;
using EVP_KEY_ptr = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
using BIO_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;
using X509_REQ_ptr = std::unique_ptr<X509_REQ, decltype(&::X509_REQ_free)>;
using X509_ptr = std::unique_ptr<X509, decltype(&::X509_free)>;
using X509_STORE_ptr = std::unique_ptr<X509_STORE, decltype(&::X509_STORE_free)>;
using X509_STORE_CTX_ptr = std::unique_ptr<X509_STORE_CTX, decltype(&::X509_STORE_CTX_free)>;

const boost::filesystem::path CLIENT_SIDE_CERTIFICATE_FILE("certs/csc.pem");
const boost::filesystem::path CERTIFICATE_SIGNING_REQUEST_FILE("certs/csr.pem");
const boost::filesystem::path CS_ROOT_CA_FILE("certs/cs_root_ca.pem");
const boost::filesystem::path MF_ROOT_CA_FILE("certs/mf_root_ca.pem");
const boost::filesystem::path PUBLIC_KEY_FILE("certs/pubkey.pem");
const boost::filesystem::path PRIVATE_KEY_FILE("certs/prvkey.pem");

class PkiHandler {

private:
    boost::filesystem::path maindir;

    std::string get_issuer_name_hash(X509_ptr& x509);
    std::string get_serial(X509_ptr& x509);
    std::string get_issuer_key_hash(X509_ptr& x509);
    std::vector<X509*> get_ca_certificates(CertificateUseEnumType type);

public:
    PkiHandler(std::string maindir);
    bool validateCertificate(const std::string& certificateChain, const std::string& charge_box_serial_number);
    std::string generateCsr(const char* szCountry, const char* szProvince, const char* szCity,
                            const char* szOrganization, const char* szCommon);
    bool isCentralSystemRootCertificateInstalled();
    boost::optional<std::vector<CertificateHashDataType>> getRootCertificateHashData(CertificateUseEnumType type);
    static X509* readX509FromFile(const boost::filesystem::path& path);
    static EVP_PKEY* readEvpKey(const boost::filesystem::path& path);
    static X509* readX509FromString(const std::string& str);
    static std::string readFileToString(const boost::filesystem::path& path);
};

} // namespace ocpp1_6

#endif // OCPP1_6_PKI_HANDLER