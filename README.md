# RAK3172 - BME680 Environmental monitoring

RAK3172 LoRaWAN module from RAKwireless with a BME680 environmental sensor device connected to it.

[Device](img/endDevice.png)

## BME680

BME680 is connected via I2C and measures:

* Temperature
* Humidity
* Barometric pressure
* VOC gas

The measurements are sent via LoRaWAN uplink on port 1 based on the configured send interval (10 minutes by default).

## LoRaWAN downlink

The firmware decodes the downlink in the following way:

* byte[0] = Green LED ON/OFF
* byte[1] = Red LED ON/OFF
* byte[2] = Send interval in minutes
* byte[4] = An "incrementing" version byte. This one is used in the cloud service to keep track on the status of the device config.

The firmware is configured to *echo* the downlink message it receives as back as an uplink to LoRaWAN port 2.

The reason behind this behavior is that a cloud service is built around this demo device that needs to keep track on which "version" of the settings that are applied on the node.

## More information

More information about the solution built with MongoDB Atlas services for this end-device can be found here: [Integrating The Things Network with MongoDB Atlas]([img/endDevice.png](https://www.joholtech.com/blog/2022/08/20/mongodbatlas-ttn.htm))
