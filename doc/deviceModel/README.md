# Access and control of variables and configuration keys

Libocpp provides an API to access and control variables and configuration keys. The term **configuration keys** is used in connection with the OCPP1.6 specification and the term **variables** is used in connection with the OCPP2.0.1 specification.

The interfaces for these operations differ between OCPP1.6 and OCPP2.0.1 in this library. This section provides an overview of the mechanisms used for the different protocol versions.

Note that almost all standardized OCPP1.6 configuration keys configure the business logic within the libocpp library itself. This is very different for OCPP2.0.1, where the device model includes all kinds of different telemetry and configuration data of physical and logical components of a charging station. 

## OCPP1.6

The OCPP1.6 specification distributes standardized configuration keys into the functional blocks that it defines. This design is also applied by libocpp. Libocpp uses a JSON config to load the configuration keys into memory at startup.

### Getting variables

### Setting variables

The following sequence diagram shows how libocpp handles an incoming ChangeConfiguration.req message by the CSMS:

![Sequence Diagram for ChangeConfiguration.req](csms_change_configuration_key.puml)

If the CSMS uses a ChangeConfiguration.req to change a configuration key, libocpp validates the request and if the request is valid it writes the updated configuration key into a user_config.json. This user config serves as a persistent overlay of the original config and is patched on top of the original config at startup.

If a ChangeConfiguration.req could be applied successfully, libocpp uses two callback mechanism to notify the consumer application about the change. The consumer can register a callback for a single configuration key. This is executed if this configuration is changed. The other or additional option is to register a generic callback that is executed if no specific callback is registered for the changed configuration key.

The following sequence diagram shows how libocpp a request of the consumer to change a custom configuration key.

![Sequence Diagram for setting a custom configuration key](set_custom_configuration_key.puml)

The libocpp API for OCPP1.6 only allows to set custom configuration keys. It does not allow to set any of the standardized configuration keys, since all of them are internally managed. Except for this the sequence is similar to the mechanism described previously.

## OCPP2.0.1

Libocpp applies the design and data structures of the device model for Component, Variables, VariableCharacteristics, VariableAttributes and VariableMonitors that is described in part 1 Architecture & Topology of the OCPP2.0.1 specification.

The following simplified UML diagram provides an overview of the relationship between the relevant classes:

### Device Model Integration

![UML Diagram](device_model_uml.png)

The ChargePoint class contains a reference to the DeviceModel. This implementation is within libocpp. This allows to access the device model structure and to do sanity checks and validations before requesting operations at the actual DeviceModelStorage. Libocpp currently provides an implementation of the DeviceModelStorage (DeviceModelStorageSqlite) but it can also be provided externally, to allow the usage of different storage achitectures and designs.

### Internally and Externally Managed Variables

As mentioned in the introduction the device model can contain various physical or logical components and variables. While in OCPP1.6 almost all of the standardized configuration keys are used to influence the control flow of libocpp, in OCPP2.0.1 the configuration and telemtry variables that can be part of the device model go beyond the control or reporting capabilities of only libocpp. Still there is a large share of standardized variables in OCPP2.0.1 that do influence the control flow of libocpp. Therefore it is requirement to make a distinction between externally and internally managed variables of the device model inside libocpp:

* Internally managed variables: Internally managed variables influence the control flow or can be reported by libocpp and are therefore owned by libocpp. If the mutability of such a variable is ReadWrite, the CSMS or the consumer of libocpp can set its value and libocpp is responsible for updating this value in the device model. 
  Examples: HeartbeatInterval, AuthorizeRemoteStart, SampledDataTxEndedMeasurands, AuthCacheStorage
* Externally managed variables: These variables do not influence the control flow of libocpp and libocpp has no information to access or update the values except for using the device model reference. Even if the mutability of this variable is ReadWrite, the consumer of libocpp can not update the value of these variables, since they are not owned it. The CSMS can use a SetVariables.req to update the value though, in which case libocpp will use the device model reference to request an update.

### Getting variables

Shall libocpp provide an API to access externally managed variables?

### Setting variables

The following sections describe how libocpp manages requests to set variable values in the device model.

![UML Diagram](csms_set_variables_sequence.puml)

A SetVariables.req initiated from the CSMS can contain requests for multiple variables at the same time. For each of the requests libocpp validates if the requests can be validated logically (e.g. NetworkConfigurationPriority is invalid) and using the VariableCharacteristics (e.g. value invalid for datatype, value exceeds defined maxLimit, etc.). In case the validation is successful, libocpp uses the device model reference to request to set the value of the variable. The response for each individual request is sent back in the SetVariables.conf.

After the SetVariables.conf message was sent to the CSMS, each successful request to change a value of a variable is processed internally (e.g. to update the HeartbeatInterval timer) and the variable_changed_callback is executed to notify the libocpp consumer application about the change. 

![UML Diagram](core_set_variables_sequence.puml)

If the libocpp consumer application attempts to set a variable using the set_variables function, the flow is almost the same as for if the CSMS is using a SetVariables.req . The only difference is that the libocpp API to set variables only provides access to set internally managed variables. Libocpp does not serve as a gateway to control externally managed variables in the device model.

# Conclusions
* Device Model Storage implementation should be able to differentiate between variables managed by libocpp and externally managed variables (e.g. config paramaters of other EVerest modules)
* The `set_value` function call needs to end up in the module that owns the variable!
* Currently also externally managed variables can be changed in the device model and the variables_changed_callback is used to actually apply them and feed them back into EVerest. This is an issue since the module that owns the variable does not have influence on the setting of the variable in the storage and it cannot e.g. respond with something like `RebootRequired`

# Questions
* Do we need to seperate between DeviceModelStorage implementations for internally managed variables and externally managed variables?
* Where to put this layer to differentiate (as part of the DeviceModelStorage implementation)?
* What happens with variables like AvailabilityState (which are strictly not owned internally) but libocpp has all information available to set these variables
* How does the path to apply a possible new architecture look like?
* Shall we pull out the device model implementation out of libocpp and put it to e.g. everest-core (OCPP201) module? --> Would strongly increase the value of using libocpp together with everest-core  
