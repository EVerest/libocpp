// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_COMMON_EVSE_SECURITY
#define OCPP_COMMON_EVSE_SECURITY

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include <ocpp/common/types.hpp>

namespace ocpp {

class EvseSecurity {

//TODO(piet): add method docs

public:
    virtual InstallCertificateResult install_ca_certificate(const std::string& certificate,
                                                            const CaCertificateType& certificate_type) = 0;
    virtual DeleteCertificateResult delete_certificate(const CertificateHashDataType& certificate_hash_data) = 0;
    virtual InstallCertificateResult update_leaf_certificate(const std::string& certificate_chain,
                            const CertificateSigningUseEnum& certificate_type) = 0;
    virtual std::vector<CertificateHashDataChain>
    get_installed_certificates(const std::vector<CaCertificateType>& ca_certificate_types,
                               const std::vector<CertificateSigningUseEnum>& leaf_certificate_types) = 0;
    virtual std::vector<OCSPRequestData> get_ocsp_request_data() = 0;
    virtual void update_ocsp_cache(const CertificateHashDataType& certificate_hash_data,
                                   const std::string& ocsp_response) = 0;
    virtual bool is_ca_certificate_installed(const CaCertificateType& certificate_type) = 0;
    virtual std::string
    generate_certificate_signing_request(const CertificateSigningUseEnum& certificate_type,
                                         const std::string& country, const std::string& organization, const std::string& common) = 0;
    virtual std::optional<KeyPair> get_key_pair(const CertificateSigningUseEnum& certificate_type) = 0;
    virtual std::string get_verify_file(const CaCertificateType& certificate_type) = 0;
    virtual std::string get_verify_path(const CaCertificateType& certificate_type) = 0;
};

} // namespace ocpp

#endif // OCPP_COMMON_EVSE_SECURITY
