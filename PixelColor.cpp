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

#include "PixelColor.h"

PixelColor::PixelColor() {
	R = 0;
	G = 0;
	B = 0;
}

PixelColor::PixelColor(byte r, byte g, byte b) {
	R = r;
	G = g;
	B = b;
}

PixelColor::PixelColor(unsigned long c) {
	byte controlByte = (c >> 24) & 0xFF;
	
	if (controlByte == 0x01) {
		// Generate Random Color
		randomSeed(analogRead(0));
		c = random(16777216);  // overwrite c
	}
	
	R = (c >> 16) & 0xFF;
	G = (c >> 8) & 0xFF;
	B = c & 0xFF;
}

unsigned long PixelColor::getLongVal() {
	return ((unsigned long)R << 16) | ((unsigned long)G <<  8) | B;
}

PixelColor PixelColor::addThisColor(PixelColor pc) {
	// Add 'c' to the existing color AND return the result (in case user wants it).
	R = (R + pc.R > 0xFF) ? 0xFF : R + pc.R;
	G = (G + pc.G > 0xFF) ? 0xFF : G + pc.G;
	B = (B + pc.B > 0xFF) ? 0xFF : B + pc.B;
	
	return getLongVal();
}

void PixelColor::println() {
	Serial.print(R, DEC); Serial.print(", ");
	Serial.print(G, DEC); Serial.print(", ");
	Serial.println(B, DEC);
}
