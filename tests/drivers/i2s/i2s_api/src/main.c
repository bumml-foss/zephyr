/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include "i2s_api_test.h"

ZTEST_DMEM const struct device *dev_i2s_rx =
	DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_RX);
ZTEST_DMEM const struct device *dev_i2s_tx =
	DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_TX);
ZTEST_DMEM const struct device *dev_i2s =
	DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_RX);
ZTEST_DMEM bool dir_both_supported;

static void *setup(void)
{
	k_thread_access_grant(k_current_get(),
			      &rx_mem_slab, &tx_mem_slab);
	k_object_access_grant(dev_i2s_rx, k_current_get());
	k_object_access_grant(dev_i2s_tx, k_current_get());

	return NULL;
}

/* Configure RX and TX as separate streams with the given channel count. */
static void configure_separate(uint8_t channels)
{
	int ret;

	zassert_not_null(dev_i2s_rx, "RX device not found");
	zassert_true(device_is_ready(dev_i2s_rx), "device %s is not ready", dev_i2s_rx->name);
	zassert_not_null(dev_i2s_tx, "TX device not found");
	zassert_true(device_is_ready(dev_i2s_tx), "device %s is not ready", dev_i2s_tx->name);

	ret = configure_stream_channels(dev_i2s_rx, I2S_DIR_RX, channels);
	zassert_equal(ret, TC_PASS, "%u-channel RX configuration rejected", channels);

	ret = configure_stream_channels(dev_i2s_tx, I2S_DIR_TX, channels);
	zassert_equal(ret, TC_PASS, "%u-channel TX configuration rejected", channels);
}

/* Configure one device for both directions with the given channel count. */
static void configure_both(uint8_t channels)
{
	int ret;

	zassert_not_null(dev_i2s, "TX/RX device not found");
	zassert_true(device_is_ready(dev_i2s), "device %s is not ready", dev_i2s->name);

	ret = configure_stream_channels(dev_i2s, I2S_DIR_BOTH, channels);
	zassert_equal(ret, TC_PASS, "%u-channel configuration rejected", channels);

	/* Check if the tested driver supports the I2S_DIR_BOTH value.
	 * Use the DROP trigger for this, as in the current state of the driver
	 * (READY, both TX and RX queues empty) it is actually a no-op.
	 */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
	dir_both_supported = (ret == 0);

	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		zassert_true(dir_both_supported,
			     "I2S_DIR_BOTH value is supposed to be supported.");
	}
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	configure_separate(2U);
}

static void before_dir_both(void *fixture)
{
	ARG_UNUSED(fixture);

	configure_both(2U);
}

#ifdef CONFIG_I2S_TEST_MONO
/* Mono in the form the board uses for stereo: one device with I2S_DIR_BOTH
 * where the driver wants both directions started together, otherwise RX and
 * TX as separate streams.
 */
static void before_mono(void *fixture)
{
	ARG_UNUSED(fixture);

	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		configure_both(1U);
	} else {
		configure_separate(1U);
	}
}

/* A mono case that fails mid-transfer leaves the stream running, and the
 * next configure would be refused; drop whatever is queued so the suites
 * after this one start from READY.
 */
static void after_mono(void *fixture)
{
	ARG_UNUSED(fixture);

	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		(void)i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
		return;
	}

	(void)i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
	(void)i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_DROP);
}
#endif /* CONFIG_I2S_TEST_MONO */

ZTEST_SUITE(i2s_loopback, NULL, setup, before, NULL, NULL);
ZTEST_SUITE(i2s_states, NULL, setup, before, NULL, NULL);
ZTEST_SUITE(i2s_dir_both_states, NULL, setup, before_dir_both, NULL, NULL);
ZTEST_SUITE(i2s_dir_both_loopback, NULL, setup, before_dir_both, NULL, NULL);
ZTEST_SUITE(i2s_errors, NULL, setup, before, NULL, NULL);
#ifdef CONFIG_I2S_TEST_MONO
ZTEST_SUITE(i2s_mono, NULL, setup, before_mono, after_mono, NULL);
#endif /* CONFIG_I2S_TEST_MONO */
