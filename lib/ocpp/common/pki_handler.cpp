// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <everest/logging.hpp>
#include <regex>
#include <streambuf>

#include <ocpp/common/pki_handler.hpp>

namespace ocpp {

CABundleType convertPkiEnumToCABundleType(const PkiEnum pkiEnum) {
    switch (pkiEnum) {
    case PkiEnum::CSMS:
        return CABundleType::CSMS;
    case PkiEnum::CSO:
        return CABundleType::CSO;
    case PkiEnum::MF:
        return CABundleType::MF;
    case PkiEnum::MO:
        return CABundleType::MO;
    case PkiEnum::V2G:
        return CABundleType::V2G;
    }
    throw std::runtime_error("Cannot convert PkiEnum to CABundleType");
}

bool removeCertFromFile(const std::string& cert, const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return false;
    }
    // Read the content of the file
    std::ifstream inFile(path);
    if (!inFile) {
        EVLOG_error << "Error opening file: " << path;
        return false;
    }

    std::string fileContent((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    size_t pos = fileContent.find(cert);
    if (pos == std::string::npos) {
        return false;
    }

    fileContent.erase(pos, cert.length());

    std::ofstream outFile(path);
    if (!outFile) {
        EVLOG_error << "Error opening file for writing: " << path;
        return false;
    }
    outFile << fileContent;
    outFile.close();
    return true;
}

X509Certificate::X509Certificate(std::filesystem::path path, X509* x509, std::string str) {
    this->path = path;
    this->x509 = x509;
    this->str = str;
}

X509Certificate::~X509Certificate() {
    X509_free(this->x509);
}

bool X509Certificate::write(std::ios::openmode mode) {
    std::ofstream fs(this->path.string(), mode);
    fs << this->str << std::endl;
    fs.close();
    return true;
}

std::string X509Certificate::getCommonName() {
    X509_NAME* subject = X509_get_subject_name(this->x509);
    int nid = OBJ_txt2nid("CN");
    int index = X509_NAME_get_index_by_NID(subject, nid, -1);
    X509_NAME_ENTRY* entry = X509_NAME_get_entry(subject, index);
    ASN1_STRING* cnAsn1 = X509_NAME_ENTRY_get_data(entry);
    const unsigned char* cnStr = ASN1_STRING_get0_data(cnAsn1);
    std::string commonName(reinterpret_cast<const char*>(cnStr), ASN1_STRING_length(cnAsn1));
    return commonName;
}

std::string X509Certificate::getIssuerNameHash() {
    unsigned char md[SHA256_DIGEST_LENGTH];
    X509_NAME* name = X509_get_issuer_name(this->x509);
    X509_NAME_digest(name, EVP_sha256(), md, NULL);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::setw(2) << std::setfill('0') << std::hex << (int)md[i];
    }
    std::string str = ss.str();
    return str;
}

std::string X509Certificate::getSerialNumber() {
    ASN1_INTEGER* serial = X509_get_serialNumber(this->x509);
    BIGNUM* bnser = ASN1_INTEGER_to_BN(serial, NULL);
    char* hex = BN_bn2hex(bnser);
    std::string str(hex);
    str.erase(0, std::min(str.find_first_not_of('0'), str.size() - 1));
    return str;
}

std::string X509Certificate::getIssuerKeyHash() {
    unsigned char tmphash[SHA256_DIGEST_LENGTH];
    X509_pubkey_digest(this->x509, EVP_sha256(), tmphash, NULL);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::setw(2) << std::setfill('0') << std::hex << (int)tmphash[i];
    }
    std::string str = ss.str();
    return str;
}

std::string X509Certificate::getResponderURL() {
    const auto ocsp = X509_get1_ocsp(this->x509);
    std::string responderUrl;
    for (int i = 0; i < sk_OPENSSL_STRING_num(ocsp); i++) {
        responderUrl.append(sk_OPENSSL_STRING_value(ocsp, i));
    }

    if (responderUrl.empty()) {
        EVLOG_warning << "Could not retrieve OCSP Responder URL from certificate";
    }

    return responderUrl;
}

OCSPRequestData X509Certificate::getOCSPRequestData() {
    OCSPRequestData requestData;
    requestData.hashAlgorithm = HashAlgorithmEnumType::SHA256;
    requestData.issuerKeyHash = this->getIssuerKeyHash();
    requestData.issuerNameHash = this->getIssuerNameHash();
    requestData.serialNumber = this->getSerialNumber();
    requestData.responderUrl = this->getResponderURL();
    return requestData;
}

PkiEnum getPkiEnumFromCertificateType(const CertificateType& type) {
    switch (type) {
    case CertificateType::CentralSystemRootCertificate:
        return PkiEnum::CSMS;
    case CertificateType::ManufacturerRootCertificate:
        return PkiEnum::MF;
    case CertificateType::MORootCertificate:
        return PkiEnum::MO;
    case CertificateType::V2GCertificateChain:
        return PkiEnum::CSO;
    case CertificateType::V2GRootCertificate:
        return PkiEnum::V2G;
    case CertificateType::CSMSRootCertificate:
        return PkiEnum::CSMS;
    default:
        EVLOG_warning << "Conversion of CertificateType: " << conversions::certificate_type_to_string(type)
                      << " to PkiEnum was not successful";
        return PkiEnum::V2G;
    }
}

