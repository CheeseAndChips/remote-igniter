#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "web_include.h"

#define DEBUG
#ifdef DEBUG
#define DEBUG_NO_AP

#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

const char *ssid = "degam";
const char *password = "noriuAsDegt44";

#define SWITCH_CONNECTION_GPIO D5
#define RELAY_CONNECTION_GPIO D6
#define HEATER_CONNECTION_GPIO D7
#define RELAY_OUTPUT_GPIO D2

//const int8_t allPins[] = {5, 4, 14, 12, 13};
//const int8_t allPins[] = {D1, D2, D5, D6, D7};

enum state_t {
	IDLE,
	WAITING,
	FIRING
} state;

char pingBuf[64];
unsigned long firingStart = 0;
unsigned long firingUntil = 0;

ESP8266WebServer server(80);

void handlePing();
void handleFire();

bool verifyString(const String& s){
	for(size_t i = 0; i < s.length(); i++){
		if(!('0' <= s[i] && s[i] <= '9')) return false;
	}
	return true;
}

void setup() {
	state = state_t::IDLE;

	pinMode(SWITCH_CONNECTION_GPIO, INPUT_PULLUP);
	pinMode(RELAY_CONNECTION_GPIO, INPUT_PULLUP);
	pinMode(HEATER_CONNECTION_GPIO, INPUT_PULLUP);
	pinMode(RELAY_OUTPUT_GPIO, OUTPUT);
	digitalWrite(RELAY_OUTPUT_GPIO, LOW);

	delay(1000);
	Serial.begin(115200);

	#ifndef DEBUG_NO_AP // debugging mode not enabled: set up AP
	debug("Configuring access point...");
	WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
	WiFi.mode(WIFI_AP);
	WiFi.softAP(ssid, password);

	IPAddress myIP = WiFi.softAPIP();
	debug("AP IP address: ");
	debugln(myIP);
	#else // connect to router
	debugln("Connecting to Wi-Fi...");
	WiFi.mode(WIFI_STA);
	WiFi.begin("NANET", "Ara8a1Vald0");
	debugln("");

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		debug(".");
	}
	debugln("");
	debug("Connected IP address: ");
	debugln(WiFi.localIP());
	#endif

	server.on("/", []() {
		debugln("Sending root");
		server.send(200, "text/html", web);
	});

	server.on("/ping", handlePing);
	server.on("/fire", handleFire);

	server.on("/cancel", []() {
		state = state_t::IDLE;
		digitalWrite(RELAY_OUTPUT_GPIO, LOW);
		debugln("Cancelled");
		server.send(200, "text/html", "ok");
	});
	server.begin();
	debugln("HTTP server started");
}

void loop() {
	server.handleClient();

	switch(state){
		case state_t::IDLE:
			break;
		case state_t::WAITING:
			if(millis() >= firingStart){
				state = state_t::FIRING;
				digitalWrite(RELAY_OUTPUT_GPIO, HIGH);
				debug("Started firing ");
				debugln(millis());
			}
			break;
		case state_t::FIRING:
			if(millis() >= firingUntil){
				state = state_t::IDLE;
				digitalWrite(RELAY_OUTPUT_GPIO, LOW);
				debug("Stopped firing ");
				debugln(millis());
			}
			break;
	}
}

void handlePing() {
	pingBuf[0] = (!digitalRead(SWITCH_CONNECTION_GPIO)) ? '1' : '0';
	pingBuf[1] = (!digitalRead(RELAY_CONNECTION_GPIO)) ? '1' : '0';
	pingBuf[2] = (!digitalRead(HEATER_CONNECTION_GPIO)) ? '1' : '0';
	pingBuf[3] = 0;

	if(state != state_t::IDLE) {
		debugln("state is not idle");
		String s_timeLeft = String((int)firingStart - (int)millis());
		pingBuf[3] = ';';
		const int offset = 4;
		strncpy(pingBuf + offset, s_timeLeft.c_str(), sizeof(pingBuf) - offset - 4);
		debugln(pingBuf);
	}

	server.send(200, "text/html", pingBuf);
}

void handleFire() {
	if(!server.hasArg("delay") || !server.hasArg("length")){
		server.send(400, "text/html", "Nera delay arba length");
	}

	String delayStr = server.arg("delay");
	String lengthStr = server.arg("length");

	if(!verifyString(delayStr) || !verifyString(lengthStr)){
		server.send(400, "text/html", "Delay arba length nera skaiciai");
	}

	int delay = delayStr.toInt();
	int length = lengthStr.toInt();

	if(state == state_t::IDLE){
		state = state_t::WAITING;

		firingStart = millis() + delay * 1000;
		firingUntil = firingStart + length * 1000;
		if(delay == 0){
			state = state_t::FIRING;
			digitalWrite(RELAY_OUTPUT_GPIO, HIGH);
			debugln("Started firing instantly");
		}
		debugln(firingStart);
		debugln(firingUntil);
		server.send(200, "text/html", "ok");
	} else { server.send(400, "text/html", "State nera idle"); }
}