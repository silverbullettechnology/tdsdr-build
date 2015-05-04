/** 
 *
 *  \author    Morgan Hughes <morgan.hughes@silver-bullet-tech.com>
 *  \version   v0.0
 *  \copyright Copyright 2014,2015 Silver Bullet Technology
 *
 *             Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *             use this file except in compliance with the License.  You may obtain a copy
 *             of the License at: 
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *             Unless required by applicable law or agreed to in writing, software
 *             distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *             WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 *             License for the specific language governing permissions and limitations
 *             under the License.
 *
 *  vim:ts=4:noexpandtab
 */
#ifndef UNIT_TEST
#error  Unit-test code should only be compiled by the unit-test harness/Makefile
#endif 

#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include <lib/log.h>
#include <lib/mbuf.h>

#include <common/vita49/common.h>
#include <common/vita49/command.h>
#include <common/vita49/context.h>


LOG_MODULE_STATIC("unit_test", LOG_LEVEL_DEBUG);


struct parse_test
{
	const char *desc;
	int         want;
	uint32_t   *buff;
	size_t      size;
};


// Context packet header
static uint32_t context_head[] = 
{
	0x49D00000,  // Header: Type 4, Stream Identifier, Coarse Timing, TSI & TSF
	0x000000a0,  // Stream Identifier: assigned for stream
	V49_SBT_OUI, // Class Identifier: SBT OUI 11:22:33, 
	V49_SBT_XCC, // Information Class Code 1, Packet Class Code 1.
	0x54fa2579,  // Integer-seconds Timestamp
	0x00000000,  // Fractional-seconds Timestamp (MSW)
	0x00000001,  // Fractional-seconds Timestamp (LSW)
};
struct parse_test  parse_test_context_short =
{
	.desc = "context_short", .want = V49_ERR_SHORT,
	.buff = context_head,    .size = sizeof(context_head),
};

// Context packet body with all options
static uint32_t context_xmas[] = 
{
	0x49D00000,  // Header: Type 4, Stream Identifier, Coarse Timing, TSI & TSF
	0x000000a0,  // Stream Identifier: assigned for stream
	V49_SBT_OUI, // Class Identifier: SBT OUI 11:22:33, 
	V49_SBT_XCC, // Information Class Code 1, Packet Class Code 1.
	0x54fa2579,  // Integer-seconds Timestamp
	0x00000000,  // Fractional-seconds Timestamp (MSW)
	0x00000001,  // Fractional-seconds Timestamp (LSW)

	0xFFFFFFFF,  // Indicator: all the fields
	0x00000123,  // Reference Point Identifier: 291
	0x00000356,  // Bandwidth MSW: 3500000.999999Hz
	0x7E0fffff,  // Bandwidth LSW: 3500000.999999Hz
	0x000001E8,  // IF Reference Frequency MSW: 2000000.50000 Hz
	0x48080000,  // IF Reference Frequency LSW: 2000000.50000 Hz
	0x0009141a,  // RF Reference Frequency MSW: 2437 MHz
	0xb4000000,  // RF Reference Frequency LSW: 2437 MHz
	0x00000000,  // RF Reference Frequency Offset MSW: 0Hz
	0x00000000,  // RF Reference Frequency Offset LSW: 0Hz
	0x00000000,  // IF Band Offset MSW: 0Hz
	0x00000000,  // IF Band Offset LSW: 0Hz
	0x000002FF,  // Reference Level: +5.99 dBm
	0x00A00140,  // Gain: 1.25dB (stage 1), 2.50 dB (stage 2)
	0x00001234,  // Over-range Count: 4660
	0x000003D0,  // Sample Rate MSW: 4.0MHz
	0x90000000,  // Sample Rate LSW: 4.0MHz 
	0x00000000,  // Timestamp Adjustment MSW
	0x00000000,  // Timestamp Adjustment LSW
	0x54F7BA31,  // Timestamp Calibration Time
	0x00000773,  // Temperature (29.8C)
	V49_SBT_OUI, // Device Identifier OUI
	0x00000001,  // Device Identifier Code (1)
	0x00000000,  // State and Event Indicators
	0x00000000,  // Data Packet Payload Format 1
	0x00000000,  // Data Packet Payload Format 2

	// Formatted GPS (Global Positioning System) Geolocation (11 words)
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,

	// Formatted INS (Inertial Navigation System) Geolocation (11 words)
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,

	// ECEF (Earth-Centered, Earth-Fixed) Ephemeris (13 words)
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,

	// Relative Ephemeris (13 words)
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,

	0x00000000,  // Ephemeris Reference Identifier

	// GPS ASCII (2 words fixed + 4 words variable)
	V49_SBT_OUI, 0x00000004, 0x41424344, 0x45464748, 0x494a4b4c, 0x4d4e4f50,

	// Context Association List:
	// Source List { 0x00010002 0x00030004 }
	// System List { 0x00050006 0x00070008 }
	// Vector-Component List { 0x00090010 0x00110012 }
	// Asynchronous-Channel List { 0x00130014 0x00150016 }
	// Asynchronous-Channel Tag List { 0x00170018 0x00190020 }
	0x00020002, 0x00028002, 0x00010002, 0x00030004, 0x00050006, 0x00070008,
	0x00090010, 0x00110012, 0x00130014, 0x00150016, 0x00170018, 0x00190020,
};
struct parse_test  parse_test_context_xmas =
{
	.desc = "context_xmas", .want = V49_OK_COMPLETE,
	.buff = context_xmas,   .size = sizeof(context_xmas),
};


