/*
    This sketch shows the Ethernet event usage and Read modbus data over TCP/IP
    Ethernet  = LAN8720
    Modbus Server  =  IO-8AIIS-E ( analog output )
    Modbus Client  = ESP32
*/

// Important to be defined BEFORE including ETH.h for ETH.begin() to work.

//  RMII LAN8720
#define ETH_PHY_TYPE        ETH_PHY_LAN8720
#define ETH_PHY_ADDR         1
#define ETH_PHY_MDC         33
#define ETH_PHY_MDIO        32
#define ETH_PHY_POWER       -1
#define ETH_CLK_MODE        ETH_CLOCK_GPIO0_IN

#include <ETH.h>                  // Including the ETH.h library after defining settings
#include <ModbusIP_ESP8266.h>

ModbusIP mb;  //ModbusIP object
IPAddress IP(192, 168, 1, 12);  // Address of Modbus Slave (Server) device
uint16_t res;

// Register addresses
const uint16_t REG_ANALOG_INPUT_START = 1;
const uint16_t REG_ANALOG_INPUT_COUNT = 8;


static bool eth_connected = false;

// WARNING: WiFiEvent is called from a separate FreeRTOS task (thread)!, no need create seperate rtos in setup, i tested , while running, if esp32 is disconnect, this function will call, if ethernet is connected it will again run Modbus data transmission 
void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");                                // Set the hostname after interface starts but before DHCP.
      ETH.setHostname("esp32-ethernet");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.println("ETH Got IP");
      Serial.print("IP Address: ");
      Serial.println(ETH.localIP());
      Serial.print("Subnet Mask: ");
      Serial.println(ETH.subnetMask());
      Serial.print("Gateway: ");
      Serial.println(ETH.gatewayIP());
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

void setup()
{
  Serial.begin(115200);
  WiFi.onEvent(WiFiEvent);                                        // Register WiFiEvent callback for Ethernet events
  ETH.begin();                                                    // Initialize Ethernet
  mb.client();
  mb.connect(IP);
  if (mb.isConnected(IP)) {                                        // Check if connection to Modbus Slave is established
        Serial.println("Modbus is connected\n");
        readModbusRegisters();
  }
}

void loop()
{
  Serial.println();
  if (mb.isConnected(IP)) {                                        // Check if connection to Modbus Slave is established
        //Serial.println("Modbus is connected\n");
        readModbusRegisters();
  }else {
        mb.connect(IP);                                             // Try to connect if no connection
        Serial.println("Modbus is connecting.......\n");
        delay(1000);
  }
}

void readModbusRegisters() {                                            // Read analog inputs
  
  for (uint16_t i = 0; i < REG_ANALOG_INPUT_COUNT; ++i) {
    uint16_t address = REG_ANALOG_INPUT_START + i;
    uint16_t Data = mb.readHreg(IP , address, &res);
    mb.task();                                                          // Common local Modbus task
    delay(100);                                                         // Pulling interval
    waitForReadModbusTCPIP(Data, "Analog Input " + String(i + 1));

  }
}


void waitForReadModbusTCPIP(uint16_t Data, const String& registerName) {  // Function to handle Modbus transaction and print results
  
  while (mb.isTransaction(Data)) {
    mb.task();
    delay(1000);
  }
  if (Data) {
    Serial.printf("%s: %u\n", registerName.c_str(), res);
  } else {
    Serial.printf("Failed to read %s\n", registerName.c_str());
  }
}
