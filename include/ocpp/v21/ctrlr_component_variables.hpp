// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 -  Pionix GmbH and Contributors to EVerest

#pragma once

#include <ocpp/v201/ocpp_types.hpp>

namespace ocpp::v21 {

namespace ControllerComponentVariables {
extern const v201::ComponentVariable ChargingProfilePersistenceTxProfile;
extern const v201::ComponentVariable ChargingProfilePersistenceChargingStationExternalConstraints;
extern const v201::ComponentVariable ChargingProfilePersistenceLocalGeneration;
extern const v201::ComponentVariable ChargingProfileUpdateRateLimit;
extern const v201::ComponentVariable MaxExternalConstraintsId;
extern const v201::ComponentVariable SupportedAdditionalPurposes;
extern const v201::ComponentVariable SupportsDynamicProfiles;
extern const v201::ComponentVariable SupportsUseLocalTime;
extern const v201::ComponentVariable SupportsRandomizedDelay;
extern const v201::ComponentVariable SupportsLimitAtSoC;
extern const v201::ComponentVariable SupportsEvseSleep;
} // namespace ControllerComponentVariables

namespace EvseComponentVariables {
extern const v201::Variable DCInputPhaseControl;
} // namespace EvseComponentVariables
} // namespace ocpp::v21
