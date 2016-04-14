/*
 Name:		SentireArduino.ino
 Created:	4/9/2016 2:03:30 AM
 Author:	songer
*/

// the setup function runs once when you press reset or power the board
#include <SD.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <Intel_Edison_BT_SPP.h>
#include <ArduinoJson.h>
#include <SSD1331.h>
#include <Adafruit_DRV2605.h>
#include <ADXL345.h>
#include <iostream>
#include <vector>
#include <string>
#include "myUtils.h"
#include "Pattern.h"

#define STANDARD_MODE 1
#define TESTING_MODE 2

#define RESPONSE_JSON_DATA_LINENNO 10

#define cs     10
#define dc     12
#define mosi   11
#define sclk   13

Adafruit_DRV2605 drv;
SSD1331 oled = SSD1331(cs, dc);
Intel_Edison_BT_SPP spp;
ADXL345 adxl;
WiFiClient client;

File offlineFile;

//char ssid[] = "songer1993"; //  your network SSID (name)
//char pass[] = "150151325wqs";    // your net ork password (use for WPA, or use as key for WEP)
char ssid[] = "VM236337-2G"; //  your network SSID (name)
char pass[] = "jedrtdsf";    // your net ork password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;

const char* server = "songerarduinotest.azure-mobile.net";
const char* ams_key = "IBmOdZkslBSsjrCkJeQNvpjHOpTQYr42";
const char* table_names[2] = { "vibrationpattern", "lightpattern" };
const char* emotion_types[6] = { "happy", "fearful", "surprised", "sad", "disgusted", "angry" };
char* offline_file_name = "offline_patterns.txt";
int decision = -1;
char buffer[2048];

int mode = STANDARD_MODE;
int double_tap_counter = 0;
unsigned long timestamp;
bool connection_failure = false;
bool replay = false;
std::vector<String> responses;

int gesture_state = 0;
byte interrupts;
double rawxyz[3];
int xyz[2][3];
int shake_counter = 0;

void setup() {
	// Initialise serial port and oled and display starting page
	oled.init();
	oled.clearScreen();
	oled.drawString("Sentire", 0, 0, 2, COLOR_WHITE);
	sprintf(buffer, "Starting...");
	Serial.begin(115200);
	Serial.println(buffer);
	Serial.println();
	oled.drawString(buffer, 10, 32, 1, COLOR_WHITE);

	// Initialise ohter modules
	drv.begin();
	drv.selectLibrary(1);
	spp.open();
	adxl.setup();

	//connectWifi();

	refreshAllPatternsFromLocalStore();
	Serial.println("Setup Done!");
	shortVibrationPulse();
	oled.clearScreen();
}

// the loop function runs over and over again until power down or reset
void loop() {
	stateMachine();
}

void stateMachine() {
	switch (mode) {
	case STANDARD_MODE:
		standardMode();
		break;
	case TESTING_MODE:
		testingMode();
		break;
	}
}

void standardMode() {
	ssize_t size = spp.read();

	if (((size != -1) && (size != 0)) || replay) {
		adxl.setInterrupt(ADXL345_INT_SINGLE_TAP_BIT, 0);
		adxl.setInterrupt(ADXL345_INT_DOUBLE_TAP_BIT, 0);
		if (!replay) {
			const char* buf = spp.getBuf();
			Serial.println(buf);
			if (*(buf + 1) != ':') {
				decision = atoi(buf);
				if (decision >= 0 && decision < 6)
					parseDecision(decision);
			}
		}
		else {
			if (decision >= 0 && decision < 6)
				parseDecision(decision);
			replay = false;
		}

		adxl.setInterrupt(ADXL345_INT_SINGLE_TAP_BIT, 1);
		adxl.setInterrupt(ADXL345_INT_DOUBLE_TAP_BIT, 1);
	}

	gestureStateMachineInStandardMode();
}

