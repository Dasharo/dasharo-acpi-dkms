# dasharo-acpi

dasharo-acpi is a Linux kernel driver for hardware monitoring on supported
platforms with Dasharo firmware.

Supported platforms:

| Platform | Starting with FW version |
| --- | --- |
| NovaCustom 11th Gen | v1.6.0 |
| NovaCustom 12th Gen | v1.8.0 |
| NovaCustom 14th Gen | v1.0.0 |

## Requirements

- dkms
- kernel headers

## Installation

### Install dependencies

Install dependencies:

Debian / Ubuntu:

```
sudo apt install dkms
```

Fedora:

```
sudo dnf install dkms kernel-headers
```

### Download

Download the package for your distro from
https://github.com/Dasharo/dasharo-acpi-dkms/releases/latest

For Debian or Ubuntu, download the .deb package. For Fedora, download the .rpm
package.

### Install

From the location you downloaded the package to in the previous step, install
the package:

Ubuntu:

```
sudo apt install ./dasharo-acpi-dkms_*.deb
```

Fedora:

```
sudo dnf install ./dasharo-acpi-dkms-*.rpm
```

To load the driver, simply reboot, or run the command:

```
sudo modprobe dasharo-acpi
```

## Usage

lm-sensors is one of many utilities that can make use of the driver. Install
lm-sensors:

Ubuntu:

```
sudo apt install lm-sensors
```

Fedora:

```
sudo dnf install lm_sensors
```

Run `sensors` in a terminal and observe the output. Under the dasharo-acpi
sensor, you should now see your CPU and GPU (if present) temperatures, along
with speeds of their fans:

```
dasharo_acpi-acpi-0
Adapter: ACPI interface
CPU 0:         1005 RPM
GPU 0:            0 RPM
CPU Package 0:  +56.0°C
GPU 0:          +58.0°C
```
