// SPDX-License-Identifier: GPL-2.0+
/*
 * Dasharo ACPI Driver
 */

#include <linux/acpi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/types.h>

enum dasharo_feature {
	DASHARO_FEATURE_TEMPERATURE = 0,
	DASHARO_FEATURE_FAN_PWM,
	DASHARO_FEATURE_FAN_TACH,
	DASHARO_FEATURE_MAX,
};

enum dasharo_temperature {
	DASHARO_TEMPERATURE_CPU_PACKAGE = 0,
	DASHARO_TEMPERATURE_CPU_CORE,
	DASHARO_TEMPERATURE_GPU,
	DASHARO_TEMPERATURE_BOARD,
	DASHARO_TEMPERATURE_CHASSIS,
	DASHARO_TEMPERATURE_MAX,
};

enum dasharo_fan {
	DASHARO_FAN_CPU = 0,
	DASHARO_FAN_GPU,
	DASHARO_FAN_CHASSIS,
	DASHARO_FAN_MAX,
};

#define MAX_GROUPS_PER_FEAT 8

static char *dasharo_group_names[DASHARO_FEATURE_MAX][MAX_GROUPS_PER_FEAT] = {
	[DASHARO_FEATURE_TEMPERATURE] = {
		[DASHARO_TEMPERATURE_CPU_PACKAGE] = "CPU Package",
		[DASHARO_TEMPERATURE_CPU_CORE] = "CPU Core",
		[DASHARO_TEMPERATURE_GPU] = "GPU",
		[DASHARO_TEMPERATURE_BOARD] = "Board",
		[DASHARO_TEMPERATURE_CHASSIS] = "Chassis",
	},
	[DASHARO_FEATURE_FAN_PWM] = {
		[DASHARO_FAN_CPU] = "CPU",
		[DASHARO_FAN_GPU] = "GPU",
		[DASHARO_FAN_CHASSIS] = "Chassis",
	},
	[DASHARO_FEATURE_FAN_TACH] = {
		[DASHARO_FAN_CPU] = "CPU",
		[DASHARO_FAN_GPU] = "GPU",
		[DASHARO_FAN_CHASSIS] = "Chassis",
	},
};

#define MAX_CAP_NAME_LEN 16

struct dasharo_capability {
	int cap;
	int index;
	char name[MAX_CAP_NAME_LEN];
};

#define MAX_CAPS_PER_FEAT 24

struct dasharo_data {
	struct acpi_device *acpi_dev;
	int cap_counts[DASHARO_FEATURE_MAX];
	struct dasharo_capability capabilities[DASHARO_FEATURE_MAX][MAX_CAPS_PER_FEAT];
	struct device *hwmon;
};

static int dasharo_get_feature_cap_count(struct dasharo_data *data, int feat, int cap)
{
	struct acpi_object_list obj_list;
	unsigned long long count = 0;
	union acpi_object obj[2];
	acpi_handle handle;
	acpi_status status;

	obj[0].type = ACPI_TYPE_INTEGER;
	obj[0].integer.value = feat;
	obj[1].type = ACPI_TYPE_INTEGER;
	obj[1].integer.value = cap;
	obj_list.count = 2;
	obj_list.pointer = &obj[0];

	handle = acpi_device_handle(data->acpi_dev);
	status = acpi_evaluate_integer(handle, "GFCP", &obj_list, &count);
	if (!ACPI_SUCCESS(status))
		return -ENODEV;

	return count;
}

static int dasharo_read_value_by_cap_idx(struct dasharo_data *data, char *method, int cap, int index, long *value)
{
	struct acpi_object_list obj_list;
	unsigned long long val = 0;
	union acpi_object obj[2];
	acpi_handle handle;
	acpi_status status;

	obj[0].type = ACPI_TYPE_INTEGER;
	obj[0].integer.value = cap;
	obj[1].type = ACPI_TYPE_INTEGER;
	obj[1].integer.value = index;
	obj_list.count = 2;
	obj_list.pointer = &obj[0];

	handle = acpi_device_handle(data->acpi_dev);
	status = acpi_evaluate_integer(handle, method, &obj_list, &val);
	if (!ACPI_SUCCESS(status))
		return -ENODEV;

	*value = val;
	return val;
}

static int dasharo_hwmon_read(struct device *dev, enum hwmon_sensor_types type,
			      u32 attr, int channel, long *val)
{
	struct dasharo_data *data = dev_get_drvdata(dev);
	int ret = 0;
	long value;

	switch (type) {
	case hwmon_temp:
		if (attr == hwmon_temp_input) {
			ret = dasharo_read_value_by_cap_idx(data,
				"GTMP",
				data->capabilities[DASHARO_FEATURE_TEMPERATURE][channel].cap,
				data->capabilities[DASHARO_FEATURE_TEMPERATURE][channel].index,
				&value);

			if (ret > 0)
				*val = value * 1000;
		}
		break;
	case hwmon_fan:
		if (attr == hwmon_fan_input) {
			ret = dasharo_read_value_by_cap_idx(data,
				"GFTH",
				data->capabilities[DASHARO_FEATURE_FAN_TACH][channel].cap,
				data->capabilities[DASHARO_FEATURE_FAN_TACH][channel].index,
				&value);

			if (ret > 0)
				*val = value;
		}
		break;
	case hwmon_pwm:
		if (attr == hwmon_pwm_input) {
			ret = dasharo_read_value_by_cap_idx(data,
				"GFDC",
				data->capabilities[DASHARO_FEATURE_FAN_PWM][channel].cap,
				data->capabilities[DASHARO_FEATURE_FAN_PWM][channel].index,
				&value);

			if (ret > 0)
				*val = value;
		}
		break;
	default:
		break;
	}

	return 0;
}

