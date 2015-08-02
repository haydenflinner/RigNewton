
//KIRK NOTES

// Welcome to RigNewton.com
//FTDI LIBMPSSE read/write option bits
//  The default read option for I2C_DeviceRead is 0x03 which is binary 0000.0011 meaning that none of the following options apply as bit 6 is conditional:
// Bit 0 is 0 -  If set then a start condition is generated in the I2C bus before the transfer begins
// Bit 1 is 0 -  If set then a stop condition is generated in the I2C bus after the transfer ends
// Bit 2 is 0 -  Reserved (only used in I2C_DeviceWrite)  On writes,  if set then the function will return when a device nAcks after a byte has been transferred.If not setthen the function will continue transferring the stream of bytes even if the device nAcks.
// Bit 3 is 0 -  Some I2C slaves require the I2C master togenerate a NAK for the last data byte read. Setting this bit enables working with such I2C slaves
// Bit 4 is 0 -  Setting this bit will invoke a multi byte I2C transfer without having delays between the START,ADDRESS, DATA and STOP phases.Size of thetransfer in parameters sizeToTransfer and sizeTransferred are in bytes.
// Bit 5 is 0 -  Setting this bit would invoke a multi bit transfer without having delays between the START, ADDRESS, DATA and STOP phases.Size of the transfer in parameters sizeToTransfer and sizeTransferred are in bytes.
// Bit 6 is 1 -  The deviceAddress parameter is ignored if this bit is set.Setting this bit is effective only when either I2C_TRANSFER_OPTIONS_FAST_TRANSFER_BYTES or I2C_TRANSFER_OPTIONS_FAST_TRANSFER_BITS is set.
// Bit 7 is 1 -   Reserved
// 0x0F which is 0000.1111 
// 0x03 which is 0000.0011 (default)


//++++  Precision Microdevices C10-100 Specifications
//Rated Operating Voltage : 2 V
//Rated Resonant Frequency : 175 Hz[+/ -5]
// Typical Rated Operating Current: 69 mA



// AN_190_C232HM_libMPSSE_I2C.cpp : Defines the entry point for the console application.
//
/*
========================================================================
Copy the following files from the libMPSSE folder to your project folder
- ftd2xx.h		// This is the base FTDI USB header
- libMPSSE.h	// This is the header to define the libMPSSE calls
- libMPSSE.dll	// This is the DLL
- libMPSSE.a	// This is the libarary (as opposed to a .lib file)
- ftd2xx.lib	// This is the main D2XX library - only needed to set
				// the "only drive zero" feature of the FT232H

Next, open the Project menu item and select <Project Name> Properties 
Under the Linker tree, select Input.  
Add libmpsse.a and ftd2xx.lib to the Additional Dependencies
========================================================================
*/
// Included by Visual Studio for a standard console app
#include "stdafx.h"

// Include standard windows and C++ headers
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <ctime>
#include <iostream>

// Include the header files for the base FTDI and libMPSSE libraries
// Welcome to RigNewton.com!!
#include "ftd2xx.h"
#include "libMPSSE_i2c.h"

