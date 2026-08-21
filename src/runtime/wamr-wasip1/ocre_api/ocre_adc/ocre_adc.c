/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/util.h>

#include <wasm_export.h>

#include <string.h>

#include "ocre_adc.h"

LOG_MODULE_REGISTER(ocre_adc, CONFIG_OCRE_LOG_LEVEL);

#ifndef CONFIG_OCRE_ADC_MAX_CHANNELS
#define CONFIG_OCRE_ADC_MAX_CHANNELS 4
#endif

#ifndef CONFIG_OCRE_ADC_VREF_MV
#define CONFIG_OCRE_ADC_VREF_MV 3300
#endif

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

typedef struct {
	const char *name;
	struct adc_dt_spec spec;
	bool ready;
} ocre_adc_channel_t;

static ocre_adc_channel_t adc_channels[CONFIG_OCRE_ADC_MAX_CHANNELS];
static int adc_channel_count;
static bool adc_system_initialized;

/* ADC_DT_SPEC_GET_BY_NAME_OR() resolves entirely at compile time (it's a
 * COND_CODE_1 over a devicetree existence check), so it's always safe to
 * call even when "name" isn't defined on this board -- spec.dev is simply
 * NULL in that case, checked at runtime below. Only known, fixed channel
 * names are supported (mirrors ocre_gpio's by-name aliases); which of them
 * actually exist depends entirely on the board's devicetree overlay. */
#define ADD_ADC_CHANNEL_IF_PRESENT(name_str, token)                                                                   \
	do {                                                                                                           \
		if (adc_channel_count < CONFIG_OCRE_ADC_MAX_CHANNELS) {                                               \
			struct adc_dt_spec spec =                                                                     \
				ADC_DT_SPEC_GET_BY_NAME_OR(ZEPHYR_USER_NODE, token, ((struct adc_dt_spec){0}));      \
			if (spec.dev) {                                                                               \
				if (!adc_is_ready_dt(&spec)) {                                                        \
					LOG_ERR("ADC device for channel '%s' not ready", name_str);                   \
				} else if (adc_channel_setup_dt(&spec) != 0) {                                        \
					LOG_ERR("Failed to configure ADC channel '%s'", name_str);                    \
				} else {                                                                              \
					adc_channels[adc_channel_count].name = name_str;                              \
					adc_channels[adc_channel_count].spec = spec;                                  \
					adc_channels[adc_channel_count].ready = true;                                 \
					adc_channel_count++;                                                          \
					LOG_INF("ADC channel '%s' ready (channel_id=%d)", name_str,                   \
						spec.channel_id);                                                     \
				}                                                                                     \
			}                                                                                              \
		}                                                                                                      \
	} while (0)

int ocre_adc_init(void)
{
	if (adc_system_initialized) {
		LOG_INF("ADC system already initialized");
		return 0;
	}

	ADD_ADC_CHANNEL_IF_PRESENT("a0", a0);
	ADD_ADC_CHANNEL_IF_PRESENT("a1", a1);
	ADD_ADC_CHANNEL_IF_PRESENT("a2", a2);
	ADD_ADC_CHANNEL_IF_PRESENT("a3", a3);

	if (adc_channel_count == 0) {
		LOG_ERR("No ADC channels available -- does this board's devicetree define "
			"a zephyr,user io-channels entry?");
		return -ENODEV;
	}

	adc_system_initialized = true;
	LOG_INF("ADC system initialized, %d channel(s) available", adc_channel_count);
	return 0;
}

int ocre_adc_read_by_name(const char *name)
{
	if (!name || !adc_system_initialized) {
		LOG_ERR("ADC not initialized or invalid name");
		return -EINVAL;
	}

	ocre_adc_channel_t *chan = NULL;

	for (int i = 0; i < adc_channel_count; i++) {
		if (adc_channels[i].ready && strcmp(adc_channels[i].name, name) == 0) {
			chan = &adc_channels[i];
			break;
		}
	}

	if (!chan) {
		LOG_ERR("Unknown or unavailable ADC channel '%s'", name);
		return -ENODEV;
	}

	int16_t raw;
	struct adc_sequence sequence = {
		.buffer = &raw,
		.buffer_size = sizeof(raw),
	};

	int ret = adc_sequence_init_dt(&chan->spec, &sequence);
	if (ret != 0) {
		LOG_ERR("Failed to init ADC sequence for '%s': %d", name, ret);
		return ret;
	}

	ret = adc_read_dt(&chan->spec, &sequence);
	if (ret != 0) {
		LOG_ERR("Failed to read ADC channel '%s': %d", name, ret);
		return ret;
	}

	int32_t val_mv = raw;

	if (adc_raw_to_millivolts_dt(&chan->spec, &val_mv) == 0) {
		/* Reference voltage/gain known from devicetree: scale the
		 * calibrated millivolt reading against the configured full-scale
		 * range. */
		int scaled = (val_mv * 255) / CONFIG_OCRE_ADC_VREF_MV;
		return CLAMP(scaled, 0, 255);
	}

	/* Reference voltage not resolvable (e.g. ADC_REF_INTERNAL without a
	 * devicetree-provided zephyr,vref-mv) -- scale the raw code directly
	 * against the channel's configured resolution instead. */
	int max_raw = (1 << chan->spec.resolution) - 1;
	int scaled = ((int)raw * 255) / (max_raw > 0 ? max_raw : 1);
	return CLAMP(scaled, 0, 255);
}

int ocre_adc_wasm_init(wasm_exec_env_t exec_env)
{
	return ocre_adc_init();
}

int ocre_adc_wasm_read_by_name(wasm_exec_env_t exec_env, const char *name)
{
	if (!name) {
		LOG_ERR("Invalid name parameter");
		return -EINVAL;
	}

	return ocre_adc_read_by_name(name);
}
