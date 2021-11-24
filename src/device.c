#include <stdio.h> 
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>  
#include <net/if_arp.h>  
#include <netinet/in.h>  
#include <pthread.h>

//#define HOST_IPADDR        "192.168.0.224"

#define PORT               18378
#define ARTNET_PORT        6454

#define OpPoll             0x2000  // This is an ArtPoll packet, no other data is contained in this UDP packet
#define OpPollReply        0x2100  // This is an ArtPollReply Packet. It contains device status information.
#define OpDiagData         0x2300  // Diagnostics and data logging packet.
#define OpCommand          0x2400  // Used to send text based parameter commands.
#define OpOutput           0x5000  // This is an ArtDmx data packet. It contains zero start code DMX512 information for a single Universe.
#define OpDmx              0x5000  // This is an ArtDmx data packet. It contains zero start code DMX512 information for a single Universe.
#define OpNzs              0x5100  // This is an ArtNzs data packet. It contains non-zero start code (except RDM) DMX512 information for a single Universe.
#define OpAddress          0x6000  // This is an ArtAddress packet. It contains remote programming information for a Node.
#define OpInput            0x7000  // This is an ArtInput packet. It contains enable – disable data for DMX inputs.
#define OpTodRequest       0x8000  // This is an ArtTodRequest packet. It is used to request a Table of Devices (ToD) for RDM discovery.
#define OpTodData          0x8100  // This is an ArtTodData packet. It is used to send a Table of Devices (ToD) for RDM discovery.
#define OpTodControl       0x8200  // This is an ArtTodControl packet. It is used to send RDM discovery control messages.
#define OpRdm              0x8300  // This is an ArtRdm packet. It is used to send all non discovery RDM messages.
#define OpRdmSub           0x8400  // This is an ArtRdmSub packet. It is used to send compressed, RDM Sub-Device data.
#define OpVideoSetup       0xa010  // This is an ArtVideoSetup packet. It contains video screen setup information for nodes that implement the extended video features.
#define OpVideoPalette     0xa020  // This is an ArtVideoPalette packet. It contains colour palette setup information for nodes that implement the extended video features.
#define OpVideoData        0xa040  // This is an ArtVideoData packet. It contains display data for nodes that implement the extended video features.
#define OpMacMaster        0xf000  // This is an ArtMacMaster packet. It is used to program the Node’s MAC address, Oem device type and ESTA manufacturer code. This is for factory initialisation of a Node. It is not to be used by applications.
#define OpMacSlave         0xf100  // This is an ArtMacSlave packet. It is returned by the node to acknowledge receipt of an ArtMacMaster packet.
#define OpFirmwareMaster   0xf200  // This is an ArtFirmwareMaster packet. It is used to upload new firmware or firmware extensions to the Node.
#define OpFirmwareReply    0xf300  // This is an ArtFirmwareReply packet. It is returned by the node to acknowledge receipt of an ArtFirmwareMaster packet or ArtFileTnMaster packet.
#define OpFileTnMaster     0xf400  // Uploads user file to node.
#define OpFileFnMaster     0xf500  // Downloads user file from node.
#define OpFileFnReply      0xf600  // Node acknowledge for downloads.
#define OpIpProg           0xf800  // This is an ArtIpProg packet. It is used to reprogramme the IP, Mask and Port address of the Node.
#define OpIpProgReply      0xf900  // This is an ArtIpProgReply packet. It is returned by the node to acknowledge receipt of an ArtIpProg packet.
#define OpMedia            0x9000  // This is an ArtMedia packet. It is Unicast by a Media Server and acted upon by a Controller.
#define OpMediaPatch       0x9100  // This is an ArtMediaPatch packet. It is Unicast by a Controller and acted upon by a Media Server.
#define OpMediaControl     0x9200  // This is an ArtMediaControl packet. It is Unicast by a Controller and acted upon by a Media Server.
#define OpMediaContrlReply 0x9300  // This is an ArtMediaControlReply packet. It is Unicast by a Media Server and acted upon by a Controller.
#define OpTimeCode         0x9700  // This is an ArtTimeCode packet. It is used to transport time code over the network.
#define OpTimeSync         0x9800  // Used to synchronise real time date and clock
#define OpTrigger          0x9900  // Used to send trigger macros
#define OpDirectory        0x9a00  // Requests a node's file list
#define OpDirectoryReply   0x9b00  // Replies to OpDirectory with file list

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

