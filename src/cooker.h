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

void cooker_start_control_loop(s32_t duration);
void cooker_stop_control_loop(void);

void cooker_set_control_setpoint(s32_t setpoint);
void cooker_set_control_pid_p_value(s32_t p);
void cooker_set_control_pid_i_value(s32_t i);
void cooker_set_control_pid_d_value(s32_t d);

#ifdef __cplusplus
}
#endif
