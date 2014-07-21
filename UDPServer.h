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

#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <Arduino.h>

#include "ColorRing_CC3000.h"
#include "utility/socket.h"

class UDPServer {

private:
	uint16_t _port;
	int _socket;
	
public:
	UDPServer(uint16_t port);
	
	bool begin();
	bool available();
	int readData(char *buffer, int bufferSize);
};

#endif  // UDPSERVER_H