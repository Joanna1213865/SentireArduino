/*
* SSD1331.cpp
* A customised library for Adafruit 0.96" RGB OLED module
* in working with Xadow Wearable Kit for Intel Edison
*
* This library is based on Adafruit's SSD1331-OLED-Driver-Library and
* and Seeed Studio's SSD1331 library, some ajustments were made in order
* to make the display module work with Xadow Edison Kit.
* A few functions were added for creating simple light patterns.
*/

#include "SSD1331.h"
#include <SPI.h>
#include <iostream>
#include <vector>


SSD1331::SSD1331(uint8_t cs, uint8_t dc, uint8_t mosi, uint8_t sck)
{
	_cs = cs;
	_dc = dc;
	_mosi = mosi;
	_sck = sck;
};


SSD1331::SSD1331(uint8_t cs, uint8_t dc)
{
	_cs = cs;
	_dc = dc;
	_mosi = 0; // default 0 in hardware SPI mode
	_sck = 0;
};

// To send data, first bring dc and cs low,
// then transfer data, finally bring cs high
void SSD1331::_sendCmd(uint8_t c)
{
	digitalWrite(_dc, LOW);
	digitalWrite(_cs, LOW);
	spiwrite(c);
	digitalWrite(_cs, HIGH);
}

void SSD1331::spiwrite(uint8_t c) {
	// _mosi is 0 when using hardware SPI
	// then directly using SPI library funtion
	if (!_mosi) {
		SPI.transfer(c);
		return;
	}

	// Otherwise, generate SCK and write MOSI following the timing diagram
	int8_t i;

	digitalWrite(_sck, HIGH);

	for (i = 7; i >= 0; i--) {
		digitalWrite(_sck, LOW);

		if (c) {
			digitalWrite(_mosi, HIGH);
		}
		else {
			digitalWrite(_mosi, LOW);
		}

		digitalWrite(_sck, HIGH);
	}
}

void SSD1331::init(void)
{
	pinMode(_dc, OUTPUT);

	if (_sck) {
		pinMode(_sck, OUTPUT);
		pinMode(_mosi, OUTPUT);
	}
	else {
		// using the hardware SPI
		SPI.begin();
		SPI.setDataMode(SPI_MODE3);
	}

	pinMode(_cs, OUTPUT);
	digitalWrite(_cs, LOW);

	_sendCmd(CMD_DISPLAY_OFF);          //Display Off
	_sendCmd(CMD_SET_CONTRAST_A);       //Set contrast for color A
	_sendCmd(0x91);                     //145
	_sendCmd(CMD_SET_CONTRAST_B);       //Set contrast for color B
	_sendCmd(0x50);                     //80
	_sendCmd(CMD_SET_CONTRAST_C);       //Set contrast for color C
	_sendCmd(0x7D);                     //125
	_sendCmd(CMD_MASTER_CURRENT_CONTROL);//master current control
	_sendCmd(0x06);                     //6
	_sendCmd(CMD_SET_PRECHARGE_SPEED_A);//Set Second Pre-change Speed For ColorA
	_sendCmd(0x64);                     //100
	_sendCmd(CMD_SET_PRECHARGE_SPEED_B);//Set Second Pre-change Speed For ColorB
	_sendCmd(0x78);                     //120
	_sendCmd(CMD_SET_PRECHARGE_SPEED_C);//Set Second Pre-change Speed For ColorC
	_sendCmd(0x64);                     //100
	_sendCmd(CMD_SET_REMAP);            //set remap & data format
	_sendCmd(0x60);                     //0x60: Not flipped and RGB
	_sendCmd(CMD_SET_DISPLAY_START_LINE);//Set display Start Line
	_sendCmd(0x0);
	_sendCmd(CMD_SET_DISPLAY_OFFSET);   //Set display offset
	_sendCmd(0x0);
	_sendCmd(CMD_NORMAL_DISPLAY);       //Set display mode
	_sendCmd(CMD_SET_MULTIPLEX_RATIO);  //Set multiplex ratio
	_sendCmd(0x3F);
	_sendCmd(CMD_SET_MASTER_CONFIGURE); //Set master configuration
	_sendCmd(0x8E);
	_sendCmd(CMD_POWER_SAVE_MODE);      //Set Power Save Mode
	_sendCmd(0x00);                     //0x00
	_sendCmd(CMD_PHASE_PERIOD_ADJUSTMENT);//phase 1 and 2 period adjustment
	_sendCmd(0x31);                     //0x31
	_sendCmd(CMD_DISPLAY_CLOCK_DIV);    //display clock divider/oscillator frequency
	_sendCmd(0xF0);
	_sendCmd(CMD_SET_PRECHARGE_VOLTAGE);//Set Pre-Change Level
	_sendCmd(0x3A);
	_sendCmd(CMD_SET_V_VOLTAGE);        //Set vcomH
	_sendCmd(0x3E);
	_sendCmd(CMD_DEACTIVE_SCROLLING);   //disable scrolling
	_sendCmd(CMD_NORMAL_BRIGHTNESS_DISPLAY_ON);//set display on
}

