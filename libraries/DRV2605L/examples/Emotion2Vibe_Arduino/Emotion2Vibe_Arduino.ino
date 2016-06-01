

#include <SSD1331.h>
#include <Intel_Edison_BT_SPP.h>
#include <Adafruit_DRV2605.h>
#include <ADXL345.h>
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include <WiFi.h>
#include <Dns.h>
#include <Dhcp.h>
#include <Wire.h>
#include <SPI.h>
#include <iostream>
#include <vector>
#include <string>
#include "myUtils.h"

#define cs     10
#define dc     12
#define mosi   11
#define sclk   13


SSD1331 oled = SSD1331(cs, dc);
Adafruit_DRV2605 drv;
Intel_Edison_BT_SPP spp;
//AlarmMotor alarm_motor;
ADXL345 adxl;
//Gesture gesture;


void setup() {
	Serial.begin(115200);
	Serial.println("Start");
	//spp.open();
	drv.begin();
	drv.selectLibrary(1);

	drv.setMode(DRV2605_MODE_INTTRIG);
	drv.setWaveform(0, 84);  // ramp up medium 1, see datasheet part 11.2
	drv.setWaveform(1, 1);  // strong click 100%, see datasheet part 11.2
	drv.setWaveform(2, 0);  // end of waveform


	drv.go();
	Serial.println("begin");
	//oled.init();
	//delay(1000);
	//oled.fillScreen(COLOR_BLACK);
	//delay(1000);
	//oled.drawBitMap(0, 0, SeeedLogo, 96, 64, COLOR_YELLOW);
	/*
	for (int i = 30; i > 0; i--) {
		oled.drawCircle(48, 32, i, COLOR_CYAN);
		delay(5);
	}
	for (int i = 1; i <= 30; i++) {
		oled.drawCircle(48, 32, i, COLOR_RED);
		delay(5);
	}
	for (int i = 30; i > 0; i--) {
		oled.drawCircle(48, 32, i, COLOR_PURPLE);
		delay(5);
	}
	for (int i = 1; i <= 30; i++) {
		oled.drawCircle(48, 32, i, COLOR_GOLDEN);
		delay(5);
	}
	*/
	//delay(1000);
	//oled.clearScreen();
	//gesture.init();


	//adxl.setup();
	//alarm_motor.init();
//	spp.open();

	//drv.begin();
	//drv.selectLibrary(1);
	//drv.setMode(DRV2605_MODE_INTTRIG); 
	Serial.println("Finish setup");
}

bool replay = false;
std::string Str_copy = "";
std::vector<std::string> splitted_strings;

