| Supported Targets | ESP32 |
| ----------------- | ----- |

ESP BLE Mesh and MQTT Gateway
========================

This example is used to test BLE Mesh realizes normal network distribution and normal functions of sending and receiving messages. Users can implement their own application scenarios based on the throughput value in this example. If the user is not familiar with the use of ESP32 BLE Mesh, please refer to [BLE Mesh Node Example](https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/esp_ble_mesh/README.md).

This demo is mainly divided into two parts:
Requirement: [ESP-IDF V4.3+](https://docs.espressif.com/projects/esp-idf/en/v4.3/esp32/index.html)

- MQTT Gateway: 
	+ AP mode: webpage to config WiFi connection with CaptiveDNS supporting and OTA update firmware
	+ STA mode: MQTT connection
- BLE Mesh fast provision Server

BLE Mesh can use fast provsion for network configuration. This demo only implements the fast provsion server function. The BLE Mesh module of this demo implements an Element and the following Models:

- Config Server Model: This Model is used when the other party needs to configure APP Key, Dev Key, and other information
- Config Client Model: This model is used when you need to configure APP Key, Dev Key, and other information
- Generic OnOff Server Model: This Model exposes its OnOff State to realize the switch function of LED lights
- Generic OnOff Client Model: Use this model to realize the switch function and control the switch of the LED lights of other nodes
- Fast Provision Server Model: This Model is a custom model implemented for rapid network provisioning. When the node is used as a temporary provisioner for network provisioning through this Model, the generated Element address needs to be passed to the provisioner through this Model.
- Fast Provision Client Model: This Model and Fast Provision Server Model are used in conjunction