// Command packet header
static uint32_t command_head[] = 
{
	0x59D00000,  // Header: Type 5, Stream Identifier, Coarse Timing, TSI & TSF
	0x000000a0,  // Stream Identifier: assigned for stream
	V49_SBT_OUI, // Class Identifier: SBT OUI 11:22:33, 
	V49_SBT_XCC, // Information Class Code 1, Packet Class Code 1.
	0x54F7BA31,  // Integer-seconds Timestamp
	0x00000000,  // Fractional-seconds Timestamp (MSW)
	0x00000001,  // Fractional-seconds Timestamp (LSW)
};
struct parse_test  parse_test_command_short =
{
	.desc = "command_short", .want = V49_ERR_SHORT,
	.buff = command_head,    .size = sizeof(command_head),
};

// Command packet body with all options
static uint32_t command_xmas[] = 
{
	0x59D00000,  // Header: Type 5, Stream Identifier, Coarse Timing, TSI & TSF
	0x000000a0,  // Stream Identifier: assigned for stream
	V49_SBT_OUI, // Class Identifier: SBT OUI 11:22:33, 
	V49_SBT_XCC, // Information Class Code 1, Packet Class Code 1.
	0x54F7BA31,  // Integer-seconds Timestamp
	0x00000000,  // Fractional-seconds Timestamp (MSW)
	0x00000001,  // Fractional-seconds Timestamp (LSW)

	0x00000000,  // Request / Response: Request, Discovery
	0xFFFFFFFF,  // Indicator: all the fields
	0x00010001,  // Message Paging: 1 of 1

	// Client Identifier: e4567f9b-0330-4d23-a590-4d19f608c75f
	0xe4567f9b, 0x03304d23, 0xa5904d19, 0xf608c75f,

	0x00028000,  // Request Priority: 2.50

	// Resource Identifier List: 2 UUIDs, 8 words:
	// - 0dc1faf6-5c0c-4b5b-97b9-66ee5364efab
	// - fdf87fdf-3c22-4582-a2ff-800984bdc05c
	0x00000008,
	0x0dc1faf6, 0x5c0c4b5b, 0x97b966ee, 0x5364efab,
	0xfdf87fdf, 0x3c224582, 0xa2ff8009, 0x84bdc05c,

	// Resource Information List:
	// - 0dc1faf6-5c0c-4b5b-97b9-66ee5364efab
	//   Name "AD1R1" - no TX, 1 RX channel, 61.44MSPS, 70MHz to 6GHz
	// - fdf87fdf-3c22-4582-a2ff-800984bdc05c
	//   Name "AD1T1" - 1 TX channel, no RX, 61.44MSPS, 70MHz to 6GHz
	0x00000018,
	0x0dc1faf6, 0x5c0c4b5b, 0x97b966ee, 0x5364efab,
	0x65683082, 0x30000000, 0x00000000, 0x00000000,
	0x01003d70, 0x00461770, 0x00000000, 0x00000000,
	0xfdf87fdf, 0x3c224582, 0xa2ff8009, 0x84bdc05c,
	0x65683084, 0x30000000, 0x00000000, 0x00000000,
	0x10003d70, 0x00461770, 0x00000000, 0x00000000,

	0x000000a0, // Stream Identifier Assignment: 160
	0x00000001, // Timestamp Interpretation: Absolute
};
struct parse_test  parse_test_command_xmas =
{
	.desc = "command_xmas", .want = V49_OK_COMPLETE,
	.buff = command_xmas,   .size = sizeof(command_xmas),
};


