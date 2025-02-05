// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 -  Pionix GmbH and Contributors to EVerest

#include <ocpp/v21/ctrlr_component_variables.hpp>

#include <ocpp/v201/ctrlr_component_variables.hpp>

namespace ocpp::v21 {
namespace ControllerComponentVariables {

const v201::ComponentVariable ChargingProfilePersistenceTxProfile = {
    v201::ControllerComponents::SmartChargingCtrlr,
    std::optional<v201::Variable>({"ChargingProfilePersistence", "TxProfile", std::nullopt}), std::nullopt};

const v201::ComponentVariable ChargingProfilePersistenceChargingStationExternalConstraints = {
    v201::ControllerComponents::SmartChargingCtrlr,
    std::optional<v201::Variable>({"ChargingProfilePersistence", "ChargingStationExternalConstraints", std::nullopt}),
    std::nullopt};

const v201::ComponentVariable ChargingProfilePersistenceLocalGeneration = {
    v201::ControllerComponents::SmartChargingCtrlr,
    std::optional<v201::Variable>({"ChargingProfilePersistence", "LocalGeneration", std::nullopt}), std::nullopt};

const v201::ComponentVariable ChargingProfileUpdateRateLimit = {
    v201::ControllerComponents::SmartChargingCtrlr,
    std::optional<v201::Variable>({"UpdateRateLimit", std::nullopt, std::nullopt}), std::nullopt};

const v201::ComponentVariable MaxExternalConstraintsId = {
    v201::ControllerComponents::SmartChargingCtrlr,
    std::optional<v201::Variable>({"MaxExternalConstraintsId", std::nullopt, std::nullopt}), std::nullopt};

const v201::ComponentVariable SupportedAdditionalPurposes = {
    v201::ControllerComponents::SmartChargingCtrlr,
    std::optional<v201::Variable>({"SupportedAdditionalPurposes", std::nullopt, std::nullopt}), std::nullopt};

const v201::ComponentVariable SupportsDynamicProfiles = {
    v201::ControllerComponents::SmartChargingCtrlr,
    std::optional<v201::Variable>({"SupportsFeature", "DynamicProfiles", std::nullopt}), std::nullopt};

const v201::ComponentVariable SupportsUseLocalTime = {
    v201::ControllerComponents::SmartChargingCtrlr,
    std::optional<v201::Variable>({"SupportsFeature", "UseLocalTime", std::nullopt}), std::nullopt};

const v201::ComponentVariable SupportsRandomizedDelay = {
    v201::ControllerComponents::SmartChargingCtrlr,
    std::optional<v201::Variable>({"SupportsFeature", "RandomizedDelay", std::nullopt}), std::nullopt};

const v201::ComponentVariable SupportsLimitAtSoC = {
    v201::ControllerComponents::SmartChargingCtrlr,
    std::optional<v201::Variable>({"SupportsFeature", "LimitAtSoC", std::nullopt}), std::nullopt};

const v201::ComponentVariable SupportsEvseSleep = {
    v201::ControllerComponents::SmartChargingCtrlr,
    std::optional<v201::Variable>({"SupportsFeature", "EvseSleep", std::nullopt}), std::nullopt};
} // namespace ControllerComponentVariables

namespace EvseComponentVariables {
const v201::Variable DCInputPhaseControl = {"DCInputPhaseControl", std::nullopt, std::nullopt};
} // namespace EvseComponentVariables
} // namespace ocpp::v21
