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

#include "RTColorServer.h"

#define UDP_READ_BUFFER_SIZE 20

RTColorServer::RTColorServer(uint16_t port) : UDPServer(port) {
	isColorGrabbed = true;
}

bool RTColorServer::handleNewColorPacket(PixelColor &newColor, byte &colorUsage) {
	// Check for New Color Packet from UDP port.
	// If none, return false.
	// If exists, set pixelColor & colorUsage, & return true.
	
	if (available()) {
		// Read data
		char buffer[UDP_READ_BUFFER_SIZE];
		int n = readData(buffer, UDP_READ_BUFFER_SIZE);  // n contains # of bytes of buffer
		//Serial.print("n: "); Serial.println(n);
	
		if (n % 4 == 0) {  // ONLY expecting 4-byte packets
			byte r=0,g=0,b=0;

			PixelColor currColor;
			for (int i = 0; i < n; ++i) {
				uint8_t v = buffer[i];
				//cout << "v: " << (int)v << endl;
				if (i % 4 == 0) {
					colorUsage = v;
				} else if (i % 4 == 1) {
					r = v;
				} else if (i % 4 == 2) {
					g = v;
				} else {
					b = v;
				}
			}
		
			currColor.R = r;
			currColor.G = g;
			currColor.B = b;

			_lastColor = currColor;
			isColorGrabbed = false;
		}
	}
	
	if (!isColorGrabbed) {
		newColor = _lastColor;
		isColorGrabbed = true;
		//Serial.print("COLOR ");Serial.print(newColor.R);Serial.print(",");Serial.print(newColor.G);Serial.print(",");Serial.println(newColor.B);
		return true;
	}
	
	//if (isOutside) { Serial.print("OUT "); } else { Serial.print("IN "); }
	//Serial.print(r);Serial.print(",");Serial.print(g);Serial.print(",");Serial.print(b);Serial.print("  n:");Serial.println(n);
	//Serial.println("no new color packet.");
	
	return false;
}