// Protocol Exchange Example

// Packet 4-1: Client probes Agent
static uint32_t exchange_pkt_01[] =
{
	0x5900000a, // Header: Type 5, Stream Identifier, Coarse Timing, 10 words length
	0x00000000, // Stream Identifier: reserved for probe
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x00000000, // Request / Response: Request, Discovery
	0x40000000, // Parameter Indicator: Client Identifier

 	// Client Identifier: e4567f9b-0330-4d23-a590-4d19f608c75f
	0xe4567f9b, 0x03304d23, 0xa5904d19, 0xf608c75f,
};
struct parse_test  parse_test_exchange_pkt_01 =
{
	.desc = "exchange_pkt_01", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_01,   .size = sizeof(exchange_pkt_01),
};

// Packet 4-2: Agent answers Client probe, includes Enumeration
static uint32_t exchange_pkt_02[] =
{
	0x59000023, // Header: Type 5, Stream Identifier, Coarse Timing, 35 words length
	0x00000000, // Stream Identifier: reserved for probe
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x40000000, // Request / Response: Response, Discovery, Success
	0x48000000, // Parameter Indicator: Client Identifier, Resource Information List

 	// Client Identifier: e4567f9b-0330-4d23-a590-4d19f608c75f
	0xe4567f9b, 0x03304d23, 0xa5904d19, 0xf608c75f,

	// Resource Information List: 2 resources:
	// - 0dc1faf6-5c0c-4b5b-97b9-66ee5364efab
	//   Name "AD1R1" - no TX, 1 RX channel, 61.44MSPS, 70MHz to 6GHz
	// - fdf87fdf-3c22-4582-a2ff-800984bdc05c
	//   Name "AD1T1" - 1 TX channel, no RX, 61.44MSPS, 70MHz to 6GHz
	0x00000018,
	0x0dc1faf6, 0x5c0c4b5b, 0x97b966ee, 0x5364efab,
	0x65683082, 0x30000000, 0x00000000, 0x00000000,
	0x01003d70, 0x00461770, 0x00000000, 0x00000000,
	0xfdf87fdf, 0x3c224582, 0xa2ff8009, 0x84bdc05c,
	0x65683084, 0x30000000, 0x00000000, 0x00000000,
	0x10003d70, 0x00461770, 0x00000000, 0x00000000,
};
struct parse_test  parse_test_exchange_pkt_02 =
{
	.desc = "exchange_pkt_02", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_02,   .size = sizeof(exchange_pkt_02),
};

// Packet 4-3: Client requests Access to resource AD1R1
static uint32_t exchange_pkt_03[] =
{
	0x59000010, // Header: Type 5, Stream Identifier, Coarse Timing, 16 words length
	0x00000000, // Stream Identifier: reserved for probe
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x00000002, // Request / Response: Request, Access
	0x70000000, // Parameter Indicator: Client Identifier, Request Priority, Resource Identifier List

 	// Client Identifier: e4567f9b-0330-4d23-a590-4d19f608c75f
	0xe4567f9b, 0x03304d23, 0xa5904d19, 0xf608c75f,

	0x00018000, // Request Priority: 1.5

	// Resource Identifier List: 1 UUID, 4 words:
	// - 0dc1faf6-5c0c-4b5b-97b9-66ee5364efab
	0x00000004,
	0x0dc1faf6, 0x5c0c4b5b, 0x97b966ee, 0x5364efab,

};
struct parse_test  parse_test_exchange_pkt_03 =
{
	.desc = "exchange_pkt_03", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_03,   .size = sizeof(exchange_pkt_03),
};

