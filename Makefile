install:
	mkdir -p /usr/src/dasharo-acpi-0.9.1 && cp -r src/* /usr/src/dasharo-acpi-0.9.1 && dkms build -m dasharo-acpi -v 0.9.1 --force && dkms install -m dasharo-acpi -v 0.9.1 --force

