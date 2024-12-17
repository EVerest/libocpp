#pragma once

#include <gmock/gmock.h>

#include <ocpp/common/evse_security.hpp>

namespace ocpp {
class EvseSecurityMock : public EvseSecurity {
    MOCK_METHOD(InstallCertificateResult, install_ca_certificate,
                (const std::string& certificate, const CaCertificateType& certificate_type));
    MOCK_METHOD(DeleteCertificateResult, delete_certificate, (const CertificateHashDataType& certificate_hash_data));
    MOCK_METHOD(InstallCertificateResult, update_leaf_certificate,
                (const std::string& certificate_chain, const CertificateSigningUseEnum& certificate_type));
    MOCK_METHOD(CertificateValidationResult, verify_certificate,
                (const std::string& certificate_chain, const LeafCertificateType& certificate_type));
    MOCK_METHOD(std::vector<CertificateHashDataChain>, get_installed_certificates,
                (const std::vector<CertificateType>& certificate_types));
    MOCK_METHOD(std::vector<OCSPRequestData>, get_v2g_ocsp_request_data, ());
    MOCK_METHOD(std::vector<OCSPRequestData>, get_mo_ocsp_request_data, (const std::string& certificate_chain));
    MOCK_METHOD(void, update_ocsp_cache,
                (const CertificateHashDataType& certificate_hash_data, const std::string& ocsp_response));
    MOCK_METHOD(bool, is_ca_certificate_installed, (const CaCertificateType& certificate_type));
    MOCK_METHOD(GetCertificateSignRequestResult, generate_certificate_signing_request,
                (const CertificateSigningUseEnum& certificate_type, const std::string& country,
                 const std::string& organization, const std::string& common, bool use_tpm));
    MOCK_METHOD(GetCertificateInfoResult, get_leaf_certificate_info,
                (const CertificateSigningUseEnum& certificate_type, bool include_ocsp));
    MOCK_METHOD(bool, update_certificate_links, (const CertificateSigningUseEnum& certificate_type));
    MOCK_METHOD(std::string, get_verify_file, (const CaCertificateType& certificate_type));
    MOCK_METHOD(std::string, get_verify_location, (const CaCertificateType& certificate_type));
    MOCK_METHOD(int, get_leaf_expiry_days_count, (const CertificateSigningUseEnum& certificate_type));
};
} // namespace ocpp
