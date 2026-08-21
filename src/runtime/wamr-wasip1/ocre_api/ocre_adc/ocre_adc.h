/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_ADC_H
#define OCRE_ADC_H

#include <wasm_export.h>

/**
 * Portable analog input, following the same devicetree "zephyr,user" +
 * io-channels convention as Zephyr's own samples/drivers/adc/adc_dt sample
 * (see that sample for the full pattern this mirrors). A board makes a
 * channel available under a name (e.g. "a0") via a devicetree overlay:
 *
 *   / {
 *       zephyr,user {
 *           io-channels = <&adc0 0>;
 *           io-channel-names = "a0";
 *       };
 *   };
 *
 * This native API only knows about a fixed set of channel names
 * ("a0".."a3", matching common Arduino-style board silkscreens); which of
 * them actually resolve depends entirely on what the board's devicetree
 * defines -- the C code here is unchanged across boards. Values are
 * returned pre-scaled to 0-255 (see CONFIG_OCRE_ADC_VREF_MV) so containers
 * don't need to know the channel's resolution or reference voltage.
 *
 * @return 0 on success, negative error code on failure.
 */
int ocre_adc_init(void);

/**
 * @brief Reads the named analog input channel, scaled to 0-255.
 *
 * @param name Channel name (see ocre_adc_init()).
 * @return 0-255 on success, negative error code on failure (e.g. unknown
 *         channel name, or the channel isn't defined on this board).
 */
int ocre_adc_read_by_name(const char *name);

int ocre_adc_wasm_init(wasm_exec_env_t exec_env);
int ocre_adc_wasm_read_by_name(wasm_exec_env_t exec_env, const char *name);

#endif /* OCRE_ADC_H */