void gestureStateMachineInStandardMode() {
	Serial.print("Gesture state: "); Serial.println(gesture_state);

	switch (gesture_state) {
	case 0:
		adxl.getAcceleration(rawxyz);
		xyz[0][0] = thresholding(rawxyz[0], 128);
		xyz[0][1] = thresholding(rawxyz[1], 128);
		xyz[0][2] = thresholding(rawxyz[2], 128);
		interrupts = adxl.getInterruptSource();
		if (adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)) {
			double_tap_counter += 1;
			timestamp = millis();
			gesture_state = 2;
		}
		else if (adxl.triggered(interrupts, ADXL345_SINGLE_TAP)) {
			gesture_state = 0;
			Serial.println("Single tap");
			sprintf(buffer, ":)");
			Serial.println(buffer);
			Serial.println();
			oled.clearScreen();
			oled.drawString(buffer, 0, 0, 2, COLOR_WHITE);
			delay(1000);
			oled.clearScreen();
			if (decision >= 0 && decision < 6)
				updatePatternScore(emotion_types[decision], true);
		}
		else {
			gesture_state = 1;
			timestamp = millis();
		}

		break;
	case 1:
		adxl.getAcceleration(rawxyz);
		xyz[1][0] = thresholding(rawxyz[0], 128);
		xyz[1][1] = thresholding(rawxyz[1], 128);
		xyz[1][2] = thresholding(rawxyz[2], 128);
		interrupts = adxl.getInterruptSource();
		if ((xyz[0][0] ^ xyz[1][0]) || (xyz[0][1] ^ xyz[1][1])) {
			gesture_state = 3;
		}
		else if (adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)) {
			double_tap_counter += 1;
			timestamp = millis();
			gesture_state = 2;
		}
		else if (adxl.triggered(interrupts, ADXL345_SINGLE_TAP)) {
			gesture_state = 0;
			Serial.println("Single tap");
			sprintf(buffer, ":)");
			Serial.println(buffer);
			Serial.println();
			oled.clearScreen();
			oled.drawString(buffer, 0, 0, 2, COLOR_WHITE);
			delay(1000);
			oled.clearScreen();
			if (decision >= 0 && decision < 6)
				updatePatternScore(emotion_types[decision], true);
		}
		else {
			gesture_state = 0;
		}
		break;
	case 2:
		interrupts = adxl.getInterruptSource();
		if (adxl.triggered(interrupts, ADXL345_DOUBLE_TAP) && ((millis() - timestamp) < 2500)) {
			Serial.println("Double tap twice");
			gesture_state = 0;
			double_tap_counter = 0;
			sprintf(buffer, "Testing");
			Serial.println(buffer);
			Serial.println();
			oled.clearScreen();
			oled.drawString(buffer, 0, 0, 2, COLOR_WHITE);
			delay(1000);
			oled.clearScreen();
			mode = TESTING_MODE;
		}
		else if ((millis() - timestamp) >= 2050) {
			Serial.println("Double tap once");
			gesture_state = 0;
			double_tap_counter = 0;
			sprintf(buffer, ":(");
			Serial.println(buffer);
			Serial.println();
			oled.clearScreen();
			oled.drawString(buffer, 0, 0, 2, COLOR_WHITE);
			delay(1000);
			oled.clearScreen();
			if (decision >= 0 && decision < 6)
				updatePatternScore(emotion_types[decision], false);
		}
		break;
	case 3:
		shake_counter = 0;
		while (shake_counter < 3 && millis() - timestamp < 1500) {
			//Serial.println(shake_counter);
			adxl.getAcceleration(rawxyz);
			xyz[0][0] = thresholding(rawxyz[0], 128);
			xyz[0][1] = thresholding(rawxyz[1], 128);
			xyz[0][2] = thresholding(rawxyz[2], 128);
			delay(100);
			adxl.getAcceleration(rawxyz);
			xyz[1][0] = thresholding(rawxyz[0], 128);
			xyz[1][1] = thresholding(rawxyz[1], 128);
			xyz[1][2] = thresholding(rawxyz[2], 128);
			delay(100);
			if ((xyz[0][0] ^ xyz[1][0]) || (xyz[0][1] ^ xyz[1][1])) {
				shake_counter += 1;
			}
		}
		if (shake_counter >= 3) {
			if (decision >= 0 && decision < 6)
				replay = true;
			Serial.println("Shaking");
		}
		gesture_state = 0;
		shake_counter = 0;
		break;
	}

	delay(500);
}