// Packet 4-4: Agent grants Client Access to resource
static uint32_t exchange_pkt_04[] =
{
	0x5900000b, // Header: Type 5, Stream Identifier, Coarse Timing, 11 words length
	0x00000000, // Stream Identifier: reserved for probe
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x40000002, // Request / Response: Response, Access, Success
	0x44000000, // Parameter Indicator: Client Identifier, Stream Identifier Assignment

 	// Client Identifier: e4567f9b-0330-4d23-a590-4d19f608c75f
	0xe4567f9b, 0x03304d23, 0xa5904d19, 0xf608c75f,

	0x000000a0, // Stream Identifier Assignment: 160
};
struct parse_test  parse_test_exchange_pkt_04 =
{
	.desc = "exchange_pkt_04", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_04,   .size = sizeof(exchange_pkt_04),
};

// Packet 4-5: Client requests to open resource
static uint32_t exchange_pkt_05[] =
{
	0x59000006, // Header: Type 5, Stream Identifier, Coarse Timing, 6 words length
	0x000000a0, // Stream Identifier: assigned for stream
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x00000003, // Request / Response: Request, Open/Resume
	0x00000000, // Parameter Indicator: none
};
struct parse_test  parse_test_exchange_pkt_05 =
{
	.desc = "exchange_pkt_05", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_05,   .size = sizeof(exchange_pkt_05),
};

// Packet 4-6: Client success in opening resource
static uint32_t exchange_pkt_06[] =
{
	0x59000006, // Header: Type 5, Stream Identifier, Coarse Timing, 6 words length
	0x000000a0, // Stream Identifier: assigned for stream
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x40000003, // Request / Response: Result, Open/Resume, Success
	0x00000000, // Parameter Indicator: none
};
struct parse_test  parse_test_exchange_pkt_06 =
{
	.desc = "exchange_pkt_06", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_06,   .size = sizeof(exchange_pkt_06),
};


// Packet 4-7: Client configures the resource with standard context
// Note: The Agent accepts a subset of standard context fields as configuration
// parameters.  Request type "00000100" (4) is reserved for other configuration tasks
// which can't be represented by the standard packet.  The Agent produces a Response type
// "00000100" (4) to report success/failure in applying the values in the standard context
// packet.
static uint32_t exchange_pkt_07[] =
{
	0x4900000b, // Header: Type 4, Stream Identifier, Coarse Timing, 11 words length
	0x000000a0, // Stream Identifier: assigned for stream
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x28200000, // Context Indicator: Bandwidth, RF Reference Frequency, Sample Rate
	0x00000356, // Bandwidth: 3.5 MHz
	0x7E000000, // Bandwidth: 3.5 MHz
	0x0009141a, // RF Reference Frequency: 2.437MHz
	0xb4000000, // RF Reference Frequency: 2.437MHz
	0x000003D0, // Sample Rate: 4.0 MSPS
	0x90000000, // Sample Rate: 4.0 MSPS
};
struct parse_test  parse_test_exchange_pkt_07 =
{
	.desc = "exchange_pkt_07", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_07,   .size = sizeof(exchange_pkt_07),
};

// Packet 4-8: Client success in configuring
static uint32_t exchange_pkt_08[] =
{
	0x59000006, // Header: Type 5, Stream Identifier, Coarse Timing, 6 words length
	0x000000a0, // Stream Identifier: assigned for stream
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x40000004, // Request / Response: Result, Configure, Success
	0x00000000, // Parameter Indicator: none
};
struct parse_test  parse_test_exchange_pkt_08 =
{
	.desc = "exchange_pkt_08", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_08,   .size = sizeof(exchange_pkt_08),
};

