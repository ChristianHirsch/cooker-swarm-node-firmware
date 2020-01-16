/*
 * =====================================================================================
 *
 *       Filename:  cooker.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  01/13/20 21:26:23
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void cooker_init();

u16_t get_input_power_level();

void set_output_power_level(u8_t level);
u8_t get_output_power_level();

u8_t get_input_io_level();

void set_output_io_level(u8_t level);
u8_t get_output_io_level();

#ifdef __cplusplus
}
#endif