int thresholding(double input, double threshold) {
	if (input > threshold) {
		return 1;
	}
	else {
		return 0;
	}
}

void testingMode() {
	ssize_t size = spp.read();

	if ((size != -1) && (size != 0)) {
		adxl.setInterrupt(ADXL345_INT_SINGLE_TAP_BIT, 0);
		adxl.setInterrupt(ADXL345_INT_DOUBLE_TAP_BIT, 0);
		const char* buf = spp.getBuf();
		Serial.println(buf);
		parsePattern(buf, "");
		delay(4000);

		adxl.setInterrupt(ADXL345_INT_SINGLE_TAP_BIT, 1);
		adxl.setInterrupt(ADXL345_INT_DOUBLE_TAP_BIT, 1);
	}

	gestureStateMachineInTestingMode();
}

void gestureStateMachineInTestingMode() {
	switch (gesture_state) {
	case 0:
		interrupts = adxl.getInterruptSource();
		if (adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)) {
			double_tap_counter += 1;
			timestamp = millis();
			gesture_state = 1;
		}
		else if (adxl.triggered(interrupts, ADXL345_SINGLE_TAP)) {
			Serial.println("Single tap");
			gesture_state = 0;
			//const char* str = "1";
			//spp.write(str);
			sprintf(buffer, ":)");
			Serial.println(buffer);
			Serial.println();
			oled.clearScreen();
			oled.drawString(buffer, 0, 0, 2, COLOR_WHITE);
			delay(1000);
			oled.clearScreen();
		}
		break;
	case 1:
		interrupts = adxl.getInterruptSource();
		if (adxl.triggered(interrupts, ADXL345_DOUBLE_TAP) && ((millis() - timestamp) < 2000)) {
			Serial.println("Double tap twice");
			gesture_state = 0;
			double_tap_counter = 0;
			sprintf(buffer, "Standard");
			Serial.println(buffer);
			Serial.println();
			oled.clearScreen();
			oled.drawString(buffer, 0, 0, 2, COLOR_WHITE);
			delay(1000);
			oled.clearScreen();
			refreshAllPatternsFromCloud();
			mode = STANDARD_MODE;
		}
		else if ((millis() - timestamp) >= 2000) {
			Serial.println("Double tap once");
			gesture_state = 0;
			double_tap_counter = 0;
			//const char* str = "0";
			//spp.write(str);
			sprintf(buffer, ":(");
			Serial.println(buffer);
			Serial.println();
			oled.clearScreen();
			oled.drawString(buffer, 0, 0, 2, COLOR_WHITE);
			delay(1000);
			oled.clearScreen();
		}
		break;
	}

	// wait a bit
	delay(500);
}

void refreshAllPatternsFromLocalStore() {
	Serial.print("Initializing SD card...");

	if (!SD.begin(4)) {
		Serial.println("initialization failed!");
		return;
	}
	Serial.println("initialization done.");

	if (SD.exists(offline_file_name)) {
		Serial.print(offline_file_name);
		Serial.println(" exists.");
	}
	else {
		Serial.print(offline_file_name);
		Serial.println(" doesn't exist.");
		refreshAllPatternsFromCloud();
		return;
	}

	// open a new file and immediately close it:
	offlineFile = SD.open(offline_file_name, FILE_READ);
	while (offlineFile.available()) {
		String line = offlineFile.readStringUntil('\n');
		responses.push_back(line);
	}
	offlineFile.close();
}

