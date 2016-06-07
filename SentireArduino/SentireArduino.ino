/*
 Name:		SentireArduino.ino
 Created:	4/9/2016 2:03:30 AM
 Author:	Qisong Wang
 Modified:  7/6/2016
 */

// Libraries used
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

// Symbolic mode names
#define STANDARD_MODE	1
#define TESTING_MODE	2

// Maxmum length of JSON response
#define RESPONSE_JSON_DATA_LINENNO 10

// OLED Pin Name    // Pin Name on Xadow-Accelerometer
#define cs     10   // MISO
#define dc     12   // A5
#define mosi   11   // MOSI
#define sclk   13   // SCK

// Declaration of device objects
SSD1331 oled = SSD1331(cs, dc); // Colour OLED display using dedicated hardware SPI
Adafruit_DRV2605 drv;			// Haptic motor drivr
Intel_Edison_BT_SPP spp;		// Bluetooth SPP library
ADXL345 adxl;					// Accelerometer
WiFiClient client;				// Wifi client
File offlineFile;				// Offline file pointer

// Wifi connection details
String ssid;
String pass;
int keyIndex = 0;            // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;

// Microsoft Azure cloud database related details
//const char* server = "songerarduinotest.azure-mobile.net";
//const char* ams_key = "IBmOdZkslBSsjrCkJeQNvpjHOpTQYr42";
const char* server = "sentire.azure-mobile.net";
const char* ams_key = "mYwbeuzHpCbzQXEuRDMJKrGrronplS68";
const char* table_names[2] = { "vibrationpattern", "lightpattern" };
const char* emotion_types[6] = { "happy", "fearful", "surprised", "sad", "disgusted", "angry" };

// Offline file name
char* offline_file_name = "offline_patterns.txt";
char* wifi_file_name = "wifi.txt";

// State, flag and counter variables
int mode = STANDARD_MODE;
int gesture_state = 0;
bool connection_failure = false;
bool replay = false;
int double_tap_counter = 0;
int shake_counter = 0;

// Runtime data holder
std::vector<String> responses; // contains full pattern information in runtime

// Temporary data variables
int decision = -1; // decision signal received last time, 0-happy, 1-fearful, 2-surprised, 3-sad, 4-disgusted, 5-angry
char buffer[2048];
unsigned long timestamp;
byte interrupts;
double rawxyz[3];
int xyz[2][3];

// Setup at the begining of the program
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

	// Load local patterns to runtime data holder
	refreshAllPatternsFromLocalStore();
	initialiseWifiCredential();
	Serial.println(ssid.c_str());
	Serial.println(pass.c_str());
	// Finish setup
	Serial.println("Setup Done!");
	shortVibrationPulse();
	delay(2000);
	oled.clearScreen();
}

// the loop function runs over and over again until power down or reset
void loop() {
	stateMachine();
}

// Main statemachine contains two modes
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

// Standard working mode
void standardMode() {
	// Read bluetooth spp
	ssize_t size = spp.read();

	// If there is a valid message, i.e., a single number 0-5,
	// display the corresponding vibration and light patterns
	// Or if replay request is detected, replay the last pair of patterns
	if (((size != -1) && (size != 0)) || replay) {
		// Disable tapping detection during vibraton
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

		// Enbale tapping detection agian
		adxl.setInterrupt(ADXL345_INT_SINGLE_TAP_BIT, 1);
		adxl.setInterrupt(ADXL345_INT_DOUBLE_TAP_BIT, 1);
	}

	gestureStateMachineInStandardMode();
}

