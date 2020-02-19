
#ifdef __cplusplus
extern "C" {
#endif

void cos_init(void);

int cos_input_io_notify(u8_t value);
int cos_input_power_notify(u8_t value);
int cos_output_io_notify(u8_t value);
int cos_output_power_notify(u8_t value);

#ifdef __cplusplus
}
#endif
