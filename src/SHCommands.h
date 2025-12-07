#define MESSAGE_HEADER 0x03
// Debug UART defined in main.cpp
extern Stream* DebugPort;


void Command_Hello()
{
	if (DebugPort) {
		DebugPort->println("[DBG] Command_Hello called!");
		DebugPort->flush();
	}
	FlowSerialTimedRead();  // Read trailer byte
	delay(10);
	FlowSerialPrint(VERSION);  // Send only VERSION character
	FlowSerialFlush();
	if (DebugPort) {
		DebugPort->println("[DBG] Command_Hello response sent!");
		DebugPort->flush();
	}
}

void Command_SetBaudrate()
{
	SetBaudrate();
}

void Command_ButtonsCount()
{
	FlowSerialWrite((byte)(0));
	FlowSerialFlush();
}

void Command_TM1638Count()
{
	FlowSerialWrite((byte)(0));
	FlowSerialFlush();
}

void Command_SimpleModulesCount()
{
	FlowSerialWrite((byte)(0));
	FlowSerialFlush();
}

void Command_Acq()
{
	FlowSerialWrite(0x03);
	FlowSerialFlush();
}

void Command_DeviceName()
{
	FlowSerialPrint(DEVICE_NAME);
	FlowSerialPrint("\n");
	FlowSerialFlush();
}

void Command_UniqueId()
{
	auto id = getUniqueId();
	FlowSerialPrint(id);
	FlowSerialPrint("\n");
	FlowSerialFlush();
}

void Command_MCUType()
{
	FlowSerialPrint(SIGNATURE_0);
	FlowSerialPrint(SIGNATURE_1);
	FlowSerialPrint(SIGNATURE_2);
	FlowSerialFlush();
}


void Command_ExpandedCommandsList()
{
	FlowSerialPrintLn("mcutype");
	FlowSerialPrintLn("keepalive");
	FlowSerialPrintLn();
	FlowSerialFlush();
}

void Command_Features()
{
	delay(10);

	// Name
	FlowSerialPrint("N");

	// UniqueID
	FlowSerialPrint("I");

	// Custom Protocol Support
	FlowSerialPrint("P");

	// Xpanded support
	FlowSerialPrint("X");

	FlowSerialPrint("\n");
	FlowSerialFlush();
}

void Command_RGBLEDSCount()
{
#ifdef INCLUDE_RGB_LEDS_NEOPIXELBUS
	FlowSerialWrite((byte)(neoPixelBusCount()));
#else
	FlowSerialWrite((byte)(0));
#endif
	FlowSerialFlush();
}

void Command_RGBLEDSData()
{
#ifdef INCLUDE_RGB_LEDS_NEOPIXELBUS
	neoPixelBusRead();
	neoPixelBusShow();
#endif
	// Acq !
	FlowSerialWrite(0x15);
}

void Command_RGBMatrixData()
{
	// Acq !
	FlowSerialWrite(0x15);
}

void Command_GearData()
{
	char gear = FlowSerialTimedRead();
}

void Command_CustomProtocolData()
{
	shCustomProtocol.read();
	FlowSerialWrite(0x15);
}