// State machine used to detect several actions
// One single tap: Update the most recent patterns' scroe with positive feedback
// One double tap: Update the most recent patterns' scroe with negative feedback
// Two double taps: Switch mode to Testing mode
// High speed shaking: Replay the most recent patterns
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
			adxl.setInterrupt(ADXL345_INT_SINGLE_TAP_BIT, 0);
			adxl.setInterrupt(ADXL345_INT_DOUBLE_TAP_BIT, 0);
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
			adxl.setInterrupt(ADXL345_INT_SINGLE_TAP_BIT, 0);
			adxl.setInterrupt(ADXL345_INT_DOUBLE_TAP_BIT, 0);
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
		adxl.setInterrupt(ADXL345_INT_SINGLE_TAP_BIT, 1);
		adxl.setInterrupt(ADXL345_INT_DOUBLE_TAP_BIT, 1);
		interrupts = adxl.getInterruptSource();
		if (adxl.triggered(interrupts, ADXL345_DOUBLE_TAP) && ((millis() - timestamp) < 2000)) {
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
		else if (!(adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)) && ((millis() - timestamp) >= 2000)) {
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
		adxl.setInterrupt(ADXL345_INT_SINGLE_TAP_BIT, 0);
		adxl.setInterrupt(ADXL345_INT_DOUBLE_TAP_BIT, 0);
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
		adxl.setInterrupt(ADXL345_INT_SINGLE_TAP_BIT, 1);
		adxl.setInterrupt(ADXL345_INT_DOUBLE_TAP_BIT, 1);
		break;
	}

	delay(500);
}

// A helpler function for simplifying accelerometer data further to 0 or 1
int thresholding(double input, double threshold) {
	if (input > threshold) {
		return 1;
	}
	else {
		return 0;
	}
}

// Testing mode
void testingMode() {
	// Read Bluetooth SPP
	ssize_t size = spp.read();

	// If there is a valid pattern value string, parse and play it
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

// State machine used to detect several actions
// One single tap: Temporarily no corresponding action
// One double tap: Temporarily no corresponding action
// Two double taps: Switch back to Standard Mode and try to update patterns
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

// Load local wifi credential to runtime
void initialiseWifiCredential() {
	Serial.print("Initializing SD card...");

	if (!SD.begin(4)) {
		Serial.println("initialization failed!");
		return;
	}
	Serial.println("initialization done.");

	if (SD.exists(wifi_file_name)) {
		Serial.print(wifi_file_name);
		Serial.println(" exists.");
	}
	else {
		Serial.print(wifi_file_name);
		Serial.println(" doesn't exist.");
		return;
	}

	// open a new file and immediately close it:
	offlineFile = SD.open(wifi_file_name, FILE_READ);
	ssid = offlineFile.readStringUntil(',');
	pass = offlineFile.readStringUntil('\n');
	offlineFile.close();
}

// Update the local wifi credential by overwritting it
void refreshLocalWifiCredential() {
	Serial.print("Initializing SD card...");

	if (!SD.begin(4)) {
		Serial.println("initialization failed!");
		return;
	}
	Serial.println("initialization done.");

	if (SD.exists(wifi_file_name)) {
		Serial.print(wifi_file_name);
		Serial.println(" exists.");
		SD.remove(wifi_file_name);
		Serial.print(wifi_file_name);
		Serial.println(" removed.");
	}
	else {
		Serial.print(wifi_file_name);
		Serial.println(" doesn't exist.");
	}

	// open a new file and immediately close it:
	Serial.print("Creating ");
	Serial.print(wifi_file_name);
	Serial.println("...");
	offlineFile = SD.open(wifi_file_name, FILE_WRITE);
	offlineFile.print(ssid);
	offlineFile.print(',');
	offlineFile.print(pass);
	offlineFile.close();
}

// Load local pattern data to runtime
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

// Update local and runtime pattern data from Cloud
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

// Try to connect wifi
void connectWifi() {
	sprintf(buffer, "Connecting Wifi");
	Serial.println();
	oled.clearScreen();
	oled.drawString(buffer, 0, 0, 2, COLOR_WHITE);
	// Check for the presence of the shield:
	if (WiFi.status() == WL_NO_SHIELD) {
		Serial.println("WiFi shield not present");
		// don't continue:
		while (true);
	}

	// Check firmaware version
	String fv = WiFi.firmwareVersion();
	if (fv != "1.1.0")
		Serial.println("Please upgrade the firmware");

	timestamp = millis();
	// Attempt to connect to Wifi network:
	while ((status != WL_CONNECTED) && ((millis() - timestamp) < 40000)) {
		sprintf(buffer, "Attempting to conncect to SSID: %s", ssid.c_str());
		Serial.println(buffer);
		oled.drawString(buffer, 0, 32, 1, COLOR_WHITE);
		// Connect to WPA/WPA2 network. Change this line if using open or WEP network:
		status = WiFi.begin(&ssid[0u], &pass[0u]);
	}

	// Confirm connected
	if (status == WL_CONNECTED) {
		sprintf(buffer, "Wifi Connected");
		Serial.println(buffer);
		oled.clearScreen();
		oled.drawString(buffer, 0, 0, 2, COLOR_WHITE);
		delay(1000);
		oled.clearScreen();
	}
	else {
		connection_failure = true;
		sprintf(buffer, "Wifi Not Connected");
		Serial.println(buffer);
		oled.clearScreen();
		oled.drawString(buffer, 0, 0, 2, COLOR_WHITE);
		delay(1000);
		oled.clearScreen();
		return;
	}
}

// Update the local pattern data by overwritting it
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
	Serial.print("Creating ");
	Serial.print(offline_file_name);
	Serial.println("...");
	offlineFile = SD.open(offline_file_name, FILE_WRITE);
	for (int i = 0; i < 12; i++) {
		offlineFile.println(responses.at(i).c_str());
	}
	offlineFile.close();
}

