//ADD SSID, Password, Static or DHCP IP, RTU config from Serial to flash(eeprom)

#include <ESP8266WiFi.h>
#include <ModbusTCP.h>
#include <ModbusRTU.h>
#include <SoftwareSerial.h>

SoftwareSerial S;

int DE_RE = 2; //D4  (02) For MAX485 chip

ModbusRTU rtu;
ModbusTCP tcp;

IPAddress srcIp;        //Current Ip address of Modbus Client

uint16_t transRunning = 0;  // Currently executed ModbusTCP transaction
uint8_t slaveRunning = 0;   // Current request slave


/// USER CONFIG ///
uint16_t ModbusBaudRate = 19200; // Set Modbus RTU Baudrate

const char* ip_mode = "STATIC" // Write anything else for DHCP, "STATIC" for STATIC IP 
const char* ssid     = "SSID"; //Set Your WIFI SSID
const char* password = "PASSWORD"; //Set Your WIFI Password

IPAddress local_IP(192, 168, 1, 2); // Set your Static IP address
IPAddress gateway(192, 168, 1, 1); // Set your Gateway IP address
IPAddress subnet(255, 255, 255, 0); // Set your subnet
/// END OF USER CONFIG

bool cbRtuTrans(Modbus::ResultCode event, uint16_t transactionId, void* data) {
    if (event != Modbus::EX_SUCCESS)                  // If transaction got an error
      Serial.printf("ModbusRTU result: %02X, Mem: %d\n\r", event, ESP.getFreeHeap());  // Display Modbus error code (222527)
    if (event == Modbus::EX_TIMEOUT) {    // If Transaction timeout took place zero out values so that state logic works and no response is sent to TCP
      Serial.print("RTU Timeout");
      transRunning = 0;
      slaveRunning = 0;
      srcIp = (0,0,0,0);
    }  
    return true;
}

// Callback receives raw data from TCP Client
Modbus::ResultCode cbTcpRaw(uint8_t* data, uint8_t len, void* custom) {
  auto src = (Modbus::frame_arg_t*) custom;
  
  if (transRunning) { // Note that we can't process new requests from TCP-side while waiting for responce from RTU-side.
    if (srcIp != src->ipaddr) //check that the pending request did not come from the same Client, we don't want to send a response to a pending event
      tcp.setTransactionId(src->transactionId); // Set transaction id as per incoming request
      tcp.errorResponce(src->ipaddr, (Modbus::FunctionCode)data[0], Modbus::EX_SLAVE_DEVICE_BUSY);
    return Modbus::EX_SLAVE_DEVICE_BUSY; //stop frame processing
  }
  
  if (src->slaveId > 247){ //Handle illegal RTU addresses, there is no response from RTU so normal processing does not work
    tcp.setTransactionId(src->transactionId); // Set transaction id as per incoming request
    tcp.errorResponce(src->ipaddr, (Modbus::FunctionCode)data[0], Modbus::EX_DEVICE_FAILED_TO_RESPOND);
    return Modbus::EX_DEVICE_FAILED_TO_RESPOND;
  }

   if (!src->slaveId){
    tcp.setTransactionId(src->transactionId); // Set transaction id as per incoming request
    tcp.errorResponce(src->ipaddr, (Modbus::FunctionCode)data[0], Modbus::EX_ACKNOWLEDGE);
  }

  srcIp = src->ipaddr;
  slaveRunning = src->slaveId; 
  transRunning = src->transactionId;
  
  rtu.rawRequest(slaveRunning, data, len, cbRtuTrans); //Send request to RTU

  return Modbus::EX_SUCCESS;  
}


// Callback receives raw data from ModbusRTU and sends it on behalf of slave (slaveRunning) to tcp client
Modbus::ResultCode cbRtuRaw(uint8_t* data, uint8_t len, void* custom) {
  auto src = (Modbus::frame_arg_t*) custom;
  
  tcp.setTransactionId(transRunning); // Set transaction id as per incoming request
  tcp.rawResponce(srcIp, data, len, slaveRunning); //Send data to tcp client

  transRunning = 0; //Reset parameters for new request
  slaveRunning = 0;
  srcIp = (0,0,0,0);
  return Modbus::EX_PASSTHROUGH;
}

void setup() {
  Serial.begin(115200);

  // Configures static IP address
  if (ip_mode == "STATIC")
    if (!WiFi.config(local_IP, gateway, subnet)) {
      Serial.println("STATIC Failed to configure");
    }
  
  WiFi.begin(ssid, password); // start WIFI
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
    
  tcp.server(); // Initialize ModbusTCP to process as server
  tcp.onRaw(cbTcpRaw); // Assign raw data processing callback
  
  S.begin(ModbusBaudRate, SWSERIAL_8E1, 13, 15, false, 256, 0);
  rtu.begin(&S, DE_RE);  // Specify RE_DE control pin
  rtu.master(); // Initialize ModbusRTU as master
  rtu.onRaw(cbRtuRaw); // Assign raw data processing callback
}

void loop() { 
  rtu.task();
  tcp.task();
  yield();
}
