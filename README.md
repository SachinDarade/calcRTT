# Sachin's RTT Example

It introduces a quick way to build an ESP-WIFI-MESH network without a router and to calculate RTT and avg RTT. For another detailed network configuration method, please refer to [examples/function_demo/mwifi](../function_demo/mwifi/README.md). Before running the example, please read the documents [README](../../README_en.md) and [ESP-WIFI-MESH](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html).

## Configure

To run this example, you need at least two development boards, one configured as a root node, and the other a non-root node. In this example, all the devices are non-root nodes by default.

- Root node: There is only one root node in an ESP-WIFI-MESH network. `MESH` networks can be differentiated by their `MESH_ID` and channels.
- Non-root node: Include leaf nodes and intermediate nodes, which automatically select their parent nodes according to the network conditions.

You need to go to the submenu `Example Configuration` and configure one device as a root node, and the others as non-root nodes with `make menuconfig`(Make) or `idf.py menuconfig`(CMake).

You can also go to the submenu `Component config -> MDF Mwifi`, and configure the ESP-WIFI-MESH related parameters like max number of layers, the number of the connected devices on each layer, the broadcast interval, etc.


## Output
We print if the transmision/reception of messages was succesfull or not. In addition we output the RTT for the packet and the avg RTT till that time.

