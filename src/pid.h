#ifndef PID_H_
#define PID_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

struct pid_state {
	s32_t p, i, d;
	s32_t int_error, int_error_max, int_error_min;
	s32_t pre_error;

	s32_t setpoint;
	u32_t timestamp;
	s32_t output_max, output_min;
};

void pid_initialize_state(struct pid_state *state);
s32_t pid_calc_control_effort(struct pid_state *state, s32_t value);

#ifdef __cplusplus
}
#endif

#endif /* PID_H_ */
