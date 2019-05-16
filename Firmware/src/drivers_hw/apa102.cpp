#include "apa102.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "config/board_config.h"
#include "string.h" // for memset

using namespace Config;

#define DOTSTAR_BRG (1 | (2 << 2) | (0 << 4))
#define OFFSET_RED (DOTSTAR_BRG & 3)
#define OFFSET_GREEN ((DOTSTAR_BRG >> 2) & 3)
#define OFFSET_BLUE ((DOTSTAR_BRG >> 4) & 3)

namespace DriversHW
{
namespace APA102
{
	static uint8_t pixels[MAX_LED_COUNT * 3]; // LED RGB values (3 bytes ea.)
	static uint8_t numLEDs;
	static uint8_t dataPin;
	static uint8_t clockPin;
	static uint8_t powerPin;

	void init() {

		// Cache configuration data
		auto board = BoardManager::getBoard();
		dataPin = board->ledDataPin;
		clockPin = board->ledClockPin;
		powerPin = board->ledPowerPin;
		numLEDs = board->ledCount;
		clear();

		// Initialize the pins
		nrf_gpio_cfg_output(dataPin);
		nrf_gpio_cfg_output(clockPin);
		nrf_gpio_cfg_output(powerPin);
		nrf_gpio_pin_clear(dataPin);
		nrf_gpio_pin_clear(clockPin);
		nrf_gpio_pin_clear(powerPin);
	}

	void clear() {
		memset(pixels, 0, numLEDs * 3);
	}

	#define spi_out(n) swSpiOut(n)

	void swSpiOut(uint8_t n) { // Bitbang SPI write
		for (uint8_t i = 8; i--; n <<= 1) {
			if (n & 0x80) {
				nrf_gpio_pin_set(dataPin);
			} else {
				nrf_gpio_pin_clear(dataPin);
			}
			nrf_gpio_pin_set(clockPin);
			nrf_delay_us(1);
			nrf_gpio_pin_clear(clockPin);
			nrf_delay_us(1);
		}
	}

	/* ISSUE DATA TO LED STRIP -------------------------------------------------

	From the Adafruit original code:
	"Although the LED driver has an additional per-pixel 5-bit brightness
	setting, it is NOT used or supported here because it's a brain-dead
	misfeature that's counter to the whole point of Dot Stars, which is to
	have a much faster PWM rate than NeoPixels.  It gates the high-speed
	PWM output through a second, much slower PWM (about 400 Hz), rendering
	it useless for POV.  This brings NOTHING to the table that can't be
	already handled better in one's sketch code.  If you really can't live
	without this abomination, you can fork the library and add it for your
	own use, but any pull requests for this will NOT be merged, nuh uh!"
	*/

	void show(void) {

		if (!pixels) return;

		// Turn power on so we display something!!!
		nrf_gpio_pin_set(powerPin);

		// May need a delay here...
		//nrf_delay_ms(1);

		uint8_t *ptr = pixels;            // -> LED data
		uint16_t n = numLEDs;              // Counter

		bool allOff = true;
		for (int i = 0; i < 4; i++) {
			swSpiOut(0);    // Start-frame marker
		}
		do {                               // For each pixel...
			swSpiOut(0xFF);                //  Pixel start
			for (int i = 0; i < 3; i++) {
				uint8_t comp = *ptr;
				swSpiOut(comp); // R,G,B
				if (comp != 0) {
					// At least one component of one led was not 0
					allOff = false;
				}
				ptr++;
			}
		} while (--n);
		for (int i = 0; i < ((numLEDs + 15) / 16); i++) {
			swSpiOut(0xFF); // End-frame marker (see note above)
		}

		// Drop lines low again, reduces power consumption
		nrf_gpio_pin_clear(dataPin);
		nrf_gpio_pin_clear(clockPin);

		if (allOff) {
			// Turn power off too
			nrf_gpio_pin_clear(powerPin);
		}
	}

	// Set pixel color, separate R,G,B values (0-255 ea.)
	void setPixelColor(
		uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
		if (n < numLEDs) {
			uint8_t *p = &pixels[n * 3];
			p[OFFSET_RED] = r;
			p[OFFSET_GREEN] = g;
			p[OFFSET_BLUE] = b;
		}
	}

