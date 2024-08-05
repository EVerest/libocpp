
Ideas to improve Device Model Integration:
* Differentiate between variables
  * Internally managed/owned (by libocpp)
  * Externally managed/owned (by core)
* Internally managed variables can only be changed internally
* Externally managed variables can only be changed by e.g. core

If CSMS wants to set an internally managed variable, checks can be done internally
If CSMS wants to set an externally managed variable, check needs to be done by core callback

Who sets the value finally in the device model storage?

Device model abstraction layer for diffentiation between externally and internally managed
