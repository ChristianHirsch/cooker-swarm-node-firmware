#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void cooker_init();

u8_t get_input_power_level();

void set_output_power_level(u8_t level);
u8_t get_output_power_level();

u8_t get_input_io_level();

void set_output_io_level(u8_t level);
u8_t get_output_io_level();

void cooker_start_auto_control_loop(void);

#ifdef __cplusplus
}
#endif
