
#include <assert.h>
#include <stdint.h>

#include "ws28xx.h"
#include "util.h"
#include "bcm2835.h"
#include "bcm2835_spi.h"
#include "arm/synchronize.h"

#define WS2812_HIGH_CODE			0xF0		///< b11110000
#define WS2812_LOW_CODE				0xC0		///< b11000000
#define WS2812B_HIGH_CODE			0xF8		///< b11111000
#define WS2812B_LOW_CODE			0xC0		///< b11000000

static uint16_t led_count ALIGNED;
static _ws28xxx_type led_type ALIGNED = WS2812B;

static uint8_t spi_buffer[4 * 512 * 3 * 8] ALIGNED;	///<
static uint16_t buf_len ALIGNED;

/**
 *
 * @return
 */
const uint16_t ws28xx_get_led_count(void) {
	return led_count;
}

/**
 *
 * @return
 */
const _ws28xxx_type ws28xx_get_led_type(void) {
	return led_type;
}

/**
 *
 * @param index
 * @param value
 */
static void set_color_ws2812(uint16_t offset, const uint8_t value) {
	const uint8_t high_code = led_type == WS2812 ? WS2812_HIGH_CODE : WS2812B_HIGH_CODE;
	uint8_t mask;

	assert(offset + 7 < sizeof(spi_buffer));

	for (mask = 0x80; mask != 0; mask >>= 1) {
		if (value & mask) {
			spi_buffer[offset] = high_code;
		} else {
			spi_buffer[offset] = WS2812_LOW_CODE;	// Same for both
		}

		offset++;
	}
}

/**
 *
 * @param index
 * @param red
 * @param green
 * @param blue
 */
void ws28xx_set_led(const uint16_t index, const uint8_t red, const uint8_t green, const uint8_t blue) {
	uint16_t offset = index * 3;

	assert (index < led_count);

	if (led_type == WS2801) {
		assert(offset + 2 < sizeof(spi_buffer));

		spi_buffer[offset] = red;
		spi_buffer[offset + 1] = green;
		spi_buffer[offset + 2] = blue;
	} else {
		offset *= 8;

		set_color_ws2812(offset, green);
		set_color_ws2812(offset + 8, red);
		set_color_ws2812(offset + 16, blue);
	}

}

/**
 *
 */
void ws28xx_update(void) {
	dmb();
	bcm2835_spi_writenb((char *)spi_buffer, buf_len);
	dmb();
}

/**
 *
 * @param count
 * @param type
 * @param spi_speed
 */
void ws28xx_init(const uint16_t count, const _ws28xxx_type type, const uint32_t spi_speed) {
	uint16_t i;

	led_count = count;
	led_type = type;
	buf_len = led_count * 3;

	if (led_type == WS2812 || led_type == WS2812B) {
		buf_len *= 8;
	}

	for (i = 0; i < led_count; i++) {
		ws28xx_set_led(i, 0, 0, 0);
	}

	bcm2835_spi_begin();

	if (led_type == WS2801) {
		if (spi_speed == 0) {
			bcm2835_spi_setClockDivider((uint32_t) BCM2835_CORE_CLK_HZ / WS2801_SPI_SPEED_DEFAULT_HZ);
		} else {
			bcm2835_spi_setClockDivider((uint32_t) BCM2835_CORE_CLK_HZ / spi_speed);
		}
	} else {
		bcm2835_spi_setClockDivider((uint16_t) ((uint32_t) BCM2835_CORE_CLK_HZ / (uint32_t) 6400000));
	}

	bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
	bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

	ws28xx_update();
}

