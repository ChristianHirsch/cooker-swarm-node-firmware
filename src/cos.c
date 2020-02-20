#include <zephyr.h>
#include <kernel.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/addr.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "cooker.h"
#include "remote_temp.h"
#include "uuid.h"

#include "cos.h"

u8_t input_io_notify_enabled = 0;
u8_t input_power_notify_enabled = 0;

u8_t output_io_notify_enabled = 0;
u8_t output_power_notify_enabled = 0;

bt_addr_le_t remote_temp_device;
u16_t remote_temp_handle;

extern struct k_mutex cooker_control_mutex;

static void cos_set_input_io_ccc_cfg(const struct bt_gatt_attr *attr,
				 u16_t value);
static ssize_t cos_get_input_io_level(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset);

static void cos_set_input_power_ccc_cfg(const struct bt_gatt_attr *attr,
				 u16_t value);
static ssize_t cos_get_input_power_level(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset);

static void cos_set_output_io_ccc_cfg(const struct bt_gatt_attr *attr,
				 u16_t value);
static ssize_t cos_set_output_io_level(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, u16_t len, u16_t offset,
                        u8_t flags);

static void cos_set_output_power_ccc_cfg(const struct bt_gatt_attr *attr,
				 u16_t value);
static ssize_t cos_set_output_power_level(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, u16_t len, u16_t offset,
                        u8_t flags);

static ssize_t cos_set_remote_temp_probe(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, u16_t len, u16_t offset,
                        u8_t flags);

static ssize_t cos_set_control_state(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, u16_t len, u16_t offset,
                        u8_t flags);

static ssize_t cos_set_control_setpoint(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, u16_t len, u16_t offset,
                        u8_t flags);

static ssize_t cos_set_control_pid(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, u16_t len, u16_t offset,
                        u8_t flags);