	// Set pixel color, 'packed' RGB value (0x000000 - 0xFFFFFF)
	void setPixelColor(uint16_t n, uint32_t c) {
		if (n < numLEDs) {
			uint8_t *p = &pixels[n * 3];
			p[OFFSET_RED] = (uint8_t)(c >> 16);
			p[OFFSET_GREEN] = (uint8_t)(c >> 8);
			p[OFFSET_BLUE] = (uint8_t)c;
		}
	}

	// Convert separate R,G,B to packed value
	uint32_t color(uint8_t r, uint8_t g, uint8_t b) {
		return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
	}

	// Read color from previously-set pixel, returns packed RGB value.
	uint32_t getPixelColor(uint16_t n) {
		if (n >= numLEDs) return 0;
		uint8_t *p = &pixels[n * 3];
		return ((uint32_t)p[OFFSET_RED] << 16) |
			((uint32_t)p[OFFSET_GREEN] << 8) |
			(uint32_t)p[OFFSET_BLUE];
	}

	uint16_t numPixels() { // Ret. strip length
		return numLEDs;
	}

	// Return pointer to the library's pixel data buffer.  Use carefully,
	// much opportunity for mayhem.  It's mostly for code that needs fast
	// transfers, e.g. SD card to LEDs.  Color data is in BGR order.
	uint8_t* getPixels() {
		return pixels;
	}
}
}

/* A PROGMEM (flash mem) table containing 8-bit unsigned sine wave (0-255).
   Copy & paste this snippet into a Python REPL to regenerate:
import math
for x in range(256):
	print("{:3},".format(int((math.sin(x/128.0*math.pi)+1.0)*127.5+0.5))),
	if x&15 == 15: print
*/
static const uint8_t _sineTable[256] = {
  128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
  176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,
  218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,
  245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
  255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
  245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,
  218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,
  176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
  128,124,121,118,115,112,109,106,103,100, 97, 93, 90, 88, 85, 82,
   79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40,
   37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11,
   10,  9,  7,  6,  5,  5,  4,  3,  2,  2,  1,  1,  1,  0,  0,  0,
	0,  0,  0,  0,  1,  1,  1,  2,  2,  3,  4,  5,  5,  6,  7,  9,
   10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35,
   37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76,
   79, 82, 85, 88, 90, 93, 97,100,103,106,109,112,115,118,121,124 };

/* Similar to above, but for an 8-bit gamma-correction table.
   Copy & paste this snippet into a Python REPL to regenerate:
import math
gamma=2.6
for x in range(256):
	print("{:3},".format(int(math.pow((x)/255.0,gamma)*255.0+0.5))),
	if x&15 == 15: print
*/
static const uint8_t _gammaTable[256] = {
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,
	1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,
	3,  3,  4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  7,
	7,  7,  8,  8,  8,  9,  9,  9, 10, 10, 10, 11, 11, 11, 12, 12,
   13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20,
   20, 21, 21, 22, 22, 23, 24, 24, 25, 25, 26, 27, 27, 28, 29, 29,
   30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 38, 38, 39, 40, 41, 42,
   42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
   58, 59, 60, 61, 62, 63, 64, 65, 66, 68, 69, 70, 71, 72, 73, 75,
   76, 77, 78, 80, 81, 82, 84, 85, 86, 88, 89, 90, 92, 93, 94, 96,
   97, 99,100,102,103,105,106,108,109,111,112,114,115,117,119,120,
  122,124,125,127,129,130,132,134,136,137,139,141,143,145,146,148,
  150,152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,
  182,184,186,188,191,193,195,197,199,202,204,206,209,211,213,215,
  218,220,223,225,227,230,232,235,237,240,242,245,247,250,252,255 };

uint8_t sine8(uint8_t x) {
	return _sineTable[x]; // 0-255 in, 0-255 out
}

uint8_t gamma8(uint8_t x) {
	return _gammaTable[x]; // 0-255 in, 0-255 out
}
