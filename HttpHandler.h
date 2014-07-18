/*
	Copyright 2014 Mark Briggs

	This file is part of ColorRing.

	ColorRing is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any later version.

	ColorRing is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	There is a copy of the GNU General Public License in the
	accompanying COPYING file
*/

/* 
	This started its life in the form of Marco Schwartz's aREST.h library (https://github.com/marcoschwartz/aREST).
	However I have stripped out everything I don't need & changed a few of things.
	But I want to give credit to Marco because his code gave me a great working solution to start with - thank you Marco!
*/

#include "Arduino.h"

#define NUMBER_VARIABLES 9
#define NUMBER_FUNCTIONS 3

class HttpHandler {

private:
	String answer;
	String command;
	int idx;
	boolean state_selected;
	boolean command_sent;
	String name;
	String arguments;
	char charArr[300];  // should be long enough for anything

	// Variables arrays
	uint8_t variables_index;
	int * int_variables[NUMBER_VARIABLES];
	String int_variables_names[NUMBER_VARIABLES];

	// Functions array
	uint8_t functions_index;
	int (*functions[NUMBER_FUNCTIONS])(String);
	String functions_names[NUMBER_FUNCTIONS];

public:
	HttpHandler() {  // Constructor
		command = "";
		state_selected = false;
		command_sent = false;
		variables_index = 0;
		functions_index = 0;
	}

	String get_http_headers() {
		return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n";
	}

	String get_end_answer() {
		return "\"name\": \"" + name + "\", \"connected\": true}\r\n";
	}

	// Reset variables after a request
	void reset_status() {
		answer = "";
		command = "";
		state_selected = false;
		command_sent = false;
		arguments = "";
	}

	void handle(Adafruit_CC3000_ClientRef client) {
		if (client.available()) {
			// Handle request
			handle_proto(client, true);

			// Give the web browser time to receive the data
			delay(5);
			client.close();  

			// Reset variables for the next command
			reset_status();
		} 
	}

	void handle_proto(Adafruit_CC3000_ClientRef serial, bool headers) {
		Serial.println("HttpHandler::handle_proto()");

		// Check if there is data available to read
		while (serial.available()) {

			// Get the server answer
			char c = serial.read();
			delay(1);
			answer = answer + c;
			//Serial.print(c);

			// Check if we are receveing useful data and process it
			if ((c == '/' || c == '\r') && state_selected == false) {

				//Serial.println(answer);
				Serial.print(F("answer: ")); Serial.println(answer); 

				// Check if variable name is in array
				for (int i = 0; i < variables_index; i++) {
					if(answer.startsWith(int_variables_names[i])) {
						// Serial.println(F("Variable found")); 

						state_selected = true;
						command = "variable";
						idx = i;
					}
				}

				// Check if function name is in array
				for (int i = 0; i < functions_index; i++) {
					if(answer.startsWith(functions_names[i])) {
						//Serial.println(F("Function found"));

						state_selected = true;
						command = "function";
						idx = i;

						// Get command
						int header_length = functions_names[i].length() + 8;
						arguments = answer.substring(header_length);
					}
				}

				answer = "";
			}

			// Send commands
			if (state_selected && !command_sent) {
				//Serial.println("Sending command: " + command + String(pin) + state);

				// Start of message
				String fullPacket = get_http_headers();

				// Variable selected
				if (command == "variable") {
					//Serial.println("variable");
					fullPacket += "{\"" + int_variables_names[idx] + "\": " + String(*int_variables[idx]) + ", ";
					Serial.print(F("Variable: ")); Serial.println(String(*int_variables[idx]));
				}

				// Function selected
				if (command == "function") {
					//Serial.println("function");

					// Execute function
					int result = functions[idx](arguments);

					fullPacket += "{\"return_value\": " + String(result) + ", \"message\": \"FnExecd\", ";
				}

				// End of message
				fullPacket += get_end_answer();

				if (fullPacket.length()+1 > 300) {
					Serial.print(F("*!fullPacket.length()+1: "));
					Serial.print(String(fullPacket.length()+1));
					Serial.println(F(". (CAN'T be >300) Better make the following string smaller:")); 
					// if >300, increase charArr (and probably TXBUFFERSIZE / RXBUFFERSIZE), (and probably CC3000_MINIMAL_TX_SIZE / CC3000_MINIMAL_RX_SIZE), but watch for RAM consumption

					Serial.println(fullPacket);
				}

				Serial.print(F("Free RAM HttpHandler: ")); Serial.println(getFreeRam(), DEC);

				fullPacket.toCharArray(charArr, fullPacket.length()+1);
				serial.fastrprint(charArr);

				// End here
				command_sent = true;
			}
		}
	}

	void variable(String variable_name, int *variable) {
		int_variables[variables_index] = variable;
		int_variables_names[variables_index] = variable_name;
		variables_index++;
	}

	void function(String function_name, int (*f)(String)) {
		functions_names[functions_index] = function_name;
		functions[functions_index] = f;
		functions_index++;
	}

	void set_name(String device_name) {
		name = device_name;
	}

};