CertificateType getRootCertificateTypeFromPki(const PkiEnum& pki) {
    switch (pki) {
    case PkiEnum::CSMS:
        return CertificateType::CentralSystemRootCertificate;
    case PkiEnum::CSO:
        return CertificateType::CentralSystemRootCertificate;
    case PkiEnum::MF:
        return CertificateType::ManufacturerRootCertificate;
    case PkiEnum::MO:
        return CertificateType::MORootCertificate;
    case PkiEnum::V2G:
        return CertificateType::V2GRootCertificate;
    default:
        return CertificateType::V2GRootCertificate;
    }
}

std::shared_ptr<X509Certificate> loadFromFile(const std::filesystem::path& path) {
    try {
        X509* x509;
        std::string fileStr;

        if (path.extension().string() == ".der") {
            std::ifstream t(path.c_str(), std::ios::binary);
            fileStr = std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
            t.close();

            const char* certDataPtr = fileStr.data();
            const int certDataLen = fileStr.size();

            // Convert certificate data to X509 certificate
            x509 = d2i_X509(NULL, (const unsigned char**)&certDataPtr, certDataLen);
            if (x509 == NULL) {
                EVLOG_warning << "Error loading X509 certificate from " << path.string();
                return nullptr;
            }
        } else {
            std::ifstream t(path.c_str());
            fileStr = std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
            t.close();
            BIO_ptr b(BIO_new(BIO_s_mem()), ::BIO_free);
            BIO_puts(b.get(), fileStr.c_str());
            x509 = PEM_read_bio_X509(b.get(), NULL, NULL, NULL);
            if (x509 == NULL) {
                EVLOG_error << "Error loading X509 certificate from " << path.string();
                return nullptr;
            }
        }
        std::shared_ptr<X509Certificate> cert = std::make_shared<X509Certificate>(path, x509, fileStr);
        cert->validIn = X509_cmp_current_time(X509_get_notBefore(cert->x509));
        cert->validTo = X509_cmp_current_time(X509_get_notAfter(cert->x509));

        return cert;
    } catch (const std::exception& e) {
        EVLOG_error << "Unknown error occured while loading certificate from file: " << e.what();
        return nullptr;
    }
}

std::shared_ptr<X509Certificate> loadFromString(const std::string& str) {
    BIO_ptr b(BIO_new(BIO_s_mem()), ::BIO_free);
    BIO_puts(b.get(), str.c_str());
    X509* x509 = PEM_read_bio_X509(b.get(), NULL, NULL, NULL);

    if (x509 == NULL) {
        EVLOG_error << "Could not parse str to X509";
        return nullptr;
    }

    std::shared_ptr<X509Certificate> cert = std::make_shared<X509Certificate>();
    cert->str = str;
    cert->x509 = x509;
    return cert;
}

PkiHandler::PkiHandler(const CertificateFilePaths& files, const bool multipleCsmsCaNotAllowed) {

    this->pki_bundle_map[CABundleType::CSMS] = files.csms_ca_bundle;
    this->pki_bundle_map[CABundleType::CSMS_BACKUP] = files.csms_ca_backup_bundle;
    this->pki_bundle_map[CABundleType::CSO] = files.cso_ca_bundle;
    this->pki_bundle_map[CABundleType::MF] = files.mf_ca_bundle;
    this->pki_bundle_map[CABundleType::MO] = files.mo_ca_bundle;
    this->pki_bundle_map[CABundleType::V2G] = files.v2g_ca_bundle;

    this->csms_leaf_cert = files.csms_leaf_cert;
    this->csms_leaf_key = files.csms_leaf_key;
    this->csms_leaf_key_backup = files.csms_leaf_key_backup;
    this->csms_leaf_csr = files.csms_leaf_csr;
    this->secc_leaf_cert = files.secc_leaf_cert;
    this->secc_leaf_key = files.secc_leaf_key;
    this->secc_leaf_key_backup = files.secc_leaf_key_backup;
    this->secc_leaf_csr = files.secc_leaf_csr;

    if (multipleCsmsCaNotAllowed) {
        if (this->getNumberOfCsmsCaCertificates() > 1) {
            EVLOG_error << "Multiple CSMS CA certificates installed while additionalRootCertificateCheck is true. This "
                           "is not allowed";
            EVLOG_AND_THROW(std::runtime_error("Multiple CSMS CA not allowed!"));
        }
    }
}

std::optional<std::filesystem::path> PkiHandler::getCsmsCaPath() {
    if (std::filesystem::exists(this->pki_bundle_map.at(CABundleType::CSMS))) {
        return this->pki_bundle_map.at(CABundleType::CSMS);
    }
    return std::nullopt;
}

std::optional<std::filesystem::path> PkiHandler::getCsmsBackupCaPath() {
    if (std::filesystem::exists(this->pki_bundle_map.at(CABundleType::CSMS_BACKUP))) {
        this->pki_bundle_map.at(CABundleType::CSMS_BACKUP);
    }
    return std::nullopt;
}

