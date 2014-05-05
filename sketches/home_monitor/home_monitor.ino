#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_TSL2561_U.h>

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>

#include <bumble_proto.h>


// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed but DI
                                         
Adafruit_CC3000_Client client;
                                         
#define CONNECTION_TIMEOUT (15L * 1000L)

#define WLAN_SSID       ""        // cannot be longer than 32 characters!
#define WLAN_PASS       ""
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

// sensorsjjj
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);


/**************************************************************************/
/*
    Arduino setup function (automatically called at startup)
*/
/**************************************************************************/
void setup(void) 
{
  Serial.begin(9600);
  if (!Serial)
  {
    while(1);
  }
  
  // configure sensors
  
  configure_bmp();
  configure_tsl();
  
  /* We're ready to go! */
  Serial.println("");
}

/**************************************************************************/
/*
    Displays some basic information on this sensor from the unified
    sensor API sensor_t type (see Adafruit_Sensor for more information)
*/
/**************************************************************************/
static void configure_bmp(void)
{
  /* Initialise the sensor */
  if(!bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    Serial.print(F("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!"));
    while(1);
  }
}

/**************************************************************************/
/*
    Configures the gain and integration time for the TSL2561
*/
/**************************************************************************/
static void configure_tsl(void)
{
  /* Initialise the sensor */
  if(!tsl.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.print(F("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!"));
    while(1);
  }
  
  /* You can also manually set the gain or enable auto-gain support */
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  
  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
//  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
//  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  /* Update these values depending on what you've set above! */
  Serial.println(F("TSL configuration"));
  Serial.println(F("------------------------------------"));
  Serial.print  (F("Gain:         ")); Serial.println(F("Auto"));
  Serial.print  (F("Timing:       ")); Serial.println(F("101 ms"));
  Serial.println(F("------------------------------------"));
}

static void configure_cc3000(void)
{
  /* Initialise the module */
  Serial.println(F("\nInitialising the CC3000 ..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
    while(1);
  }
  
  /* Delete any old connection data on the module */
  Serial.println(F("\nDeleting old connection profiles"));
  if (!cc3000.deleteProfiles()) {
    Serial.println(F("Failed!"));
    while(1);
  }
  
  /* Attempt to connect to an access point */
  char *ssid = WLAN_SSID;             /* Max 32 chars */
  Serial.print(F("\nAttempting to connect to ")); Serial.println(ssid);
  
  /* NOTE: Secure connections are not available in 'Tiny' mode! */
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  } 
  
  /* Display the IP address DNS, Gateway, etc. */  
  while (! cc3000_displayConnectionDetails()) {
    delay(1000);
  }
  
  Serial.println(F("Attempting connection..."));
  
  // connect to UDP
  unsigned long ip = cc3000.IP2U32(239, 0, 100, 1),
    curTime,
    startTime = millis();
  
  do {
    client = cc3000.connectUDP(ip, 5354);
    curTime = millis();
  } while (!client.connected() && (curTime - startTime) < CONNECTION_TIMEOUT);
  
  if (curTime - startTime >= CONNECTION_TIMEOUT) {
    Serial.println(F("Connection timeout."));
  }
}

/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool cc3000_displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}


#define RESET_WIFI_COUNT 100
int count = 1;

/**************************************************************************/
/*
    Arduino loop function, called once 'setup' is complete (your own code
    should go here)
*/
/**************************************************************************/
void loop(void) 
{
  configure_cc3000();
  
  acquireBmpEvent();
  acquireTslEvent();
  
  cc3000_send_multicast();
  
  int avail = freeRam();
  Serial.print(F("Free ram: ")); Serial.println(avail);
  
  client.close();
  cc3000.disconnect();
  
  delay(60000);
}

static sensors_event_t bmpEvent;
static sensors_event_t tslEvent;

static void acquireBmpEvent() {
  /* BMP */
  
  bmp.getEvent(&bmpEvent);
 
  /* Display the results (barometric pressure is measure in hPa) */
  if (bmpEvent.pressure)
  {
    /* Display atmospheric pressue in hPa */
    Serial.print(F("Pressure:    "));
    Serial.print(bmpEvent.pressure);
    Serial.println(F(" hPa"));
    
    /* Calculating altitude with reasonable accuracy requires pressure    *
     * sea level pressure for your position at the moment the data is     *
     * converted, as well as the ambient temperature in degress           *
     * celcius.  If you don't have these values, a 'generic' value of     *
     * 1013.25 hPa can be used (defined as SENSORS_PRESSURE_SEALEVELHPA   *
     * in sensors.h), but this isn't ideal and will give variable         *
     * results from one day to the next.                                  *
     *                                                                    *
     * You can usually find the current SLP value by looking at weather   *
     * websites or from environmental information centers near any major  *
     * airport.                                                           *
     *                                                                    *
     * For example, for Paris, France you can check the current mean      *
     * pressure and sea level at: http://bit.ly/16Au8ol                   */
     
    /* First we get the current temperature from the BMP085 */
    float temperature;
    bmp.getTemperature(&temperature);
    Serial.print(F("Temperature: "));
    Serial.print(temperature);
    Serial.println(F(" C"));
  }
  else
  {
    Serial.println(F("BMP sensor error"));
  }
}

static void acquireTslEvent() {
  /* TSL */
  tsl.getEvent(&tslEvent);
 
  /* Display the results (light is measured in lux) */
  if (tslEvent.light)
  {
    Serial.print(tslEvent.light); Serial.println(F(" lux"));
  }
  else
  {
    /* If event.light = 0 lux the sensor is probably saturated
       and no reliable data could be generated! */
    Serial.println(F("Light sensor overload"));
  }
}

bumble_t *packet = create_bumble_packet(3);

static void cc3000_send_multicast(void)
{
  
  packet->items[0].type = BUMBLE_SENSOR_TYPE_TEMPERATURE;
  packet->items[0].data_type = BUMBLE_ITEM_FLOAT;
  bmp.getTemperature(&(packet->items[0].data.f));
  
  packet->items[1].type = BUMBLE_SENSOR_TYPE_BAROMETRIC;
  packet->items[1].data_type = BUMBLE_ITEM_FLOAT;
  packet->items[1].data.f = bmpEvent.pressure;
  
  packet->items[2].type = BUMBLE_SENSOR_TYPE_LUMINOSITY;
  packet->items[2].data_type = BUMBLE_ITEM_FLOAT;
  packet->items[2].data.f = tslEvent.light;
  
  if (client.connected()) {
    // assemble and send packet
    size_t packet_size = sizeof_bumble_packet(packet);
    client.write(packet, packet_size, 0);
    
    Serial.println(F("Packet sent!"));
  }
  else {
    Serial.println(F("Sent failed!"));
  }
  
//  destroy_bumble_packet(packet);
}

static int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
