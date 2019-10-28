#include "Adafruit_DAP.h"

#define SWDIO 11
#define SWCLK 12
#define SWRST 13

#define BUFSIZE   (16*1024)

// buffer should be word algined for STM32
uint8_t buf[BUFSIZE]  __attribute__ ((aligned(4)));

//create a DAP for programming Atmel SAM devices
Adafruit_DAP_STM32 dap;

// Function called when there's an SWD error
void error(const char *text) {
  Serial.println(text);
  while (1);
}

void print_memory(uint32_t addr, uint8_t* buffer, uint32_t bufsize)
{
  memset(buffer, 0xff, bufsize);
  dap.dap_read_block(addr, buffer, bufsize);

  for(int i=0; i < bufsize; i++)
  {
    if (i % 16 == 0) 
    {
      if ( i != 0 ) Serial.println();
      // print offset
      if ( i < 0x100 ) Serial.print("0");
      if ( i < 0x10  ) Serial.print("0");
      Serial.print(i, HEX);
      Serial.print(": ");
    }

    if ( buffer[i] < 0x10 ) Serial.print("0");
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();  
}

void setup() {
  pinMode(13, OUTPUT);
  Serial.begin(115200);
  while(!Serial) {
    delay(1);         // will pause the chip until it opens serial console
  }

  dap.begin(SWCLK, SWDIO, SWRST, &error);

  Serial.print("Connecting...");
  if (! dap.dap_disconnect())                      error(dap.error_message);

  char debuggername[100];
  if (! dap.dap_get_debugger_info(debuggername))   error(dap.error_message);
  Serial.print(debuggername); Serial.print("\n\r");

  if (! dap.dap_connect())                         error(dap.error_message);

  if (! dap.dap_transfer_configure(0, 128, 128))   error(dap.error_message);
  if (! dap.dap_swd_configure(0))                  error(dap.error_message);
  if (! dap.dap_reset_link())                      error(dap.error_message);
  if (! dap.dap_swj_clock(50))                     error(dap.error_message);
  dap.dap_target_prepare();

  uint32_t dsu_did;
  if (! dap.select(&dsu_did)) {
    error("No STM32 device found!");
  }

  Serial.print("Found Target\t");
  Serial.println(dap.target_device.name);
  Serial.print("Flash size\t");
  Serial.print(dap.target_device.flash_size / 1024);
  Serial.println(" KBs");
  Serial.print("Flash pages\t");
  Serial.println(dap.target_device.n_pages);

  uint32_t start_ms, duaration;;
  
  Serial.print("Erasing... ");
  
  start_ms = millis();
  dap.erase();
  duaration = millis()-start_ms;
  
  Serial.print(" done in ");
  Serial.print(duaration);
  Serial.println(" ms");

  print_memory(0, buf, sizeof(buf));  

  Serial.print("Programming 4K ... ");

  // prepare data
  for(int i=0; i<sizeof(buf); i++) buf[i] = i;

  start_ms = millis();

  uint32_t addr = 0;
  dap.programBlock(addr, buf, sizeof(buf));
  addr += 4096;

  duaration = millis()-start_ms;

  Serial.print("done in ");
  Serial.print(duaration);
  Serial.println(" ms");

  Serial.print("Speed ");
  Serial.print((double) sizeof(buf)/(duaration*1.024) );
  Serial.println(" KBs/s");


  // read back
  print_memory(0, buf, sizeof(buf));

  dap.deselect();
  dap.dap_disconnect();
}

void loop() {
  //blink led on the host to show we're done
  digitalWrite(13, HIGH);
  delay(500);
  digitalWrite(13, LOW);
  delay(500);
}
