// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef OCPP1_6_PKI_HANDLER
#define OCPP1_6_PKI_HANDLER

#include <boost/filesystem.hpp>
#include <memory>
#include <openssl/bio.h>
#include <openssl/err.h>
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
using EVP_KEY_ptr = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
using BIO_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;
using X509_NAME_ptr = std::unique_ptr<X509_NAME, decltype(&::X509_NAME_free)>;
using X509_STORE_ptr = std::unique_ptr<X509_STORE, decltype(&::X509_STORE_free)>;
using X509_STORE_CTX_ptr = std::unique_ptr<X509_STORE_CTX, decltype(&::X509_STORE_CTX_free)>;

const boost::filesystem::path CLIENT_SIDE_CERTIFICATE_FILE("CP_RSA.pem");
const boost::filesystem::path CERTIFICATE_SIGNING_REQUEST_FILE("CP_CSR.pem");
const boost::filesystem::path CS_ROOT_CA_FILE("CSMS_RootCA_RSA.pem");
const boost::filesystem::path CS_ROOT_CA_FILE_BACKUP_FILE("CSMS_RootCA_RSA_backup.pem");
const boost::filesystem::path MF_ROOT_CA_FILE("MF_RootCA_RSA.pem");
const boost::filesystem::path PUBLIC_KEY_FILE("CP_PUBLIC_KEY_RSA.pem");
const boost::filesystem::path PRIVATE_KEY_FILE("CP_PRIVATE_KEY_RSA.pem");

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
    int validIn; // seconds. If > 0 cert is not valid yet
    int validTo; // seconds. If < 0 cert has expired

    X509Certificate(){};
    X509Certificate(boost::filesystem::path path, X509* x509, std::string str);
    X509Certificate(boost::filesystem::path path, X509* x509, std::string str, CertificateType type);
    ~X509Certificate();

    bool write();
    bool write(const boost::filesystem::path& path);
};

std::shared_ptr<X509Certificate> loadFromFile(const boost::filesystem::path& path);
std::shared_ptr<X509Certificate> loadFromString(std::string& str);

std::string readFileToString(const boost::filesystem::path& path);
class PkiHandler {

private:
    boost::filesystem::path maindir;
    bool useRootCaFallback;

    std::string getIssuerNameHash(std::shared_ptr<X509Certificate> cert);
    std::string getSerialNumber(std::shared_ptr<X509Certificate> cert);
    std::string getIssuerKeyHash(std::shared_ptr<X509Certificate> cert);
    std::vector<std::shared_ptr<X509Certificate>> getCaCertificates(CertificateUseEnumType type);
    std::vector<std::shared_ptr<X509Certificate>> getCaCertificates();
    bool verifyCertificateChain(std::shared_ptr<X509Certificate> rootCA, std::shared_ptr<X509Certificate> cert);
    bool verifySignature(std::shared_ptr<X509Certificate> rootCA, std::shared_ptr<X509Certificate> newCA);
    std::shared_ptr<X509Certificate> getRootCertificate(CertificateUseEnumType type);

public:
    PkiHandler(std::string maindir);
    bool verifyCertificate(const std::string& certificateChain, const std::string& charge_box_serial_number);
    void writeClientCertificate(const std::string certificate);
    std::string generateCsr(const char* szCountry, const char* szProvince, const char* szCity,
                            const char* szOrganization, const char* szCommon);
    bool isCentralSystemRootCertificateInstalled();
    bool isManufacturerRootCertificateInstalled();
    bool isRootCertificateInstalled(CertificateUseEnumType type);
    bool verifyFirmwareCertificate(const std::string& firmwareCertificate);
    boost::optional<std::vector<CertificateHashDataType>> getRootCertificateHashData(CertificateUseEnumType type);
    DeleteCertificateStatusEnumType deleteCertificate(CertificateHashDataType certificate_hash_data);
    InstallCertificateStatusEnumType installCertificate(InstallCertificateRequest msg,
                                                        boost::optional<int32_t> certificateStoreMaxLength,
                                                        boost::optional<bool> additionalRootCertificateCheck);
    boost::filesystem::path getCertsPath();
    boost::filesystem::path getFile(boost::filesystem::path fileName);
    std::shared_ptr<X509Certificate> getClientCertificate();
    void removeFallbackCA();
    int validIn(const std::string certificate);
};

} // namespace ocpp1_6

#endif // OCPP1_6_PKI_HANDLER