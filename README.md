# C++ implementation of OCPP

![Github Actions](https://github.com/EVerest/libocpp/actions/workflows/build_and_test.yaml/badge.svg)

This is a C++ library implementation of OCPP for version 1.6, 2.0.1 and 2.1.
(see [OCPP protocols at OCA website](https://openchargealliance.org/protocols/open-charge-point-protocol/)).
The OCPP2.0.1 implementation of libocpp has been certified by the OCA for multiple hardware platforms.

Libocpp's approach to implementing the  OCPP protocol is to address as much functional requirements as possible as part of the library.
Since OCPP is a protocol that affects, controls, and monitors many areas of a charging station's operation this library needs to be
integrated with your charging station firmware.

## Integration with EVerest

This library is integrated within the [OCPP](https://github.com/EVerest/everest-core/tree/main/modules/OCPP) and [OCPP201](https://github.com/EVerest/everest-core/tree/main/modules/OCPP201)
module within [everest-core](https://github.com/EVerest/everest-core) - the complete software stack for your charging station. It is recommended to use EVerest together with this OCPP implementation.

## Getting Started

Check out the [Getting Started guide](doc/common/getting_started.md). It should be you starting point if you want to integrate this library with your charging station firmware.

## Get Involved

See the [COMMUNITY.md](https://github.com/EVerest/EVerest/blob/main/COMMUNITY.md) and [CONTRIBUTING.md](https://github.com/EVerest/EVerest/blob/main/CONTRIBUTING.md) of the EVerest project to get involved.

## OCPP1.6

### Supported Feature Profiles

OCPP1.6 is fully implemented.

| Feature Profile            | Supported                 |
| -------------------------- | ------------------------- |
| Core                       | :heavy_check_mark: yes    |
| Firmware Management        | :heavy_check_mark: yes    |
| Local Auth List Management | :heavy_check_mark: yes    |
| Reservation                | :heavy_check_mark: yes    |
| Smart Charging             | :heavy_check_mark: yes    |
| Remote Trigger             | :heavy_check_mark: yes    |

| Whitepapers & Application Notes                                                                                                                              | Supported              |
| ----------------------------------------------------------------------------------------------------------------------------------------- | ---------------------- |
| [OCPP 1.6 Security Whitepaper (3rd edition)](https://www.openchargealliance.org/uploads/files/OCPP-1.6-security-whitepaper-edition-3.zip) | :heavy_check_mark: yes |
| [Using ISO 15118 Plug & Charge with OCPP 1.6](https://www.openchargealliance.org/uploads/files/ocpp_1_6_ISO_15118_v10.pdf)                | :heavy_check_mark: yes                    |
| [OCPP & California Pricing Requirements](https://www.openchargealliance.org/uploads/files/ocpp_and_dms_evse_regulation-v2.0.pdf)          | :heavy_check_mark: yes |

### CSMS Compatibility

The EVerest implementation of OCPP 1.6 has been tested against the
OCPP Compliance Test Tool (OCTT and OCTT2) during the implementation.

The following table shows the known CSMS with which this library was tested.

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

## OCPP2.0.1

### Supported Functional Blocks

| Feature Profile                      | Supported                 |
| -------------------------------------| ------------------------- |
| A. Security                          | :heavy_check_mark: yes  |
| B. Provisioning                      | :heavy_check_mark: yes  |
| C. Authorization                     | :heavy_check_mark: yes  |
| D. LocalAuthorizationList Management | :heavy_check_mark: yes  |
| E. Transactions                      | :heavy_check_mark: yes  |
| F. RemoteControl                     | :heavy_check_mark: yes  |
| G. Availability                      | :heavy_check_mark: yes  |
| H. Reservation                       | WIP                       |
| I. TariffAndCost                     | :heavy_check_mark: yes  |
| J. MeterValues                       | :heavy_check_mark: yes  |
| K. SmartCharging                     | WIP                       |
| L. FirmwareManagement                | :heavy_check_mark: yes  |
| M. ISO 15118 CertificateManagement   | :heavy_check_mark: yes  |
| N. Diagnostics                       | :heavy_check_mark: yes  |
| O. DisplayMessage                    | :heavy_check_mark: yes  |
| P. DataTransfer                      | :heavy_check_mark: yes  |

The development of OCPP2.0.1 is in progress. Check the [detailed current implementation status.](/doc/ocpp_201_status.md).

| Whitepapers & Application Notes                                                                                                                              | Supported              |
| ----------------------------------------------------------------------------------------------------------------------------------------- | ---------------------- |
| [OCPP & California Pricing Requirements](https://www.openchargealliance.org/uploads/files/ocpp_and_dms_evse_regulation-v2.0.pdf)          | :heavy_check_mark: yes                  |

### CSMS Compatibility OCPP 2.0.1

The current, ongoing implementation of OCPP 2.0.1 has been tested against a
few CSMS and is continuously tested against OCTT2.

Additionally, the implementation has been tested against these CSMS:

- [CitrineOS](https://lfenergy.org/projects/citrineos/)
- Chargepoint
- Current
- ihomer (Infuse CPMS)
- Instituto Tecnológico de la Energía (ITE)
- [MaEVe (Thoughtworks)](https://github.com/thoughtworks/maeve-csms)
- [Monta](https://monta.com)
- [Open Charging Cloud (GraphDefined)](https://github.com/OpenChargingCloud/WWCP_OCPP)
- Switch EV
- SWTCH

## OCPP2.1

The implementation of OCPP2.1 is currently under development.
