// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest

#include <everest/logging.hpp>
#include <fstream>
#include <streambuf>

#include <ocpp1_6/pki_handler.hpp>

namespace ocpp1_6 {

PkiHandler::PkiHandler(std::string maindir) : maindir(boost::filesystem::path(maindir)) {
}

std::string PkiHandler::readFileToString(const boost::filesystem::path& path) {
    std::ifstream t(path.c_str());
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    return str;
}

X509* PkiHandler::readX509FromFile(const boost::filesystem::path& path) {
    BIO_ptr b(BIO_new(BIO_s_mem()), ::BIO_free);
    std::string file_str = readFileToString(path);
    BIO_puts(b.get(), file_str.c_str());
    return PEM_read_bio_X509(b.get(), NULL, NULL, NULL);
}

EVP_PKEY* PkiHandler::readEvpKey(const boost::filesystem::path& path) {
    BIO_ptr b(BIO_new(BIO_s_mem()), ::BIO_free);
    std::string file_str = readFileToString(path);
    BIO_write(b.get(), file_str.c_str(), strlen(file_str.c_str()));
    EVP_PKEY* pkey = 0;
    PEM_read_bio_PrivateKey(b.get(), &pkey, 0, 0);
    return pkey;
}

X509* PkiHandler::readX509FromString(const std::string& str) {
    BIO_ptr b(BIO_new(BIO_s_mem()), ::BIO_free);
    BIO_puts(b.get(), str.c_str());
    return PEM_read_bio_X509(b.get(), NULL, NULL, NULL);
}

std::string PkiHandler::generateCsr(const char* szCountry, const char* szProvince, const char* szCity,
                                    const char* szOrganization, const char* szCommon) {
    using BN_ptr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;
    using RSA_ptr = std::unique_ptr<RSA, decltype(&::RSA_free)>;
    using EVP_KEY_ptr = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
    using BIO_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;
    using X509_REQ_ptr = std::unique_ptr<X509_REQ, decltype(&::X509_REQ_free)>;

    int rc;
    RSA* r = NULL;
    BN_ptr bn(BN_new(), ::BN_free);

    int nVersion = 0;
    int bits = 2048;
    unsigned long e = RSA_F4;

    // csr req
    X509_REQ_ptr x509_req(X509_REQ_new(), ::X509_REQ_free);
    X509_NAME* x509_name = NULL;
    EVP_KEY_ptr pKey(EVP_PKEY_new(), ::EVP_PKEY_free);
    BIO_ptr out(BIO_new_file((this->maindir / CERTIFICATE_SIGNING_REQUEST_FILE).c_str(), "w"), ::BIO_free);
    BIO_ptr pbkey(BIO_new_file((this->maindir / PUBLIC_KEY_FILE).c_str(), "w"), ::BIO_free);
    BIO_ptr prkey(BIO_new_file((this->maindir / PRIVATE_KEY_FILE).c_str(), "w"), ::BIO_free);

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
    X509_REQ_set_version(x509_req.get(), nVersion);
    assert(rc == 1);

    // set subject of x509 req
    x509_name = X509_REQ_get_subject_name(x509_req.get());
    assert(rc == 1);
    X509_NAME_add_entry_by_txt(x509_name, "C", MBSTRING_ASC, (const unsigned char*)szCountry, -1, -1, 0);
    assert(rc == 1);
    X509_NAME_add_entry_by_txt(x509_name, "ST", MBSTRING_ASC, (const unsigned char*)szProvince, -1, -1, 0);
    assert(rc == 1);
    X509_NAME_add_entry_by_txt(x509_name, "L", MBSTRING_ASC, (const unsigned char*)szCity, -1, -1, 0);
    assert(rc == 1);
    X509_NAME_add_entry_by_txt(x509_name, "O", MBSTRING_ASC, (const unsigned char*)szOrganization, -1, -1, 0);
    assert(rc == 1);
    X509_NAME_add_entry_by_txt(x509_name, "CN", MBSTRING_ASC, (const unsigned char*)szCommon, -1, -1, 0);
    assert(rc == 1);

    // 5. set public key of x509 req
    EVP_PKEY_assign_RSA(pKey.get(), r);
    r = NULL;
    X509_REQ_set_pubkey(x509_req.get(), pKey.get());
    assert(rc == 1);

    // 6. set sign key of x509 req
    X509_REQ_sign(x509_req.get(), pKey.get(), EVP_sha256());
    assert(rc == 1);

    // 7. write csr to file
    PEM_write_bio_X509_REQ(out.get(), x509_req.get());
    assert(rc == 1);

    // 8. read csr from file
    out.~unique_ptr();
    return this->readFileToString(this->maindir / CERTIFICATE_SIGNING_REQUEST_FILE);
}

bool PkiHandler::validateCertificate(const std::string& certificateChain, const std::string& charge_box_serial_number) {
    X509_ptr root_ca_ptr(readX509FromFile(this->maindir / CS_ROOT_CA_FILE), ::X509_free);
    X509_ptr certificate_ptr(readX509FromString(certificateChain), ::X509_free);
    X509_STORE_ptr store_ptr(X509_STORE_new(), ::X509_STORE_free);
    X509_STORE_CTX_ptr store_ctx_ptr(X509_STORE_CTX_new(), ::X509_STORE_CTX_free);

    X509_STORE_set_verify_cb(store_ptr.get(), NULL);
    X509_STORE_add_cert(store_ptr.get(), root_ca_ptr.get());
    X509_STORE_CTX_init(store_ctx_ptr.get(), store_ptr.get(), certificate_ptr.get(), NULL);

    int rc = 0;

    // verifies the certificate chain based on ctx
    rc = X509_verify_cert(store_ctx_ptr.get());
    if (rc == 0) {
        // TODO(piet): trigger InvalidChargepointCertificate security event
        EVLOG(critical) << "Could not verify certificate chain";
        return false;
    }

    // verifies the signature of certificate x using public key of root_ca
    rc = X509_verify(certificate_ptr.get(), X509_get_pubkey(root_ca_ptr.get()));
    if (rc == 0) {
        // TODO(piet): trigger InvalidChargepointCertificate security event
        EVLOG(critical) << "Could not verify signature of certificate";
        return false;
    }

    // expiration check
    if (X509_cmp_current_time(X509_get_notBefore(certificate_ptr.get())) >= 0) {
        EVLOG(warning) << "Server certificate is not yet valid.";
    }
    if (X509_cmp_current_time(X509_get_notAfter(certificate_ptr.get())) <= 0) {
        // TODO(piet): trigger InvalidChargepointCertificate security event
        EVLOG(critical) << "Server certificate has expired.";
        return false;
    }

    rc = X509_check_host(certificate_ptr.get(), charge_box_serial_number.c_str(),
                         strlen(charge_box_serial_number.c_str()), 0, NULL);

    if (rc == 0) {
        EVLOG(warning) << "Subject field CN of certificate is not equal to ChargeBoxSerialNumber: "
                       << charge_box_serial_number;
    }
    return rc;
}

bool PkiHandler::isCentralSystemRootCertificateInstalled() {
    return true;
}

std::vector<X509*> PkiHandler::get_ca_certificates(CertificateUseEnumType type) {
    // FIXME(piet) iterate over directory and collect all ca files depending on the type
    std::vector<X509*> ca_certificates;
    if (type == CertificateUseEnumType::CentralSystemRootCertificate) {
        ca_certificates.push_back(readX509FromFile(this->maindir / CS_ROOT_CA_FILE));
    } else if (type == CertificateUseEnumType::ManufacturerRootCertificate) {
        ca_certificates.push_back(readX509FromFile(this->maindir / MF_ROOT_CA_FILE));
    }
    return ca_certificates;
}

boost::optional<std::vector<CertificateHashDataType>>
PkiHandler::getRootCertificateHashData(CertificateUseEnumType type) {
    boost::optional<std::vector<CertificateHashDataType>> certificate_hash_data_opt = boost::none;
    std::vector<CertificateHashDataType> certificate_hash_data_vec;

    std::vector<X509*> ca_certificates = this->get_ca_certificates(type);

    for (X509* ca_ptr : ca_certificates) {
        CertificateHashDataType certificate_hash_data;
        X509_ptr ca(ca_ptr, ::X509_free);
        std::string issuer_name_hash = this->get_issuer_name_hash(ca);
        std::string issuer_key_hash = this->get_issuer_key_hash(ca);
        std::string serial_number = this->get_serial(ca);
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

std::string PkiHandler::get_issuer_name_hash(X509_ptr& x509) {
    long issuer_name_hash = X509_issuer_name_hash(x509.get());
    std::stringstream stream;
    stream << std::hex << issuer_name_hash;
    std::string result(stream.str());
    return result;
}

std::string PkiHandler::get_serial(X509_ptr& x509) {
    ASN1_INTEGER* serial = X509_get_serialNumber(x509.get());
    BIGNUM* bnser = ASN1_INTEGER_to_BN(serial, NULL);
    char* hex = BN_bn2hex(bnser);
    std::string result(hex);
    return result;
}

std::string PkiHandler::get_issuer_key_hash(X509_ptr& x509) {
    BIO_ptr b(BIO_new(BIO_s_mem()), ::BIO_free);
    EVP_KEY_ptr pkey(X509_get_pubkey(x509.get()), ::EVP_PKEY_free);
    PEM_write_bio_PUBKEY(b.get(), pkey.get());
    BUF_MEM* mem = NULL;
    BIO_get_mem_ptr(b.get(), &mem);
    std::string pubkey_str(mem->data, mem->length);

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, pubkey_str.c_str(), pubkey_str.size());
    SHA256_Final(hash, &sha256);
    std::stringstream issuer_key_hash;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        issuer_key_hash << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return issuer_key_hash.str();
}

} // namespace ocpp1_6