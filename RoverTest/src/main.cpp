#define INPUT_SIZE 100

#include <Arduino.h>
#include <VescUart.h>
#include <LoRa.h>

VescUart VESC1; // Front Right
VescUart VESC2; // Front Left
// VESC 3 (Back Right) is disabled due to a hardware fault
VescUart VESC4; // Back Left
VescUart VESC5; // AUX1 (Used for Back Right)
// VESC 6 (Aux2) is not needed

#define RFM95_CS 10
#define RFM95_RST 3
#define RFM95_INT 4
#define RF95_FREQ 915E6

String outgoing;

byte msgCount = 0;        // count of outgoing messages
byte localAddress = 0xCC; // address of this device
byte destination = 0xFF;  // destination to send to

void sendMessage(String);
String onReceive(int);

float frleftMotorSpd = 0;
float frrightMotorSpd = 0;
float bkleftMotorSpd = 0;
float bkrightMotorSpd = 0;

float maxSpeed = 6000;
float currentMaxSpeed = maxSpeed;

uint32_t lastPacketTime = 0;

void setup() {
  Serial.begin(115200);
  Serial8.begin(115200);
  Serial7.begin(115200);
  Serial2.begin(115200);
  Serial5.begin(115200);

  while(!Serial && millis() < 5000)
  {
    delay(10);
  }

  VESC1.setSerialPort(&Serial8);
  VESC2.setSerialPort(&Serial7);
  VESC4.setSerialPort(&Serial2);
  VESC5.setSerialPort(&Serial5);

  if ( VESC1.getVescValues() ) 
  {
    Serial.println("VESC 1 Connected!");
    Serial.println(VESC1.data.id);
    Serial.println(VESC1.data.rpm);
    Serial.println(VESC1.data.inpVoltage);
    Serial.println(VESC1.data.ampHours);
    Serial.println(VESC1.data.tachometerAbs);
  }
  else
  {
    Serial.println("No VESC1");
  }
  if ( VESC2.getVescValues() ) 
  {
    Serial.println("VESC 2 Connected!");
    Serial.println(VESC2.data.id);
    Serial.println(VESC2.data.rpm);
    Serial.println(VESC2.data.inpVoltage);
    Serial.println(VESC2.data.ampHours);
    Serial.println(VESC2.data.tachometerAbs);
  }
  else
  {
    Serial.println("No VESC2");
  }
  if ( VESC4.getVescValues() ) 
  {
    Serial.println("VESC 4 Connected!");
    Serial.println(VESC4.data.id);
    Serial.println(VESC4.data.rpm);
    Serial.println(VESC4.data.inpVoltage);
    Serial.println(VESC4.data.ampHours);
    Serial.println(VESC4.data.tachometerAbs);
  }
  else
  {
    Serial.println("No VESC4");
  }
  if ( VESC5.getVescValues() ) 
  {
    Serial.println("VESC 5 Connected!");
    Serial.println(VESC5.data.id);
    Serial.println(VESC5.data.rpm);
    Serial.println(VESC5.data.inpVoltage);
    Serial.println(VESC5.data.ampHours);
    Serial.println(VESC5.data.tachometerAbs);
  }
  else
  {
    Serial.println("No VESC5");
  }

  // Initialize LoRa radio
  Serial.println("Initializing LoRa");
  LoRa.setPins(RFM95_CS, RFM95_RST, RFM95_INT);

  if (!LoRa.begin(RF95_FREQ))
  { // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true)
      ; // if failed, do nothing
        // TODO: change this to more gracefully deal with failure
  }
  LoRa.setTxPower(20);
  LoRa.setSignalBandwidth(125E3);
  Serial.println("LoRa init succeeded.");
}

