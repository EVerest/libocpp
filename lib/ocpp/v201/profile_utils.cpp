#include "ocpp/v201/profile_utils.hpp"
#include "ocpp/v201/ocpp_types.hpp"

namespace ocpp::v201 {

bool operator==(const ChargingProfile& lhs, const ChargingProfile& rhs) {
    return lhs.chargingProfileKind == rhs.chargingProfileKind &&
           lhs.chargingProfilePurpose == rhs.chargingProfilePurpose && lhs.id == rhs.id &&
           lhs.stackLevel == rhs.stackLevel;
}
} // namespace ocpp::v201