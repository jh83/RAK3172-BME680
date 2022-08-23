#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include "LoraMessage.h"
/*************************************

   LoRaWAN band setting:
     RAK_REGION_EU433
     RAK_REGION_CN470
     RAK_REGION_RU864
     RAK_REGION_IN865
     RAK_REGION_EU868
     RAK_REGION_US915
     RAK_REGION_AU915
     RAK_REGION_KR920
     RAK_REGION_AS923

 *************************************/
#define OTAA_BAND     (RAK_REGION_EU868)
#define OTAA_DEVEUI   {0x10, 0xB1, 0xD5, 0x7E, 0xD2, 0x07, 0x3C, 0xF3}
#define OTAA_APPEUI   {0x11, 0x11, 0x11, 0x00, 0x00, 0x00, 0x22, 0x11}
#define OTAA_APPKEY   {0xAA, 0xAA, 0xAA, 0x12, 0xFA, 0x10, 0x98, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x7B, 0x5F, 0x7C, 0x12}

int otaaPeriod =  10; // in minutes
uint8_t greenLed = PA10; // LED Connected on PIN
uint8_t redLed = PB2; // LED Connected on PIN

/** BME680 instance for Wire */
Adafruit_BME680 bme(&Wire);

// Init BME680 function
bool init_BME680(void)
{
  Wire.begin();

  if (!bme.begin(0x76))
  {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    return false;
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  return true;
}

void recvCallback(SERVICE_LORA_RECEIVE_T * data)
{
  if (data->BufferSize > 0) {
    Serial.println("Something received!");

    digitalWrite(greenLed, data->Buffer[0]); // Light the green led
    digitalWrite(redLed, data->Buffer[1]); // Light the red led

    otaaPeriod = data->Buffer[2];
    Serial.printf("Uplink period is %ums\r\n", otaaPeriod * 60000);

    for (int i = 0; i < data->BufferSize; i++) {
      Serial.printf("%x", data->Buffer[i]);
    }
    Serial.print("\r\n");

    delay(10000);
    // Echo the received settings back thru Uplink on LoRaWAN port 2
    if (api.lorawan.send(data->BufferSize, (uint8_t *) data->Buffer, 2, true, 1)) {
      Serial.println("Sending is requested");
    } else {
      Serial.println("Sending failed");
    }
  }
}

void joinCallback(int32_t status)
{
  Serial.printf("Join status: %d\r\n", status);
}

void sendCallback(int32_t status)
{
  if (status == 0) {
    Serial.println("Successfully sent");
  } else {
    Serial.println("Sending failed");
  }
}

void setup()
{
  Serial.begin(115200, RAK_AT_MODE);

  Serial.println("LoRaWAN BME680 Environmental sensor");
  Serial.println("------------------------------------------------------");

  // Define LED pins as output
  pinMode(greenLed, OUTPUT);
  pinMode(redLed, OUTPUT);

  // Init i2c communication with BME680
  bool initStatus = init_BME680();
  Serial.print("init_BME680(): ");
  Serial.println(initStatus, DEC);

  // OTAA Device EUI MSB first
  uint8_t node_device_eui[8] = OTAA_DEVEUI;
  // OTAA Application EUI MSB first
  uint8_t node_app_eui[8] = OTAA_APPEUI;
  // OTAA Application Key MSB first
  uint8_t node_app_key[16] = OTAA_APPKEY;

  if (!api.lorawan.appeui.set(node_app_eui, 8)) {
    Serial.printf("LoRaWan OTAA - set application EUI is incorrect! \r\n");
    return;
  }
  if (!api.lorawan.appkey.set(node_app_key, 16)) {
    Serial.printf("LoRaWan OTAA - set application key is incorrect! \r\n");
    return;
  }
  if (!api.lorawan.deui.set(node_device_eui, 8)) {
    Serial.printf("LoRaWan OTAA - set device EUI is incorrect! \r\n");
    return;
  }

  if (!api.lorawan.band.set(OTAA_BAND)) {
    Serial.printf("LoRaWan OTAA - set band is incorrect! \r\n");
    return;
  }
  if (!api.lorawan.deviceClass.set(RAK_LORA_CLASS_A)) {
    Serial.printf("LoRaWan OTAA - set device class is incorrect! \r\n");
    return;
  }
  if (!api.lorawan.njm.set(RAK_LORA_OTAA))	// Set the network join mode to OTAA
  {
    Serial.printf
    ("LoRaWan OTAA - set network join mode is incorrect! \r\n");
    return;
  }
  if (!api.lorawan.join())	// Join to Gateway
  {
    Serial.printf("LoRaWan OTAA - join fail! \r\n");
    return;
  }

  /** Wait for Join success */
  while (api.lorawan.njs.get() == 0) {
    Serial.print("Wait for LoRaWAN join...");
    api.lorawan.join();
    delay(10000);
  }

  if (!api.lorawan.adr.set(true)) {
    Serial.printf
    ("LoRaWan OTAA - set adaptive data rate is incorrect! \r\n");
    return;
  }
  if (!api.lorawan.rety.set(1)) {
    Serial.printf("LoRaWan OTAA - set retry times is incorrect! \r\n");
    return;
  }
  if (!api.lorawan.cfm.set(1)) {
    Serial.printf("LoRaWan OTAA - set confirm mode is incorrect! \r\n");
    return;
  }

  /** Check LoRaWan Status*/
  Serial.printf("Duty cycle is %s\r\n", api.lorawan.dcs.get() ? "ON" : "OFF");	// Check Duty Cycle status
  Serial.printf("Packet is %s\r\n", api.lorawan.cfm.get() ? "CONFIRMED" : "UNCONFIRMED");	// Check Confirm status
  uint8_t assigned_dev_addr[4] = { 0 };
  api.lorawan.daddr.get(assigned_dev_addr, 4);
  Serial.printf("Device Address is %02X%02X%02X%02X\r\n", assigned_dev_addr[0], assigned_dev_addr[1], assigned_dev_addr[2], assigned_dev_addr[3]);	// Check Device Address
  Serial.printf("Uplink period is %ums\r\n", otaaPeriod * 60000);
  Serial.println("");
  api.lorawan.registerRecvCallback(recvCallback);
  api.lorawan.registerJoinCallback(joinCallback);
  api.lorawan.registerSendCallback(sendCallback);

  delay(1000);

  // Send a uplink message
  uplink_routine();
}

void uplink_routine()
{
  
  bme.beginReading();
  
  time_t wait_start = millis();
  bool read_success = false;
  while ((millis() - wait_start) < 5000)
  {
    if (bme.endReading())
    {
      read_success = true;
    }
  }

  LoraMessage message;

  if (read_success) {
    message.addTemperature(bme.temperature);
    message.addHumidity(bme.humidity);
    message.addRawFloat(bme.pressure);
    message.addRawFloat(bme.gas_resistance);

    Serial.print("Temperature: ");
    Serial.println(bme.temperature);
    Serial.print("Humidity: ");
    Serial.println(bme.humidity);
    Serial.print("Pressure: ");
    Serial.println(bme.pressure / 100.0);
    Serial.print("Gas Resistance: ");
    Serial.println(bme.gas_resistance / 1000.0);
  }

  /** Send the data package */
  if (api.lorawan.send(message.getLength(), message.getBytes(), 1, true, 1)) {
    Serial.println("Sending is requested");
  } else {
    Serial.println("Sending failed");
  }
}

void loop()
{
  static uint64_t last = 0;
  static uint64_t elapsed;

  if ((elapsed = millis() - last) > otaaPeriod * 60000) {
    uplink_routine();
    last = millis();
  }
  //Serial.printf("Try sleep %ums..", OTAA_PERIOD);
  api.system.sleep.all(otaaPeriod * 60000);
  //Serial.println("Wakeup..");
}