// Packet 4-9: Client requests to stop traffic
// Note: By issuing a relative Stop request before the Start, the Client is setting up a
// fixed-length one-shot event.  It can later trigger the event multiple times with single
// Start requests.
static uint32_t exchange_pkt_09[] =
{
	0x59100009, // Header: Type 5, Stream Identifier, TSF in Samples, Coarse Timing, 9 words length
	0x000000a0, // Stream Identifier: assigned for stream
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x00000000, // Fractional-seconds Timestamp: 1M samples
	0x000F4240,
	0x00000006, // Request / Response: Request, Stop
	0x02000000, // Parameter Indicator: Timestamp Interpretation
	0x00000002, // Timestamp Interpretation: Relative (to start time)
};
struct parse_test  parse_test_exchange_pkt_09 =
{
	.desc = "exchange_pkt_09", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_09,   .size = sizeof(exchange_pkt_09),
};

// Packet 4-10: Client success in setting Stop condition
static uint32_t exchange_pkt_10[] =
{
	0x5900000a, // Header: Type 5, Stream Identifier, Coarse Timing, 6 words length
	0x000000a0, // Stream Identifier: assigned for stream
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x40000006, // Request / Response: Result, Stop, Success
	0x00000000, // Parameter Indicator: none
};
struct parse_test  parse_test_exchange_pkt_10 =
{
	.desc = "exchange_pkt_10", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_10,   .size = sizeof(exchange_pkt_10),
};

// Packet 4-11: Client requests to start traffic
static uint32_t exchange_pkt_11[] =
{
	0x59000006, // Header: Type 5, Stream Identifier, Coarse Timing, 6 words length
	0x000000a0, // Stream Identifier: assigned for stream
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x00000005, // Request / Response: Request, Start
	0x00000000, // Parameter Indicator: none
};
struct parse_test  parse_test_exchange_pkt_11 =
{
	.desc = "exchange_pkt_11", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_11,   .size = sizeof(exchange_pkt_11),
};

// Packet 4-12: Client success in opening resource
// Note: The Client should be prepared to handle received data before it issues an
// immediate Start request, as the data flow may begin before the Start response arrives.
// The Client may receive a Start request with a failure code instead, if the Agent was
// unable to start the flow
static uint32_t exchange_pkt_12[] =
{
	0x59000006, // Header: Type 5, Stream Identifier, Coarse Timing, 6 words length
	0x000000a0, // Stream Identifier: assigned for stream
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x40000005, // Request / Response: Result, Start, Success
	0x00000000, // Parameter Indicator: none
};
struct parse_test  parse_test_exchange_pkt_12 =
{
	.desc = "exchange_pkt_12", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_12,   .size = sizeof(exchange_pkt_12),
};

// Packet 4-13: Client requests to Close/Suspend resource
static uint32_t exchange_pkt_13[] =
{
	0x59000006, // Header: Type 5, Stream Identifier, Coarse Timing, 6 words length
	0x000000a0, // Stream Identifier: assigned for stream
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x00000007, // Request / Response: Request, Close/Suspend
	0x00000000, // Parameter Indicator: none
};
struct parse_test  parse_test_exchange_pkt_13 =
{
	.desc = "exchange_pkt_13", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_13,   .size = sizeof(exchange_pkt_13),
};

// Packet 4-14: Close/Suspend success
// Note: at this point the Stream Identifier previously allocated is still valid, and the
// Client could issue a new Open/Resume request to use the resource again.  The Agent will
// automatically cache the configuration that was last applied, and re-apply it on a
// subsequent Open/Resume request.
static uint32_t exchange_pkt_14[] =
{
	0x59000006, // Header: Type 5, Stream Identifier, Coarse Timing, 6 words length
	0x000000a0, // Stream Identifier: assigned for stream
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x40000007, // Request / Response: Response, Close/Suspend, Success
	0x00000000, // Parameter Indicator: none
};
struct parse_test  parse_test_exchange_pkt_14 =
{
	.desc = "exchange_pkt_14", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_14,   .size = sizeof(exchange_pkt_14),
};

