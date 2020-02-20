#include <zephyr.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>

#include <logging/log.h>

#include "ess.h"

static bt_addr_le_t *remote_addr = NULL;
static struct bt_conn *remote_conn = NULL;
static struct bt_gatt_read_params read_params;

static void connected(struct bt_conn *conn, u8_t err);
static void disconnected(struct bt_conn *conn, u8_t reason);
static u8_t remote_read_cb(struct bt_conn *conn, u8_t err,
		struct bt_gatt_read_params *params,
		const void *data, u16_t length);
static void disconnect_timeout(struct k_timer *timer);

static struct {
	u8_t  buf[10];
	u16_t length;
} remote_data;

K_THREAD_STACK_DEFINE(remote_read_stack_area, 1024);
struct k_thread remote_read_data;
k_tid_t remote_read_tid;

static void remote_read_worker(void *, void *, void *);

K_SEM_DEFINE(read_remote_handle_mutex, 0, 1);
K_SEM_DEFINE(connect_remote_device, 0, 1);
K_TIMER_DEFINE(disconnect_timer, disconnect_timeout, NULL);

LOG_MODULE_REGISTER(remote_temp);

static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
};

void initilize_remote(void)
{
	bt_conn_cb_register(&conn_callbacks);

	read_params.func = remote_read_cb;
}

void remote_set_addr(bt_addr_le_t *addr)
{
	remote_addr = addr;
}

void remote_set_handle(u16_t handle)
{
	read_params.handle_count = 1;
	read_params.single.handle = handle;
	read_params.single.offset = 0;
}

static int remote_connect(void)
{
	if (remote_addr == NULL)
		return -1;

	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(remote_addr, addr, sizeof(addr));

	LOG_INF("Connect to remote device %s", addr);
	remote_conn = bt_conn_create_le(remote_addr, BT_LE_CONN_PARAM_DEFAULT);

	return 0;
}

static void remote_disconnect(void)
{
	bt_conn_disconnect(remote_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		LOG_INF("Connection failed (err %u)", err);
		return;
	}

	if (remote_conn != conn) {
		return;
	}
	LOG_INF("Conncted to remote device");

	k_sem_give(&connect_remote_device);
	k_timer_start(&disconnect_timer, K_MINUTES(5), 0);
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	if (remote_conn != conn) {
		return;
	}

	remote_conn = NULL;
	k_timer_stop(&disconnect_timer);
}

static u8_t remote_read_cb(struct bt_conn *conn, u8_t err,
		struct bt_gatt_read_params *params,
		const void *data, u16_t length)
{
	LOG_INF("Received remote data");

	remote_data.length = length;
	memcpy(remote_data.buf, data, length);
	if (k_sem_take(&read_remote_handle_mutex, K_NO_WAIT) != 0) {
		k_sem_give(&read_remote_handle_mutex);
	}

	ess_temp_notify(*(s16_t *)remote_data.buf);

	return 0;
}

u16_t remote_read_handle(void **buf)
{
	remote_read_tid = k_thread_create(&remote_read_data, remote_read_stack_area,
			K_THREAD_STACK_SIZEOF(remote_read_stack_area),
			remote_read_worker,
			NULL, NULL, NULL,
			5, 0, K_NO_WAIT);
	return 0;
}

static void disconnect_timeout(struct k_timer *timer)
{
	remote_disconnect();
}

static void remote_read_worker(void *b1, void *b2, void *b3)
{
	if (remote_conn == NULL) {
		if (remote_connect() != 0) {
			LOG_ERR("Error connecting to remote device");
			return;
		}
		LOG_INF("Waiting for connection");
		if(k_sem_take(&connect_remote_device, K_SECONDS(30)) != 0) {
			LOG_ERR("Connect to remote device timed out");
			return;
		}
		k_sem_give(&connect_remote_device);
	}
	k_timer_start(&disconnect_timer, K_MINUTES(5), 0);

	LOG_INF("Reading remote handle");

	bt_gatt_read(remote_conn, &read_params);
}

s16_t remote_read_temp(void)
{
	remote_read_tid = k_thread_create(&remote_read_data, remote_read_stack_area,
			K_THREAD_STACK_SIZEOF(remote_read_stack_area),
			remote_read_worker,
			NULL, NULL, NULL,
			5, 0, K_NO_WAIT);

	if (k_sem_take(&read_remote_handle_mutex, K_SECONDS(10)) != 0) {
		LOG_ERR("Read remote handle timed out");
		return -32767;
	}

	return *(s16_t *)remote_data.buf;
}