typedef struct 
{
	char id[8];
	uint16_t opCode; 
	uint16_t protver;
	uint8_t sequence;
	uint8_t physical;
	uint16_t universe_addr;
	uint16_t universe_len;
	uint8_t data[512];
}art_dmx_t;

art_dmx_t art_dmx;

uint8_t send_poll1[] = {0x4d, 0x52, 0x4b, 0x4a, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t send_poll2[] = {0x4d, 0x52, 0x4b, 0x4a, 0xff, 0xff, 0x00, 0x0e, 0x00, 0x10, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00};

uint8_t frame_header[34][14] = {
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x02, 0x01, 0xb0, 0x05, 0x00, 0x00, 0xdc, 0x04,    
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x01, 0x01, 0xd0, 0x00, 0x01, 0x00, 0xb0, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x02, 0x01, 0x80, 0x06, 0x01, 0x00, 0x0c, 0x04,   
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x01, 0x01, 0xa0, 0x01, 0x02, 0x00, 0xb0, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x02, 0x01, 0x50, 0x07, 0x02, 0x00, 0x3c, 0x03,    
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x01, 0x01, 0x70, 0x02, 0x03, 0x00, 0xb0, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x02, 0x01, 0x20, 0x08, 0x03, 0x00, 0x6c, 0x02,    
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x01, 0x01, 0x40, 0x03, 0x04, 0x00, 0xb0, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x02, 0x01, 0xf0, 0x08, 0x04, 0x00, 0x9c, 0x01,    
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x01, 0x01, 0x10, 0x04, 0x05, 0x00, 0xb0, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x02, 0x01, 0xc0, 0x09, 0x05, 0x00, 0xcc, 0x00,    
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x01, 0x01, 0xe0, 0x04, 0x06, 0x00, 0xac, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x07, 0x00, 0xb0, 0x05,    
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x01, 0x01, 0xb0, 0x05, 0x07, 0x00, 0xdc, 0x04,

    0x4d, 0x52, 0x4b, 0x4a, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x02, 0x00, 0x02, 0x01, 0xb0, 0x05, 0x00, 0x00, 0xdc, 0x04,   
    0x4d, 0x52, 0x4b, 0x4a, 0x02, 0x00, 0x01, 0x01, 0xd0, 0x00, 0x01, 0x00, 0xb0, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x02, 0x00, 0x02, 0x01, 0x80, 0x06, 0x01, 0x00, 0x0c, 0x04,
    0x4d, 0x52, 0x4b, 0x4a, 0x02, 0x00, 0x01, 0x01, 0xa0, 0x01, 0x02, 0x00, 0xb0, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x02, 0x00, 0x02, 0x01, 0x50, 0x07, 0x02, 0x00, 0x3c, 0x03,
    0x4d, 0x52, 0x4b, 0x4a, 0x02, 0x00, 0x01, 0x01, 0x70, 0x02, 0x03, 0x00, 0xb0, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x02, 0x00, 0x02, 0x01, 0x20, 0x08, 0x03, 0x00, 0x6c, 0x02,
    0x4d, 0x52, 0x4b, 0x4a, 0x02, 0x00, 0x01, 0x01, 0x40, 0x03, 0x04, 0x00, 0xb0, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x02, 0x00, 0x02, 0x01, 0xf0, 0x08, 0x04, 0x00, 0x9c, 0x01,
    0x4d, 0x52, 0x4b, 0x4a, 0x02, 0x00, 0x01, 0x01, 0x10, 0x04, 0x05, 0x00, 0xb0, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x02, 0x00, 0x02, 0x01, 0xc0, 0x09, 0x05, 0x00, 0xcc, 0x00,
    0x4d, 0x52, 0x4b, 0x4a, 0x02, 0x00, 0x01, 0x01, 0xe0, 0x04, 0x06, 0x00, 0xac, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0x07, 0x00, 0xb0, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x02, 0x00, 0x01, 0x01, 0xb0, 0x05, 0x07, 0x00, 0xdc, 0x04,

    0x4d, 0x52, 0x4b, 0x4a, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x03, 0x00, 0x02, 0x01, 0xb0, 0x05, 0x00, 0x00, 0xdc, 0x04,   
    0x4d, 0x52, 0x4b, 0x4a, 0x03, 0x00, 0x01, 0x01, 0xd0, 0x00, 0x01, 0x00, 0xb0, 0x05,
    0x4d, 0x52, 0x4b, 0x4a, 0x03, 0x00, 0x02, 0x01, 0x80, 0x06, 0x01, 0x00, 0x0c, 0x04
};

char mr_framebuffer[35][1472] = {0};

char rgb_buf[] = {
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00,

    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00,

    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0x00, 0x00
};

//char mrdev_buf[1472] = {0x4d, 0x52, 0x4b, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//char mrdev_buf[1472] = {0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x02, 0x01, 0xb0, 0x05, 0x00, 0x00, 0x50, 0x00};
char mrdev_buf[1472] = {0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x02, 0x01, 0x5c, 0x05, 0x01, 0x00, 0xa4, 0x00};

char framebuffer[96][510];

struct replyPollPacket pollPacket;  
void initPacket(void);

char sendbuf[1024];
char recvbuf[1024];
//char artdmx[530];

typedef struct
{   
    char PollDevice_ACK[200];
    char SwitchMode_ACK[50];
}dev_proctol_t;
dev_proctol_t dev_proctol;

typedef struct  
{   
    uint16_t host_port;
    char ip_adr[15];
    uint8_t mac_adr[6];
}devinfo_t;

devinfo_t devinfo;

struct sockaddr_in host;
struct sockaddr_in artnet_host;

int mr_socket;
struct sockaddr_in mr_local;
struct sockaddr_in mr_dev;

int artnet_sock;
socklen_t len;
int send_length;

pthread_cond_t condv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER; 

// 与主机保持UDP通信(端口号: 18379)
void *thread_contact(void *arg)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in local;

    bzero(&local,sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = inet_addr(devinfo.ip_adr); 
    local.sin_port = htons(PORT);

    if(bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0){
        perror("bind");
        exit(1);
    }

    while(1)
    {
        //recv
        ssize_t s = recvfrom(sock, recvbuf, sizeof(recvbuf)-1, 0, (struct sockaddr*)&host, &len);

        if(s > 0){
            recvbuf[s] = 0;
            printf("host %s: port %d: say# %s \n", inet_ntoa(host.sin_addr), ntohs(host.sin_port), recvbuf);

            //send
            if(strstr(recvbuf, "[LNX001]REQ=SwitchMode&RunMode=ArtNet Node") != NULL){
                memcpy(sendbuf, dev_proctol.SwitchMode_ACK, strlen(dev_proctol.SwitchMode_ACK)+1);
                send_length = sendto(sock, sendbuf, strlen(sendbuf)+1, 0, (struct sockaddr *) &host, len);
                if( send_length < 0){
                    perror("sendto");
                }
                printf("send_data = %s\n", sendbuf);
            }
        }
    }
}

// 向主机发送ArtPollReply(端口号: 6454)
void *thread_artreply(void *arg)
{
    while(1)
    {
        for(int i = 0; i < 96; i++) {
            if(i < 16){
                pollPacket.SwOut[0] = i;
                pollPacket.NetSwitch = 0;
                pollPacket.SubSwitch = 0;
            }else{
                pollPacket.SwOut[0] = i%16;
                pollPacket.NetSwitch = i/256;
                pollPacket.SubSwitch = i/16;
            }

            pollPacket.BindIndex = i+1;

            sprintf(pollPacket.ShortName, "LINETX1.Node%d", i+1);
            memset(sendbuf, 0, sizeof(sendbuf));
            memcpy(sendbuf, &pollPacket, sizeof(pollPacket));

            send_length = sendto(artnet_sock, sendbuf, sizeof(pollPacket)-1, 0, (struct sockaddr *) &artnet_host, len);
            if( send_length < 0){
                perror("sendto");
            }
            //printf("send_length = %d\n", send_length);
        }

        sleep(5);
    }

    return NULL;
}

void *thread_ws2812b(void *arg){
    // sed rgb data to ws2812b...
    while(1)
    {
        pthread_cond_wait(&condv, &mlock);
        //printf("ws2812b working...\n");    

        char *pframebuf = (char *)framebuffer;
        
        uint8_t tmp_data = 0;
        char *temp = (char *)framebuffer;

        //printf("%02x ", (uint8_t)framebuffer[0][0]);

        for(int j=0; j<96; j++)
        {
            for(int i=0; i<510; i++)
            {
                switch(i%3){
                    case 0:
                        tmp_data = temp[i];
                        break;
                    case 1:
                        temp[i-1]= temp[i];
                        temp[i] = tmp_data;
                        tmp_data = 0;
                        break;
                    case 2:
                        break;
                }               
            }   

            temp+=510;
        } 

        //printf("%02x\n", (uint8_t)framebuffer[0][0]);

        //填充mr_framebuffer
        char (*pmrbuf)[1472] = (char (*)[1472])mr_framebuffer;
        int index = 0;

        // 第1帧
        memcpy(((char *)pmrbuf[0] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第2帧
        memcpy(((char *)pmrbuf[1] + 14), (char *)(pframebuf+index), 1244);
        index += 1244;
        memcpy(((char *)pmrbuf[1] + 1244 + 20), (char *)(pframebuf+index), 208);
        index += 208;

        // 第3帧
        memcpy(((char *)pmrbuf[2] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第4帧
        memcpy(((char *)pmrbuf[3] + 14), (char *)(pframebuf+index), 1036);
        index += 1036;
        memcpy(((char *)pmrbuf[3] + 1036 + 20), (char *)(pframebuf+index), 416);
        index += 416;

        // 第5帧
        memcpy(((char *)pmrbuf[4] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第6帧
        memcpy(((char *)pmrbuf[5] + 14), (char *)(pframebuf+index), 828);
        index += 828;
        memcpy(((char *)pmrbuf[5] + 828 + 20), (char *)(pframebuf+index), 624);
        index += 624;

        // 第7帧
        memcpy(((char *)pmrbuf[6] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第8帧
        memcpy(((char *)pmrbuf[7] + 14), (char *)(pframebuf+index), 620);
        index += 620;
        memcpy(((char *)pmrbuf[7] + 620 + 20), (char *)(pframebuf+index), 832);
        index += 832;

        // 第9帧
        memcpy(((char *)pmrbuf[8] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第10帧
        memcpy(((char *)pmrbuf[9] + 14), (char *)(pframebuf+index), 412);
        index += 412;
        memcpy(((char *)pmrbuf[9] + 412 + 20), (char *)(pframebuf+index), 1040);
        index += 1040;

        // 第11帧
        memcpy(((char *)pmrbuf[10] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第12帧
        memcpy(((char *)pmrbuf[11] + 14), (char *)(pframebuf+index), 204);
        index += 204;
        memcpy(((char *)pmrbuf[11] + 204 + 20), (char *)(pframebuf+index), 1248);
        index += 1248;

        // 第13帧
        memcpy(((char *)pmrbuf[12] + 14), (char *)(pframebuf+index), 1452);
        index += 1452;

        // 第14帧
        memcpy(((char *)pmrbuf[13] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第15帧
        memcpy(((char *)pmrbuf[14] + 14), (char *)(pframebuf+index), 1244);
        index += 1244;

        //-----------------------------------------------------------------------------------------------------------------------
        // 第16帧
        memcpy(((char *)pmrbuf[15] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第17帧
        memcpy(((char *)pmrbuf[16] + 14), (char *)(pframebuf+index), 1244);
        index += 1244;
        memcpy(((char *)pmrbuf[16] + 1244 + 20), (char *)(pframebuf+index), 208);
        index += 208;

        // 第18帧
        memcpy(((char *)pmrbuf[17] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第19帧
        memcpy(((char *)pmrbuf[18] + 14), (char *)(pframebuf+index), 1036);
        index += 1036;
        memcpy(((char *)pmrbuf[18] + 1036 + 20), (char *)(pframebuf+index), 416);
        index += 416;

        // 第20帧
        memcpy(((char *)pmrbuf[19] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第21帧
        memcpy(((char *)pmrbuf[20] + 14), (char *)(pframebuf+index), 828);
        index += 828;
        memcpy(((char *)pmrbuf[20] + 828 + 20), (char *)(pframebuf+index), 624);
        index += 624;

        // 第22帧
        memcpy(((char *)pmrbuf[21] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第23帧
        memcpy(((char *)pmrbuf[22] + 14), (char *)(pframebuf+index), 620);
        index += 620;
        memcpy(((char *)pmrbuf[22] + 620 + 20), (char *)(pframebuf+index), 832);
        index += 832;

        // 第24帧
        memcpy(((char *)pmrbuf[23] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第25帧
        memcpy(((char *)pmrbuf[24] + 14), (char *)(pframebuf+index), 412);
        index += 412;
        memcpy(((char *)pmrbuf[24] + 412 + 20), (char *)(pframebuf+index), 1040);
        index += 1040;

        // 第26帧
        memcpy(((char *)pmrbuf[25] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第27帧
        memcpy(((char *)pmrbuf[26] + 14), (char *)(pframebuf+index), 204);
        index += 204;
        memcpy(((char *)pmrbuf[26] + 204 + 20), (char *)(pframebuf+index), 1248);
        index += 1248;

        // 第28帧
        memcpy(((char *)pmrbuf[27] + 14), (char *)(pframebuf+index), 1452);
        index += 1452;

        // 第29帧
        memcpy(((char *)pmrbuf[28] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第30帧
        memcpy(((char *)pmrbuf[29] + 14), (char *)(pframebuf+index), 1244);
        index += 1244;

        //-----------------------------------------------------------------------------------------------------------------------
        // 第31帧
        memcpy(((char *)pmrbuf[30] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第32帧
        memcpy(((char *)pmrbuf[31] + 14), (char *)(pframebuf+index), 1244);
        index += 1244;
        memcpy(((char *)pmrbuf[31] + 1244 + 20), (char *)(pframebuf+index), 208);
        index += 208;

        // 第33帧
        memcpy(((char *)pmrbuf[32] + 14), (char *)(pframebuf+index), 1456);
        index += 1456;

        // 第34帧
        memcpy(((char *)pmrbuf[33] + 14), (char *)(pframebuf+index), 1036);
        index += 1036;
        memcpy(((char *)pmrbuf[33] + 1036 + 20), (char *)(pframebuf+index), 416);
        index += 416;

        for(int i=0; i<34; i++){
            sendto(mr_socket, (char *)mr_framebuffer[i], sizeof(mrdev_buf), 0, (struct sockaddr *) &mr_dev, len);
            usleep(100);
            //printf("sendto\n");
        }

        //----------------------------------------------------------------------
        
        sendto(mr_socket, send_poll1, sizeof(send_poll1), 0, (struct sockaddr *) &mr_dev, len);
        
        usleep(1);
        sendto(mr_socket, send_poll2, sizeof(send_poll2), 0, (struct sockaddr *) &mr_dev, len);
    }
}

struct ifreq buf[2];      /* ifreq结构体  */  
struct ifconf ifc;        /* ifconf结构体 */  

int if_len;     /* 接口数量 */  

int main(void)
{
    /* 建立IPv4的UDP套接字sock */  
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if(sock < 0){
        perror("socket");
        return -1;
    }

    //获取本机ip地址和mac地址
    ifc.ifc_len = sizeof(buf);  
    ifc.ifc_buf = (caddr_t) buf;

    if_len = ifc.ifc_len / sizeof(struct ifreq); /* 接口数量 */  

    if (ioctl(sock, SIOCGIFCONF, (char *)&ifc) == -1){  
        perror("SIOCGIFCONF ioctl");  
        return -1;  
    }  
    
    /* 获取IP地址 */  
    if (!(ioctl(sock, SIOCGIFADDR, (char *) &buf[if_len-1]))){  
        char *ip_adr = inet_ntoa(((struct sockaddr_in*) (&buf[if_len-1].ifr_addr))->sin_addr);
        printf("IP Address:%s\n", ip_adr);  
        memcpy(devinfo.ip_adr, ip_adr, strlen(ip_adr)+1);
    }else{  
        char str[256];  
        sprintf(str, "SIOCGIFADDR ioctl %s", buf[if_len-1].ifr_name);  
        perror(str);  
    }  

    //bind 
    struct sockaddr_in local;

    bzero(&local,sizeof(local));
    local.sin_family = AF_INET;
    //local.sin_addr.s_addr = (((struct sockaddr_in*) &buf[if_len-1].ifr_addr))->sin_addr.s_addr;
    local.sin_addr.s_addr = inet_addr("255.255.255.255");    // 接收广播包
    local.sin_port = htons(PORT);

    if(bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0){
        perror("bind");
        return -1;
    }

    /* 获取MAC地址 */  
    if (!(ioctl(sock, SIOCGIFHWADDR, (char *) &buf[if_len-1]))){  
        devinfo.mac_adr[0] = buf[if_len-1].ifr_hwaddr.sa_data[0];
        devinfo.mac_adr[1] = buf[if_len-1].ifr_hwaddr.sa_data[1];
        devinfo.mac_adr[2] = buf[if_len-1].ifr_hwaddr.sa_data[2];
        devinfo.mac_adr[3] = buf[if_len-1].ifr_hwaddr.sa_data[3];
        devinfo.mac_adr[4] = buf[if_len-1].ifr_hwaddr.sa_data[4];
        devinfo.mac_adr[5] = buf[if_len-1].ifr_hwaddr.sa_data[5];

        printf("MAC Address:%02x:%02x:%02x:%02x:%02x:%02x\n\n", devinfo.mac_adr[0],devinfo.mac_adr[1],devinfo.mac_adr[2],devinfo.mac_adr[3],devinfo.mac_adr[4],devinfo.mac_adr[5]);  
    }else{  
        char str[256];  
        sprintf(str, "SIOCGIFHWADDR ioctl %s", buf[if_len-1].ifr_name);  
        perror(str);  
    }  

    initPacket();
    
    len = sizeof(host);
    int send_length = 0;

    while(1)
    {
        ssize_t s = recvfrom(sock, recvbuf, sizeof(recvbuf)-1, 0, (struct sockaddr*)&host, &len);

        if(s > 0){
                recvbuf[s] = 0;
                printf("host %s: port %d: say# %s \n", inet_ntoa(host.sin_addr), ntohs(host.sin_port), recvbuf);

                if(strstr(recvbuf, "[LNX01]REQ=PollDevice") != NULL){
                    memcpy(sendbuf, dev_proctol.PollDevice_ACK, strlen(dev_proctol.PollDevice_ACK)+1);
                    send_length = sendto(sock, sendbuf, strlen(sendbuf)+1, 0, (struct sockaddr *) &host, len);
                    if( send_length < 0){
                        perror("sendto");
                    }

                    printf("send_data = %s\n", sendbuf);

                    close(sock);

                    pthread_t tid;
                    pthread_create(&tid, NULL, (void *)thread_contact, NULL);

                    break;
                }
        }else{
            printf("recv error!\n");
        }
    }

    //------------------------------------------------------------------------------
    // Art-Net

    artnet_sock = socket(AF_INET, SOCK_DGRAM, 0);

    if(artnet_sock < 0){
        perror("socket");
        return -1;
    }

    //bind 
    struct sockaddr_in artnet_local;

    bzero(&artnet_local,sizeof(artnet_local));
    artnet_local.sin_family = AF_INET;
    artnet_local.sin_addr.s_addr = inet_addr(devinfo.ip_adr);
    artnet_local.sin_port = htons(ARTNET_PORT);

    if(bind(artnet_sock, (struct sockaddr*)&artnet_local, sizeof(artnet_local)) < 0){
        perror("bind");
        return -1;
    }

    memcpy(&artnet_host, &host, sizeof(host));
    artnet_host.sin_port = htons(ARTNET_PORT);

    pthread_t tid1, tid2;
    pthread_create(&tid1, NULL, (void *)thread_artreply, NULL);
    pthread_create(&tid2, NULL, (void *)thread_ws2812b, NULL);

    char (*pbuf)[510] = framebuffer;
    int index = 0;

    // 明瑞分控

    mr_socket = socket(AF_INET, SOCK_DGRAM, 0);

    int iOptval = 1;

    if (setsockopt(mr_socket, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &iOptval, sizeof(int)) < 0){
        printf("setsockopt failed!");
    }

    mr_local.sin_family = AF_INET;
    //mr_local.sin_addr.s_addr = inet_addr(devinfo.ip_adr);
    mr_local.sin_addr.s_addr = inet_addr("199.199.199.199");
    mr_local.sin_port = htons(19794);

    if(bind(mr_socket, (struct sockaddr*)&mr_local, sizeof(mr_local)) < 0){
        perror("bind");
        return -1;
    }

    mr_dev.sin_family = AF_INET;
    inet_aton("255.255.255.255", &mr_dev.sin_addr);
    mr_dev.sin_port = htons(19274);


    while(1)
    {
        ssize_t s = recvfrom(artnet_sock, (unsigned char *)&art_dmx, sizeof(art_dmx), 0, (struct sockaddr*)&artnet_host, &len);

        if(s > 0){
            memcpy(pbuf[index++], (unsigned char *)art_dmx.data, 510);

            if(index == 96){
                //printf("frame ok!\n");
                index = 0;
                //printf("%02x ", (uint8_t)framebuffer[0][0]);

                pthread_cond_signal(&condv);
            }else{
                //...
            }
        }
    }

    return 0;
}

void initPacket(void)
{
	snprintf(dev_proctol.PollDevice_ACK, sizeof(dev_proctol.PollDevice_ACK), 
        "[LNX01]ACK=PollDevice&SerialNum=LN37023b&DevName=LINETX&DevType=LNX-378D&DevDesp=ArtNet LED Controller SDPlay/Record/WEB supported&IP=%s&MAC=%02x:%02x:%02x:%02x:%02x:%02x", 
        devinfo.ip_adr,devinfo.mac_adr[0], devinfo.mac_adr[1], devinfo.mac_adr[2], devinfo.mac_adr[3], devinfo.mac_adr[4], devinfo.mac_adr[5]);

    //printf("%s\n", dev_proctol.PollDevice_ACK);

    snprintf(dev_proctol.SwitchMode_ACK, sizeof(dev_proctol.SwitchMode_ACK), "[LNX001]ACK=SwitchMode&EXEC=OK");

    //ArtPollReply

    uint32_t ipaddr = 0;

    ipaddr = ntohl(inet_addr(devinfo.ip_adr));

    sprintf(pollPacket.ID, "Art-Net");
	pollPacket.OpCode = OpPollReply;

	pollPacket.IPAddr[0] = (uint8_t)(ipaddr >> 24);
	pollPacket.IPAddr[1] = (uint8_t)(ipaddr >> 16);
	pollPacket.IPAddr[2] = (uint8_t)(ipaddr >> 8);
	pollPacket.IPAddr[3] = (uint8_t)(ipaddr >> 0);

	pollPacket.Port = htons(0x3619);
	pollPacket.VersionInfoHi = 0;
	pollPacket.VersionInfoLo = 0;
	pollPacket.NetSwitch = 0;
	pollPacket.SubSwitch = 0;
	pollPacket.Oem = htons(0x3600);
	pollPacket.UbeaVersion = 0;
	pollPacket.Status = 0xd2;
	pollPacket.EstaMan[0] = 0x66;
	pollPacket.EstaMan[1] = 0x37;
	memset(pollPacket.ShortName, 0, sizeof(pollPacket.ShortName));
	sprintf(pollPacket.ShortName, "LINETX1.Node1");
	memset(pollPacket.LongName, 0, sizeof(pollPacket.LongName));
	sprintf(pollPacket.LongName, "ArtNet LED Controller SDPlay/Record/WEB supported");
	memset(pollPacket.NodeReport, 0, sizeof(pollPacket.NodeReport));
    sprintf(pollPacket.NodeReport, "#0x0001[0]power on successful");
	pollPacket.NumPortsHi = 0;
	pollPacket.NumPortsLo = 1;

	memset(pollPacket.PortTypes, 0, sizeof(pollPacket.PortTypes));
    pollPacket.PortTypes[0] = 0x80;
    pollPacket.PortTypes[1] = 0x00;
    pollPacket.PortTypes[2] = 0x00;
    pollPacket.PortTypes[3] = 0x00;

	memset(pollPacket.GoodInput, 0, sizeof(pollPacket.GoodInput));
    pollPacket.GoodInput[0] = 0x08;
    pollPacket.GoodInput[1] = 0x08;
    pollPacket.GoodInput[2] = 0x08;
    pollPacket.GoodInput[3] = 0x08;

	memset(pollPacket.GoodOutput, 0, sizeof(pollPacket.GoodOutput));
    pollPacket.GoodOutput[0] = 0x80;
    pollPacket.GoodOutput[1] = 0x80;
    pollPacket.GoodOutput[2] = 0x80;
    pollPacket.GoodOutput[3] = 0x80;

	memset(pollPacket.SwIn, 0, sizeof(pollPacket.SwIn));
	memset(pollPacket.SwOut, 0, sizeof(pollPacket.SwOut));

	pollPacket.SwVideo = 0;
	pollPacket.SwMacro = 0;
	pollPacket.SwRemote = 0;
	pollPacket.Spare1 = 0;
	pollPacket.Spare2 = 0;
	pollPacket.Spare3 = 0;
	pollPacket.Style = 0;

	pollPacket.Mac[0] = devinfo.mac_adr[0];
	pollPacket.Mac[1] = devinfo.mac_adr[1];
	pollPacket.Mac[2] = devinfo.mac_adr[2];
	pollPacket.Mac[3] = devinfo.mac_adr[3];
	pollPacket.Mac[4] = devinfo.mac_adr[4];
	pollPacket.Mac[5] = devinfo.mac_adr[5];
	
	memset(pollPacket.BindIp, 0, sizeof(pollPacket.BindIp));
    pollPacket.BindIp[0] = (uint8_t)(ipaddr >> 24);
    pollPacket.BindIp[1] = (uint8_t)(ipaddr >> 16);
    pollPacket.BindIp[2] = (uint8_t)(ipaddr >> 8);
    pollPacket.BindIp[3] = (uint8_t)(ipaddr >> 0);

	pollPacket.BindIndex = 1;
	pollPacket.Status2 = 0x09;

	memset(pollPacket.Filler, 0, sizeof(pollPacket.Filler));

    // initial mr_framebuffer
    char (*pbuf)[1472] = (char (*)[1472])mr_framebuffer;
    char (*ph)[14] = (char (*)[14])frame_header;

    for(int i=0; i<34; i++)
    {
        memcpy((char *)pbuf, (char *)ph, 14);
        pbuf++;
        ph++;
    }

    pbuf = (char (*)[1472])mr_framebuffer;

    //-----------------------------------------------------------------------------
    //第2帧
    pbuf[1][1258] = 0x00;
    pbuf[1][1259] = 0x00;
    pbuf[1][1260] = 0x01;
    pbuf[1][1261] = 0x00;
    pbuf[1][1262] = 0xd0;
    pbuf[1][1263] = 0x00;

    //第4帧
    pbuf[3][1050] = 0x00;
    pbuf[3][1051] = 0x00;
    pbuf[3][1052] = 0x02;
    pbuf[3][1053] = 0x00;
    pbuf[3][1054] = 0xa0;
    pbuf[3][1055] = 0x01;

    //第6帧
    pbuf[5][842] = 0x00;
    pbuf[5][843] = 0x00;
    pbuf[5][844] = 0x03;
    pbuf[5][845] = 0x00;
    pbuf[5][846] = 0x70;
    pbuf[5][847] = 0x02;

    //第8帧
    pbuf[7][634] = 0x00;
    pbuf[7][635] = 0x00;
    pbuf[7][636] = 0x04;
    pbuf[7][637] = 0x00;
    pbuf[7][638] = 0x40;
    pbuf[7][639] = 0x03;

    //第10帧
    pbuf[9][426] = 0x00;
    pbuf[9][427] = 0x00;
    pbuf[9][428] = 0x05;
    pbuf[9][429] = 0x00;
    pbuf[9][430] = 0x10;
    pbuf[9][431] = 0x04;

    //第12帧
    pbuf[11][218] = 0x00;
    pbuf[11][219] = 0x00;
    pbuf[11][220] = 0x06;
    pbuf[11][221] = 0x00;
    pbuf[11][222] = 0xe0;
    pbuf[11][223] = 0x04;

    //-----------------------------------------------------------------------------

    //第17帧
    pbuf[16][1258] = 0x00;
    pbuf[16][1259] = 0x00;
    pbuf[16][1260] = 0x01;
    pbuf[16][1261] = 0x00;
    pbuf[16][1262] = 0xd0;
    pbuf[16][1263] = 0x00;

    //第19帧
    pbuf[18][1050] = 0x00;
    pbuf[18][1051] = 0x00;
    pbuf[18][1052] = 0x02;
    pbuf[18][1053] = 0x00;
    pbuf[18][1054] = 0xa0;
    pbuf[18][1055] = 0x01;

    //第21帧
    pbuf[20][842] = 0x00;
    pbuf[20][843] = 0x00;
    pbuf[20][844] = 0x03;
    pbuf[20][845] = 0x00;
    pbuf[20][846] = 0x70;
    pbuf[20][847] = 0x02;

    //第23帧
    pbuf[22][634] = 0x00;
    pbuf[22][635] = 0x00;
    pbuf[22][636] = 0x04;
    pbuf[22][637] = 0x00;
    pbuf[22][638] = 0x40;
    pbuf[22][639] = 0x03;

    //第25帧
    pbuf[24][426] = 0x00;
    pbuf[24][427] = 0x00;
    pbuf[24][428] = 0x05;
    pbuf[24][429] = 0x00;
    pbuf[24][430] = 0x10;
    pbuf[24][431] = 0x04;

    //第27帧
    pbuf[26][218] = 0x00;
    pbuf[26][219] = 0x00;
    pbuf[26][220] = 0x06;
    pbuf[26][221] = 0x00;
    pbuf[26][222] = 0xe0;
    pbuf[26][223] = 0x04;

    //------------------------------------------------------------------
    //第32帧
    pbuf[31][1258] = 0x00;
    pbuf[31][1259] = 0x00;
    pbuf[31][1260] = 0x01;
    pbuf[31][1261] = 0x00;
    pbuf[31][1262] = 0xd0;
    pbuf[31][1263] = 0x00;

    //第34帧
    pbuf[33][1050] = 0x00;
    pbuf[33][1051] = 0x00;
    pbuf[33][1052] = 0x02;
    pbuf[33][1053] = 0x00;
    pbuf[33][1054] = 0xa0;
    pbuf[33][1055] = 0x01;

    for(int i=0; i<34; i++)
    {
        for(int j=0; j<14; j++)
            printf("%02x ", (unsigned char)mr_framebuffer[i][j]);
        printf("\n");
    }
}
