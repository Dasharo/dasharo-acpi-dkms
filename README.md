# dasharo-acpi

## Requirements

- dkms
- kernel headers

## Download

Download the package for your distro from:
https://github.com/Dasharo/dasharo-acpi-dkms/actions

## Installation

Install dependencies:

Ubuntu:

```
sudo apt install dkms
```

Fedora:

```
sudo dnf install dkms kernel-headers
```

Install the driver:

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

Install, for example: lm-sensors

Ubuntu:

```
sudo apt install lm-sensors
```

Fedora:

```
sudo dnf install lm_sensors
```

Run `sensors` in a terminal and observe the output. You should
see:

```
dasharo_acpi-acpi-0
Adapter: ACPI interface
CPU 0:         1005 RPM
GPU 0:            0 RPM
CPU Package 0:  +56.0°C
GPU 0:          +58.0°C
```
