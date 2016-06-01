#include <Wire.h>
#include <Intel_Edison_BT_SPP.h>

Intel_Edison_BT_SPP spp = Intel_Edison_BT_SPP();

void setup() {
	Serial.begin(115200);
	Serial.println("Intel Edison BT SPP test!");
	
	spp.open();
}

void loop() {

	ssize_t size = spp.read();

	if (size != -1) {
		Serial.println(spp.getBuf());
	} else {
		// Well, some error here 
		delay(500);
	}
}
