/* adding Bluetooth LE to an infrared pulse sensor.
 *  Advertising data will broadcast current BPM
 *  and the characteristic for BPM will be updated every 10 seconds
 */

/*  Pulse Sensor Amped 1.4    by Joel Murphy and Yury Gitman   http://www.pulsesensor.com

----------------------  Notes ----------------------  ---------------------- 
This code:
1) Blinks an LED to User's Live Heartbeat   PIN 13
2) Fades an LED to User's Live HeartBeat
3) Determines BPM
4) Prints All of the Above to Serial

Read Me:
https://github.com/WorldFamousElectronics/PulseSensor_Amped_Arduino/blob/master/README.md   
 ----------------------       ----------------------  ----------------------
*/

/* Adding bluetooth to this.  
 *  BLE code thanks to https://github.com/don/ITP-BluetoothLE
 */
#include <SPI.h>
#include <BLEPeripheral.h>
// define pins (varies per shield/board)
// https://github.com/sandeepmistry/arduino-BLEPeripheral#pinouts
// Blend
#define BLE_REQ 9
#define BLE_RDY 8
#define BLE_RST 5

#define HEART_RATE_SERVICE_UID  "180d"
#define HEART_RATE_CHARACTERISTIC_UID "2a37"

BLEPeripheral blePeripheral = BLEPeripheral(BLE_REQ, BLE_RDY, BLE_RST);
BLEService pulseService = BLEService(HEART_RATE_SERVICE_UID);
BLEIntCharacteristic bpmCharacteristic = BLEIntCharacteristic(HEART_RATE_CHARACTERISTIC_UID, BLERead | BLENotify | BLEBroadcast);
BLEDescriptor bpmDescriptor = BLEDescriptor("2901", "Heart Rate Measurement");

long previousMillis = 0;  // will store last time BPM was broadcast
long interval = 10000;     // interval at which to broadcast BPM



//  Variables
int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 13;                // pin to blink led at each beat
int fadePin = 5;                  // pin to do fancy classy fading blink at each beat
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin

// Volatile Variables, used in the interrupt service routine!
volatile int BPM;                   // Int that holds the heart-rate value, derived every beat, from averaging previous 10 IBI values.
volatile int Signal;                // int that holds raw Analog in 0. updated every 2mS
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded! 
volatile boolean Pulse = false;     // "True" when User's live heartbeat is detected. "False" when not a "live beat". 
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.

// Regards Serial OutPut  -- Set This Up to your needs
static boolean serialVisual = true;   // Set to 'false' by Default.  Re-set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse 

// Flags field is lowest byte in data
// See: https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.heart_rate_measurement.xml 
byte flags = B00000000;

void setup(){
  pinMode(blinkPin,OUTPUT);         // pin that will blink to your heartbeat!
  pinMode(fadePin,OUTPUT);          // pin that will fade to your heartbeat!
  Serial.begin(115200);             // we agree to talk fast!
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 
   // IF YOU ARE POWERING The Pulse Sensor AT VOLTAGE LESS THAN THE BOARD VOLTAGE, 
   // UN-COMMENT THE NEXT LINE AND APPLY THAT VOLTAGE TO THE A-REF PIN
//   analogReference(EXTERNAL);   

//BLE
  // set advertised name and service
  blePeripheral.setLocalName("my Pulse");
  blePeripheral.setDeviceName("my Pulse");
  blePeripheral.setAdvertisedServiceUuid(pulseService.uuid());

  // add service and characteristic
  blePeripheral.addAttribute(pulseService);
  blePeripheral.addAttribute(bpmCharacteristic);
  blePeripheral.addAttribute(bpmDescriptor);
  
  blePeripheral.begin();

  bpmCharacteristic.broadcast();
}


//  Where the Magic Happens
void loop(){
    // Tell the bluetooth radio to do whatever it should be working on
  blePeripheral.poll();
  
    serialOutput() ;       
    
  if (QS == true){     // A Heartbeat Was Found
                       // BPM and IBI have been Determined
                       // Quantified Self "QS" true when arduino finds a heartbeat
        digitalWrite(blinkPin,HIGH);     // Blink LED, we got a beat. 
        fadeRate = 255;         // Makes the LED Fade Effect Happen
                                // Set 'fadeRate' Variable to 255 to fade LED with pulse
        serialOutputWhenBeatHappens();   // A Beat Happened, Output that to serial.     
        QS = false;                      // reset the Quantified Self flag for next time    

        if(millis() - previousMillis > interval) {
          // convert BPM value to unsigned int
          unsigned int u_bpm = unsigned(BPM);
          // and append the flags byte as LSO
          unsigned int out_bpm = (u_bpm << 8) + flags;
          bpmCharacteristic.setValue(out_bpm);
          previousMillis = millis();
        }
  }
     
  ledFadeToBeat();                      // Makes the LED Fade Effect Happen 
  delay(20);                             //  take a break
}





void ledFadeToBeat(){
    fadeRate -= 15;                         //  set LED fade value
    fadeRate = constrain(fadeRate,0,255);   //  keep LED fade value from going into negative numbers!
    analogWrite(fadePin,fadeRate);          //  fade LED
  }