int PkiHandler::getNumberOfCsmsCaCertificates() {
    if (!this->pki_bundle_map.count(CABundleType::CSMS)) {
        return 0;
    }

    const auto caBundle = loadFromFile(this->pki_bundle_map.at(CABundleType::CSMS));
    if (caBundle == nullptr) {
        return 0;
    }
    return this->getCertificatesFromChain(caBundle->str).size();
}

CertificateVerificationResult PkiHandler::verifyChargepointCertificate(const std::string& certificateChain,
                                                                       const std::string& chargeBoxSerialNumber) {

    if (!this->isCentralSystemRootCertificateInstalled()) {
        EVLOG_warning << "Could not verify certificate because no Central System Root Certificate is installed";
        return CertificateVerificationResult::NoRootCertificateInstalled;
    }

    const auto certificates = this->getCertificatesFromChain(certificateChain);

    if (!certificates.empty()) {
        return this->verifyCertificate(PkiEnum::CSMS, certificates, chargeBoxSerialNumber);
    } else {
        return CertificateVerificationResult::InvalidCertificateChain;
    }
}

CertificateVerificationResult
PkiHandler::verifyV2GChargingStationCertificate(const std::string& certificateChain,
                                                const std::string& chargeBoxSerialNumber) {
    if (!this->isV2GRootCertificateInstalled()) {
        EVLOG_warning << "Could not verify certificate because no V2GRootCertificate is installed";
        return CertificateVerificationResult::NoRootCertificateInstalled;
    }

    const auto certificates = this->getCertificatesFromChain(certificateChain);

    if (!certificates.empty()) {
        return this->verifyCertificate(PkiEnum::CSO, certificates, chargeBoxSerialNumber);
    } else {
        return CertificateVerificationResult::InvalidCertificateChain;
    }
}

CertificateVerificationResult PkiHandler::verifyFirmwareCertificate(const std::string& firmwareCertificate) {
    if (!this->isManufacturerRootCertificateInstalled()) {
        EVLOG_warning << "Could not verify certificate because no MF root CA is installed";
        return CertificateVerificationResult::NoRootCertificateInstalled;
    }

    const auto certificates = this->getCertificatesFromChain(firmwareCertificate);

    if (!certificates.empty()) {
        return this->verifyCertificate(PkiEnum::MF, certificates);
    } else {
        return CertificateVerificationResult::InvalidCertificateChain;
    }
}

void PkiHandler::writeClientCertificate(const std::string& certificateChain,
                                        const CertificateSigningUseEnum& certificate_use) {

    const auto certificates = this->getCertificatesFromChain(certificateChain);

    if (certificates.empty()) {
        EVLOG_error << "Failed to write client certificate";
    } else {
        auto leafCert = certificates.at(0);
        std::string newPath;
        std::filesystem::path leafPath;

        if (certificate_use == CertificateSigningUseEnum::ChargingStationCertificate) {
            leafPath = this->csms_leaf_cert;
        } else {
            leafPath = this->secc_leaf_cert;
        }

        if (std::filesystem::exists(leafPath)) {
            const auto oldLeafPath =
                leafPath.parent_path() /
                (leafPath.filename().string().substr(0, leafPath.filename().string().find_last_of(".")) +
                 DateTime().to_rfc3339() + ".pem");
            std::rename(leafPath.c_str(), oldLeafPath.c_str());
        }
        leafCert->path = leafPath;
        leafCert->write(std::ios::out);
    }
}

