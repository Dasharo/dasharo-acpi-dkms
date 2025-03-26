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
sudo dnf install dkms
```

Install the driver:

Ubuntu:

```
sudo apt install ./dasharo-acpi-dkms_*.deb
```

Fedora:

```
sudo dnf install ./dasharo-acpi-dkms_*.rpm
```

To load the driver, simply reboot, or run the command:

```
sudo modprobe dasharo-acpi
```
