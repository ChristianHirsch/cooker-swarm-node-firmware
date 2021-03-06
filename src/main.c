/* main.c - Application main entry point */

/*
 *
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <misc/byteorder.h>
#include <logging/log.h>
#include <settings/settings.h>
#include <device.h>
#include <gpio.h>
#include <soc.h>

#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "cooker.h"
#include "cos.h"
#include "cts.h"

#include "config.h"

#include <drivers/pwm.h>


#ifndef IBEACON_RSSI
#define IBEACON_RSSI 0xc8
#endif

#define SENSE_INTERVAL 5
#define MEASUREMENTS_SIZE  10

/* in microseconds */
#define MIN_PERIOD	(USEC_PER_SEC / 64U)

/* in microseconds */
#define MAX_PERIOD	USEC_PER_SEC

#define BT_LE_ADV_CONN_NAME_ID BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | \
                         BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_USE_IDENTITY, \
                         BT_GAP_ADV_SLOW_INT_MIN, \
                         BT_GAP_ADV_SLOW_INT_MAX)

// BT_GAP_ADV_SLOW_INT_MIN = 0x0640 ^= 1s
// BT_GAP_ADV_SLOW_INT_MAX = 0x0780 ^= 1.2s
#define BT_LE_ADV_NCONN_NAME_ID BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_NAME | \
						 BT_LE_ADV_OPT_USE_IDENTITY, \
                         BT_GAP_ADV_SLOW_INT_MIN, \
                         BT_GAP_ADV_SLOW_INT_MAX)

LOG_MODULE_REGISTER(main);

void create_device_list(void);

struct {
	u32_t timestamp;
	u16_t battery;
} measurements[MEASUREMENTS_SIZE];

s16_t buf_start = 0;
s16_t buf_end   = 0;
u32_t sense_interval_mult = 1;

static bool is_client_connected  = false;
bool is_sync_enabled = false;

extern u8_t cts_notify_enabled;

//struct bt_le_adv_param *adv_params_conn = BT_LE_ADV_CONN_NAME_ID;
//struct bt_le_adv_param *adv_params_nconn = BT_LE_ADV_NCONN_NAME_ID;
struct bt_le_adv_param *adv_params = BT_LE_ADV_CONN_NAME_ID;

enum {
	NO_DATA_AVAILABLE = 0,
	DATA_AVAILABLE = BIT(0)
};

/*
 * Set iBeacon demo advertisement data. These values are for
 * demonstration only and must be changed for production environments!
 *
 * UUID (v1):  77e11000-37af-11ea-a7a1-507b9de20712
 * UUID (v4):  b5e9d1f2-cdb3-4758-a1b0-1d6ddd22dd0d
 * Major: 102
 * Minor[1]: Status Flags
 * Minor[0]: SW Version
 * RSSI:  -56 dBm
 */
static const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
        BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA,
                      0x4c, 0x00, /* Apple */
                      0x02, 0x15, /* iBeacon */
                      0x77, 0xe1, 0x10, 0x00, /* UUID[15..12] */
                      0x37, 0xaf, /* UUID[11..10] */
                      0x11, 0xea, /* UUID[9..8] */
                      0xa7, 0xa1, /* UUID[7..6] */
                      0x50, 0x7b, 0x9d, 0xe2, 0x07, 0x12, /* UUID[5..0] */
                      0x00, 0x01, /* Major */
                      0x00, 0x01, /* Minor */
                      IBEACON_RSSI) /* Calibrated RSSI @ 1m */
};

void set_beacon_flags(u8_t flags) {
	*((u8_t *)(ad[1].data + 22)) = flags;
}

u8_t get_beacon_flags(void) {
	return *(ad[1].data + 22);
}

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		LOG_INF("Connection failed (err %u)\n", err);
		return;
	}

	is_client_connected = true;
	bt_le_adv_stop();
	LOG_INF("Connected\n");
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	LOG_INF("Disconnected (reason %u)\n", reason);
	is_client_connected = false;
	cts_notify_enabled = 0;

	//adv_params = adv_params_nconn;
	if (buf_start == buf_end) {
		set_beacon_flags(NO_DATA_AVAILABLE);
	}
	int err = bt_le_adv_start(adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}
    LOG_INF("Advertising successfully started");
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(int err)
{
	if (err) {
		return;
	}

	cts_init();

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	set_beacon_flags(NO_DATA_AVAILABLE);
	err = bt_le_adv_start(adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}
}

void main(void)
{
	int err = 0;

    u32_t entry = k_uptime_get_32();
    u32_t exit = entry;

	err = bt_enable(bt_ready);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		//return;
	}
    bt_conn_cb_register(&conn_callbacks);

    struct device *adc_dev = device_get_binding(DT_ALIAS_ADC_0_LABEL);
	if (!adc_dev) {
		LOG_ERR("Cannot find %s!", DT_ALIAS_ADC_0_LABEL);
		return;
	}

	struct device *pwm_dev = device_get_binding(PWM_DRIVER);
	if (!pwm_dev) {
		LOG_ERR("Cannot find %s!", PWM_DRIVER);
		return;
	}

	cooker_init();

	while (1) {

		u32_t time_to_sleep = 10;	// 10 ms = 100 Hz
		if (entry < exit) {
			time_to_sleep -= (exit - entry);
		}
		k_sleep(time_to_sleep);

		entry = k_uptime_get_32();
        exit = k_uptime_get_32();
	}
}
