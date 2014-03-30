#include "Cosa/Trace.hh"
#include "Cosa/OWI/Driver/DS18B20.hh"
#include "Cosa/IOStream/Driver/UART.hh"
#include "Cosa/Watchdog.hh"
#include "Cosa/RTC.hh"

// Configuration; network and device addresses
#define NETWORK 0xC05A
#define DEVICE 0x01
#define USE_VWI

#include "Cosa/Wireless/Driver/VWI.hh"
#include "Cosa/Wireless/Driver/VWI/Codec/VirtualWireCodec.hh"
VirtualWireCodec codec;
#define SPEED 4000
VWI rf(NETWORK, DEVICE, SPEED, Board::D7, Board::D8, &codec);

void setup()
{
  uart.begin(9600);
  trace.begin(&uart, PSTR("CosaWirelessReceiver: started"));
  Watchdog::begin();
  RTC::begin();
  rf.begin();
}

struct dt_msg_t {
  uint8_t nr;
  int16_t temp;
  int16_t door;
  uint16_t battery;
};
static const uint8_t DIGITAL_TEMPERATURE_TYPE = 0x02;

IOStream& operator<<(IOStream& outs, dt_msg_t* msg)
{
  outs << PSTR("nr=") << msg->nr
       << PSTR(",temp=");
  DS18B20::print(outs, msg->temp);
  outs << PSTR(",door=") << msg->door
       << PSTR(",battery=") << msg->battery
       << endl;
  return (outs);
}

void loop()
{
  // Receive a message
  const uint32_t TIMEOUT = 15000;
  const uint8_t MSG_MAX = 32;
  uint8_t msg[MSG_MAX];
  uint8_t src;
  uint8_t port;
  int count = rf.recv(src, port, msg, sizeof(msg), TIMEOUT);

  // Print the message header
  if (count >= 0) {
    trace << PSTR("src=") << hex << src 
	  << PSTR(",port=") << hex << port
	  << PSTR(",dest=") 
	  << hex << (rf.is_broadcast() ? 0 : rf.get_device_address())
	  << PSTR(",len=") << count
#if defined(__COSA_WIRELESS_DRIVER_CC1101_HH__)
	  << PSTR(",rssi=") << rf.get_input_power_level() 
	  << PSTR(",lqi=") << rf.get_link_quality_indicator()
#endif
	  << PSTR(":");

    // Print the message payload according to port/message type
    switch (port) {
    case DIGITAL_TEMPERATURE_TYPE: 
      trace << (dt_msg_t*) msg; 
      break;
    default:
      trace << PSTR("msg=");
      trace.print(msg, count, IOStream::hex);
    }
  }

  // Check error codes
  else if (count == -1) {
    trace << PSTR("error:illegal frame size(-1)\n");
  }
  else if (count == -2) {
    trace << PSTR("error:timeout(-2)\n");
  }
  else if (count < 0) {
    trace << PSTR("error(") << count << PSTR(")\n");
  }
}