void SSD1331::drawPixel(uint16_t x, uint16_t y, uint16_t color)
{
	drawLine(x, y, x, y, color);
}

void SSD1331::drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
	if ((x0 < 0) || (y0 < 0) || (x1 < 0) || (y1 < 0))
		return;

	if (x0 >= RGB_OLED_WIDTH)  x0 = RGB_OLED_WIDTH - 1;
	if (y0 >= RGB_OLED_HEIGHT) y0 = RGB_OLED_HEIGHT - 1;
	if (x1 >= RGB_OLED_WIDTH)  x1 = RGB_OLED_WIDTH - 1;
	if (y1 >= RGB_OLED_HEIGHT) y1 = RGB_OLED_HEIGHT - 1;

	_sendCmd(CMD_DRAW_LINE);//draw line
	_sendCmd(x0);//start column
	_sendCmd(y0);//start row
	_sendCmd(x1);//end column
	_sendCmd(y1);//end row
				 //delay(SSD1331_DELAYS_HWLINE);
	_sendCmd((uint8_t)((color >> 11) & 0x1F));//R
	_sendCmd((uint8_t)((color >> 5) & 0x3F));//G
	_sendCmd((uint8_t)(color & 0x1F));//B
	delay(SSD1331_DELAYS_HWLINE);
}

void SSD1331::drawFrame(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t outColor, uint16_t fillColor)
{
	if ((x0 < 0) || (y0 < 0) || (x1 < 0) || (y1 < 0))
		return;

	if (x0 >= RGB_OLED_WIDTH)  x0 = RGB_OLED_WIDTH - 1;
	if (y0 >= RGB_OLED_HEIGHT) y0 = RGB_OLED_HEIGHT - 1;
	if (x1 >= RGB_OLED_WIDTH)  x1 = RGB_OLED_WIDTH - 1;
	if (y1 >= RGB_OLED_HEIGHT) y1 = RGB_OLED_HEIGHT - 1;

	_sendCmd(CMD_FILL_WINDOW);//fill window
	_sendCmd(ENABLE_FILL);
	_sendCmd(CMD_DRAW_RECTANGLE);//draw rectangle
	_sendCmd(x0);//start column
	_sendCmd(y0);//start row
	_sendCmd(x1);//end column
	_sendCmd(y1);//end row
				 //delay(SSD1331_DELAYS_HWLINE);
	_sendCmd((uint8_t)((outColor >> 11) & 0x1F));//R
	_sendCmd((uint8_t)((outColor >> 5) & 0x3F));//G
	_sendCmd((uint8_t)(outColor & 0x1F));//B
	_sendCmd((uint8_t)((fillColor >> 11) & 0x1F));//R
	_sendCmd((uint8_t)((fillColor >> 5) & 0x3F));//G
	_sendCmd((uint8_t)(fillColor & 0x1F));//B
	delay(SSD1331_DELAYS_HWFILL);
}

/**************************************************************************/
/*!
@brief  Draws a filled rectangle using HW acceleration
*/
/**************************************************************************/
///*
void SSD1331::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t fillcolor)
{
	// Send Fill
	_sendCmd(SSD1331_CMD_FILL);
	_sendCmd(0x01);

	_sendCmd(SSD1331_CMD_DRAWRECT);
	_sendCmd(x & 0xFF);							// Starting column
	_sendCmd(y & 0xFF);							// Starting row
	_sendCmd((x + w - 1) & 0xFF);	// End column
	_sendCmd((y + h - 1) & 0xFF);	// End row

									// Outline color
	_sendCmd((uint8_t)((fillcolor >> 11) << 1));
	_sendCmd((uint8_t)((fillcolor >> 5) & 0x3F));
	_sendCmd((uint8_t)((fillcolor << 1) & 0x3F));
	// Fill color
	_sendCmd((uint8_t)((fillcolor >> 11) << 1));
	_sendCmd((uint8_t)((fillcolor >> 5) & 0x3F));
	_sendCmd((uint8_t)((fillcolor << 1) & 0x3F));

	// Delay while the fill completes
	delay(SSD1331_DELAYS_HWFILL);
}

