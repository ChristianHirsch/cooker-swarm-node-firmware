#include <bluetooth/addr.h>

void  initilize_remote(void);
void  remote_set_addr(bt_addr_le_t *addr);
void  remote_set_handle(u16_t handle);
u16_t remote_read_handle(void **buf);
s16_t remote_read_temp(void);
