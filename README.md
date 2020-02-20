This firmware let's you control a 3500W Hendi induction hob remotly with a smartphone or other Bluetooth Low Energy (BLE) powered devices using this [hardware](https://github.com/ChristianHirsch/cooker-swarm-node-hardware). The firmware is based on the [Zephyr Project](https://www.zephyrproject.org/).

# Build

Build the firmware with Zephyr's meta-tool [West](https://docs.zephyrproject.org/latest/guides/west/index.html).
 
```bash
west build
```

# Usage

The firmware let's you control the induction hob in three ways: manually, remotely and with a control loop (and external temperature sensor).

## Manual control

In manual control mode, the induction hob's inputs are read and set accordingly. It is possible to connect to the device and get device notifiaction about input and output state changes.

## Remote control

In remote control mode, the induction hob is controlled with a remote device, e.g., a smartphone, laptop etc. The output state (switched on or off) is controlled with the characteristic `77e12a03-37af-11ea-a7a1-507b9de20712`, whereas the output power is controlled with the characteristics `77e12a04-37af-11ea-a7a1-507b9de20712`.

```bash
# switch device on
gatttool -t random -b 01:23:45:67:89:ab --char-write-req --handle=0x0018 --value=01

# set power to minimum value 0
gatttool -t random -b 01:23:45:67:89:ab --char-write-req --handle=0x001b --value=00

# set power to maximum value 255
gatttool -t random -b 01:23:45:67:89:ab --char-write-req --handle=0x001b --value=ff

# switch device off
gatttool -t random -b 01:23:45:67:89:ab --char-write-req --handle=0x0018 --value=00
```

## Control loop

In the control loop mode, the firmware controls the output with a PID controller and a remote temperature sensor.
The remote temperature sensors needs to conform to the [Bluetooth SIG temperature characteristics] (https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.temperature.xml).
To enable control loop mode, you need to set the remote temperature sensor by setting its type, address and the handle of the temperature readings to the characteristic `77e12a05-37af-11ea-a7a1-507b9de20712`. The control loop's setpoint is set with characteristic `77e12a06-37af-11ea-a7a1-507b9de20712` and the state with characteristic `77e12a07-37af-11ea-a7a1-507b9de20712`. PID control values can be set with characteristics `77e12a08-37af-11ea-a7a1-507b9de20712`.

```bash
# set remote device address 12:34:56:78:9a:bc (random) and handle 0x0016:
gatttool -t random -b 01:23:45:67:89:ab --char-write-req --handle=0x001e --value=01bc9a785634121600

# set control loop setpoint to 50.00, i.e., 5000 (0x00001388):
gatttool -t random -b 01:23:45:67:89:ab --char-write-req --handle=0x0024 --value=88130000

# enable control loop operation
gatttool -t random -b 01:23:45:67:89:ab --char-write-req --handle=0x0021 --value=01

# disable control loop operation
gatttool -t random -b 01:23:45:67:89:ab --char-write-req --handle=0x0021 --value=00
```

Beware that the values are transmitted in little endian order.
During operation, you can get notifications of temperature readings and output values.