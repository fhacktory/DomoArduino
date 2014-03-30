#include "Cosa/Pins.hh"
#include "Cosa/OWI.hh"
#include "Cosa/OWI/Driver/DS18B20.hh"
#include "Cosa/Power.hh"
#include "Cosa/Watchdog.hh"
#include "Cosa/RTC.hh"

#define NETWORK 0xC05A
#define DEVICE 0x30

#include "Cosa/Wireless/Driver/VWI.hh"
#include "Cosa/Wireless/Driver/VWI/Codec/VirtualWireCodec.hh"
VirtualWireCodec codec;
#define SPEED 4000
VWI rf(NETWORK, DEVICE, SPEED, Board::D1, Board::D0, &codec);

OWI owi(Board::D3);
DS18B20 temp(&owi);

OutputPin pw(Board::D4);
InputPin door(Board::D2);

void setup()
{
  Watchdog::begin(128, SLEEP_MODE_PWR_DOWN);
  RTC::begin();

  rf.begin();
  rf.powerdown();

  pw.on();
  temp.connect(0);
  pw.off();

  Power::all_disable();
}

struct dt_msg_t {
  uint8_t nr;
  int16_t temp;
  int16_t door;
  uint16_t battery;
};
static const uint8_t DIGITAL_TEMP_TYPE = 0x02;

void loop()
{
  static uint8_t nr = 0;

  pw.on();
  DS18B20::convert_request(&owi, 12, true);
  temp.read_scratchpad();
  pw.off();
 
  Power::all_enable();

  dt_msg_t msg;
  msg.nr = nr++;
  msg.temp = temp.get_temperature();
  msg.door = door.read();
  msg.battery = AnalogPin::bandgap(1100);

  for (int i=0; i<3; i++) {
    rf.broadcast(DIGITAL_TEMP_TYPE, &msg, sizeof(msg));
    Watchdog::delay(1);
  }
  
  rf.powerdown();

  Power::all_disable();
  SLEEP(5);
}