int _tmain(int argc, _TCHAR* argv[])
{
	
	//Start Timer
	std::clock_t start;
	double duration;
	start = std::clock();
	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Timer start: " << duration << '\n';
	
	
	// General variables
	uint32 i, index;
	unsigned char j;

	// MAX17061 variable for SMBus address
	uint32 DRV2605;
	uint32 TCA9554Addr;
	uint32 TCA9548Addr;
	uint32 BlinkMLED1;

	// This is the I2C address (fixed at 0101100 for the MAX17061         -----------------------    BLINK M!!!  Address is 0x09
	// Note this is right-justified for the libMPSSE calls.  Bit 7 is unused.)
	DRV2605 = 0x5A;						// DRV Address
	TCA9554Addr = 0x20;
	TCA9548Addr = 0x70;
	BlinkMLED1 = 0x09;



	// libMPSSE-related variables
	FT_STATUS i2cStatus = 0;
	uint32 i2cNumChannels = 0;
	FT_DEVICE_LIST_INFO_NODE i2cChannelInfo[4];
	FT_HANDLE i2cHandle;
	ChannelConfig i2cChannel;


	unsigned char i2cData[16];			// Initialize a data buffer
	uint32 i2cNumBytesToTx;				// Initialize a counter for the buffer
	uint32 i2cNumBytesTxd;				// Initialize another counter for the number of bytes actually sent
	uint32 i2cNumBytesToRx;
	uint32 i2cNumBytesRxd;

	// Initialize the MPSSE libraries
	Init_libMPSSE();

	// First, find out how many devices are capable of MPSSE commands
	i2cStatus = I2C_GetNumChannels(&i2cNumChannels);
	if (i2cStatus)						// Any value besides zero is an error
	{
		printf("ERROR %X - Cannot execute I2C_GenNumChannels.  \nPress <Enter> to continue.\n", i2cStatus);
		getchar();
		return 1;
	}

	if (i2cNumChannels > 4)
	{
		printf("This program can only identify up to 4 MPSSE devices\n");
		getchar();
		return 1;
	}

	if (i2cNumChannels < 1)
	{
		printf("No C232HM cables were found\n");
		getchar();
		return 1;
	}

	// Next, get the information about each one
	for (i=0; i < i2cNumChannels; i++)
	{
		// Get the information from each MPSSE device
		i2cStatus = I2C_GetChannelInfo(i, &i2cChannelInfo[i]);
		if (i2cStatus)
		{
			printf("ERROR %X - Cannot execute I2C_GetChannelInfo.  \nPress <Enter> to continue.\n", i2cStatus);
			getchar();
			return 1;
		}

		// If it's one of the C232HM calbes, then stop - we'll take the first one we find
		if ((i2cChannelInfo[i].Description == "CH232HM-EDHSL-0") | (i2cChannelInfo[i].Description == "CH232HM-DDHSL-0"))
			break;
	}

	index = i - 1;	// The index is zero-based
	
	// Nofity what was found and what we're using
	printf("\n");
	printf("  %d MPSSE devices found.  Device %d will be used. \n", i2cNumChannels, index);
	printf("\n");
	
	// We found the cable, now let's open it

	i2cStatus = I2C_OpenChannel(index, &i2cHandle);
	if (i2cStatus) 
	{
		printf("ERROR %X - Cannot execute I2C_OpenChannel.  \nPress <Enter> to continue.\n", i2cStatus);
		getchar();
		return 1;
	}

	// Set up the various I2C parameters
	i2cChannel.ClockRate = I2C_CLOCK_STANDARD_MODE;	// 100Kbps
	i2cChannel.LatencyTimer = 1;						// 1mS latency timer
	i2cChannel.Options = 0;								// 3-phase clocking enabled


	    //I2C_CLOCK_STANDARD_MODE = 100000,							// 100kb/sec
		//I2C_CLOCK_STANDARD_MODE_3P = 133333,						// 100kb/sec with 3-phase clocks
		//I2C_CLOCK_FAST_MODE = 400000,								// 400kb/sec
		//I2C_CLOCK_FAST_MODE_PLUS = 1000000, 						// 1000kb/sec
		//I2C_CLOCK_HIGH_SPEED_MODE = 3400000 					    // 3.4Mb/sec



	i2cStatus = I2C_InitChannel(i2cHandle, &i2cChannel);
	if (i2cStatus) 
	{
		printf("ERROR %X - Cannot execute I2C_InitChannel.  \nPress <Enter> to continue.\n", i2cStatus);
		getchar();
		return 1;
	}

	// For the FT232H, the "write on zero only" command is not yet implemented.
	// These commands will enable this feature
//	i2cNumBytesToTx = 0;	// Initialize data counter
//	i2cData[i2cNumBytesToTx++] = 0x9E;		// Enable write zero only
//	i2cData[i2cNumBytesToTx++] = 0x03;		// ADBUS0 and ADBUS1 only
//	i2cData[i2cNumBytesToTx++] = 0x00;

//	unsigned long sent;
//	FT_Write(i2cHandle, i2cData, i2cNumBytesToTx, &sent);









	getchar();


/// +++++++++++++++++++++++++++++++++    INITIALIZE DEVICES (in a loop ideally)


	//   Write to TCA9554  ++++++++++++++++++++ Attempt to enable all channels
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x03;		// Register ID
	i2cData[i2cNumBytesToTx++] = 0x00;       // Channel Enable    0x00 enables all
	i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9554Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Configured TCA9554 IO expander for output port at channel x");
	//getchar();

	//   Write to TCA9554 Enable Light
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x01;		// Register ID
	i2cData[i2cNumBytesToTx++] = 0xFF;       //   was 0x10 for channel 5 --- 0x80 for channel 8
	i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9554Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated TCA9554 \n");
	//getchar();

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "TCA9554 configured: " << duration << '\n';



// Motor Setup +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//             +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		//Motor 1 Setup

	//   Write to TCA9548 -  Channel Selection
	i2cNumBytesToTx = 0;	
	i2cData[i2cNumBytesToTx++] = 0x01;		// Register for channel selection   was 0x10 for channel 5 --- 0x80 for channel 8
	i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated TCA9548 with channel select \n");
	//   Write to DRV2605 EVM - Set to RTP mode
	i2cNumBytesToTx = 0;	
	i2cData[i2cNumBytesToTx++] = 0x01;		// Register ID
	i2cData[i2cNumBytesToTx++] = 0x05;       // RTP Mode set
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with RTP mode \n");
	//   Write to DRV2605 EVM - Set to LRA mode
	i2cNumBytesToTx = 0;	
	i2cData[i2cNumBytesToTx++] = 0x1A;		// LRA Register Address
	i2cData[i2cNumBytesToTx++] = 0xB6;      // LRA Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with LRA mode \n");
	//   Write to DRV2605 EVM - Set to Unidirectional mode
	i2cNumBytesToTx = 0;	
	i2cData[i2cNumBytesToTx++] = 0x1C;		// Directional Register Address
	i2cData[i2cNumBytesToTx++] = 0x75;      // Unidirectional Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unidirectional mode \n");
	//   Write to DRV2605 EVM - Set to Unsigned
	i2cNumBytesToTx = 0;	
	i2cData[i2cNumBytesToTx++] = 0x1D;		// Signed/Unsigned Register Address
	i2cData[i2cNumBytesToTx++] = 0x88;      // Unsigned Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unsigned mode \n");
	//   Write to DRV2605 EVM - Set to 2v Rated Voltage
	i2cNumBytesToTx = 0;	
	i2cData[i2cNumBytesToTx++] = 0x16;		// Signed/Unsigned Register Address
	i2cData[i2cNumBytesToTx++] = 0x53;      // Unsigned Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 2V mode (C10-100 motor \n");
	printf("Motor n Setup complete \n \n");


	//Motor 2 Setup

	//   Write to TCA9548 -  Channel Selection
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x02;		// Register for channel selection   was 0x10 for channel 5 --- 0x80 for channel 8
	i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated TCA9548 with channel select \n");
	//   Write to DRV2605 EVM - Set to RTP mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x01;		// Register ID
	i2cData[i2cNumBytesToTx++] = 0x05;       // RTP Mode set
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with RTP mode \n");
	//   Write to DRV2605 EVM - Set to LRA mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1A;		// LRA Register Address
	i2cData[i2cNumBytesToTx++] = 0xB6;      // LRA Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with LRA mode \n");
	//   Write to DRV2605 EVM - Set to Unidirectional mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1C;		// Directional Register Address
	i2cData[i2cNumBytesToTx++] = 0x75;      // Unidirectional Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unidirectional mode \n");
	//   Write to DRV2605 EVM - Set to Unsigned
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1D;		// Signed/Unsigned Register Address
	i2cData[i2cNumBytesToTx++] = 0x88;      // Unsigned Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unsigned mode \n");
	//   Write to DRV2605 EVM - Set to 2v Rated Voltage
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x16;		// Voltage Register Address
	i2cData[i2cNumBytesToTx++] = 0x53;      // Voltage value 2v
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 2V mode (C10-100 motor \n");
	printf("Motor n Setup complete \n \n");


	//Motor 3 Setup

	//   Write to TCA9548 -  Channel Selection
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x04;		// Register for channel selection   
	i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated TCA9548 with channel select \n");
	//   Write to DRV2605 EVM - Set to RTP mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x01;		// Register ID
	i2cData[i2cNumBytesToTx++] = 0x05;       // RTP Mode set
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with RTP mode \n");
	//   Write to DRV2605 EVM - Set to LRA mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1A;		// LRA Register Address
	i2cData[i2cNumBytesToTx++] = 0xB6;      // LRA Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with LRA mode \n");
	//   Write to DRV2605 EVM - Set to Unidirectional mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1C;		// Directional Register Address
	i2cData[i2cNumBytesToTx++] = 0x75;      // Unidirectional Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unidirectional mode \n");
	//   Write to DRV2605 EVM - Set to Unsigned
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1D;		// Signed/Unsigned Register Address
	i2cData[i2cNumBytesToTx++] = 0x88;      // Unsigned Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unsigned mode \n");
	//   Write to DRV2605 EVM - Set to 2v Rated Voltage
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x16;		// Voltage Register Address
	i2cData[i2cNumBytesToTx++] = 0x53;      // Voltage value 2v
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 2V mode (C10-100 motor \n");
	printf("Motor n Setup complete \n \n");


	//Motor 4 Setup

	//   Write to TCA9548 -  Channel Selection
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x08;		// Register for channel selection   
	i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated TCA9548 with channel select \n");
	//   Write to DRV2605 EVM - Set to RTP mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x01;		// Register ID
	i2cData[i2cNumBytesToTx++] = 0x05;       // RTP Mode set
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with RTP mode \n");
	//   Write to DRV2605 EVM - Set to LRA mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1A;		// LRA Register Address
	i2cData[i2cNumBytesToTx++] = 0xB6;      // LRA Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with LRA mode \n");
	//   Write to DRV2605 EVM - Set to Unidirectional mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1C;		// Directional Register Address
	i2cData[i2cNumBytesToTx++] = 0x75;      // Unidirectional Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unidirectional mode \n");
	//   Write to DRV2605 EVM - Set to Unsigned
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1D;		// Signed/Unsigned Register Address
	i2cData[i2cNumBytesToTx++] = 0x88;      // Unsigned Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unsigned mode \n");
	//   Write to DRV2605 EVM - Set to 2v Rated Voltage
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x16;		// Voltage Register Address
	i2cData[i2cNumBytesToTx++] = 0x53;      // Voltage value 2v
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 2V mode (C10-100 motor \n");
	printf("Motor n Setup complete \n \n");


//Motor 5 Setup

	//   Write to TCA9548 -  Channel Selection
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x10;		// Register for channel selection   
	i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated TCA9548 with channel select \n");
	//   Write to DRV2605 EVM - Set to RTP mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x01;		// Register ID
	i2cData[i2cNumBytesToTx++] = 0x05;       // RTP Mode set
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with RTP mode \n");
	//   Write to DRV2605 EVM - Set to LRA mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1A;		// LRA Register Address
	i2cData[i2cNumBytesToTx++] = 0xB6;      // LRA Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with LRA mode \n");
	//   Write to DRV2605 EVM - Set to Unidirectional mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1C;		// Directional Register Address
	i2cData[i2cNumBytesToTx++] = 0x75;      // Unidirectional Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unidirectional mode \n");
	//   Write to DRV2605 EVM - Set to Unsigned
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1D;		// Signed/Unsigned Register Address
	i2cData[i2cNumBytesToTx++] = 0x88;      // Unsigned Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unsigned mode \n");
	//   Write to DRV2605 EVM - Set to 2v Rated Voltage
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x16;		// Voltage Register Address
	i2cData[i2cNumBytesToTx++] = 0x53;      // Voltage value 2v
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 2V mode (C10-100 motor \n");
	printf("Motor n Setup complete \n \n");
	

	//Motor 6 Setup

	//   Write to TCA9548 -  Channel Selection
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x20;		// Register for channel selection   
	i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated TCA9548 with channel select \n");
	//   Write to DRV2605 EVM - Set to RTP mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x01;		// Register ID
	i2cData[i2cNumBytesToTx++] = 0x05;       // RTP Mode set
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with RTP mode \n");
	//   Write to DRV2605 EVM - Set to LRA mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1A;		// LRA Register Address
	i2cData[i2cNumBytesToTx++] = 0xB6;      // LRA Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with LRA mode \n");
	//   Write to DRV2605 EVM - Set to Unidirectional mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1C;		// Directional Register Address
	i2cData[i2cNumBytesToTx++] = 0x75;      // Unidirectional Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unidirectional mode \n");
	//   Write to DRV2605 EVM - Set to Unsigned
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1D;		// Signed/Unsigned Register Address
	i2cData[i2cNumBytesToTx++] = 0x88;      // Unsigned Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unsigned mode \n");
	//   Write to DRV2605 EVM - Set to 2v Rated Voltage
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x16;		// Voltage Register Address
	i2cData[i2cNumBytesToTx++] = 0x53;      // Voltage value 2v
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 2V mode (C10-100 motor \n");
	printf("Motor n Setup complete \n \n");


	//Motor 7 Setup

	//   Write to TCA9548 -  Channel Selection
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x40;		// Register for channel selection   
	i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated TCA9548 with channel select \n");
	//   Write to DRV2605 EVM - Set to RTP mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x01;		// Register ID
	i2cData[i2cNumBytesToTx++] = 0x05;       // RTP Mode set
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with RTP mode \n");
	//   Write to DRV2605 EVM - Set to LRA mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1A;		// LRA Register Address
	i2cData[i2cNumBytesToTx++] = 0xB6;      // LRA Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with LRA mode \n");
	//   Write to DRV2605 EVM - Set to Unidirectional mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1C;		// Directional Register Address
	i2cData[i2cNumBytesToTx++] = 0x75;      // Unidirectional Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unidirectional mode \n");
	//   Write to DRV2605 EVM - Set to Unsigned
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1D;		// Signed/Unsigned Register Address
	i2cData[i2cNumBytesToTx++] = 0x88;      // Unsigned Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unsigned mode \n");
	//   Write to DRV2605 EVM - Set to 2v Rated Voltage
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x16;		// Voltage Register Address
	i2cData[i2cNumBytesToTx++] = 0x53;      // Voltage value 2v
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 2V mode (C10-100 motor \n");
	printf("Motor n Setup complete \n \n");



	//Motor 8 Setup

	//   Write to TCA9548 -  Channel Selection
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x80;		// Register for channel selection   
	i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated TCA9548 with channel select \n");
	//   Write to DRV2605 EVM - Set to RTP mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x01;		// Register ID
	i2cData[i2cNumBytesToTx++] = 0x05;       // RTP Mode set
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with RTP mode \n");
	//   Write to DRV2605 EVM - Set to LRA mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1A;		// LRA Register Address
	i2cData[i2cNumBytesToTx++] = 0xB6;      // LRA Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with LRA mode \n");
	//   Write to DRV2605 EVM - Set to Unidirectional mode
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1C;		// Directional Register Address
	i2cData[i2cNumBytesToTx++] = 0x75;      // Unidirectional Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unidirectional mode \n");
	//   Write to DRV2605 EVM - Set to Unsigned
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x1D;		// Signed/Unsigned Register Address
	i2cData[i2cNumBytesToTx++] = 0x88;      // Unsigned Mode
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 with Unsigned mode \n");
	//   Write to DRV2605 EVM - Set to 2v Rated Voltage
	i2cNumBytesToTx = 0;
	i2cData[i2cNumBytesToTx++] = 0x16;		// Voltage Register Address
	i2cData[i2cNumBytesToTx++] = 0x53;      // Voltage value 2v
	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x03);
	printf("Updated DRV2605 2V mode (C10-100 motor \n");
	printf("Motor 8 Setup complete \n \n");




// Loop Test  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Timer: " << duration << '\n';
	printf("Motor Setup complete.  Press a key to enter motor test loop...\n");
	
	// Testing printf with Bytes
	int m = 0;
	int mhex = byte(m);
	for (m = 0; m < 9; m++)	{

		printf("M= %a \n", mhex );
	}

	getchar();

	start = std::clock();
	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;

	//Start by setting all motors to max force until keypress


// Set number of initial test loops 
int a;
for (a = 0; a < 1; a++)
{

	//int counterInt = 0;
	int forceConvert = 0;
	int z;
	for (z = 255; z > 0; z=z-5)
	{

		forceConvert = (byte)z;

		// Write same value to all motors...

		//   Write to TCA9548 -  Channel Selection - Motor 1
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x01;		// Register ID   was 0x10 for channel 5 --- 0x80 for channel 8    0x01 for channel 1
		i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
		// Now send the RTP Value to the RTP Register
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x02;		// RTP Register Address
		i2cData[i2cNumBytesToTx++] = forceConvert;      // you say vibrate, I say "how high"
		i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
		printf("%i  Motor 1 RTP ", z);
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		std::cout << " Total Time: " << duration << '\n';



	//   Write to TCA9548 -  Channel Selection - Motor 2
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x02;		// Register ID   was 0x10 for channel 5 --- 0x80 for channel 8    0x01 for channel 1
		i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
		// Now send the RTP Value to the RTP Register
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x02;		// RTP Register Address
		i2cData[i2cNumBytesToTx++] = forceConvert;      // you say vibrate, I say "how high"
		i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
		printf("%i  Motor 2 RTP ", z);
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		std::cout << " Total Time: " << duration << '\n';


	//   Write to TCA9548 -  Channel Selection - Motor 3
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x04;		// Register ID   was 0x10 for channel 5 --- 0x80 for channel 8    0x01 for channel 1
		i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
		// Now send the RTP Value to the RTP Register
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x02;		// RTP Register Address
		i2cData[i2cNumBytesToTx++] = forceConvert;      // you say vibrate, I say "how high"
		i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
		printf("%i  Motor 3 RTP ", z);
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		std::cout << " Total Time: " << duration << '\n';


		//   Write to TCA9548 -  Channel Selection - Motor 4
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x08;		// Register ID   was 0x10 for channel 5 --- 0x80 for channel 8    0x01 for channel 1
		i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
		// Now send the RTP Value to the RTP Register
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x02;		// RTP Register Address
		i2cData[i2cNumBytesToTx++] = forceConvert;      // you say vibrate, I say "how high"
		i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
		printf("%i  Motor 4 RTP ", z);
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		std::cout << " Total Time: " << duration << '\n';


		//   Write to TCA9548 -  Channel Selection - Motor 5
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x10;		// Register ID   was 0x10 for channel 5 --- 0x80 for channel 8    0x01 for channel 1
		i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
		// Now send the RTP Value to the RTP Register
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x02;		// RTP Register Address
		i2cData[i2cNumBytesToTx++] = forceConvert;      // you say vibrate, I say "how high"
		i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
		printf("%i  Motor 5 RTP ", z);
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		std::cout << " Total Time: " << duration << '\n';


		//   Write to TCA9548 -  Channel Selection - Motor 6
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x20;		// Register ID   was 0x10 for channel 5 --- 0x80 for channel 8    0x01 for channel 1
		i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
		// Now send the RTP Value to the RTP Register
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x02;		// RTP Register Address
		i2cData[i2cNumBytesToTx++] = forceConvert;      // you say vibrate, I say "how high"
		i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
		printf("%i  Motor 6 RTP ", z);
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		std::cout << " Total Time: " << duration << '\n';



		//   Write to TCA9548 -  Channel Selection - Motor 7
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x40;		// Register ID   was 0x10 for channel 5 --- 0x80 for channel 8    0x01 for channel 1
		i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
		// Now send the RTP Value to the RTP Register
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x02;		// RTP Register Address
		i2cData[i2cNumBytesToTx++] = forceConvert;      // you say vibrate, I say "how high"
		i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
		printf("%i  Motor 7 RTP ", z);
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		std::cout << " Total Time: " << duration << '\n';



		//   Write to TCA9548 -  Channel Selection - Motor 8
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x40;		// Register ID   was 0x10 for channel 5 --- 0x80 for channel 8    0x01 for channel 1
		i2cStatus = I2C_DeviceWrite(i2cHandle, TCA9548Addr, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
		// Now send the RTP Value to the RTP Register
		i2cNumBytesToTx = 0;	// Initialize data counter so we can calculate the number of bytes written
		i2cData[i2cNumBytesToTx++] = 0x02;		// RTP Register Address
		i2cData[i2cNumBytesToTx++] = forceConvert;      // you say vibrate, I say "how high"
		i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, I2C_TRANSFER_OPTIONS_START_BIT|I2C_TRANSFER_OPTIONS_STOP_BIT);
		printf("%i  Motor 8 RTP ", z);
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		std::cout << " Total Time: " << duration << '\n';


		// Pauses the loop at max force before dropping it lower
		if (z == 255) {
			printf("Max force set, press a key to descend to zero force");
			getchar();
		}


	}


}
	printf("Finished Loop!");
	//getchar();



	// READ DATA +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	i2cNumBytesToTx = 0;					// Initialize data counter
	i2cData[i2cNumBytesToTx++] = 0x00;		// Status register
	//	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x01);
	i2cNumBytesToRx = 1;
	i2cStatus += I2C_DeviceRead(i2cHandle, DRV2605, i2cNumBytesToRx, &i2cData[0], &i2cNumBytesRxd, 0x03);
	if (i2cStatus)
	{
		printf("ERROR %X - Cannot read register.  \nPress <Enter> to continue.\n", i2cStatus);
		getchar();
	//	return 0;
	}

	printf("The DRV2605 Status value is 0x%X\n", i2cData[0]);
	printf("\n");


	i2cNumBytesToTx = 0;					// Initialize data counter
	i2cData[i2cNumBytesToTx++] = 0x01;		// Status register
	//	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x01);
	i2cNumBytesToRx = 1;
	i2cStatus += I2C_DeviceRead(i2cHandle, DRV2605, i2cNumBytesToRx, &i2cData[0], &i2cNumBytesRxd, 0x03);
	if (i2cStatus)
	{
		printf("ERROR %X - Cannot read register.  \nPress <Enter> to continue.\n", i2cStatus);
		getchar();
	//	return 0;
	}

	printf("The DRV2605 ERM/LRA Mode value is 0x%X\n", i2cData[0]);
	printf("\n");



	i2cNumBytesToTx = 0;					// Initialize data counter
	i2cData[i2cNumBytesToTx++] = 0x16;		// Status register
	//	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x01);
	i2cNumBytesToRx = 1;
	i2cStatus += I2C_DeviceRead(i2cHandle, DRV2605, i2cNumBytesToRx, &i2cData[0], &i2cNumBytesRxd, 0x03);
	if (i2cStatus)
	{
		printf("ERROR %X - Cannot read register.  \nPress <Enter> to continue.\n", i2cStatus);
		getchar();
	//	return 0;
	}
	printf("The DRV2605 Rated Voltage is 0x%X\n", i2cData[0]);
	printf("\n");




	i2cNumBytesToTx = 0;					// Initialize data counter
	i2cData[i2cNumBytesToTx++] = 0x1A;		// Status register
	//	i2cStatus = I2C_DeviceWrite(i2cHandle, DRV2605, i2cNumBytesToTx, i2cData, &i2cNumBytesTxd, 0x01);
	i2cNumBytesToRx = 1;
	i2cStatus += I2C_DeviceRead(i2cHandle, DRV2605, i2cNumBytesToRx, &i2cData[0], &i2cNumBytesRxd, 0x03);
	if (i2cStatus)
	{
		printf("ERROR %X - Cannot read register.  \nPress <Enter> to continue.\n", i2cStatus);
		getchar();
	//	return 0;
	}

	printf("The DRV2605 Feedback Control  is 0x%X\n", i2cData[0]);
	printf("\n");







	////  CLEAN UP ========================================================================================================


	// Time to close up shop - close the channel, then ...
	printf("  Closing the CH232M MPSSE channel\n");
	printf("\n");
	i2cStatus = I2C_CloseChannel(i2cHandle);
	if (i2cStatus) 
	{
		printf("ERROR %X - Cannot execute I2C_CloseChannel.  \nPress <Enter> to continue.\n", i2cStatus);
		getchar();
		return 1;
	}

	// ... close out the MPSSE libraries
	printf("  Cleaning up the libMPSSE library\n");
	printf("\n");
	Cleanup_libMPSSE();
	
	printf("  Exit program.  \nPress <Enter> to continue.\n");
	printf("\n");
	return 0;
}