// Play vibration and light patterns based on the decision received
void parseDecision(int decision) {
	Serial.print("Decision received: "); Serial.println(emotion_types[decision]);
	Pattern vibrationPattern = parseResponse(responses.at((decision * 2 + 0)).c_str());
	parsePattern(vibrationPattern.getValue(), vibrationPattern.getEmotion());
	Pattern lightPattern = parseResponse(responses.at((decision * 2 + 1)).c_str());
	parsePattern(lightPattern.getValue(), lightPattern.getEmotion());
}

// Determine which type of pattern is going to be played,
// and use relevant helpler function
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
		case 4:
			std::vector<std::string> wifi_confidential;
			wifi_confidential = splitString(splitted_strings[1], ",");
			const char* ssid_c = wifi_confidential[0].c_str();
			const char* pass_c = wifi_confidential[1].c_str();
			ssid = ssid_c;
			pass = pass_c;
			ssid.trim();
			pass.trim();
			refreshLocalWifiCredential();
			break;
		}
	}
}

// Parse the response in JSON format and convert it to a Pattern object
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
}

// Update a pattern's score by first downloading the newest version of it.
// then add or decrease its latest score by sending update request
void updatePatternScore(const char* emotion, bool positive_feedback)
{
	connection_failure = false;
	if (status != WL_CONNECTED) {
		connectWifi();
	}
	if (status == WL_CONNECTED) {
		// Loop to update both vibration and light patterns' scores
		for (int i = 0; i < 2; i++) {
			// Download the latest information of the target pattern
			sendQueryRequest(server, ams_key, table_names[i], emotion);
			waitResponse();
			String response = readQueryResponse();
			endRequest();
			Pattern patternToBeRated = parseResponse(response.c_str());

			// Process its score according to the feedback
			int newScore;
			if (positive_feedback) {
				newScore = patternToBeRated.getScore() + 1;
			}
			else {
				newScore = patternToBeRated.getScore() - 1;
			}

			// Update the pattern in the cloud
			sendUpdateRequest(server, ams_key, table_names[i], patternToBeRated.getId(), newScore);
			waitResponse();
			readUpdateResponse();
			endRequest();
		}
	}
}

// Download a single pattern from Cloud
void refreshPatternFromCloud(const char* server, const char* ams_key, const char* table_name, const char* emotion)
{
	sendQueryRequest(server, ams_key, table_name, emotion);
	waitResponse();
	responses.push_back(readQueryResponse());
	endRequest();
}

// Send query request to Cloud
void sendQueryRequest(const char* server, const char* ams_key, const char* table_name, const char* emotion)
{
	Serial.println("\nconnecting...");

	if (client.connect(server, 80))
	{
		// Start query request
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

// Send a request to update one pattern with a new score
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

// Wait for a response from Cloud
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

// Recive the query response and add save it in runtime
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

// Receive the update response, no other actions.
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

// End request
void endRequest()
{
	client.stop();
}

// A short vibraion pulse prior to the patterns to alert the user
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