/* Battery Service Declaration */
BT_GATT_SERVICE_DEFINE(cooker_service_cvs,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_COS),

	BT_GATT_CHARACTERISTIC(BT_UUID_COS_INPUT_IO,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, cos_get_input_io_level, NULL, NULL),
	BT_GATT_CCC(cos_set_input_io_ccc_cfg, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	
	BT_GATT_CHARACTERISTIC(BT_UUID_COS_INPUT_POWER,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, cos_get_input_power_level, NULL, NULL),
	BT_GATT_CCC(cos_set_input_power_ccc_cfg, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	
	BT_GATT_CHARACTERISTIC(BT_UUID_COS_OUTPUT_IO,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_WRITE, NULL, cos_set_output_io_level, NULL),
	BT_GATT_CCC(cos_set_output_io_ccc_cfg, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	
	BT_GATT_CHARACTERISTIC(BT_UUID_COS_OUTPUT_POWER,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_WRITE, NULL, cos_set_output_power_level, NULL),
	BT_GATT_CCC(cos_set_output_power_ccc_cfg, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	BT_GATT_CHARACTERISTIC(BT_UUID_COS_REMOTE_TEMP_PROBE,
				BT_GATT_CHRC_WRITE,
				BT_GATT_PERM_WRITE, NULL, cos_set_remote_temp_probe, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_NONE),

	BT_GATT_CHARACTERISTIC(BT_UUID_COS_CONTROL_STATE,
				BT_GATT_CHRC_WRITE,
				BT_GATT_PERM_WRITE, NULL, cos_set_control_state, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_NONE),

	BT_GATT_CHARACTERISTIC(BT_UUID_COS_CONTROL_SETPOINT,
			BT_GATT_CHRC_WRITE,
			BT_GATT_PERM_WRITE, NULL, cos_set_control_setpoint, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_NONE),

	BT_GATT_CHARACTERISTIC(BT_UUID_COS_CONTROL_PID,
			BT_GATT_CHRC_WRITE,
			BT_GATT_PERM_WRITE, NULL, cos_set_control_pid, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_NONE),
);

static void cos_set_input_io_ccc_cfg(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	input_io_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t cos_get_input_io_level(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	u8_t value;
	int err = bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(u8_t));

	return err;
}

static void cos_set_input_power_ccc_cfg(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	input_power_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t cos_get_input_power_level(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	u8_t value = get_input_power_level();
	int err = bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(u8_t));

	return err;
}

static void cos_set_output_io_ccc_cfg(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	output_io_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t cos_set_output_io_level(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, u16_t len, u16_t offset,
                        u8_t flags)
{
        u8_t value;

        if (offset + len > sizeof(value)) {
                return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
        }

        memcpy((&value) + offset, buf, len);

        if (k_mutex_lock(&cooker_control_mutex, K_NO_WAIT) == 0)
        {
        	value = (value == 1);
        	set_output_io_level(value);
        	k_mutex_unlock(&cooker_control_mutex);
        }

        return len;
}

static void cos_set_output_power_ccc_cfg(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	output_power_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t cos_set_output_power_level(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, u16_t len, u16_t offset,
                        u8_t flags)
{
        u8_t value;

        if (offset + len > sizeof(value)) {
                return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
        }

        memcpy((&value) + offset, buf, len);

        if (k_mutex_lock(&cooker_control_mutex, K_NO_WAIT) == 0)
        {
        	set_output_power_level(value);

        	k_mutex_unlock(&cooker_control_mutex);
        }

        return len;
}

static ssize_t cos_set_remote_temp_probe(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		const void *buf, u16_t len, u16_t offset,
		u8_t flags)
{
	static u8_t value[9] = { 0 };

	if (offset + len >
		(sizeof(remote_temp_device.type)
				+ sizeof(remote_temp_device.a)
				+ sizeof(remote_temp_handle) )) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy((&value) + offset, buf, len);

	memcpy(&(remote_temp_device.type), &(value[0]), sizeof(remote_temp_device.type));
	memcpy(&(remote_temp_device.a.val), &(value[1]), sizeof(remote_temp_device.a.val));
	memcpy(&remote_temp_handle, &(value[7]), sizeof(remote_temp_handle));

	remote_set_addr(&remote_temp_device);
	remote_set_handle(remote_temp_handle);

	return len;
}

static ssize_t cos_set_control_state(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, u16_t len, u16_t offset,
                        u8_t flags)
{
	u8_t state;
	if ((offset + len) > sizeof(state)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy((&state) + offset, buf, len);

	if (state == 1) {
		cooker_start_control_loop(K_NO_WAIT);
	} else {
		cooker_stop_control_loop();
	}

	return len;
}

static ssize_t cos_set_control_setpoint(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, u16_t len, u16_t offset,
                        u8_t flags)
{
	s32_t setpoint;
	if ((offset + len) > sizeof(setpoint)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy((&setpoint) + offset, buf, len);

	cooker_set_control_setpoint(setpoint);

	return len;
}

static ssize_t cos_set_control_pid(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, u16_t len, u16_t offset,
                        u8_t flags)
{
	s32_t values[10];

	if ((offset + len) > sizeof(values)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy((&values) + offset, buf, len);

	cooker_set_control_pid_p_value(values[0]);
	cooker_set_control_pid_p_value(values[1]);
	cooker_set_control_pid_p_value(values[2]);

	return len;
}

int cos_input_io_notify(u8_t value)
{
	if (!input_io_notify_enabled) {
		return 0;
	}

	return bt_gatt_notify(NULL, &cooker_service_cvs.attrs[1], &value, sizeof(u8_t));
}

int cos_input_power_notify(u8_t value)
{
	if (!input_power_notify_enabled) {
		return 0;
	}

	return bt_gatt_notify(NULL, &cooker_service_cvs.attrs[3], &value, sizeof(u8_t));
}

int cos_output_io_notify(u8_t value)
{
	if (!output_io_notify_enabled) {
		return 0;
	}

	return bt_gatt_notify(NULL, &cooker_service_cvs.attrs[5], &value, sizeof(u8_t));
}

int cos_output_power_notify(u8_t value)
{
	if (!output_power_notify_enabled) {
		return 0;
	}

	return bt_gatt_notify(NULL, &cooker_service_cvs.attrs[7], &value, sizeof(u8_t));
}