void loop() {
	ssize_t size = spp.read();
	//Serial.println(size);
	if (size != -1 || replay) {
	//	adxl.setInterrupt(ADXL345_INT_SINGLE_TAP_BIT, 0);
	//	adxl.setInterrupt(ADXL345_INT_DOUBLE_TAP_BIT, 0);

		if (!replay) {
			//alarm_motor.doublePulse(100);
			
			delay(500);

			const char * buf = spp.getBuf();
			std::string Str(buf);
			splitted_strings = splitString(Str, ":");
			Str_copy = Str;
			Serial.println(buf);

		}
		else {
			splitted_strings = splitString(Str_copy, ":");
			replay = false;
		}

		//Serial.println(splitted_strings.size());
		
		//Serial.println(splitted_strings.size());
		if (splitted_strings.size() == 2) {
			int mode_no = atoi(splitted_strings[0].c_str());
			std::vector<int> effects = extractEffects(splitted_strings[1]);
			switch (mode_no) {
				case 1:
					oled.patternWithTextAndFlash(COLOR_BLUE, &splitted_strings[1][0], 10, 10, 2, 500, 10, 2);
					oled.clearScreen();

					Serial.print("Number of effects: ");  Serial.println(effects.size());
					drv.setMode(DRV2605_MODE_INTTRIG);
					drv.playLibraryEffects(effects);
					break;
				case 2:
					drv.setMode(DRV2605_MODE_REALTIME);
					Serial.print("Number of effects: ");  Serial.println(effects.size());
					drv.playRealTimeEffects(effects, 100);
				default:
					oled.patternWithTextAndFlash(COLOR_BLUE, &splitted_strings[1][0], 10, 10, 2, 500, 10, 10);
			}


		}
		//else if (splitted_strings.size() == 1) {
		//	std::vector<int> effects = extractEffects(splitted_strings[0]);
		//	drv.setMode(DRV2605_MODE_INTTRIG);
		//	Serial.print("Number of effects: ");  Serial.println(effects.size());
		//	drv.playLibraryEffects(effects);
		//}

		delay(500);
	}

	
	else {
		/*
		byte interrupts = adxl.getInterruptSource();

		int tap = adxl.triggered(interrupts, ADXL345_SINGLE_TAP);
		int double_tap = adxl.triggered(interrupts, ADXL345_DOUBLE_TAP);

		//double tap
		if (double_tap) {
			Serial.println("double tap");
			char* str = "Happy!";
			oled.init();
			oled.clearScreen();
			oled.patternWithTextAndFlash(COLOR_GOLDEN, str, 10, 10, 2, 1000, 10, 10);
			//add code here to do when a tap is sensed
			delay(1000);
			oled._sendCmd(SSD1331_CMD_DISPLAYALLOFF);

		}


		//tap
		if (tap && !double_tap) {
			Serial.println("tap");
			oled.init();
			//oled.drawFrame(0, 0, 95, 63, COLOR_YELLOW, COLOR_YELLOW);
			//delay(2000);
			oled.fullScreenFlash(COLOR_PURPLE, 100, COLOR_BLACK, 50, 10);
			oled.drawBitMap(0, 0, SeeedLogo, 96, 64, COLOR_YELLOW);
			//add code here to do when a tap is sensed
			delay(1000);
			oled._sendCmd(SSD1331_CMD_DISPLAYALLOFF);

		}

		delay(1000);
		*/
		/*
		adxl.setInterrupt(ADXL345_INT_SINGLE_TAP_BIT, 1);
		adxl.setInterrupt(ADXL345_INT_DOUBLE_TAP_BIT, 1);
		byte interrupts = adxl.getInterruptSource();
		bool doubleTap = adxl.triggered(interrupts, ADXL345_DOUBLE_TAP);
		bool singleTap = adxl.triggered(interrupts, ADXL345_SINGLE_TAP);

		//double tap
		if (doubleTap && singleTap) {
			Serial.println("double tap");
			//add code here to do when a 2X tap is sensed
			//doubleTap = false;
			replay = true;

		}

		//tap
		if (singleTap && !doubleTap) {
			Serial.println("single tap");
			//add code here to do when a tap is sensed
			//singleTap = false;
			/*
			delay(200);//debug
			if (!gesture.samplingAccelerateData) {
				gesture.checkMoveStart();
			}
			if (gesture.samplingAccelerateData) {
				if (0 != gesture.getAccelerateData()) {
					Serial.print("\r\n get accelerate data error.");
				}
			}
			if (gesture.calculatingAccelerateData) {
				gesture.calculateAccelerateData();
			}
		}

		
		gesture.wakeUp();
		if (gesture.gestureWakeUp) {
			gesture.gestureWakeUp = 0;
			Serial.println("Wake up");
			delay(3000);

			int mode = gesture.getVergence();
			switch (mode) {
			case 1:
 
				Serial.println(" Gesture Mode"); //Print the String

				delay(2000);
				enterGestureMode();
				break;
			default:
				break;
			}
			

		}
		*/



	}
	
  // wait a bit
	delay(500);

}



/*
void enterGestureMode(void)
{
	if (!gesture.samplingAccelerateData) {
		gesture.checkMoveStart();
	}
	if (gesture.samplingAccelerateData) {
		if (0 != gesture.getAccelerateData()) {
			Serial.println("Gesture Error!"); //Print the String
		}
	}
	if (gesture.calculatingAccelerateData) {
		int matchingResult = gesture.calculateAccelerateData();
		Serial.println(" matching:");
		//char number[2];
		//Serial.println(matchingResult); //Print the String
		if ((matchingResult >= 0) && (matchingResult <= 3)) {
			delay(3000);
			Serial.println("So, do something...");
		}
	}
}
*/