void loop() {
  
  VESC1.getVescValues();
  if (VESC1.data.inpVoltage < 28)
  {
    currentMaxSpeed = 1500;
  }
  else
  {
    currentMaxSpeed = maxSpeed;
  }

  // Check for LoRa messages
  String message = "";
  if (!((message = onReceive(LoRa.parsePacket())).equals("")))
  {

    // Parse message to control commands
    message.trim();

    int horz = message.substring(1, message.indexOf(',')).toInt();
    int vert = message.substring(message.indexOf('V') + 1, message.indexOf(',', message.indexOf('V'))).toInt();

    // Serial.print(horz);
    // Serial.print(", ");
    // Serial.println(vert);

    // Generate motor speeds from control commands
    frleftMotorSpd = constrain(-vert - horz, -currentMaxSpeed, currentMaxSpeed);
    frrightMotorSpd = constrain(-vert + horz, -currentMaxSpeed, currentMaxSpeed);
    bkleftMotorSpd = constrain(-vert - horz, -currentMaxSpeed, currentMaxSpeed);
    bkrightMotorSpd = constrain(-vert + horz, -currentMaxSpeed, currentMaxSpeed);

    if (abs(frleftMotorSpd) < 1000) {
      frleftMotorSpd = 0;
    }
    if (abs(frrightMotorSpd) < 1000) {
      frrightMotorSpd = 0;
    }
    if (abs(bkleftMotorSpd) < 1000) {
      bkleftMotorSpd = 0;
    }
    if (abs(bkrightMotorSpd) < 1000) {
      bkrightMotorSpd = 0;
    }

    Serial.print(frleftMotorSpd);
    Serial.print(", ");
    Serial.print(bkleftMotorSpd);
    Serial.print(", ");
    Serial.print(frrightMotorSpd);
    Serial.print(", ");
    Serial.println(bkrightMotorSpd);

    lastPacketTime = millis();

    //sendMessage(buffer);
    //Serial.print("Sending ");
    //Serial.print(buffer);
  }


  // If no LoRa message has been recieved for 0.25 seconds or battery voltage is too low,
  // set motor speeds to zero
  if (millis() - lastPacketTime > 250)
  {
    frleftMotorSpd = 0;
    frrightMotorSpd = 0;
    bkleftMotorSpd = 0;
    bkrightMotorSpd = 0;
  }

  // Set motor speeds

  VESC1.setRPM(frrightMotorSpd);
  VESC2.setRPM(frleftMotorSpd);
  VESC4.setRPM(bkleftMotorSpd);
  VESC5.setRPM(bkrightMotorSpd);

}

void sendMessage(String outgoing)
{
  LoRa.beginPacket();            // start packet
  LoRa.write(destination);       // add destination address
  LoRa.write(localAddress);      // add sender address
  LoRa.write(msgCount);          // add message ID
  LoRa.write(outgoing.length()); // add payload length
  LoRa.print(outgoing);          // add payload
  LoRa.endPacket();              // finish packet and send it
  msgCount++;                    // increment message ID
}

String onReceive(int packetSize)
{
  if (packetSize == 0)
    return ""; // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();       // recipient address
  byte sender = LoRa.read();         // sender address
  byte incomingMsgId = LoRa.read();  // incoming msg ID
  byte incomingLength = LoRa.read(); // incoming msg length

  String incoming = "";

  while (LoRa.available())
  {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length())
  { // check length for error
    Serial.println("error: message length does not match length");
    return ""; // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF)
  {
    Serial.println("This message is not for me.");
    return ""; // skip rest of function
  }

  // // if message is for this device, or broadcast, print details:
  // Serial.println("Received from: 0x" + String(sender, HEX));
  // Serial.println("Sent to: 0x" + String(recipient, HEX));
  // Serial.println("Message ID: " + String(incomingMsgId));
  // Serial.println("Message length: " + String(incomingLength));
  // Serial.println("Message: " + incoming);
  // Serial.println("RSSI: " + String(LoRa.packetRssi()));
  // Serial.println("Snr: " + String(LoRa.packetSnr()));
  // Serial.println();
  return incoming;
}