// Packet 4-15: Client requests to Release resource
static uint32_t exchange_pkt_15[] =
{
	0x59000006, // Header: Type 5, Stream Identifier, Coarse Timing, 6 words length
	0x000000a0, // Stream Identifier: assigned for stream
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x00000008, // Request / Response: Request, Release
	0x00000000, // Parameter Indicator: none
};
struct parse_test  parse_test_exchange_pkt_15 =
{
	.desc = "exchange_pkt_15", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_15,   .size = sizeof(exchange_pkt_15),
};

// Packet 4-16: Release success
static uint32_t exchange_pkt_16[] =
{
	0x59000006, // Header: Type 5, Stream Identifier, Coarse Timing, 6 words length
	0x000000a0, // Stream Identifier: assigned for stream
	0x00112233, // Class Identifier: SBT OUI 11:22:33
	0x00010001, // Class Identifier: Information Class Code 1, Packet Class Code 1.
	0x40000008, // Request / Response: Response, Release, Success
	0x00000000, // Parameter Indicator: none
};
struct parse_test  parse_test_exchange_pkt_16 =
{
	.desc = "exchange_pkt_16", .want = V49_OK_COMPLETE,
	.buff = exchange_pkt_16,   .size = sizeof(exchange_pkt_16),
};



struct parse_test *parse_test_list[] =
{
	&parse_test_context_short,
	&parse_test_context_xmas,
	&parse_test_command_short,
	&parse_test_command_xmas,
	&parse_test_exchange_pkt_01,
	&parse_test_exchange_pkt_02,
	&parse_test_exchange_pkt_03,
	&parse_test_exchange_pkt_04,
	&parse_test_exchange_pkt_05,
	&parse_test_exchange_pkt_06,
	&parse_test_exchange_pkt_07,
	&parse_test_exchange_pkt_08,
	&parse_test_exchange_pkt_09,
	&parse_test_exchange_pkt_10,
	&parse_test_exchange_pkt_11,
	&parse_test_exchange_pkt_12,
	&parse_test_exchange_pkt_13,
	&parse_test_exchange_pkt_14,
	&parse_test_exchange_pkt_15,
	&parse_test_exchange_pkt_16,
	NULL
};

void load (struct mbuf *dst, uint32_t *src, size_t len)
{
	while ( len >= sizeof(uint32_t) )
	{
		mbuf_set_be32(dst, *src++);
		len -= sizeof(uint32_t);
	}
}

// length end - beg
void fix_length (struct mbuf *mbuf)
{
	int len = mbuf_end_ptr(mbuf) - mbuf_beg_ptr(mbuf);
	int cur = mbuf_cur_get(mbuf);

	mbuf_cur_set_beg(mbuf);
	mbuf_cur_adj(mbuf, 2);
	mbuf_set_be16(mbuf, len >> 2);

	mbuf_cur_set(mbuf, cur);
}

void parse_setup (struct mbuf *mbuf, const char *desc, ...)
{
	LOG_FOCUS("\n-----\nSetup %s packet:\n", desc);
	mbuf_beg_set(mbuf, 0);
	mbuf_cur_set_beg(mbuf);
	mbuf_end_set_beg(mbuf);

	va_list   ap;
	uint32_t *cp;
	size_t    cs;

	va_start(ap, desc);
	while ( (cp = va_arg(ap, uint32_t *)) )
	{
		cs = va_arg(ap, size_t);
		LOG_FOCUS("Chunk: base %p size %zu\n", cp, cs);
		if ( cs )
			load(mbuf, cp, cs);
	}
	va_end(ap);

	mbuf_cur_set_beg(mbuf);
	fix_length(mbuf);
}


void cleanup_growlists (struct v49_common *v49)
{
	if ( v49->type != V49_TYPE_COMMAND )
		return;

	if ( v49->command.rid_list )
	{
		growlist_done(v49->command.rid_list, gen_free, NULL);
		free(v49->command.rid_list);
		v49->command.rid_list = NULL;
	}

	if ( v49->command.res_info )
	{
		growlist_done(v49->command.res_info, gen_free, NULL);
		free(v49->command.res_info);
		v49->command.res_info = NULL;
	}
}


