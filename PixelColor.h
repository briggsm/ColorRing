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

#ifndef PIXELCOLOR_H
#define PIXELCOLOR_H

#include <Arduino.h>

class PixelColor {
private:
		
public:
	// Data - keep public to make it easier to access
	byte R;
	byte G;
	byte B;
	
	// Methods
	PixelColor();
	PixelColor(byte r, byte g, byte b);
	PixelColor(unsigned long c);
	
	unsigned long getLongVal();
	PixelColor addThisColor(PixelColor pc);
	void println();
};

#endif // PIXELCOLOR_H
