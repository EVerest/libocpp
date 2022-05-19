// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest

#include <everest/logging.hpp>
#include <fstream>
#include <streambuf>

#include <ocpp1_6/pki_handler.hpp>

namespace ocpp1_6 {

PkiHandler::PkiHandler(std::string maindir) : maindir(boost::filesystem::path(maindir)) {
}

std::string read_file_to_string(const boost::filesystem::path& path) {
    std::ifstream t(path.c_str());
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    return str;
}

X509Certificate::~X509Certificate() {
    X509_free(this->x509);
}

std::shared_ptr<X509Certificate> load_from_file(const boost::filesystem::path& path) {
    BIO_ptr b(BIO_new(BIO_s_mem()), ::BIO_free);
    std::string file_str = read_file_to_string(path);
    BIO_puts(b.get(), file_str.c_str());
    X509* x509 = PEM_read_bio_X509(b.get(), NULL, NULL, NULL);
    std::shared_ptr<X509Certificate> cert = std::make_shared<X509Certificate>(path, x509, file_str);
    return cert;
}

std::shared_ptr<X509Certificate> load_from_string(const std::string& str) {
    BIO_ptr b(BIO_new(BIO_s_mem()), ::BIO_free);
    BIO_puts(b.get(), str.c_str());
    X509* x509 = PEM_read_bio_X509(b.get(), NULL, NULL, NULL);

    if (x509 == NULL) {
        EVLOG(error) << "Could not parse str to X509";
    }

    std::shared_ptr<X509Certificate> cert = std::make_shared<X509Certificate>();
    cert->str = str;
    cert->x509 = x509;
    return cert;
}

bool X509Certificate::write() {
    std::ofstream fs(this->path);
    fs << this->str << std::endl;
    fs.close();
}

std::string PkiHandler::generateCsr(const char* szCountry, const char* szProvince, const char* szCity,
                                    const char* szOrganization, const char* szCommon) {
    int rc;
    RSA* r = NULL;
    BN_ptr bn(BN_new(), ::BN_free);

    int nVersion = 0;
    int bits = 2048;
    unsigned long e = RSA_F4;

    // csr req
    X509_REQ* x509_req = X509_REQ_new();
    EVP_KEY_ptr pKey(EVP_PKEY_new(), ::EVP_PKEY_free);
    BIO_ptr out(BIO_new_file((this->getCertsPath() / CERTIFICATE_SIGNING_REQUEST_FILE).c_str(), "w"), ::BIO_free);
    BIO_ptr pbkey(BIO_new_file((this->getCertsPath() / PUBLIC_KEY_FILE).c_str(), "w"), ::BIO_free);
    BIO_ptr prkey(BIO_new_file((this->getCertsPath() / PRIVATE_KEY_FILE).c_str(), "w"), ::BIO_free);
    BIO_ptr bio(BIO_new(BIO_s_mem()), ::BIO_free);

    // generate rsa key
    rc = BN_set_word(bn.get(), e);
    assert(rc == 1);
    r = RSA_new();
    RSA_generate_key_ex(r, bits, bn.get(), NULL);
    assert(rc == 1);

    // write keys to files
    rc = PEM_write_bio_RSAPublicKey(pbkey.get(), r);
    assert(rc == 1);
    rc = PEM_write_bio_RSAPrivateKey(prkey.get(), r, NULL, NULL, 0, NULL, NULL);
    assert(rc == 1);

    // set version of x509 req
    X509_REQ_set_version(x509_req, nVersion);
    assert(rc == 1);

    // set subject of x509 req
    X509_NAME_ptr x509_name_ptr(X509_REQ_get_subject_name(x509_req), ::X509_NAME_free);
    assert(rc == 1);
    X509_NAME_add_entry_by_txt(x509_name_ptr.get(), "C", MBSTRING_ASC, (const unsigned char*)szCountry, -1, -1, 0);
    assert(rc == 1);
    X509_NAME_add_entry_by_txt(x509_name_ptr.get(), "ST", MBSTRING_ASC, (const unsigned char*)szProvince, -1, -1, 0);
    assert(rc == 1);
    X509_NAME_add_entry_by_txt(x509_name_ptr.get(), "L", MBSTRING_ASC, (const unsigned char*)szCity, -1, -1, 0);
    assert(rc == 1);
    X509_NAME_add_entry_by_txt(x509_name_ptr.get(), "O", MBSTRING_ASC, (const unsigned char*)szOrganization, -1, -1, 0);
    assert(rc == 1);
    X509_NAME_add_entry_by_txt(x509_name_ptr.get(), "CN", MBSTRING_ASC, (const unsigned char*)szCommon, -1, -1, 0);
    assert(rc == 1);

    // 5. set public key of x509 req
    EVP_PKEY_assign_RSA(pKey.get(), r);
    r = NULL;
    X509_REQ_set_pubkey(x509_req, pKey.get());
    assert(rc == 1);

    // 6. set sign key of x509 req
    X509_REQ_sign(x509_req, pKey.get(), EVP_sha256());
    assert(rc == 1);

    // 7. write csr to file
    PEM_write_bio_X509_REQ(out.get(), x509_req);
    assert(rc == 1);

    // 8. read csr from file
    out.~unique_ptr();

    rc = PEM_write_bio_X509_REQ(bio.get(), x509_req);
    BUF_MEM* mem = NULL;
    BIO_get_mem_ptr(bio.get(), &mem);
    std::string pem(mem->data, mem->length);

    EVLOG(debug) << pem;

    return pem;
}

bool PkiHandler::validateCertificateChain(std::shared_ptr<X509Certificate> root_ca,
                                          std::shared_ptr<X509Certificate> cert) {
    X509_STORE_ptr store_ptr(X509_STORE_new(), ::X509_STORE_free);
    X509_STORE_CTX_ptr store_ctx_ptr(X509_STORE_CTX_new(), ::X509_STORE_CTX_free);

    X509_STORE_add_cert(store_ptr.get(), root_ca->x509);
    X509_STORE_CTX_init(store_ctx_ptr.get(), store_ptr.get(), cert->x509, NULL);

    int rc = 0;

    // verifies the certificate chain based on ctx
    rc = X509_verify_cert(store_ctx_ptr.get());
    if (rc == 0) {
        // TODO(piet): trigger InvalidChargepointCertificate security event
        int ec = X509_STORE_CTX_get_error(store_ctx_ptr.get());
        EVLOG(error) << "Could not verify certificate chain. Error code: " << ec
                     << " human readable: " << X509_verify_cert_error_string(ec);
        return false;
    } else {
        return true;
    }
}

bool PkiHandler::validateSignature(std::shared_ptr<X509Certificate> root_ca,
                                   std::shared_ptr<X509Certificate> new_root_ca) {
    int rc = X509_verify(new_root_ca->x509, X509_get_pubkey(root_ca->x509));
    if (rc == 0) {
        long ec = ERR_get_error();
        EVLOG(error) << "Could not verify signature of new root CA: " << ERR_error_string(ec, NULL);
        return false;
    } else {
        EVLOG(debug) << "Verified signature of new CA";
        return true;
    }
}

bool PkiHandler::verifyFirmwareCertificate(const std::string& firmwareCertificate) {
    std::shared_ptr<X509Certificate> cert = load_from_string(firmwareCertificate);
    std::shared_ptr<X509Certificate> mf_root_ca = load_from_file(this->getFile(MF_ROOT_CA_FILE));

    X509_STORE_ptr store_ptr(X509_STORE_new(), ::X509_STORE_free);
    X509_STORE_CTX_ptr store_ctx_ptr(X509_STORE_CTX_new(), ::X509_STORE_CTX_free);

    X509_STORE_add_cert(store_ptr.get(), mf_root_ca->x509);
    X509_STORE_CTX_init(store_ctx_ptr.get(), store_ptr.get(), cert->x509, NULL);

    int rc = 0;

    // verifies the certificate chain based on ctx
    rc = X509_verify_cert(store_ctx_ptr.get());
    if (rc == 0) {
        // TODO(piet): trigger InvalidChargepointCertificate security event
        int ec = X509_STORE_CTX_get_error(store_ctx_ptr.get());
        EVLOG(error) << "Could not verify certificate chain. Error code: " << ec
                     << " human readable: " << X509_verify_cert_error_string(ec);
        return false;
    } else {
        return true;
    }
}

std::shared_ptr<X509Certificate> PkiHandler::getRootCertificate(CertificateUseEnumType type) {

    if (type == CertificateUseEnumType::CentralSystemRootCertificate) {
        return load_from_file(CS_ROOT_CA_FILE);
    } else {
        return load_from_file(MF_ROOT_CA_FILE);
    }
}

bool PkiHandler::validateCertificate(const std::string& certificateChain, const std::string& charge_box_serial_number) {

    std::shared_ptr<X509Certificate> root_cert = load_from_file(this->getCertsPath() / CS_ROOT_CA_FILE);
    std::shared_ptr<X509Certificate> cert_chain = load_from_string(certificateChain);

    if (!this->validateCertificateChain(root_cert, cert_chain)) {
        return false;
    }

    // verifies the signature of certificate x using public key of root_ca
    int rc = X509_verify(cert_chain->x509, X509_get_pubkey(root_cert->x509));
    if (rc == 0) {
        // TODO(piet): trigger InvalidChargepointCertificate security event
        std::cout << "Could not verify signature of certificate" << std::endl;
        return false;
    }

    // expiration check
    if (X509_cmp_current_time(X509_get_notBefore(cert_chain->x509)) >= 0) {
        std::cout << "Server certificate is not yet valid." << std::endl;
    }

    if (X509_cmp_current_time(X509_get_notAfter(cert_chain->x509)) <= 0) {
        // TODO(piet): trigger InvalidChargepointCertificate security event
        std::cout << "Server certificate has expired." << std::endl;
        return false;
    }

    rc = X509_check_host(cert_chain->x509, charge_box_serial_number.c_str(), strlen(charge_box_serial_number.c_str()),
                         NULL, NULL);

    if (rc == 0) {
        std::cout << "Subject field CN of certificate is not equal to ChargeBoxSerialNumber: "
                  << charge_box_serial_number << std::endl;
    }

    std::cout << rc << std::endl;

    return rc;
}

bool PkiHandler::isCentralSystemRootCertificateInstalled() {
    return true;
}

boost::filesystem::path PkiHandler::getCertsPath() {
    return this->maindir / "certs";
}

boost::filesystem::path PkiHandler::getFile(boost::filesystem::path file_name) {
    boost::filesystem::path p = this->getCertsPath() / file_name;
    EVLOG(debug) << "Filename: " << p.string();
    return this->getCertsPath() / file_name;
}

std::vector<std::shared_ptr<X509Certificate>> PkiHandler::get_ca_certificates(CertificateUseEnumType type) {
    // FIXME(piet) iterate over directory and collect all ca files depending on the type
    std::vector<std::shared_ptr<X509Certificate>> ca_certificates;
    if (type == CertificateUseEnumType::CentralSystemRootCertificate) {
        ca_certificates.push_back(load_from_file(this->getCertsPath() / CS_ROOT_CA_FILE));
    } else if (type == CertificateUseEnumType::ManufacturerRootCertificate) {
        ca_certificates.push_back(load_from_file(this->getCertsPath() / MF_ROOT_CA_FILE));
    }
    return ca_certificates;
}

std::vector<std::shared_ptr<X509Certificate>> PkiHandler::get_ca_certificates() {
    // FIXME(piet) make this dynamic
    std::vector<std::shared_ptr<X509Certificate>> ca_certificates;
    if (boost::filesystem::exists(this->getCertsPath() / CS_ROOT_CA_FILE)) {
        ca_certificates.push_back(load_from_file(this->getCertsPath() / CS_ROOT_CA_FILE));
    }
    if (boost::filesystem::exists(this->getCertsPath() / MF_ROOT_CA_FILE)) {
        ca_certificates.push_back(load_from_file(this->getCertsPath() / MF_ROOT_CA_FILE));
    }
    return ca_certificates;
}

boost::optional<std::vector<CertificateHashDataType>>
PkiHandler::getRootCertificateHashData(CertificateUseEnumType type) {
    boost::optional<std::vector<CertificateHashDataType>> certificate_hash_data_opt = boost::none;
    std::vector<CertificateHashDataType> certificate_hash_data_vec;

    std::vector<std::shared_ptr<X509Certificate>> ca_certificates = this->get_ca_certificates(type);

    for (std::shared_ptr<X509Certificate> cert : ca_certificates) {
        CertificateHashDataType certificate_hash_data;
        std::string issuer_name_hash = this->get_issuer_name_hash(cert);
        std::string issuer_key_hash = this->get_issuer_key_hash(cert);
        std::string serial_number = this->get_serial(cert);
        certificate_hash_data.hashAlgorithm = HashAlgorithmEnumType::SHA256;
        certificate_hash_data.issuerNameHash = issuer_name_hash;
        certificate_hash_data.issuerKeyHash = issuer_key_hash;
        certificate_hash_data.serialNumber = serial_number;
        certificate_hash_data_vec.push_back(certificate_hash_data);
    }

    if (!ca_certificates.empty()) {
        certificate_hash_data_opt.emplace(certificate_hash_data_vec);
    }

    return certificate_hash_data_opt;
}

DeleteCertificateStatusEnumType PkiHandler::delete_certificate(CertificateHashDataType certificate_hash_data) {
    std::vector<std::shared_ptr<X509Certificate>> ca_certificates = this->get_ca_certificates();
    DeleteCertificateStatusEnumType status = DeleteCertificateStatusEnumType::NotFound;

    for (std::shared_ptr<X509Certificate> cert : ca_certificates) {
        std::string issuer_name_hash = this->get_issuer_name_hash(cert);
        std::string isser_key_hash = this->get_issuer_key_hash(cert);
        std::string serial = this->get_serial(cert);
        if (issuer_name_hash == certificate_hash_data.issuerNameHash.get() &&
            isser_key_hash == certificate_hash_data.issuerKeyHash.get() &&
            serial == certificate_hash_data.serialNumber.get()) {
            bool success = boost::filesystem::remove(cert->path);
            if (success) {
                status = DeleteCertificateStatusEnumType::Accepted;
            } else {
                status = DeleteCertificateStatusEnumType::Failed;
            }
        }
    }
    return status;
}

InstallCertificateStatusEnumType
PkiHandler::install_certificate(InstallCertificateRequest msg, boost::optional<int32_t> certificate_store_max_length,
                                boost::optional<bool> additional_root_certificate_check) {
    InstallCertificateStatusEnumType status = InstallCertificateStatusEnumType::Rejected;

    std::vector<std::shared_ptr<X509Certificate>> ca_certificates = this->get_ca_certificates();
    if (certificate_store_max_length != boost::none && ca_certificates.size() >= certificate_store_max_length.get()) {
        return status;
    }

    std::shared_ptr<X509Certificate> root_cert = this->getRootCertificate(msg.certificateType);
    std::shared_ptr<X509Certificate> cert = load_from_string(msg.certificate.get());
    if (this->validateSignature(root_cert, cert)) {
        cert->path = root_cert->path;
        if (cert->write()) {
            status = InstallCertificateStatusEnumType::Accepted;
            std::rename(CS_ROOT_CA_FILE.c_str(), (this->getCertsPath() / CS_ROOT_CA_FILE_BACKUP_FILE).c_str());
        } else {
            status = InstallCertificateStatusEnumType::Failed;
        }
    }
    return status;
}

void PkiHandler::removeFallbackCA() {
    std::remove(CS_ROOT_CA_FILE_BACKUP_FILE.c_str());
}

std::string PkiHandler::get_issuer_name_hash(std::shared_ptr<X509Certificate> cert) {
    long issuer_name_hash = X509_issuer_name_hash(cert->x509);
    std::stringstream stream;
    stream << std::hex << issuer_name_hash;
    std::string str(stream.str());
    EVLOG(debug) << "Issuer name hash: " << str;
    return str;
    // e60bd843bf2279339127ca19ab6967081dd6f95e745dc8b8632fa56031debe5b
}

std::string PkiHandler::get_serial(std::shared_ptr<X509Certificate> cert) {
    ASN1_INTEGER* serial = X509_get_serialNumber(cert->x509);
    BIGNUM* bnser = ASN1_INTEGER_to_BN(serial, NULL);
    char* hex = BN_bn2hex(bnser);
    std::string str(hex);
    str.erase(0, std::min(str.find_first_not_of('0'), str.size() - 1));
    return str;
}

std::string PkiHandler::get_issuer_key_hash(std::shared_ptr<X509Certificate> cert) {

    unsigned char tmphash[SHA256_DIGEST_LENGTH];
    X509_pubkey_digest(cert->x509, EVP_sha256(), tmphash, NULL);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << std::hex << (int)tmphash[i];
    std::string str = ss.str();
    EVLOG(debug) << "Issuer key hash: " << str;
    // 89ea6977e786fcbaeb4f04e4ccdbfaa6a6088e8ba8f7404033ac1b3a62bc36a1
    return str;
}

} // namespace ocpp1_6