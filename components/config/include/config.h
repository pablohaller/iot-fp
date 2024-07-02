#ifndef CONFIG_H
#define CONFIG_H

#include "esp_err.h"

esp_err_t config_set_value(const char *key, const char *value);
char *config_get_all_as_json(void);

#endif // CONFIG_H
