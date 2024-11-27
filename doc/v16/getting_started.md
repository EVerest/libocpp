# Getting Started

OCPP is a protocol that affects, controls, and monitors many areas of a charging station's operation. The `libocpp` library is just the messenger for this protocol. It is intended to provide mechanisms for connecting to and authenticating with a CSMS, sending and receiving the OCPP messages that govern behaviors in the standard, and to the track state required for a charging station to conform to the protocol all while minimizing hardware- or implementation-specific functionality.

The actual substance of how a charging station reacts to or initiates an OCPP command (such as `Reset.req` or `RemoteStartTransaction.req`) is left to the rest of the charging station's systems. This is done by providing means of (a) registering **callbacks** that can be triggered by a `libocpp` `ChargePoint` in response to certain events and (b) reacting to various **event handlers** defined on a `ChargePoint` in other areas of the charging station's codebase.

> [!IMPORTANT]
> Integrating this library with your charging station requires both (a) defining **callbacks** that enable control of your station by `libocpp` and (b) calling `libocpp` **event handlers** in your charging station's systems in response to new events and data in order to keep `libocpp` up to date on station information.

## Terminology

Throughout this document and the `libocpp` codebase, the following conventions are followed:

- A **callback** is a function providing the actual station-specific implementation of an OCPP command. It allows a `libocpp` `ChargePoint` to control other systems within a charging station. By convention, each callback on a `ChargePoint` has a name suffixed with `_callback` (for instance, `unlock_connector_callback`). A suitable `std::function` can be **registered** as a callback on a `ChargePoint` by providing it as an argument for the relevant `register_` function (such as `register_unlock_connector_callback`).

- An **event handler** is a public function defined on a `ChargePoint` that allows a charging station to update the state being tracked by the `ChargePoint` based on new information (meter values, charging session events, etc.) and (indirectly) send messages to a CSMS. By convention, the names of event handlers on the `ChargePoint` are each prefixed with `on_` (for instance, `on_meter_values`).

The complete set of callbacks and event handlers defined on an OCPP 1.6 `ChargePoint` can be viewed in the latter's [header file](/include/ocpp/v16/charge_point.hpp).

## ChargePoint() constructor

The main entrypoint for libocpp for OCPP1.6 is the ocpp::v16::ChargePoint constructor.
This is defined in `v16/charge_point.hpp` and takes the following parameters:

- config: a std::string that contains the libocpp 1.6 config. There are example configs that work with a [SteVe](https://github.com/steve-community/steve) installation for example `config/v16/config-docker.json`.

- share_path: a std::filesystem path containing the path to the OCPP modules folder, for example pointing to */usr/share/everest/modules/OCPP*. This path contains the following files and directories and is installed by the libocpp  install target:

  ```bash
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

- evse_security: this is a pointer to an implementation of the `common/evse_security.hpp` interface. This allows you to include your custom implementation of the security related operations according to this interface. If you set this value to nullptr, the internal implementation of the security related operations of libocpp will be used. In this case you need to specify the parameter security_configuration

- security_configuration: this parameter should only be set in case the evse_security parameter is nullptr. It specifies the file paths that are required to set up the internal evse_security implementation. Note that you need to specify bundle files for the CA certificates and directories for the certificates and keys

  The directory layout could look like this:

  ```bash
  .
  ├── ca
  │   ├── csms
  │   │   └── CSMS_ROOT_CA.pem
  │   ├── mf
  │   │   └── MF_ROOT_CA.pem
  │   ├── mo
  │   │   ├── MO_ROOT_CA.pem
  │   └── v2g
  │       └── V2G_ROOT_CA.pem
  ├── client
  │   ├── csms
  │   │   ├── CSMS_LEAF.key
  │   │   └── CSMS_LEAF.pem
  │   ├── cso
  │   │   ├── CPO_CERT_CHAIN.pem
  │   │   ├── SECC_LEAF.key
  │   │   └── SECC_LEAF.pem
  ```