void SSD1331::copyWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	_sendCmd(CMD_COPY_WINDOW);//copy window
	_sendCmd(x0);//start column
	_sendCmd(y0);//start row
	_sendCmd(x1);//end column
	_sendCmd(y1);//end row
	_sendCmd(x2);//new column
	_sendCmd(y2);//new row
}

void SSD1331::dimWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	_sendCmd(CMD_DIM_WINDOW);//copy area
	_sendCmd(x0);//start column
	_sendCmd(y0);//start row
	_sendCmd(x1);//end column
	_sendCmd(y1);//end row
}

void SSD1331::clearWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	_sendCmd(CMD_CLEAR_WINDOW);//clear window
	_sendCmd(x0);//start column
	_sendCmd(y0);//start row
	_sendCmd(x1);//end column
	_sendCmd(y1);//end row
	delay(SSD1331_DELAYS_HWFILL);
}

void SSD1331::clearScreen(void)
{
	clearWindow(0, 0, RGB_OLED_WIDTH - 1, RGB_OLED_HEIGHT - 1);
}

void SSD1331::setScolling(ScollingDirection direction, uint8_t rowAddr, uint8_t rowNum, uint8_t timeInterval)
{
	uint8_t scolling_horizontal = 0x0;
	uint8_t scolling_vertical = 0x0;
	switch (direction) {
	case Horizontal:
		scolling_horizontal = 0x01;
		scolling_vertical = 0x00;
		break;
	case Vertical:
		scolling_horizontal = 0x00;
		scolling_vertical = 0x01;
		break;
	case Diagonal:
		scolling_horizontal = 0x01;
		scolling_vertical = 0x01;
		break;
	default:
		break;
	}
	_sendCmd(CMD_CONTINUOUS_SCROLLING_SETUP);
	_sendCmd(scolling_horizontal);
	_sendCmd(rowAddr);
	_sendCmd(rowNum);
	_sendCmd(scolling_vertical);
	_sendCmd(timeInterval);
	_sendCmd(CMD_ACTIVE_SCROLLING);
}

void SSD1331::enableScolling(bool enable)
{
	if (enable)
		_sendCmd(CMD_ACTIVE_SCROLLING);
	else
		_sendCmd(CMD_DEACTIVE_SCROLLING);
}

void SSD1331::setDisplayMode(DisplayMode mode)
{
	_sendCmd(mode);
}

void SSD1331::setDisplayPower(DisplayPower power)
{
	_sendCmd(power);
}


void SSD1331::drawChar(uint8_t ascii, uint16_t x, uint16_t y, uint16_t size, uint16_t color)
{
	if ((ascii < 32) || (ascii >= 127)) {
		return;
	}

	for (int8_t i = 0; i < FONT_X; i++) {
		int8_t temp = pgm_read_byte(&simpleFont[ascii - 0x20][i]);
		int8_t inrun = 0;
		int8_t runlen = 0;
		int8_t endrun = 0;

		for (int8_t f = 0; f < FONT_Y; f++) {
			if ((temp >> f) & 0x01) {
				if (inrun) runlen += 1;
				else {
					inrun = 1;
					runlen = 1;
				}
			}
			else if (inrun) {
				endrun = 1;
				inrun = 0;
			}

			if (f == FONT_Y - 1 && inrun) {
				endrun = 1;
				// need the +1 b/c we this code is normally
				// only triggered  when f == FONT_Y, due to the
				// edge-triggered nature of this algorithm
				f += 1;
			}

			if (endrun) {
				fillRect(x + i*size, y + (f - runlen)*size, size, runlen*size, color);
				inrun = 0;
				runlen = 0;
				endrun = 0;
			}
		}
	}
}

void SSD1331::drawString(const char *string, uint16_t x, uint16_t y, uint16_t size, uint16_t color)
{
	while (*string) {
		drawChar(*string, x, y, size, color);
		*string++;
		x += FONT_SPACE*size;
		if (x >= RGB_OLED_WIDTH - 1) {
			y += FONT_Y*size;
			x = 0;
		}
	}
}