std::string PkiHandler::generateCsr(const CertificateSigningUseEnum& certificateType, const std::string& szCountry,
                                    const std::string& szOrganization, const std::string& szCommon) {
    int rc;
    int nVersion = 0;
    int bits = 256;

    // csr req
    X509_REQ_ptr x509ReqPtr(X509_REQ_new(), X509_REQ_free);
    EVP_PKEY_ptr evpKey(EVP_PKEY_new(), EVP_PKEY_free);
    EC_KEY* ecKey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    X509_NAME* x509Name = X509_REQ_get_subject_name(x509ReqPtr.get());

    std::filesystem::path csrFile;
    std::filesystem::path privateKeyFile;

    // FIXME(piet): This just overrides the private key and in case no valid certificate will be delivered by a future
    // CertificateSigned.req from CSMS for this CSR the private key is lost
    if (certificateType == CertificateSigningUseEnum::ChargingStationCertificate) {
        csrFile = this->csms_leaf_csr;
        privateKeyFile = this->csms_leaf_key;
        std::rename((this->csms_leaf_key).c_str(), (this->csms_leaf_key_backup).c_str());
    } else {
        // CertificateSigningUseEnum::CertificateSigningUseEnum::V2GCertificate
        csrFile = this->secc_leaf_csr;
        privateKeyFile = secc_leaf_key;
        std::rename(this->secc_leaf_key.c_str(), this->secc_leaf_key_backup.c_str());
    }

    BIO_ptr out(BIO_new_file(csrFile.c_str(), "w"), ::BIO_free);
    BIO_ptr prkey(BIO_new_file(privateKeyFile.c_str(), "w"), ::BIO_free);
    BIO_ptr bio(BIO_new(BIO_s_mem()), ::BIO_free);

    // generate ec key pair
    rc = EC_KEY_generate_key(ecKey);
    if (rc != 1) {
        EVLOG_error << "Failed to generate EC key pair";
    }

    rc = EVP_PKEY_assign_EC_KEY(evpKey.get(), ecKey);
    if (rc != 1) {
        EVLOG_error << "Failed to assign EC key to EVP key";
    }

    // write private key to file
    rc = PEM_write_bio_PrivateKey(prkey.get(), evpKey.get(), NULL, NULL, 0, NULL, NULL);
    if (rc != 1) {
        EVLOG_error << "Failed to write private key to file";
    }

    // set version of x509 req
    rc = X509_REQ_set_version(x509ReqPtr.get(), nVersion);
    if (rc != 1) {
        EVLOG_error << "Failed to set version of X509 request";
    }

    // set subject of x509 req
    rc = X509_NAME_add_entry_by_txt(x509Name, "C", MBSTRING_ASC,
                                    reinterpret_cast<const unsigned char*>(szCountry.c_str()), -1, -1, 0);
    if (rc != 1) {
        EVLOG_error << "Failed to add 'C' entry to X509 request";
    }

    rc = X509_NAME_add_entry_by_txt(x509Name, "O", MBSTRING_ASC,
                                    reinterpret_cast<const unsigned char*>(szOrganization.c_str()), -1, -1, 0);
    if (rc != 1) {
        EVLOG_error << "Failed to add 'O' entry to X509 request";
    }

    rc = X509_NAME_add_entry_by_txt(x509Name, "CN", MBSTRING_ASC,
                                    reinterpret_cast<const unsigned char*>(szCommon.c_str()), -1, -1, 0);
    if (rc != 1) {
        EVLOG_error << "Failed to add 'CN' entry to X509 request";
    }

    rc = X509_NAME_add_entry_by_txt(x509Name, "DC", MBSTRING_ASC, reinterpret_cast<const unsigned char*>("CPO"), -1, -1,
                                    0);

    if (rc != 1) {
        EVLOG_error << "Failed to add 'DC' entry to X509 request";
    }

    // 5. set public key of x509 req
    rc = X509_REQ_set_pubkey(x509ReqPtr.get(), evpKey.get());
    if (rc != 1) {
        EVLOG_error << "Failed to set public key of X509 request";
    }

    STACK_OF(X509_EXTENSION)* extensions = sk_X509_EXTENSION_new_null();
    X509_EXTENSION* ext_key_usage = X509V3_EXT_conf_nid(NULL, NULL, NID_key_usage, "digitalSignature, keyAgreement");
    X509_EXTENSION* ext_basic_constraints = X509V3_EXT_conf_nid(NULL, NULL, NID_basic_constraints, "critical,CA:false");
    sk_X509_EXTENSION_push(extensions, ext_key_usage);
    sk_X509_EXTENSION_push(extensions, ext_basic_constraints);

    rc = X509_REQ_add_extensions(x509ReqPtr.get(), extensions);
    if (rc != 1) {
        EVLOG_error << "Failed to add extensions to X509 request";
    }

    // 6. set sign key of x509 req
    X509_REQ_sign(x509ReqPtr.get(), evpKey.get(), EVP_sha256());

    // 7. write csr to file
    rc = PEM_write_bio_X509_REQ(out.get(), x509ReqPtr.get());
    if (rc != 1) {
        EVLOG_error << "Failed to write X509 request to file";
    }

    // 8. read csr from file
    rc = PEM_write_bio_X509_REQ(bio.get(), x509ReqPtr.get());
    if (rc != 1) {
        EVLOG_error << "Failed to read X509 request from file";
    }

    BUF_MEM* mem = NULL;
    BIO_get_mem_ptr(bio.get(), &mem);
    std::string csr(mem->data, mem->length);

    EVLOG_debug << csr;

    return csr;
}

bool PkiHandler::isCentralSystemRootCertificateInstalled() {
    return this->pki_bundle_map.count(CABundleType::CSMS);
}

bool PkiHandler::isV2GRootCertificateInstalled() {
    return this->pki_bundle_map.count(CABundleType::V2G);
}

bool PkiHandler::isManufacturerRootCertificateInstalled() {
    return this->pki_bundle_map.count(CABundleType::MF);
}

bool PkiHandler::isCsmsLeafCertificateInstalled() {
    return std::filesystem::exists(this->csms_leaf_cert);
}

