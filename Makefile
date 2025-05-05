install:
	mkdir -p /usr/src/dasharo-acpi-v0.9.0 && cp -r src/* /usr/src/dasharo-acpi-v0.9.0 && dkms build -m dasharo-acpi -v v0.9.0 --force && dkms install -m dasharo-acpi -v v0.9.0 --force