void SSD1331::drawBitMap(uint16_t x, uint16_t y, const uint8_t *bitmap, uint16_t width, int16_t height, uint16_t color)
{
	uint16_t i, j, byteWidth = (width + 7) / 8;
	for (j = 0; j < height; j++) {
		for (i = 0; i < width; i++) {
			if (pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
				drawPixel(x + i, y + j, color);
			}
		}
	}
}

void SSD1331::fillScreen(uint16_t color)
{
	fillRect(0, 0, RGB_OLED_WIDTH, RGB_OLED_HEIGHT, color);
}

void SSD1331::fullScreenFlash(uint16_t color1, uint16_t interval1, uint16_t color2, uint16_t interval2, uint16_t times) {
	for (int i = 0; i < times; i++) {
		fillScreen(color1);
		delay(interval1);
		fillScreen(color2);
		delay(interval2);
	}
}

// This helper fuction is used to return a colour with
// high contrast to the input colour
uint16_t SSD1331::getContrastYIQ(uint16_t color) {
	// Convert colour to single R, G, B values
	uint8_t R = (uint8_t)((color >> 11) << 1);
	uint8_t G = (uint8_t)((color >> 5) & 0x3F);
	uint8_t B = (uint8_t)((color << 1) & 0x3F);

	// Convert Y in YIQ colour space
	int yiq = ((R * 299) + (G * 587) + (B * 114)) / 1000;

	// Decide a high contrast colour from black and white
	if (yiq >= 128)
		return COLOR_BLACK;
	else
		return COLOR_WHITE;
}

void SSD1331::patternWithText(uint16_t background_color, const char* text) {
	uint16_t text_color = getContrastYIQ(background_color);

	fillScreen(background_color);
	drawString(text, 15, 15, 1, text_color);
}

void SSD1331::patternWithTextAndFlash(uint16_t background_color, const char* text, uint16_t on_time, uint16_t off_time, uint16_t times) {
	uint16_t text_color = getContrastYIQ(background_color);

	for (int i = 0; i < times; i++) {
		fillScreen(background_color);
		drawString(text, 15, 15, 2, text_color);
		delay(on_time);
		fillScreen(text_color);
		drawString(text, 15, 15, 2, background_color);
		delay(off_time);
	}
}

void SSD1331::patternWithTextAndFlash(uint16_t background_color, const char* text, uint8_t x, uint8_t y, uint8_t size, uint16_t on_time, uint16_t off_time, uint16_t times) {
	uint16_t text_color = getContrastYIQ(background_color);

	for (int i = 0; i < times; i++) {
		fillScreen(background_color);
		drawString(text, x, y, size, text_color);
		delay(on_time);
		fillScreen(text_color);
		//drawString(text, x, y, size, background_color);
		delay(off_time);
	}
}

void SSD1331::playSequencedColors(std::vector<int> & effects) {
	for (unsigned int i = 0; i < effects.size(); i = i + 4) {
		uint16_t background_color = RGB(effects[i], effects[i + 1], effects[i + 2]);
		fillScreen(background_color);
		delay(effects[i + 3]);
		//clearScreen();
	}
	clearScreen();
}

void SSD1331::playSequencedColors(std::vector<int> & effects, const char* text) {
	for (unsigned int i = 0; i < effects.size(); i = i + 4) {
		uint16_t background_color = RGB(effects[i], effects[i + 1], effects[i + 2]);
		uint16_t text_color = getContrastYIQ(background_color);
		fillScreen(background_color);
		drawString(text, 5, 10, 2, text_color);
		delay(effects[i + 3]);
	}
	clearScreen();
}

void SSD1331::playSequencedColors(std::vector<int> & effects, const char* text, uint8_t x, uint8_t y, uint8_t size) {
	for (unsigned int i = 0; i < effects.size(); i = i + 4) {
		uint16_t background_color = RGB(effects[i], effects[i + 1], effects[i + 2]);
		uint16_t text_color = getContrastYIQ(background_color);
		fillScreen(background_color);
		drawString(text, x, y, size, text_color);
		delay(effects[i + 3]);
	}
	clearScreen();
}

uint16_t SSD1331::max(uint16_t x, uint16_t y) {
	if (x > y)
		return x;
	else
		return y;
}

uint16_t SSD1331::min(uint16_t x, uint16_t y) {
	if (x < y)
		return x;
	else
		return y;
}