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

#ifndef ECMSERVER_H
#define ECMSERVER_H

#include <Arduino.h>
#include "UDPServer.h"
#include "PixelColor.h"

#include <StandardCplusplus.h>
#include <serstream>
using namespace std;

class EcmServer : public UDPServer{

private:
	PixelColor _lastOutColor;
	PixelColor _lastInColor;
	bool isOutColorGrabbed;
	bool isInColorGrabbed;
	
public:
	EcmServer(uint16_t port); // : UDPServer(port) { }
	bool handleNewColorPacket(PixelColor &newColor, bool isOutside);
};
#endif  // ECMSERVER_H