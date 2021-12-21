/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <chrono>
#include <iostream>
#include <sys/prctl.h>
#include <thread>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <ocpp1_6/BootNotification.hpp>
#include <ocpp1_6/schemas.hpp>
#include <ocpp1_6/types.hpp>

#include <central_system.hpp>

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    po::options_description desc("OCPP central system");

    desc.add_options()("help,h", "produce help message");
    desc.add_options()("maindir", po::value<std::string>(), "set main dir in which the schemas folder resides");
    desc.add_options()("ip", po::value<std::string>(), "ip address on which the websocket should listen on");
    desc.add_options()("port", po::value<std::string>(), "port on which the websocket should listen on");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help") != 0) {
        std::cout << desc << "\n";
        return 1;
    }

    std::string maindir = ".";
    if (vm.count("maindir") != 0) {
        maindir = vm["maindir"].as<std::string>();
    }

    std::string ip = "0.0.0.0";
    if (vm.count("ip") != 0) {
        ip = vm["ip"].as<std::string>();
    }

    std::string port = "8428";
    if (vm.count("port") != 0) {
        port = vm["port"].as<std::string>();
    }

    ocpp1_6::Schemas schemas = ocpp1_6::Schemas(maindir);

    CentralSystem central_system = CentralSystem(ip, port, schemas);

    return 0;
}
