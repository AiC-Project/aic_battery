#ifndef __AIC_BATTERY__
#define __AIC_BATTERY__

#define AIC_FAKE_POWER_SUPPLY "/sys/class/power_supply/aic_fake_path"

#ifdef __cplusplus
extern "C" {
#endif
int aic_get_value_from_proc(const char* path, char* buf, size_t size);
int aic_use_fake_battery_value(const char* path, char* buf, size_t size);
#ifdef __cplusplus
}
#endif
#endif
