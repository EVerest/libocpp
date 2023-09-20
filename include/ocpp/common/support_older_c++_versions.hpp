/*
 * support_older_c++-versions.hpp
 *
 *  Created on: 20.09.2023
 *      Author: matthias_suess
 */

#ifndef LIB_LIBOCPPMYFORK_INCLUDE_OCPP_COMMON_SUPPORT_OLDER_C___VERSIONS_HPP_
#define LIB_LIBOCPPMYFORK_INCLUDE_OCPP_COMMON_SUPPORT_OLDER_C___VERSIONS_HPP_

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




#endif /* LIB_LIBOCPPMYFORK_INCLUDE_OCPP_COMMON_SUPPORT_OLDER_C___VERSIONS_HPP_ */
