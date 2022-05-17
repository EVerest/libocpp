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

#include <ocpp1_6/messages/DeleteCertificate.hpp>
#include <ocpp1_6/messages/GetInstalledCertificateIds.hpp>
#include <ocpp1_6/messages/InstallCertificate.hpp>

namespace ocpp1_6 {
using BN_ptr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;
using RSA_ptr = std::unique_ptr<RSA, decltype(&::RSA_free)>;
using EVP_KEY_ptr = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
using BIO_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;
using X509_REQ_ptr = std::unique_ptr<X509_REQ, decltype(&::X509_REQ_free)>;
using X509_NAME_ptr = std::unique_ptr<X509_NAME, decltype(&::X509_NAME_free)>;
using X509_ptr = std::unique_ptr<X509, decltype(&::X509_free)>;
using X509_STORE_ptr = std::unique_ptr<X509_STORE, decltype(&::X509_STORE_free)>;
using X509_STORE_CTX_ptr = std::unique_ptr<X509_STORE_CTX, decltype(&::X509_STORE_CTX_free)>;

const boost::filesystem::path CLIENT_SIDE_CERTIFICATE_FILE("csc.pem");
const boost::filesystem::path CERTIFICATE_SIGNING_REQUEST_FILE("csr.pem");
const boost::filesystem::path CS_ROOT_CA_FILE("cs_root_ca.pem");
const boost::filesystem::path CS_ROOT_CA_FILE_BACKUP_FILE("fallback/cs_root_ca_backup.pem");
const boost::filesystem::path MF_ROOT_CA_FILE("mf_root_ca.pem");
const boost::filesystem::path PUBLIC_KEY_FILE("pubkey.pem");
const boost::filesystem::path PRIVATE_KEY_FILE("prvkey.pem");

enum class CertificateType
{
    CentralSystemRootCertificate,
    ManufacturerRootCertificate,
    ClientCertificate
};

struct X509Certificate {
    boost::filesystem::path path;
    X509* x509;
    std::string str;
    CertificateType type;

    bool write();
    bool write(const boost::filesystem::path& path);
};

X509Certificate load_from_file(const boost::filesystem::path& path);
X509Certificate load_from_string(std::string& str);

std::string read_file_to_string(const boost::filesystem::path& path);
class PkiHandler {

private:
    boost::filesystem::path maindir;
    bool use_root_ca_fallback;

    std::string get_issuer_name_hash(const X509Certificate& cert);
    std::string get_serial(const X509Certificate& cert);
    std::string get_issuer_key_hash(const X509Certificate& cert);
    std::vector<X509Certificate> get_ca_certificates(CertificateUseEnumType type);
    std::vector<X509Certificate> get_ca_certificates();
    bool validateCertificateChain(X509Certificate root_ca, X509Certificate cert);
    X509Certificate getRootCertificate(CertificateUseEnumType type);

public:
    PkiHandler(std::string maindir);
    bool validateCertificate(const std::string& certificateChain, const std::string& charge_box_serial_number);
    std::string generateCsr(const char* szCountry, const char* szProvince, const char* szCity,
                            const char* szOrganization, const char* szCommon);
    bool isCentralSystemRootCertificateInstalled();
    boost::optional<std::vector<CertificateHashDataType>> getRootCertificateHashData(CertificateUseEnumType type);
    DeleteCertificateStatusEnumType delete_certificate(CertificateHashDataType certificate_hash_data);
    InstallCertificateStatusEnumType install_certificate(InstallCertificateRequest msg,
                                                         boost::optional<int32_t> certificate_store_max_length,
                                                         boost::optional<bool> additional_root_certificate_check);
    boost::filesystem::path getCertsPath();
    void removeFallbackCA();
};

} // namespace ocpp1_6

#endif // OCPP1_6_PKI_HANDLER