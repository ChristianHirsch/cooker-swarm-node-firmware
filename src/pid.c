#include "pid.h"

#include <kernel.h>

void pid_initialize_state(struct pid_state *state)
{
	state->setpoint = 0;
	state->p = 0;
	state->i = 0;
	state->d = 0;
	state->int_error = 0;
	state->int_error_max = 0;
	state->int_error_min = 0;
	state->pre_error = 0;
	state->timestamp = k_uptime_get_32();
	state->output_max = 100;
	state->output_min = 0;
}

s32_t pid_calc_control_effort(struct pid_state *state, s32_t value)
{
	u32_t timestamp = k_uptime_get_32();
	s32_t dt = timestamp - state->timestamp;
	state->timestamp = timestamp;

	s32_t error = state->setpoint - value;

    s32_t deriv_error = error - state->pre_error;
    state->pre_error  = error;
    state->int_error += (error * dt) / 1000;

    if (state->int_error > state->int_error_max) {
		state->int_error = state->int_error_max;
    } else if (state->int_error < state->int_error_min) {
    	state->int_error = state->int_error_min;
    }

    s32_t output;
    output  = state->p * error;
    output += state->i * state->int_error;
    output += (state->d * deriv_error * 1000) / dt;

    if (output > state->output_max) {
    	return state->output_max;
    } else if (output < state->output_min) {
    	return state->output_min;
    }

    return output;
}