static int dasharo_hwmon_read_string(struct device *dev, enum hwmon_sensor_types type,
				     u32 attr, int channel, const char **str)
{
	struct dasharo_data *data = dev_get_drvdata(dev);

	switch (type) {
	case hwmon_temp:
		if (attr == hwmon_temp_label && channel < data->cap_counts[DASHARO_FEATURE_TEMPERATURE])
		*str = data->capabilities[DASHARO_FEATURE_TEMPERATURE][channel].name;
		break;
	case hwmon_fan:
		if (attr == hwmon_fan_label && channel < data->cap_counts[DASHARO_FEATURE_FAN_TACH])
		*str = data->capabilities[DASHARO_FEATURE_FAN_TACH][channel].name;
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static umode_t dasharo_hwmon_is_visible(const void *drvdata, enum hwmon_sensor_types type,
					u32 attr, int channel)
{
	const struct dasharo_data *data = drvdata;

	switch (type) {
	case hwmon_temp:
		if (channel < data->cap_counts[DASHARO_FEATURE_TEMPERATURE])
			return 0444;
		break;
	case hwmon_pwm:
		if (channel < data->cap_counts[DASHARO_FEATURE_FAN_PWM])
			return 0444;
		break;
	case hwmon_fan:
		if (channel < data->cap_counts[DASHARO_FEATURE_FAN_TACH])
			return 0444;
		break;
	default:
		break;
	}

	return 0;
}
static const struct hwmon_ops dasharo_hwmon_ops = {
	.is_visible = dasharo_hwmon_is_visible,
	.read_string = dasharo_hwmon_read_string,
	.read = dasharo_hwmon_read,
};

// Max 24 capabilities per feature
static const struct hwmon_channel_info * const dasharo_hwmon_info[] = {
	HWMON_CHANNEL_INFO(fan,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL),
	HWMON_CHANNEL_INFO(temp,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL,
		HWMON_T_INPUT | HWMON_T_LABEL),
	HWMON_CHANNEL_INFO(pwm,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT,
		HWMON_PWM_INPUT),
	NULL
};

static const struct hwmon_chip_info dasharo_hwmon_chip_info = {
	.ops = &dasharo_hwmon_ops,
	.info = dasharo_hwmon_info,
};

static void dasharo_fill_feature_caps(struct dasharo_data *data, int feat)
{
	int cap_count = 0;
	int count = 0;

	for (int group = 0; group < MAX_GROUPS_PER_FEAT; ++group) {
		count = dasharo_get_feature_cap_count(data, feat, group);

		for (unsigned int i = 0; i < count && cap_count < MAX_CAPS_PER_FEAT; ++i) {
			data->capabilities[feat][cap_count].cap = group;
			data->capabilities[feat][cap_count].index = i;
			scnprintf(data->capabilities[feat][cap_count].name, MAX_CAP_NAME_LEN, "%s %d", dasharo_group_names[feat][group], i);
			cap_count++;
		}
	}
	data->cap_counts[feat] = cap_count;
}

static int dasharo_add(struct acpi_device *acpi_dev)
{
	struct dasharo_data *data;

	data = devm_kzalloc(&acpi_dev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	acpi_dev->driver_data = data;
	data->acpi_dev = acpi_dev;

	for (unsigned int i = 0; i < DASHARO_FEATURE_MAX; ++i) {
		dasharo_fill_feature_caps(data, i);
	}

	data->hwmon = devm_hwmon_device_register_with_info(&acpi_dev->dev,
		"dasharo_acpi", data, &dasharo_hwmon_chip_info, NULL);

	return 0;
}

static void dasharo_remove(struct acpi_device *acpi_dev)
{
	struct dasharo_data *data = acpi_driver_data(acpi_dev);

	hwmon_device_unregister(data->hwmon);
}

static const struct acpi_device_id device_ids[] = {
	{"DSHR0001", 0},
	{}
};
MODULE_DEVICE_TABLE(acpi, device_ids);

static struct acpi_driver dasharo_driver = {
	.name = "Dasharo ACPI Driver",
	.class = "Dasharo",
	.ids = device_ids,
	.ops = {
		.add = dasharo_add,
		.remove = dasharo_remove,
	},
};
module_acpi_driver(dasharo_driver);

MODULE_DESCRIPTION("Dasharo ACPI Driver");
MODULE_AUTHOR("Michał Kopeć <michal.kopec@3mdeb.com>");
MODULE_LICENSE("GPL");
