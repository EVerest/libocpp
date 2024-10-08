![Github Actions](https://github.com/EVerest/libocpp/actions/workflows/build_and_test.yaml/badge.svg)

-----

# EVerest OCPP 1.6 Implementation (libocpp)

This C++ library provides a complete and production-ready solution for integrating OCPP 1.6 into your electric vehicle (EV) charging stations. Our implementation enables seamless communication between charging stations and central management systems, supporting remote control, monitoring, and billing functionalities.

## Table of contents

- [Functionality Support](#functionality-support)
- [CSMS Compatibility](#csms-compatibility)
- [Integration with EVerest](#integration-with-everest)
- [Standalone Integration](#standalone-integration)
  - [Overview of Expected Callbacks and Event Handlers](#overview-of-expected-callbacks-and-event-handlers)
  - [Usage for OCPP 1.6](#usage-for-ocpp-16)
    - [`ChargePoint` constructor](#chargepoint-constructor)
    - [Registering Callbacks](#registering-callbacks)
    - [Authorization](#authorization)
  - [Initialize the Database](#initialize-the-database)
- [Quickstart for OCPP 1.6](#quickstart-for-ocpp-16)
- [Build and Install `libocpp`](#build-and-install-libocpp)
- [Building the Doxygen Documentation](#building-the-doxygen-documentation)
- [Unit Testing](#unit-testing)
- [Building with FetchContent Instead of EDM](#building-with-fetchcontent-instead-of-edm)
- [Support for TPM Keys](#support-for-tpm-keys)
- [Support for `websocket++`](#support-for-websocket)
- [Support for `iface`](#support-for-iface)
 
## Functionality Support

The EVerest OCPP 1.6 implementation (`libocpp`) provides comprehensive support for OCPP 1.6 functionalities. Below is an overview of the supported features:

| OCPP 1.6 Functionality       | Supported                    |
| ---------------------------- | ---------------------------- |
| Core                         | :heavy_check_mark: Yes       |
| Firmware Management          | :heavy_check_mark: Yes       |
| Local Auth List Management   | :heavy_check_mark: Yes       |
| Reservation                  | :heavy_check_mark: Yes       |
| Smart Charging               | :heavy_check_mark: Yes       |
| Remote Trigger               | :heavy_check_mark: Yes       |

| Whitepapers & Application Notes             | Supported              |
| ------------------------------------------- | ---------------------- |
| OCPP 1.6 Security Whitepaper (3rd edition)  | :heavy_check_mark: Yes |
| Using ISO 15118 Plug & Charge with OCPP 1.6 | :heavy_check_mark: Yes |
| OCPP & California Pricing Requirements      | :heavy_check_mark: Yes |

## CSMS Compatibility

The EVerest implementation of OCPP 1.6 has been tested against the
OCPP Compliance Test Tool (OCTT and OCTT2) during the implementation.

The following table shows CSMSs with which this library was tested.
If you provide a CSMS that is not yet listed here, feel free to
[contact us](https://lists.lfenergy.org/g/everest)!

- chargecloud
- chargeIQ
- Chargetic
- Compleo
- Current
- Daimler Truck
- ev.energy
- eDRV
- Fastned
- [Open Charging Cloud (GraphDefined)](https://github.com/OpenChargingCloud/WWCP_OCPP)
- Electrip Global
- EnergyStacks
- EV-Meter
- Fraunhofer IAO (ubstack CHARGE)
- Green Motion
- gridundco
- ihomer (Infuse CPMS)
- iLumen
- JibeCompany (CharlieV CMS and Chargebroker proxy)
- MSI
- PUMP (PUMP Connect)
- Scoptvision (Scopt Powerconnect)
- Siemens
- [SteVe](https://github.com/steve-community/steve)
- Syntech
- Trialog
- ubitricity
- Weev Energy

## Integration with EVerest

The libocpp OCPP 1.6 module is automatically integrated within [everest-core](https://github.com/EVerest/everest-core) — the complete software stack for your charging station.

When you run libocpp with OCPP 1.6 through EVerest, the build process of everest-core handles the installation of all necessary dependencies. This includes initializing the device model database using the [config.json](/config/v16/config.json) file.

## Standalone Integration

OCPP is a protocol that affects, controls, and monitors many areas of a charging station's operation. The `libocpp` library is just the messenger for this protocol. It is intended to provide mechanisms for connecting to and authenticating with a CSMS, sending and receiving the OCPP messages that govern behaviors in the standard, and to the track state required for a charging station to conform to the protocol all while minimizing hardware- or implementation-specific functionality.

The actual substance of how a charging station reacts to or initiates an OCPP command (such as `Reset.req` or `RemoteStartTransaction.req`) is left to the rest of the charging station's systems. This is done by providing means of (a) registering **callbacks** that can be triggered by a `libocpp` `ChargePoint` in response to certain events and (b) reacting to various **event handlers** defined on a `ChargePoint` in other areas of the charging station's codebase.

> [!IMPORTANT] Integrating this library with your charging station requires both (a) defining **callbacks** that enable control of your station by `libocpp` and (b) calling `libocpp` **event handlers** in your charging station's systems in response to new events and data in order to keep `libocpp` up to date on station information.

### Terminology
Throughout this document and the `libocpp` codebase, the following conventions are followed:

- A **callback** is a function providing the actual station-specific implementation of an OCPP command. It allows a `libocpp` `ChargePoint` to control other systems within a charging station. By convention, each callback on a `ChargePoint` has a name suffixed with `_callback` (for instance, `unlock_connector_callback`). A suitable `std::function` can be **registered** as a callback on a `ChargePoint` by providing it as an argument for the relevant `register_` function (such as `register_unlock_connector_callback`).

- An **event handler** is a public function defined on a `ChargePoint` that allows a charging station to update the state being tracked by the `ChargePoint` based on new information (meter values, charging session events, etc.) and (indirectly) send messages to a CSMS. By convention, the names of event handlers on the `ChargePoint` are each prefixed with `on_` (for instance, `on_meter_values`).

The complete set of callbacks and event handlers defined on an OCPP 1.6 `ChargePoint` can be viewed in the latter's [header file](/include/ocpp/v16/charge_point.hpp).

### Overview of Expected Callbacks and Event Handlers

The following section will give you a high-level overview of how to integrate `libocpp` with your application. Please use the [Doxygen documentation](#building-the-doxygen-documentation) as an additional source of information about the `ChargePoint` API.

In EVerest the OCPP module leverages several other modules to perform tasks that relate to authorization, reservations, charging session handling and system tasks like rebooting or firmware updates.

- Auth orchestrates authorization, utilizing different token providers like RFID reads and token validators. Libocpp mainly acts as a token validator, but in the case of RemoteStartTransactions it acts as a token provider as well
- EvseManager manages the charging session and charging state machine by communicating with a "board support package", a driver for the charging hardware that abstracts away the control pilot, relay control, power meters, etc. The EvseManager also handles reservations.
- System handles firmware updates, log uploads and resets

The following sections explain the steps you can follow to implement their functionality on your own and integrate the libocpp directly into your charging station software without relying on EVerest. However, in most cases it's much easier to write an EVerest driver using the *everest-core/interfaces/board_support_AC.yaml* interface.

### Usage for OCPP 1.6

#### `ChargePoint` Constructor

The main entrypoint for `libocpp` for OCPP 1.6 is the `ocpp::v16::ChargePoint` constructor.
This is defined in libocpp/include/ocpp/v16/charge_point.hpp and takes the following parameters:

- config: a std::string that contains the libocpp 1.6 config. There are example configs that work with a [SteVe](https://github.com/steve-community/steve) installation [running in Docker](https://github.com/EVerest/everest-utils/tree/main/docker/steve), for example: [config-docker.json](/config/v16/config-docker.json)

- share_path: a std::filesystem path containing the path to the OCPP modules folder, for example pointing to */usr/share/everest/modules/OCPP*. This path contains the following files and directories and is installed by the libocpp  install target:

  ```bash
  .
  ├── config-docker.json
  ├── config-docker-tls.json
  ├── config.json
  ├── init.sql
  ├── logging.ini
  └── profile_schemas
      ├── Config.json
      ├── Core.json
      ├── FirmwareManagement.json
      ├── Internal.json
      ├── LocalAuthListManagement.json
      ├── PnC.json
      ├── Reservation.json
      ├── Security.json
      ├── SmartCharging.json
      └── Custom.json
  ```

  Here you can find:
  - the aforementioned config files

  - a *logging.ini* that is needed to initialize logging with Everest::Logging::init(path_to_logging_ini, "name_of_binary")

  - a *init.sql* file which contains the database schema used by libocpp for its sqlite database

  - and a *profile_schemas* directory. This contains json schema files that are used to validate the libocpp config. The schemas are split up according to the OCPP1.6 feature profiles like Core, FirmwareManagement and so on. Additionally there is a schema for "Internal" configuration options (for example the ChargePointId, or CentralSystemURI). A "PnC" schema for the ISO 15118 Plug & Charge with OCPP 1.6 Application note, a "Security" schema for the OCPP 1.6 Security Whitepaper (3rd edition) and an exemplary "Custom" schema are provided as well. The Custom.json could be modified to be able to add custom configuration keys. Finally there's a Config.json schema that ties everything together

- user_config_path: this points to a "user config", which we call a configuration file that's merged with the config that's provided in the "config" parameter. Here you can add, remove and overwrite settings without modifying the config passed in the first parameter directly. This is also used by libocpp to persistently modify config entries that are changed by the CSMS that should persist across restarts.

- database_path: this points to the location of the sqlite database that libocpp uses to keep track of connector availability, the authorization cache and auth list, charging profiles and transaction data

- sql_init_path: this points to the aforementioned init.sql file which contains the database schema used by libocpp for its sqlite database

- message_log_path: this points to the directory in which libocpp can put OCPP communication logfiles for debugging purposes. This behavior can be controlled by the "LogMessages" (set to true by default) and "LogMessagesFormat" (set to ["log", "html", "session_logging"] by default, "console" and "console_detailed" are also available) configuration keys in the "Internal" section of the config file. Please note that this is intended for debugging purposes only as it logs all communication, including authentication messages.

- evse_security: this is a pointer to an implementation of the [evse_security](/include/ocpp/common/evse_security.hpp) interface. This allows you to include your custom implementation of the security related operations according to this interface. If you set this value to nullptr, the internal implementation of the security related operations of libocpp will be used. In this case you need to specify the parameter security_configuration

- security_configuration: this parameter should only be set in case the evse_security parameter is nullptr. It specifies the file paths that are required to set up the internal evse_security implementation. Note that you need to specify bundle files for the CA certificates and directories for the certificates and keys

  The directory layout expected is as follows

  ```bash
  .
  ├── ca
  │   ├── csms
  │   │   └── CSMS_ROOT_CA.pem
  │   ├── cso
  │   │   ├── CPO_CERT_CHAIN.pem
  │   │   ├── CPO_SUB_CA1_LEAF.der
  │   │   ├── CPO_SUB_CA1.pem
  │   │   ├── CPO_SUB_CA2_LEAF.der
  │   │   └── CPO_SUB_CA2.pem
  │   ├── mf
  │   │   └── MF_ROOT_CA.pem
  │   ├── mo
  │   │   ├── INTERMEDIATE_MO_CA_CERTS.pem
  │   │   ├── MO_ROOT_CA.der
  │   │   ├── MO_ROOT_CA.pem
  │   │   ├── MO_SUB_CA1.der
  │   │   ├── MO_SUB_CA1.pem
  │   │   ├── MO_SUB_CA2.der
  │   │   └── MO_SUB_CA2.pem
  │   └── v2g
  │       ├── V2G_ROOT_CA.der
  │       └── V2G_ROOT_CA.pem
  ├── client
  │   ├── csms
  │   │   ├── CPO_CERT_CHAIN.pem
  │   │   ├── CPO_SUB_CA1.key
  │   │   ├── CPO_SUB_CA2.key
  │   │   ├── SECC_LEAF.der
  │   │   ├── SECC_LEAF.key
  │   │   └── SECC_LEAF.pem
  │   ├── cso
  │   │   ├── CPO_CERT_CHAIN.pem
  │   │   ├── CPO_SUB_CA1.key
  │   │   ├── CPO_SUB_CA2.key
  │   │   ├── SECC_LEAF.der
  │   │   ├── SECC_LEAF.key
  │   │   └── SECC_LEAF.pem
  │   └── v2g
  │       └── V2G_ROOT_CA.key
  ```

#### registering callbacks

You can (and in many cases MUST) register a number of callbacks so libocpp can interact with the charger. In EVerest most of this functionality is orchestrated by the "EvseManager" module, but you can also register your own callbacks interacting directly with your chargers software. Following is a list of callbacks that you must register and a few words about their purpose.

TODO: in a future version of libocpp the callbacks will be organised in a struct with optional members emphasizing the required and optional callbacks.

Some general notes: the "connector" parameter of some of the callbacks refers to the connector number as understood in the OCPP 1.6 specification, "0" means the whole charging station, the connectors with EVSEs used for charging cars start at "1".

| Callbacks                                        | Description                             |
| :----------------------------------------------- | :-------------------------------------- |
| `register_pause_charging_callback`               | this callback is used by libocpp to request pausing of charging, the "connector" parameter tells you which connector/EVSE has to pause charging |
| `register_resume_charging_callback`              | this callback is used by libocpp the request resuming of charging, the "connector" parameter tells you which connector/EVSE can resume charging |
| `register_stop_transaction_callback`             | in EVerest this calls the EvseManagers stop_transaction command which "Stops transactions and cancels charging externally, charging can only be resumed by replugging car. EVSE will also stop transaction automatically e.g. on disconnect, so this only needs to be called if the transaction should end before." this will then signal the following events:<br>• ChargingFinished<br>• TransactionFinished |
| `register_unlock_connector_callback`             | can be used by libocpp to force unlock a connector |
| `register_reserve_now_callback`                  | libocpp can use this to reserve a connector, reservation handling is outsourced to a reservation manager in EVerest that implements the reservation interface (everest-core/interfaces/reservation.yaml) |
| `register_upload_diagnostics_callback`           | uses a function (in EVerest provided by the System module) to upload the requested diagnostics file |
| `register_upload_logs_callback`                  | uses a function (in EVerest provided by the System module) to upload the requested log file |
| `register_update_firmware_callback`              | uses a function (in EVerest provided by the System module) to perform a firmware update |
| `register_signed_update_firmware_callback`       | uses a function (in EVerest provided by the System module) to perform a signed firmware update |
| `register_provide_token_callback`                | this callback is used in a remote start transaction to provide a token (prevalidated or not) to the authorization system |
| `register_set_connection_timeout_callback`       | used by libocpp to set the authorization or plug in connection timeout in the authorization system based on the "ConnectionTimeout" configuration key |
| `register_disable_evse_callback`                 | used to disable the EVSE (ChangeAvailability.req) |
| `register_enable_evse_callback`                  | used to enable the EVSE (ChangeAvailability.req) |
| `register_cancel_reservation_callback`           | used to cancel a reservation in the reservation manager (CancelReservation.req) |
| `register_signal_set_charging_profiles_callback` | used to signal that new charging schedule(s) have been set, you can then use get_all_composite_charging_schedules(duration_s) to get the new valid charging schedules |
| `register_is_reset_allowed_callback`             | used to inquire (in EVerest from the System module) if a reset is allowed |
| `register_reset_callback`                        | used to perform a reset of the requested type |
| `register_connection_state_changed_callback`     | used to inform about the connection state to the CSMS (connected = true, disconnected = false) |
| `register_configuration_key_changed_callback`    | used to react on a changed configuration key. This callback is called when the specified configuration key has been changed by the CSMS |

#### Functions that need to be triggered from the outside after new information is availble (on_... functions in the charge point API)

| on_ functions | Description |
| :-------- | :---------- |
| `on_log_status_notification(int32_t request_id, std::string log_status)` | can be used to notify libocpp of a log status notification |
| `on_firmware_update_status_notification(int32_t request_id, std::string firmware_update_status)` | can be used to notify libocpp of a firmware update status notification |
| `on_meter_values(int32_t connector, const Powermeter& powermeter)` | provides a Powermeter struct to libocpp (for sending meter values during charging sessions or periodically) |
| `on_max_current_offered(int32_t connector, int32_t max_current)` | the maximum current offered to the EV on this connector (in ampere) |

#### The following functions are triggered depending on different so called "Session Events" from the EvseManager

each of these functions will have a small note what the Session Event was and what it triggers in libocpp

| on _ functions                   | Description                                  |
| :------------------------------- | :----------------------------------------- |
| `on_enabled(int32_t connector)`  | Notifies libocpp that the connector is functional and operational |
| `on_disabled(int32_t connector)` | Notifies libocpp that the connector is disabled |
| `on_transaction_started`         | Notifies libocpp that a transaction at the given connector has started, this means that authorization is available and the car is plugged in.<br>Some of its parameters:<br>• session_id is an internal session_id originating in the EvseManager to keep track of the transaction, this is NOT to be mistaken for the transactionId from the StartTransactionResponse in OCPP!<br>• id_token is the token with which the transaction was authenticated<br>• meter_start contains the meter value in Wh for the connector at start of the transaction<br>• timestamp at the start of the transaction<br> |
| `on_transaction_stopped`         | Notifies libocpp that the transaction on the given connector with the given reason has been stopped.<br>Some of its parameters:<br>• timestamp at the end of the transaction<br>• energy_wh_import contains the meter value in Wh for the connector at end of the transaction<br> |
| `on_suspend_charging_ev`         | Notifies libocpp that the EV has paused charging |
| `on_suspend_charging_evse`       | Notifies libocpp that the EVSE has paused charging |
| `on_resume_charging`             | Notifies libocpp that charging has resumed |
| `on_session_started`             | this is mostly used for logging and changing the connector state |
| `on_session_stopped`             | this is mostly used for logging and changing the connector state |
| `on_error`                       | Notify libocpp of an error |
| `on_reservation_start`           | Notifies libocpp that a reservation has started |
| `on_reservation_end`             | Notifies libocpp that a reservation has ended |

#### Authorization

In EVerest authorization is handled by the Auth module and various auth token providers and validators. The OCPP module acts as both a token provider (for pre validated tokens in RemoteStartTransactions) and a token validator (using the authorize requests, or plug & charge).

To use libocpp as a auth token validator (e.g. before starting a transaction) you can call the "authorize_id_token" function of the ChargePoint object.


### Initialize the database

- Use provided sql database or implement your own storage drive

## Quickstart for OCPP 1.6

Libocpp provides a small standalone OCPP1.6 client that you can control using command line.

Install the dependencies and libocpp as described in [Build and Install libocpp](#build-and-install-libocpp).

Make sure you modify the following config entries in the [config.json](/config/v16/config.json) file according to the CSMS you want to connect to before executing make install.

```json
{
  "Internal": {
    "ChargePointId": "",
    "CentralSystemURI": ""
  }
}
```

Change into libocpp/build and execute cmake and then make install:

```bash
  cd build
  cmake -DLIBOCPP16_BUILD_EXAMPLES=ON -DCMAKE_INSTALL_PREFIX=./dist ..
  make -j$(nproc) install
```

Use the following command to start the charge point. Replace the config with [config-docker.json](/config/v16/config-docker.json) if you want to test with the [SteVe](https://github.com/steve-community/steve#docker) CSMS running in a docker container.

```bash
  ./dist/bin/charge_point \
    --maindir ./dist \
    --conf config.json
```

Type `help` to see a list of possible commands.

## Building the doxygen documentation

```bash
  cmake -S . -B build
  cmake --build build --target doxygen-ocpp
```

You will find the generated doxygen documentation at:
`build/dist/docs/html/index.html`

The main reference for the integration of libocpp for OCPP1.6 is the ocpp::v16::ChargePoint class defined in libocpp/include/ocpp/v16/charge_point.hpp.

## Build and Install libocpp

For Debian GNU/Linux 11 you will need the following dependencies:

```bash
  sudo apt install build-essential cmake python3-pip libboost-all-dev libsqlite3-dev libssl-dev
```

OpenSSL version 3.0 or above is required.

Clone this repository.

```bash
  git clone https://github.com/EVerest/libocpp
```

In the libocpp folder create a folder named build and cd into it.
Execute cmake and then make install:

```bash
  mkdir build && cd build
  cmake ..
  make install
```

## Unit Testing

GTest is required for building the test cases target.
To build the target and run the tests you can reference the script `.ci/build-kit/install_and_test.sh`.
The script allows the GitHub Actions runner to execute.

Local testing:

```bash
mkdir build
cmake -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="./dist"
cd build
make -j$(nproc) install
```

Run any required tests from build/tests.

## Building with FetchContent instead of EDM

In [doc/build-with-fetchcontent](/doc/build-with-fetchcontent) you can find an example how to build libocpp with FetchContent instead of EDM.

## Support for TPM keys

In order to use the TPM keys, it is mandatory to use the default libwebsocket implementation.

## Support for websocket++

The old websocket++ implementation has been deprecated. For enabling websocket++ support use the following cmake option:

```bash
  cmake .. -DLIBOCPP_ENABLE_DEPRECATED_WEBSOCKETPP=ON
```

## Support for iface

In order to connect through a custom network iface, a custom internal config variable 'IFace' can be used.

```json
"Internal": {        
    "IFace" : "enp43s0"
}
```

## Get Involved

See the [COMMUNITY.md](https://github.com/EVerest/EVerest/blob/main/COMMUNITY.md) and [CONTRIBUTING.md](https://github.com/EVerest/EVerest/blob/main/CONTRIBUTING.md) of the EVerest project to get involved.