void refreshAllPatternsFromCloud() {
	connection_failure = false;
	responses.clear();
	if (status != WL_CONNECTED) {
		connectWifi();
	}
	if (status == WL_CONNECTED) {
		// Display "Updating"
		Serial.println("Updating");
		Serial.println();
		oled.clearScreen();
		oled.drawString("Updating", 0, 0, 2, COLOR_WHITE);
		for (int i = 0; i < 6; i++) {
			for (int j = 0; j < 2; j++) {
				refreshPatternFromCloud(server, ams_key, table_names[j], emotion_types[i]);
			}
		}

		if (!connection_failure) {
			refreshLocalStore();
			// Display "All updated"
			Serial.println("All updated!");
			Serial.println();
			oled.clearScreen();
			oled.drawString("All updated", 0, 0, 2, COLOR_WHITE);
			delay(2000);
			oled.clearScreen();
		}
		else {
			// Display "All updated"
			Serial.println("Updating failed");
			Serial.println();
			oled.clearScreen();
			oled.drawString("Updating failed", 0, 0, 2, COLOR_WHITE);
			delay(2000);
			oled.clearScreen();
		}
	}
}

void refreshLocalStore() {
	Serial.print("Initializing SD card...");

	if (!SD.begin(4)) {
		Serial.println("initialization failed!");
		return;
	}
	Serial.println("initialization done.");

	if (SD.exists(offline_file_name)) {
		Serial.print(offline_file_name);
		Serial.println(" exists.");
		SD.remove(offline_file_name);
		Serial.print(offline_file_name);
		Serial.println(" removed.");
	}
	else {
		Serial.print(offline_file_name);
		Serial.println(" doesn't exist.");
	}

	// open a new file and immediately close it:
	Serial.println("Creating example.txt...");
	offlineFile = SD.open(offline_file_name, FILE_WRITE);
	for (int i = 0; i < 12; i++) {
		offlineFile.println(responses.at(i).c_str());
	}
	offlineFile.close();
}

void parseDecision(int decision) {
	Serial.print("Decision received: "); Serial.println(emotion_types[decision]);
	Pattern vibrationPattern = parseResponse(responses.at((decision * 2 + 0)).c_str());
	parsePattern(vibrationPattern.getValue(), vibrationPattern.getEmotion());
	Pattern lightPattern = parseResponse(responses.at((decision * 2 + 1)).c_str());
	parsePattern(lightPattern.getValue(), lightPattern.getEmotion());
}

void parsePattern(const char* patternString, const char* emotion) {
	std::string Str(patternString);
	std::vector<std::string> splitted_strings;
	splitted_strings = splitString(Str, ":");
	if (splitted_strings.size() == 2) {
		int type_no = atoi(splitted_strings[0].c_str());
		//Serial.println(type_no);
		std::vector<int> effects = extractEffects(splitted_strings[1]);
		//Serial.println(effects.size());
		switch (type_no) {
		case 1:
			shortVibrationPulse();
			delay(1000);
			Serial.print("Number of effects: ");  Serial.println(effects.size());
			drv.setMode(DRV2605_MODE_INTTRIG);
			drv.playLibraryEffects(effects);
			break;
		case 2:
			shortVibrationPulse();
			delay(1000);
			drv.setMode(DRV2605_MODE_REALTIME);
			Serial.print("Number of effects: ");  Serial.println(effects.size());
			drv.playRealTimeEffects(effects, 80);
			break;
		case 3:
			oled.clearScreen();
			Serial.print("Number of effects: ");  Serial.println((effects.size() / 4));
			oled.playSequencedColors(effects, emotion);
			break;
		}
	}
}