std::optional<std::vector<CertificateHashDataChain>>
PkiHandler::getRootCertificateHashData(std::optional<std::vector<CertificateType>> certificateTypes) {
    std::optional<std::vector<CertificateHashDataChain>> certificateHashDataChainOpt = std::nullopt;

    std::vector<CertificateHashDataChain> certificateHashDataChain;
    std::vector<std::shared_ptr<X509Certificate>> caCertificates;

    // set certificateTypes to all types if not set
    if (!certificateTypes.has_value()) {
        std::vector<CertificateType> types = {
            CertificateType::CentralSystemRootCertificate, CertificateType::CSMSRootCertificate,
            CertificateType::ManufacturerRootCertificate,  CertificateType::MORootCertificate,
            CertificateType::V2GRootCertificate,           CertificateType::V2GCertificateChain};
        certificateTypes.emplace(types);
    }

    // collect all required ca certificates
    for (const auto& certificateType : certificateTypes.value()) {
        auto pki = getPkiEnumFromCertificateType(certificateType);
        auto tempCaCertificates = this->getCaCertificates(getPkiEnumFromCertificateType(certificateType));
        caCertificates.insert(caCertificates.end(), tempCaCertificates.begin(), tempCaCertificates.end());
    }

    // collect all required ca certificate chains
    auto certChains = this->getCaCertificateChains(caCertificates);

    // special handling for V2GCertificateChain
    if (std::find(certificateTypes.value().begin(), certificateTypes.value().end(),
                  CertificateType::V2GCertificateChain) != certificateTypes.value().end()) {
        const auto v2gCertificateChain = this->getV2GCertificateChain();
        if (!v2gCertificateChain.empty()) {
            certChains.push_back(v2gCertificateChain);
        }
    }

    // iterate over certChains and add respective entries
    for (const auto& certChain : certChains) {
        CertificateHashDataChain certificateHashDataChainEntry;
        if (!certChain.empty()) {
            certificateHashDataChainEntry.certificateType = certChain.at(0)->type;
            CertificateHashDataType certificateHashData;
            certificateHashData.hashAlgorithm = HashAlgorithmEnumType::SHA256;
            certificateHashData.issuerNameHash = certChain.at(0)->getIssuerNameHash();
            certificateHashData.issuerKeyHash = certChain.at(0)->getIssuerKeyHash();
            certificateHashData.serialNumber = certChain.at(0)->getSerialNumber();
            certificateHashDataChainEntry.certificateHashData = certificateHashData;
        }
        // only add child certificate data for V2GCertificateChain
        if (certChain.size() > 1 and certChain.at(1)->type == CertificateType::V2GCertificateChain) {
            std::vector<ocpp::CertificateHashDataType> childCertificateHashData;
            for (size_t i = 1; i < certChain.size(); i++) {
                auto cert = certChain.at(i);
                CertificateHashDataType certificateHashData;
                certificateHashData.hashAlgorithm = HashAlgorithmEnumType::SHA256;
                certificateHashData.issuerNameHash = cert->getIssuerNameHash();
                certificateHashData.issuerKeyHash = cert->getIssuerKeyHash();
                certificateHashData.serialNumber = cert->getSerialNumber();
                childCertificateHashData.push_back(certificateHashData);
            }
            certificateHashDataChainEntry.childCertificateHashData.emplace(childCertificateHashData);
        }
        certificateHashDataChain.push_back(certificateHashDataChainEntry);
    }

    if (!caCertificates.empty()) {
        certificateHashDataChainOpt.emplace(certificateHashDataChain);
    }

    return certificateHashDataChainOpt;
}

DeleteCertificateResult PkiHandler::deleteRootCertificate(CertificateHashDataType certificate_hash_data,
                                                          int32_t security_profile) {
    EVLOG_info << "Attempting to delete a root certificate";
    // get ca certificates including symlinks to delete both files
    std::vector<std::shared_ptr<X509Certificate>> caCertificates = this->getCaCertificates(true);
    bool foundCertificateToDelete = false;
    bool removalFailed = false;

    const auto numberOfCsmsCa = this->getCaCertificates(PkiEnum::CSMS).size();
    for (std::shared_ptr<X509Certificate> cert : caCertificates) {
        if (cert->getIssuerNameHash() == certificate_hash_data.issuerNameHash.get() &&
            cert->getIssuerKeyHash() == certificate_hash_data.issuerKeyHash.get() &&
            cert->getSerialNumber() == certificate_hash_data.serialNumber.get()) {
            // dont delete if only one root ca is installed and tls is in use
            if (cert->type == CertificateType::CentralSystemRootCertificate && numberOfCsmsCa == 1 &&
                security_profile >= 2) {
                EVLOG_warning
                    << "Rejecting attempt to delete last CSMS root certificate while on security profile 2 or 3";
                return DeleteCertificateResult::Failed;
            }
            foundCertificateToDelete = true;
            if (removeCertFromFile(cert->str, cert->path)) {
                removalFailed = false;
            }
        }
    }

    if (!foundCertificateToDelete) {
        EVLOG_info << "Could not find root certificate to remove";
        return DeleteCertificateResult::NotFound;
    }
    if (removalFailed) {
        EVLOG_info << "Failed to remove root certificate";
        return DeleteCertificateResult::Failed;
    }
    EVLOG_info << "Removed root certificate successfully";
    return DeleteCertificateResult::Accepted;
}