int main (int argc, char **argv)
{
	struct v49_common  v49;
	struct mqueue      queue;
	struct mbuf       *parse;
	struct mbuf       *format;
	int                ret;

	if ( !(parse = mbuf_alloc(4096, 0)) )
	{
		perror("mbuf_alloc");
		return 1;
	}
	if ( !(format = mbuf_alloc(4096, 0)) )
	{
		perror("mbuf_alloc");
		return 1;
	}
	mqueue_init(&queue, 0);

	log_dupe(stdout);
	log_set_global_level(LOG_LEVEL_DEBUG);
//	log_set_module_level("vita49_common",  LOG_LEVEL_TRACE);
//	log_set_module_level("vita49_command", LOG_LEVEL_TRACE);
//	log_set_module_level("vita49_context", LOG_LEVEL_TRACE);

	struct parse_test **walk = parse_test_list;
	while ( *walk )
	{
		struct parse_test *test = *walk;

		v49_reset(&v49);
		parse_setup(parse, test->desc, test->buff, test->size, NULL);

		ret = v49_parse(&v49, parse);
		v49_dump(LOG_LEVEL_FOCUS, &v49);
		LOG_FOCUS("%s:\n  Want: %08x / %s\n  Have: %08x / %s\n", test->desc,
		          test->want, v49_return_desc(test->want),
		          ret, v49_return_desc(ret));
		if ( ret != test->want )
			goto fail;

		LOG_FOCUS("Parsed: %s\n", test->desc);

		if ( test->want != V49_OK_COMPLETE )
		{
			LOG_FOCUS("Skip format, expected failure\n");
			cleanup_growlists(&v49);
			walk++;
			continue;
		}

		ret = v49_format(&v49, format, &queue);
		if ( ret != V49_OK_COMPLETE )
			goto fail;

		LOG_FOCUS("Format: %s\n", test->desc);

		mbuf_cur_set_beg(parse);
		mbuf_cur_set_beg(format);
		if ( mbuf_get_avail(parse) != mbuf_get_avail(format) )
			LOG_FOCUS("Size mismatch: want %d have %d\n", 
			           mbuf_get_avail(parse), mbuf_get_avail(format));

		LOG_FOCUS("Offset: ____want  ____have\n");
		int       cur = mbuf_beg_get(parse);
		uint32_t  wv;
		uint32_t  hv;
		char      ws[12];
		char      hs[12];
		char      ds[12];
		while ( mbuf_get_avail(parse) || mbuf_get_avail(format) )
		{
			if ( mbuf_get_avail(parse) < sizeof(wv) )
				snprintf(ws, sizeof(ws), "--------");
			else if ( mbuf_get_be32(parse, &wv) == sizeof(wv) )
				snprintf(ws, sizeof(ws), "%08x", wv);
			else
				goto fail;

			if ( mbuf_get_avail(format) < sizeof(hv) )
				snprintf(hs, sizeof(hs), "--------");
			else if ( mbuf_get_be32(format, &hv) == sizeof(hv) )
				snprintf(hs, sizeof(hs), "%08x", hv);
			else
				goto fail;

			if ( hv == wv || *ws == '-' || *hs == '-' )
				snprintf(ds, sizeof(ds), "--------");
			else
				snprintf(ds, sizeof(ds), "%08x", wv ^ hv);

			LOG_FOCUS("0x%04x  %8s  %8s  %8s\n", cur, ws, hs, ds);

			cur += sizeof(wv);
		}

		while ( mqueue_dequeue(&queue) )
			;

		cleanup_growlists(&v49);
		walk++;
	}


	LOG_FOCUS("\n-----\nAll tests completed as expected\n");
	ret = 0;

fail:
	if ( ret < 0 )
		LOG_FOCUS("Test error: %s\n", strerror(errno));
	mbuf_deref(format);
	mbuf_deref(parse);
	return ret ? 1 : 0;
}