Pattern parseResponse(const char* response)
{
	sprintf(buffer, response);
	const int BUFFER_SIZE = JSON_OBJECT_SIZE(6) + JSON_ARRAY_SIZE(0);
	StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject(buffer);
	if (!root.success())
	{
		Serial.println("PARSING FAILED!!!");
		//return;
	}

	const char* id = root["id"];
	const char* name = root["name"];
	const char* emotion = root["emotion"];
	const char* type = root["type"];
	const char* value = root["value"];
	int score = root["score"];

	return Pattern(id, name, emotion, type, value, score);
	//parsePattern(value, emotion);
}

void connectWifi() {
	sprintf(buffer, "Connecting Wifi");
	Serial.println();
	oled.clearScreen();
	oled.drawString(buffer, 0, 0, 2, COLOR_WHITE);
	// check for the presence of the shield:
	if (WiFi.status() == WL_NO_SHIELD) {
		Serial.println("WiFi shield not present");
		// don't continue:
		while (true);
	}

	String fv = WiFi.firmwareVersion();
	if (fv != "1.1.0")
		Serial.println("Please upgrade the firmware");

	timestamp = millis();
	// attempt to connect to Wifi network:
	while ((status != WL_CONNECTED) && ((millis() - timestamp) < 30000)) {
		sprintf(buffer, "Attempting to conncect to SSID: %s", ssid);
		Serial.println(buffer);
		oled.drawString(buffer, 0, 32, 1, COLOR_WHITE);
		// Connect to WPA/WPA2 network. Change this line if using open or WEP network:
		status = WiFi.begin(ssid, pass);
	}
	if (status == WL_CONNECTED) {
		sprintf(buffer, "Wifi Connected");
		Serial.println(buffer);
		oled.clearScreen();
		oled.drawString(buffer, 0, 0, 2, COLOR_WHITE);
		delay(1000);
		oled.clearScreen();
	}
	else {
		sprintf(buffer, "Wifi Not Connected");
		Serial.println(buffer);
		oled.clearScreen();
		oled.drawString(buffer, 0, 0, 2, COLOR_WHITE);
		delay(1000);
		oled.clearScreen();
		return;
	}
}

void updatePatternScore(const char* emotion, bool positive_feedback)
{
	connection_failure = false;
	if (status != WL_CONNECTED) {
		connectWifi();
	}
	if (status == WL_CONNECTED) {
		for (int i = 0; i < 2; i++) {
			sendQueryRequest(server, ams_key, table_names[i], emotion);
			waitResponse();
			String response = readQueryResponse();
			endRequest();
			Pattern patternToBeRated = parseResponse(response.c_str());
			int newScore;
			if (positive_feedback) {
				newScore = patternToBeRated.getScore() + 1;
			}
			else {
				newScore = patternToBeRated.getScore() + 1;
			}
			sendUpdateRequest(server, ams_key, table_names[i], patternToBeRated.getId(), newScore);
			waitResponse();
			readUpdateResponse();
			endRequest();
		}
	}
}

void refreshPatternFromCloud(const char* server, const char* ams_key, const char* table_name, const char* emotion)
{
	sendQueryRequest(server, ams_key, table_name, emotion);
	waitResponse();
	responses.push_back(readQueryResponse());
	endRequest();
}

void sendQueryRequest(const char* server, const char* ams_key, const char* table_name, const char* emotion)
{
	Serial.println("\nconnecting...");

	if (client.connect(server, 80))
	{
		///*
		sprintf(buffer, "GET /tables/%s?$filter=startswith(emotion,'%s')&$orderby=score+desc&$top=1 HTTP/1.1", table_name, emotion);

		client.println(buffer);
		Serial.println(buffer);

		// Host header
		sprintf(buffer, "Host: %s", server);
		client.println(buffer);
		Serial.println(buffer);

		// Azure Mobile Services application key
		sprintf(buffer, "X-ZUMO-APPLICATION: %s", ams_key);
		client.println(buffer);
		Serial.println(buffer);

		// JSON content type
		client.println("Content-Type: application/json");
		Serial.println("Content-Type: application/json");

		// Perpare body content
		sprintf(buffer, "", "");

		// Content length
		client.print("Content-Length: ");
		client.println(strlen(buffer));
		Serial.print("Content-Length: ");
		Serial.println(strlen(buffer));

		// End of headers
		client.println();
		Serial.println();

		// Request body
		client.println(buffer);
		Serial.println(buffer);
		//*/
	}
	else
	{
		Serial.println("connection failed");
		connection_failure = true;
	}
}