InstallCertificateResult PkiHandler::installRootCertificate(const std::string& rootCertificate,
                                                            const CertificateType& certificateType,
                                                            std::optional<int32_t> certificateStoreMaxLength,
                                                            std::optional<bool> additionalRootCertificateCheck) {
    EVLOG_info << "Installing new root certificate of type: "
               << ocpp::conversions::certificate_type_to_string(certificateType);
    InstallCertificateResult installCertificateResult = InstallCertificateResult::Valid;

    if (certificateStoreMaxLength && this->getCaCertificates().size() >= size_t(certificateStoreMaxLength.value())) {
        return InstallCertificateResult::CertificateStoreMaxLengthExceeded;
    }

    std::shared_ptr<X509Certificate> cert = loadFromString(rootCertificate);
    if (cert == nullptr) {
        return InstallCertificateResult::InvalidFormat;
    }

    std::vector<std::shared_ptr<X509Certificate>> certificates;
    certificates.push_back(cert);

    if (this->isCaCertificateAlreadyInstalled(cert)) {
        EVLOG_info << "Root certificate is already installed";
        return InstallCertificateResult::WriteError;
    }

    std::string certFileName = cert->getCommonName() + "-" + DateTime().to_rfc3339() + ".pem";
    if (this->isCentralSystemRootCertificateInstalled() &&
        (certificateType == CertificateType::CentralSystemRootCertificate or
         certificateType == CertificateType::CSMSRootCertificate) &&
        additionalRootCertificateCheck.has_value() && additionalRootCertificateCheck.value()) {
        if (this->verifyCertificate(PkiEnum::CSMS, certificates) != CertificateVerificationResult::Valid) {
            installCertificateResult = InstallCertificateResult::InvalidCertificateChain;
        }

        cert->path = this->pki_bundle_map.at(CABundleType::CSMS);
        std::rename(this->pki_bundle_map.at(CABundleType::CSMS).c_str(),
                    this->pki_bundle_map.at(CABundleType::CSMS_BACKUP).c_str());

    } else {
        if (certificateType == CertificateType::CentralSystemRootCertificate or
            certificateType == CertificateType::CSMSRootCertificate) {
            cert->path = this->pki_bundle_map.at(CABundleType::CSMS);
        } else if (certificateType == CertificateType::ManufacturerRootCertificate) {
            cert->path = this->pki_bundle_map.at(CABundleType::MF);
        } else if (certificateType == CertificateType::V2GRootCertificate) {
            cert->path = this->pki_bundle_map.at(CABundleType::V2G);
        } else if (certificateType == CertificateType::MORootCertificate) {
            cert->path = this->pki_bundle_map.at(CABundleType::MO);
        }
        installCertificateResult = InstallCertificateResult::Ok;
    }

    if (installCertificateResult == InstallCertificateResult::Ok ||
        installCertificateResult == InstallCertificateResult::Valid) {
        if (!cert->write(std::ios::app)) {
            installCertificateResult = InstallCertificateResult::WriteError;
            if (certificateType == CertificateType::CentralSystemRootCertificate or
                certificateType == CertificateType::CSMSRootCertificate) {
                std::rename(this->pki_bundle_map.at(CABundleType::CSMS_BACKUP).c_str(),
                            this->pki_bundle_map.at(CABundleType::CSMS).c_str());
            }
        }
    }
    return installCertificateResult;
}

std::shared_ptr<X509Certificate>
PkiHandler::getLeafCertificate(const CertificateSigningUseEnum& certificate_signing_use) {
    std::shared_ptr<X509Certificate> cert = nullptr;
    int validIn = INT_MIN;

    std::filesystem::path leafPath;
    if (certificate_signing_use == CertificateSigningUseEnum::ChargingStationCertificate) {
        leafPath = this->csms_leaf_cert;
    } else {
        leafPath = this->secc_leaf_cert;
    }

    return loadFromFile(leafPath);
}

std::filesystem::path PkiHandler::getLeafPrivateKeyPath(const CertificateSigningUseEnum& certificate_signing_use) {
    if (certificate_signing_use == CertificateSigningUseEnum::ChargingStationCertificate) {
        return this->csms_leaf_key;
    } else {
        return this->secc_leaf_key;
    }
}

void PkiHandler::removeCentralSystemFallbackCa() {
    std::remove(this->pki_bundle_map.at(CABundleType::CSMS_BACKUP).c_str());
}

void PkiHandler::useCsmsFallbackRoot() {
    if (std::filesystem::exists(this->pki_bundle_map.at(CABundleType::CSMS_BACKUP))) {
        std::remove(this->pki_bundle_map.at(CABundleType::CSMS).c_str()); // remove recently installed ca
        std::rename(this->pki_bundle_map.at(CABundleType::CSMS_BACKUP).c_str(),
                    this->pki_bundle_map.at(CABundleType::CSMS).c_str());
    } else {
        EVLOG_warning << "Cant use csms fallback root CA because no fallback file exists";
    }
}

int PkiHandler::validIn(const std::string& certificate) {
    std::shared_ptr<X509Certificate> cert = loadFromString(certificate);
    return X509_cmp_current_time(X509_get_notBefore(cert->x509));
}

