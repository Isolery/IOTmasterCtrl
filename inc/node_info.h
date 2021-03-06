#ifndef _NODE_INFO_H_
#define _NODE_INFO_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>     

struct replyPollPacket {    
    char ID[8]; // protocol ID = "Art-Net";
    uint16_t OpCode; // == OpPollReply
    uint8_t IPAddr[4]; // 0 if not yet configured
    uint16_t Port;

    uint8_t VersionInfoHi; // The node's current FIRMWARE VERS hi
    uint8_t VersionInfoLo; // The node's current FIRMWARE VERS lo

    uint8_t NetSwitch; // Bits 14-8 of the 15 bit universe number are encoded into the bottom 7 bits of this field.
    // This is used in combination with SubSwitch and Swin[] or Swout[] to produce the full universe address.
    uint8_t SubSwitch; // Bits 7-4 of the 15 bit universe number are encoded into the bottom 4 bits of this field.
    // This is used in combination with NetSwitch and Swin[] or Swout[] to produce the full universe address.

    uint16_t Oem; // Manufacturer code, bit 15 set if
    // extended features avail

    uint8_t UbeaVersion; // Firmware version of UBEA

    uint8_t Status;
    // bit 0 = 0 UBEA not present
    // bit 0 = 1 UBEA present
    // bit 1 = 0 Not capable of RDM (Uni-directional DMX)
    // bit 1 = 1 Capable of RDM (Bi-directional DMX)
    // bit 2 = 0 Booted from flash (normal boot)
    // bit 2 = 1 Booted from ROM (possible error condition)
    // bit 3 = Not used
    // bit 54 = 00 Universe programming authority unknown
    // bit 54 = 01 Universe programming authority set by front panel controls
    // bit 54 = 10 Universe programming authority set by network
    // bit 76 = 00 Indicators Normal
    // bit 76 = 01 Indicators Locate
    // bit 76 = 10 Indicators Mute

    uint8_t EstaMan[2];      // ESTA manufacturer id, lo byte

    char ShortName[18]; // short name defaults to IP 
    char LongName[64];   // long name
    char NodeReport[64]; // Text feedback of Node status or errors
    //  also used for debug info

    uint8_t NumPortsHi; // 0
    uint8_t NumPortsLo; // 4 If num i/p ports is dif to output ports, return biggest

    uint8_t PortTypes[4];
    // bit 7 is output
    // bit 6 is input
    // bits 0-5 are protocol number (0= DMX, 1=MIDI)
    // for DMX-Hub ={0xc0,0xc0,0xc0,0xc0};

    uint8_t GoodInput[4];
    // bit 7 is data received
    // bit 6 is data includes test packets
    // bit 5 is data includes SIP's
    // bit 4 is data includes text
    // bit 3 set is input is disabled
    // bit 2 is receive errors
    // bit 1-0 not used, transmitted as zero.
    // Don't test for zero!

    uint8_t GoodOutput[4];
    // bit 7 is data is transmitting
    // bit 6 is data includes test packets
    // bit 5 is data includes SIP's
    // bit 4 is data includes text
    // bit 3 output is merging data.
    // bit 2 set if DMX output short detected on power up
    // bit 1 set if DMX output merge mode is LTP
    // bit 0 not used, transmitted as zero.

    uint8_t SwIn[4];
    // Bits 3-0 of the 15 bit universe number are encoded into the low nibble
    // This is used in combination with SubSwitch and NetSwitch to produce the full universe address.
    // THIS IS FOR INPUT - ART-NET or DMX
    // NB ON ART-NET II THESE 4 UNIVERSES WILL BE UNICAST TO.

    uint8_t SwOut[4];
    // Bits 3-0 of the 15 bit universe number are encoded into the low nibble
    // This is used in combination with SubSwitch and NetSwitch to produce the full universe address.
    // data belongs
    // THIS IS FOR OUTPUT - ART-NET or DMX.
    // NB ON ART-NET II THESE 4 UNIVERSES WILL BE UNICAST TO.

    uint8_t SwVideo;
    // Low nibble is the value of the video
    // output channel

    uint8_t SwMacro;
    // Bit 0 is Macro input 1
    // Bit 7 is Macro input 8

    uint8_t SwRemote;
    // Bit 0 is Remote input 1
    // Bit 7 is Remote input 8

    uint8_t Spare1; // Spare, currently zero
    uint8_t Spare2; // Spare, currently zero
    uint8_t Spare3; // Spare, currently zero
    uint8_t Style; // Set to Style code to describe type of equipment

    uint8_t Mac[6]; // Mac Address, zero if info not available

    uint8_t BindIp[4]; // If this unit is part of a larger or modular product, this is the IP of the root device.
    uint8_t BindIndex; // Set to zero if no binding, otherwise this number represents the order of bound devices. A lower number means closer to root device.

    uint8_t Status2;
    // bit 0 = 0 Node does not support web browser
    // bit 0 = 1 Node supports web browser configuration

    // bit 1 = 0 Node's IP address is manually configured
    // bit 1 = 1 Node's IP address is DHCP configured

    // bit 2 = 0 Node is not DHCP capable
    // bit 2 = 1 Node is DHCP capable

    // bit 2-7 not implemented, transmit as zero

    uint8_t Filler[26]; // Filler bytes, currently zero.
}; 

#if defined(__cplusplus)
}
#endif

#endif