void waitResponse()
{
	while (!client.available())
	{
		if (!client.connected())
		{
			connection_failure = true;
			return;
		}
	}
}

void sendUpdateRequest(const char* server, const char* ams_key, const char* table_name, const char* Id, int newScore)
{
	String strId(Id);
	Serial.println("\nconnecting...");

	if (client.connect(server, 80))
	{
		// PATCH URI
		sprintf(buffer, "PATCH /tables/%s/%s HTTP/1.1", table_name, strId.c_str());
		client.println(buffer);
		Serial.println(buffer);

		// Host header
		sprintf(buffer, "Host: %s", server);
		client.println(buffer);
		Serial.println(buffer);

		// Azure Mobile Services application key
		sprintf(buffer, "X-ZUMO-APPLICATION: %s", ams_key);
		client.println(buffer);
		Serial.println(buffer);

		// JSON content type
		client.println("Content-Type: application/json");
		Serial.println("Content-Type: application/json");

		// Prepare body content
		sprintf(buffer, "{\"score\": %d}", newScore);

		// Content length
		client.print("Content-Length: ");
		client.println(strlen(buffer));
		Serial.print("Content-Length: ");
		Serial.println(strlen(buffer));

		// End of headers
		client.println();
		Serial.println();

		// Request body
		client.println(buffer);
		Serial.println(buffer);
	}
	else
	{
		Serial.println("connection failed");
		connection_failure = true;
	}
}

String readQueryResponse()
{
	boolean bodyStarted;
	int jsonStringLength;
	int jsonBufferCntr = 0;
	int numline = RESPONSE_JSON_DATA_LINENNO;
	//Ignore the response except for the 10th line
	while (client.available())
	{
		//Serial.println("Reading:");
		char c = client.read();
		if (c == '\n')
		{
			numline -= 1;
		}
		else
		{
			if (numline == 0 && (c != '[') && (c != ']'))
			{
				buffer[jsonBufferCntr++] = c;

				buffer[jsonBufferCntr] = '\0';
			}
		}
	}

	Serial.println("Received:");
	Serial.println(buffer);
	Serial.println("");

	String response(buffer);
	return response;
}

void readUpdateResponse()
{
	boolean bodyStarted;
	int jsonStringLength;
	int jsonBufferCntr = 0;
	int numline = RESPONSE_JSON_DATA_LINENNO;
	//Ignore the response except for the 10th line
	while (client.available())
	{
		//Serial.println("Reading:");
		char c = client.read();
		if (c == '\n')
		{
			numline -= 1;
		}
		else
		{
			if (numline == 0 && (c != '[') && (c != ']'))
			{
				buffer[jsonBufferCntr++] = c;

				buffer[jsonBufferCntr] = '\0';
			}
		}
	}

	Serial.println("Received:");
	Serial.println(buffer);
	Serial.println("");
}

void endRequest()
{
	client.stop();
}

void shortVibrationPulse() {
	drv.setMode(DRV2605_MODE_INTTRIG);
	drv.setWaveform(1, 1);  // strong click 100%, see datasheet part 11.2
	drv.setWaveform(1, 0);  // end of waveform
	drv.go();
	delay(500);
	drv.setWaveform(1, 1);  // strong click 100%, see datasheet part 11.2
	drv.setWaveform(1, 0);  // end of waveform
	drv.go();
}