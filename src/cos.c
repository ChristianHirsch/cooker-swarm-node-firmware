#include <zephyr.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "uuid.h"

#include "cos.h"

u8_t input_io_notify_enabled = 0;
u8_t input_power_notify_enabled = 0;

static void cooker_input_io_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	input_io_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_cooker_input_io_level(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	u8_t value;
	int err = bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(u8_t));

	return err;
}

static void cooker_input_power_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	input_power_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_cooker_input_power_level(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	u8_t value;
	int err = bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(u8_t));

	return err;
}

static ssize_t write_cooker_output_io_level(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, u16_t len, u16_t offset,
                        u8_t flags)
{
        u8_t value;

        if (offset + len > sizeof(value)) {
                return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
        }

        memcpy((&value) + offset, buf, len);

        return len;
}

static ssize_t write_cooker_output_power_level(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, u16_t len, u16_t offset,
                        u8_t flags)
{
        u8_t value;

        if (offset + len > sizeof(value)) {
                return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
        }

        memcpy((&value) + offset, buf, len);

		set_output_power_level(value);

        return len;
}

static ssize_t write_cooker_output_auto_level(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, u16_t len, u16_t offset,
                        u8_t flags)
{
        u8_t value;

        if (offset + len > sizeof(value)) {
                return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
        }

        memcpy((&value) + offset, buf, len);

        return len;
}

/* Battery Service Declaration */
BT_GATT_SERVICE_DEFINE(cooker_service_cvs,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_COS),

	BT_GATT_CHARACTERISTIC(BT_UUID_COS_INPUT_IO,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_cooker_input_io_level, NULL, NULL),
	BT_GATT_CCC(cooker_input_io_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	
	BT_GATT_CHARACTERISTIC(BT_UUID_COS_INPUT_POWER,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_cooker_input_power_level, NULL, NULL),
	BT_GATT_CCC(cooker_input_power_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	
	BT_GATT_CHARACTERISTIC(BT_UUID_COS_OUTPUT_IO,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE, NULL, write_cooker_output_io_level, NULL),
	BT_GATT_CCC(NULL, NULL),
	
	BT_GATT_CHARACTERISTIC(BT_UUID_COS_OUTPUT_POWER,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE, NULL, write_cooker_output_power_level, NULL),
	BT_GATT_CCC(NULL, NULL),
/* 	
	BT_GATT_CHARACTERISTIC(BT_UUID_COS_OUTPUT_AUTO,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE, NULL, write_cooker_output_auto_level, NULL),
	BT_GATT_CCC(NULL, NULL),
*/
);

void cooker_service_init(void)
{
}

int cooker_output_io_notify(u8_t value)
{
	if (!input_io_notify_enabled) {
		return 0;
	}

	return bt_gatt_notify(NULL, &cooker_service_cvs.attrs[1], &value, sizeof(u8_t));
}

int cooker_output_power_notify(u8_t value)
{
	if (!input_power_notify_enabled) {
		return 0;
	}

	return bt_gatt_notify(NULL, &cooker_service_cvs.attrs[3], &value, sizeof(u8_t));
}
