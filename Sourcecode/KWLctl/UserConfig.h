/* https://medium.com/home-wireless/use-visual-studio-code-for-arduino-2d0cf4c1760b */
#define DEBUG

#include "KWLConfig.h"

CONFIGURE(NetworkIPAddress, 192, 168, 178, 200)
CONFIGURE(NetworkMACAddress, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF)
CONFIGURE(NetworkMQTTBroker, 192, 168, 178, 25) // raspberrypi
// CONFIGURE(PrefixMQTT, "ap300")

CONFIGURE(StandardFan1ImpulsesPerRotation, 1.0)
CONFIGURE(StandardFan2ImpulsesPerRotation, 1.0)

CONFIGURE(StandardNenndrehzahlFan, 2850)

CONFIGURE(StandardKwlMode, 1)
CONFIGURE(StandardSpeedSetpointFan1, 1100)
CONFIGURE(StandardSpeedSetpointFan2, 1100)
