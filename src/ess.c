#include <zephyr.h>
#include <stddef.h>

#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <drivers/sensor.h>

#include <logging/log.h>

#include "remote_temp.h"

static u8_t enable_temp_notifications;

LOG_MODULE_REGISTER(ess);

static void ess_temp_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value);
static ssize_t read_remote_temp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
        void *buf, u16_t len, u16_t offset);

/* ESS Declaration */
BT_GATT_SERVICE_DEFINE(ess_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),
	BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
				   BT_GATT_PERM_READ, NULL, NULL, NULL),
	BT_GATT_CCC(ess_temp_ccc_cfg_changed, BT_GATT_PERM_WRITE),
);

static void ess_temp_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	enable_temp_notifications = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_remote_temp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
        void *buf, u16_t len, u16_t offset)
{
	/*
	s16_t *temp_buf;
	s16_t temperature = -25000;

	u16_t ret = remote_read_handle(&temp_buf);
	if (ret == 0) {
		return bt_gatt_attr_read(conn, attr, buf, len, offset, &temperature,
		            sizeof(temperature));
	}

	temperature = 666;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, &temperature,
            sizeof(temperature));
            */
	return 0;
}

int ess_temp_notify(s16_t temp)
{
	LOG_INF("Notify with %d", temp);
	if (!enable_temp_notifications) {
		return 0;
	}

	return bt_gatt_notify(NULL, &ess_svc.attrs[1],
			&temp, sizeof(temp));
}
