###Changes to the [original version](https://github.com/nodemcu/nodemcu-firmware)
- GPIO Remapping removed -> GPIO 6,7,8 and 11 are not available
- node.dsleep() takes milli seconds instead of micro seconds
- i2c.scan(bus, [sda], [scl]) scans for i2c slaves on the bus  
