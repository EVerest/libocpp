// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#ifndef OCPP_COMMON_SUPPORT_OLDER_CPP_VERSIONS_HPP_
#define OCPP_COMMON_SUPPORT_OLDER_CPP_VERSIONS_HPP_

#ifndef BOOSTFILESYSTEM
#include <filesystem>
#else
#include <boost/filesystem.hpp>
#endif

#ifndef BOOSTFILESYSTEM
namespace fs = std::filesystem;
#else
namespace fs = boost::filesystem;
#endif

#endif /* OCPP_COMMON_SUPPORT_OLDER_CPP_VERSIONS_HPP_ */