int PkiHandler::getDaysUntilLeafExpires(const CertificateSigningUseEnum& certificate_signing_use) {
    std::shared_ptr<X509Certificate> cert = this->getLeafCertificate(certificate_signing_use);
    if (cert != nullptr) {
        const ASN1_TIME* expires = X509_get0_notAfter(cert->x509);
        int days, seconds;
        ASN1_TIME_diff(&days, &seconds, NULL, expires);
        return days;
    } else {
        return 0;
    }
}

void PkiHandler::updateOcspCache(const OCSPRequestData& ocspRequestData, const std::string& ocspResponse) {
    const auto caCertificates = this->getCaCertificates(PkiEnum::CSO);
    for (const auto& cert : caCertificates) {
        if (cert->getIssuerNameHash() == ocspRequestData.issuerNameHash &&
            cert->getIssuerKeyHash() == ocspRequestData.issuerKeyHash &&
            cert->getSerialNumber() == ocspRequestData.serialNumber) {
            EVLOG_info << "Writing OCSP Response to filesystem";
            const auto ocspPath = cert->path.parent_path() / "ocsp";
            if (!std::filesystem::exists(ocspPath)) {
                std::filesystem::create_directories(ocspPath);
            }
            const auto ocspFilePath = ocspPath / cert->path.filename().replace_extension(".ocsp.der");
            std::ofstream fs(ocspFilePath.string());
            fs << ocspResponse << std::endl;
            fs.close();
        }
    }
}

std::vector<OCSPRequestData> PkiHandler::getOCSPRequestData() {
    std::vector<OCSPRequestData> ocspRequestData;
    const auto caCertificates = this->getCaCertificates(PkiEnum::CSO);

    for (auto const& certificate : caCertificates) {
        ocspRequestData.push_back(certificate->getOCSPRequestData());
    }
    return ocspRequestData;
}

CertificateVerificationResult getCertificateValidationResult(const int ec) {
    switch (ec) {
    case X509_V_ERR_CERT_HAS_EXPIRED:
        EVLOG_error << "Certificate has expired";
        return CertificateVerificationResult::Expired;
    case X509_V_ERR_CERT_SIGNATURE_FAILURE:
        EVLOG_error << "Invalid signature";
        return CertificateVerificationResult::InvalidSignature;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
        EVLOG_error << "Invalid certificate chain";
        return CertificateVerificationResult::InvalidCertificateChain;
    case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
        EVLOG_error << "Unable to verify leaf signature";
        return CertificateVerificationResult::InvalidSignature;
    default:
        EVLOG_error << X509_verify_cert_error_string(ec);
        return CertificateVerificationResult::InvalidCertificateChain;
    }
}

CertificateVerificationResult
PkiHandler::verifyCertificate(const PkiEnum& pki, const std::vector<std::shared_ptr<X509Certificate>>& certificates,
                              const std::optional<std::string>& chargeBoxSerialNumber) {
    EVLOG_info << "Verifying certificate...";
    const auto leaf_certificate = certificates.at(0);
    if (leaf_certificate->x509 == NULL) {
        EVLOG_warning << "Invalid certificate chain";
        return CertificateVerificationResult::InvalidCertificateChain;
    }

    X509_STORE_ptr store_ptr(X509_STORE_new(), ::X509_STORE_free);
    X509_STORE_CTX_ptr store_ctx_ptr(X509_STORE_CTX_new(), ::X509_STORE_CTX_free);

    for (size_t i = 1; i < certificates.size(); i++) {
        if (certificates.at(i)->x509 == NULL) {
            EVLOG_warning << "Invalid certificate chain";
            return CertificateVerificationResult::InvalidCertificateChain;
        }
        X509_STORE_add_cert(store_ptr.get(), certificates.at(i)->x509);
    }

    X509_STORE_load_locations(store_ptr.get(), this->pki_bundle_map.at(convertPkiEnumToCABundleType(pki)).c_str(),
                              NULL);

    if (pki != PkiEnum::MF) {
        // we always trust V2G if its not a Manufacturer certificate
        X509_STORE_load_locations(store_ptr.get(), this->pki_bundle_map.at(CABundleType::V2G).c_str(), NULL);
    }

    // X509_STORE_set_default_paths(store_ptr.get()); //FIXME(piet): When do we load default verify paths?

    X509_STORE_CTX_init(store_ctx_ptr.get(), store_ptr.get(), leaf_certificate->x509, NULL);

    // verifies the certificate chain based on ctx
    // verifies the certificate has not expired and is already valid
    if (X509_verify_cert(store_ctx_ptr.get()) != 1) {
        int ec = X509_STORE_CTX_get_error(store_ctx_ptr.get());
        return getCertificateValidationResult(ec);
    }

    if (chargeBoxSerialNumber.has_value() and
        X509_check_host(leaf_certificate->x509, chargeBoxSerialNumber.value().c_str(),
                        chargeBoxSerialNumber.value().length(), 0, NULL) == 0) {
        EVLOG_warning << "Subject field CN of certificate is not equal to ChargeBoxSerialNumber: "
                      << chargeBoxSerialNumber.value();
        return CertificateVerificationResult::InvalidCommonName;
    }
    EVLOG_info << "Certificate is valid";
    return CertificateVerificationResult::Valid;
}

