## RGB_OLED_SSD1331
This is a library for the 0.96" 16-bit Color OLED with SSD1331 driver chip

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/684

These displays use SPI to communicate, 4 or 5 pins are required to  
interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above must be included in any redistribution

### Introduction
It is a 16 bit 96*64 dot matrix OLED display module with RGB color. RGB OLED 96*64 is based on SSD1331 module which is a single chip CMOS OLED/PLED driver with 288 segments and 64 common output, supporting up to 96RGB * 64 dot matrix display. It use SPI for communication.

### Feature
+ Resolution: 96RGB * 64 dot matrix panel
+ 65k color depth supported by embedded 96*64*16 bit GDDRAM display buffer
+ Graphic Accelerating Command (GAC) set with Continuous Horizontal, Vertical and Diagonal 
Scrolling
+ Programmable Frame Rate

### Interface
***Init the RGB OLED***

    void init(void);

***Draw Graphics***

    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
    void drawFrame(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t outColor, uint16_t fillColor);
    ……    

***Other Funny Interface***

    void copyWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,uint16_t x2, uint16_t y2);
    void dimWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    void clearWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    void setScolling(ScollingDirection direction, uint8_t rowAddr, uint8_t rowNum, uint8_t timeInterval);
    void enableScolling(bool enable);
    void setDisplayMode(DisplayMode mode);
    void setDisplayPower(DisplayPower power);

### Getting Started
Please take the example sketches in examples folder as reference, have fun!

----
This software is based on Adafruit's SSD1331 OLED Driver library and modified by lawliet zou [wei.zou@seeedstudio.com](wei.zou@seeedstudio.com) for seeed studio.  
it is licensed under [The BSD License](http://www.freebsd.org/copyright/freebsd-license.html). Check License.txt for more information.<br>

Contributing to this software is warmly welcomed. You can do this basically by [forking](https://help.github.com/articles/fork-a-repo), committing modifications and then [pulling requests](https://help.github.com/articles/using-pull-requests) (follow the links above for operating guide). Adding change log and your contact into file header is encouraged.<br>
Thanks for your contribution.

Seeed Studio is an open hardware facilitation company based in Shenzhen, China. <br>
Benefiting from local manufacture power and convenient global logistic system, <br>
we integrate resources to serve new era of innovation. Seeed also works with <br>
global distributors and partners to push open hardware movement.<br>

[![Analytics](https://ga-beacon.appspot.com/UA-46589105-3/RBG_OLED_96_64)](https://github.com/igrigorik/ga-beacon)

