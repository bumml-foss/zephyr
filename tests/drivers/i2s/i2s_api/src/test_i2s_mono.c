/*
 * Copyright (c) 2026 Benedikt Humml
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/i2s.h>
#include "i2s_api_test.h"

/* A mono block carries one sample per frame, so twice the frames of a stereo
 * block of the same size; the shared fill and verify helpers lay it out that
 * way once the stream is configured with one channel.
 */
#define MONO_FRAMES_PER_BLOCK (2 * SAMPLE_NO)

BUILD_ASSERT(MONO_FRAMES_PER_BLOCK * sizeof(int16_t) == BLOCK_SIZE);

/* One device triggered with I2S_DIR_BOTH where the driver wants both
 * directions started together, otherwise RX and TX each on their own.
 */
static int mono_trigger_start(void)
{
	int ret;

	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		return i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	}

	ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_START);
	if (ret < 0) {
		return ret;
	}

	return i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_START);
}

static int mono_trigger_drain(void)
{
	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		return i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	}

	return i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
}

/* With separate streams RX is stopped by hand once all but the last block
 * are read; with I2S_DIR_BOTH the DRAIN already stopped it.
 */
static int mono_trigger_rx_stop(void)
{
	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		return 0;
	}

	return i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_STOP);
}

static bool mono_transfer_possible(void)
{
	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH) && !dir_both_supported) {
		TC_PRINT("I2S_DIR_BOTH value is not supported.\n");
		return false;
	}

	return true;
}

/** @brief Mono streams are accepted.
 *
 * - Configuring a stream with a single channel returns success.
 */
ZTEST_USER(i2s_mono, test_i2s_mono_configure)
{
	int ret;

	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		ret = configure_stream_channels(dev_i2s, I2S_DIR_BOTH, 1U);
	} else {
		ret = configure_stream_channels(dev_i2s_rx, I2S_DIR_RX, 1U);
		if (ret == TC_PASS) {
			ret = configure_stream_channels(dev_i2s_tx, I2S_DIR_TX, 1U);
		}
	}
	zassert_equal(ret, TC_PASS, "mono configuration rejected");
}

/** @brief Short mono transfer.
 *
 * - One sample per frame goes out and comes back unchanged.
 */
ZTEST_USER(i2s_mono, test_i2s_mono_transfer_short)
{
	int ret;

	if (!mono_transfer_possible()) {
		ztest_test_skip();
		return;
	}

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s_tx, 0, 0);
	zassert_equal(ret, TC_PASS);

	ret = tx_block_write(dev_i2s_tx, 1, 0);
	zassert_equal(ret, TC_PASS);

	ret = mono_trigger_start();
	zassert_equal(ret, 0, "START trigger failed");

	ret = rx_block_read(dev_i2s_rx, 0);
	zassert_equal(ret, TC_PASS);

	ret = tx_block_write(dev_i2s_tx, 2, 0);
	zassert_equal(ret, TC_PASS);

	/* All data written, drain TX queue. */
	ret = mono_trigger_drain();
	zassert_equal(ret, 0, "DRAIN trigger failed");

	ret = rx_block_read(dev_i2s_rx, 1);
	zassert_equal(ret, TC_PASS);

	ret = mono_trigger_rx_stop();
	zassert_equal(ret, 0, "RX STOP trigger failed");

	ret = rx_block_read(dev_i2s_rx, 2);
	zassert_equal(ret, TC_PASS);
}

#define TEST_I2S_MONO_RATE_BLOCKS 100

/** @brief Mono transfer runs at the configured frame rate.
 *
 * - A loopback cannot tell a wrong bit clock from a right one, because both
 *   ends share it. Wall-clock time can: a mono block of MONO_FRAMES_PER_BLOCK
 *   frames takes MONO_FRAMES_PER_BLOCK / FRAME_CLK_FREQ seconds. A driver
 *   that derives the bit clock from the channel count instead of the slots
 *   in a frame clocks mono at half speed and misses this by a factor of two.
 */
ZTEST_USER(i2s_mono, test_i2s_mono_transfer_rate)
{
	const int64_t expected_ms = (int64_t)(TEST_I2S_MONO_RATE_BLOCKS + 1) *
				    MONO_FRAMES_PER_BLOCK * MSEC_PER_SEC / FRAME_CLK_FREQ;
	int64_t start;
	int64_t elapsed_ms;
	int ret;

	if (!mono_transfer_possible()) {
		ztest_test_skip();
		return;
	}

	/* Prefill TX queue */
	ret = tx_block_write(dev_i2s_tx, 0, 0);
	zassert_equal(ret, TC_PASS);

	start = k_uptime_get();

	ret = mono_trigger_start();
	zassert_equal(ret, 0, "START trigger failed");

	for (int i = 0; i < TEST_I2S_MONO_RATE_BLOCKS; i++) {
		ret = tx_block_write(dev_i2s_tx, 0, 0);
		zassert_equal(ret, TC_PASS);

		ret = rx_block_read(dev_i2s_rx, 0);
		zassert_equal(ret, TC_PASS);
	}

	/* All data written, all but one data block read. */
	ret = mono_trigger_drain();
	zassert_equal(ret, 0, "DRAIN trigger failed");

	ret = mono_trigger_rx_stop();
	zassert_equal(ret, 0, "RX STOP trigger failed");

	ret = rx_block_read(dev_i2s_rx, 0);
	zassert_equal(ret, TC_PASS);

	elapsed_ms = k_uptime_get() - start;

	TC_PRINT("%d mono blocks in %lld ms, expected %lld ms\n", TEST_I2S_MONO_RATE_BLOCKS + 1,
		 elapsed_ms, expected_ms);
	zassert_within(elapsed_ms, expected_ms, expected_ms / 4,
		       "mono stream ran at the wrong rate: %lld ms for %lld ms of audio",
		       elapsed_ms, expected_ms);
}