std::vector<std::shared_ptr<X509Certificate>> PkiHandler::getCertificatesFromChain(const std::string& certChain) {
    std::vector<std::shared_ptr<X509Certificate>> certificates;
    static const std::regex cert_regex("-----BEGIN CERTIFICATE-----[\\s\\S]*?-----END CERTIFICATE-----");
    std::regex_iterator<const char*> cert_begin(certChain.c_str(), certChain.c_str() + certChain.size(), cert_regex);
    std::regex_iterator<const char*> cert_end;
    for (auto i = cert_begin; i != cert_end; ++i) {
        certificates.push_back(loadFromString(i->str()));
    }
    return certificates;
}

std::vector<std::vector<std::shared_ptr<X509Certificate>>>
PkiHandler::getCaCertificateChains(const std::vector<std::shared_ptr<X509Certificate>>& certificates) {

    std::vector<std::vector<std::shared_ptr<X509Certificate>>> certChains;
    std::vector<std::shared_ptr<X509Certificate>> rootCertificates;
    std::vector<std::shared_ptr<X509Certificate>> remainingCertificates;

    // collect list of rootCertificates (self-signed) and remaining ca certificates
    for (const auto& cert : certificates) {
        // check if self signed
        if (X509_NAME_cmp(X509_get_subject_name(cert->x509), X509_get_issuer_name(cert->x509)) == 0) {
            rootCertificates.push_back(cert);
        } else {
            remainingCertificates.push_back(cert);
        }
    }

    // iterate over the rootCertificates
    for (const auto& rootCert : rootCertificates) {
        // each root certificate builds up its chain
        std::vector<std::shared_ptr<X509Certificate>> certChain;
        certChain.push_back(rootCert);

        bool keepSearching = true;
        // keep searching if a the back of the certChain is an issuer
        while (keepSearching) {
            keepSearching = false;
            for (const auto& cert : remainingCertificates) {
                // check if back of chain issued one of the remaining certificates
                if (X509_check_issued(certChain.back()->x509, cert->x509) == X509_V_OK and certChain.back() != cert) {
                    certChain.push_back(cert);
                    keepSearching = true;
                }
            }
        }
        // FIXME(piet): branch chains are not handled so far. This approach assumes a unique chain starting from root
        certChains.push_back(certChain);
    }
    return certChains;
}

std::vector<std::shared_ptr<X509Certificate>> PkiHandler::getV2GCertificateChain() {
    std::vector<std::shared_ptr<X509Certificate>> v2gCertificateChain;

    const auto v2gLeaf = this->getLeafCertificate(CertificateSigningUseEnum::V2GCertificate);
    if (v2gLeaf != nullptr) {
        v2gLeaf->type = CertificateType::V2GCertificateChain;
        v2gCertificateChain.push_back(v2gLeaf);
        const auto csoSubCaCertificates = this->getCaCertificates(PkiEnum::CSO);
        v2gCertificateChain.insert(v2gCertificateChain.end(), csoSubCaCertificates.begin(), csoSubCaCertificates.end());
    }

    return v2gCertificateChain;
}

bool PkiHandler::isCaCertificateAlreadyInstalled(const std::shared_ptr<X509Certificate>& certificate) {
    const auto caCertificates = this->getCaCertificates(false);
    for (const auto& caCert : caCertificates) {
        if (caCert->getIssuerNameHash() == certificate->getIssuerNameHash() and
            caCert->getIssuerKeyHash() == certificate->getIssuerKeyHash() and
            caCert->getSerialNumber() == certificate->getSerialNumber()) {
            return true;
        }
    }
    return false;
}

std::vector<std::shared_ptr<X509Certificate>> PkiHandler::getCaCertificates(const PkiEnum& pki,
                                                                            const bool includeSymlinks) {
    std::vector<std::shared_ptr<X509Certificate>> caCertificates;

    const auto caBundle = loadFromFile(this->pki_bundle_map.at(convertPkiEnumToCABundleType(pki)));
    if (caBundle == nullptr) {
        return caCertificates;
    }
    caCertificates = this->getCertificatesFromChain(caBundle->str);
    for (const auto& cert : caCertificates) {
        // add path to it for further handling
        cert->path = caBundle->path;
    }
    return caCertificates;
}

std::vector<std::shared_ptr<X509Certificate>> PkiHandler::getCaCertificates(const bool includeSymlinks) {
    std::vector<std::shared_ptr<X509Certificate>> caCertificates;
    std::vector<PkiEnum> pkis = {PkiEnum::CSO, PkiEnum::CSMS, PkiEnum::MF, PkiEnum::MO, PkiEnum::V2G};

    for (const auto& pki : pkis) {
        auto certs = this->getCaCertificates(pki, includeSymlinks);
        caCertificates.insert(caCertificates.end(), certs.begin(), certs.end());
    }
    return caCertificates;
}

} // namespace ocpp
