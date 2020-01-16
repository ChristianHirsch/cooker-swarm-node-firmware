This firmware let's you control a 3500W Hendi induction hob remotly with a smartphone or other Bluetooth Low Energy (BLE) powered devices using this [hardware](https://github.com/ChristianHirsch/cooker-swarm-node-hardware). The firmware is based on the [Zephyr Project](https://www.zephyrproject.org/).

# Build
```bash
west build -b cooker_swarm_node_hardware -- -DBOARD_ROOT=./
```
