// This #include statement was automatically added by the Particle IDE.
#include "spark-web-embd-rest-json/spark-web-embd-rest-json.h"

// This #include statement was automatically added by the Particle IDE.
#include "Grove_LCD_RGB_Backlight/Grove_LCD_RGB_Backlight.h"


//
// Function set Digital output HIGH or LOW
//
// Argument syntax: A1,HIGH or D4,LOW
int CloudWriteDigitalPin(String command) {
    //convert ascii to integer
    int pinNumber = command.charAt(1) - '0';

    //Sanity check to see if the pin numbers are within limits
    if (pinNumber< 0 || pinNumber >7) {
        return -1;
    }

    // Compute real pinNumber based on pin type A(Analog) or D(Digital)
    if (command.startsWith("D")) {
        pinNumber=pinNumber;
    } else if (command.startsWith("A")) {
        pinNumber=pinNumber+10;
    } else {
        return -2;
    }
    
digitalWrite(D7, LOW);
delay(1000);
digitalWrite(D7, HIGH);
delay(1000);

    // find out the state to set pin
    if (command.substring(3,7) == "HIGH") {
        digitalWrite(D4, HIGH);
        return 1;
    } else if (command.substring(3,6) == "LOW") {
        digitalWrite(D4, LOW);
        return 0;
    } else {
        return -1;
    }
}



//
// Function to Read inputs as digital with HIGH or LOW
// Note: Assume pinmode has been set
//
int ReadDigitalPin(int pin) {
   return digitalRead(pin);
}


//
// Function set Digital output HIGH or LOW
//
// Argument A1,HIGH or D4,LOW
int WriteDigitalPin(int pin, int state) {

    // find out the state to set pin
    if (state == HIGH) {
        digitalWrite(pin, state);
        return 1;
    } else if (state == LOW) {
        digitalWrite(pin, state);
        return 0;
    } else {
        return -1;
    }
}


// 
// Function to check if physical switch has chnaged state(been flipped)
//
int CheckSwitchStateChanged(int switch_pin, int current_switch_state, int previous_switch_state, int relayin, int relayin_state) {
    
    bool success;

	// Check if switch pin state changed
	if (current_switch_state != previous_switch_state) {
		if(relayin_state == HIGH) {
			WriteDigitalPin(relayin, LOW);
            success = Particle.publish("State_Changed", String::format("light:%s, relayin:%i, relayin_state:%i, switch_pin:%i, current_switch_state:%i, previous_switch_state:%i", "ON", relayin, relayin_state, switch_pin, current_switch_state, previous_switch_state));
			return 0;
		} else {
			WriteDigitalPin(relayin, HIGH);
            success = Particle.publish("State_Changed", String::format("light:%s, relayin:%i, relayin_state:%i, switch_pin:%i, current_switch_state:%i, previous_switch_state:%i", "OFF", relayin, relayin_state, switch_pin, current_switch_state, previous_switch_state));
			return 1;
		}
	} else {
	    return relayin_state;
	}
}


//
//
// MAIN
//
//

// -----------------------------------
// Controlling LEDs/Circuits over the Internet
// -----------------------------------

// First, let's create our "shorthand" for the pins used
int ic2_sda = D0;
int i2c_scl = D1;
int RelayIn1 = D2;
int RelayIn2 = D3;
int RelayIn3 = D4;
int RelayIn4 = D5;
int RelayIn1_State = HIGH;
int RelayIn2_State = HIGH;
int RelayIn3_State = HIGH;
int RelayIn4_State = HIGH;
int Switch1 = A2;
int Switch2 = A3;
int Switch3 = A4;
int Switch4 = A5;
int Switch1_State = HIGH;
int Switch2_State = HIGH;
int Switch3_State = HIGH;
int Switch4_State = HIGH;
int Prev_Switch1_State = HIGH;
int Prev_Switch2_State = HIGH;
int Prev_Switch3_State = HIGH;
int Prev_Switch4_State = HIGH;

int Led1 = D6;
int Led2 = D7;

// Save IP,SSID & MAC to variables
char myIpAddress[24];
String SSID = "";
byte MAC[6];


// Time syncing data
#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
unsigned long lastSync = millis();

// Web server config.
TCPServer server = TCPServer(23);
TCPClient client;


// setup function that executes at the start of code only
void setup()
{
    // Here's the pin Mode configuration
    pinMode(Led1, OUTPUT);
    pinMode(Led2, OUTPUT);
    pinMode(Switch1, INPUT);
    pinMode(Switch2, INPUT);
    pinMode(Switch3, INPUT);
    pinMode(Switch4, INPUT);
    pinMode(RelayIn1, OUTPUT);
    pinMode(RelayIn2, OUTPUT);
    pinMode(RelayIn3, OUTPUT);
    pinMode(RelayIn4, OUTPUT);

    // Let's also make sure both LEDs are off when we start.
    digitalWrite(Led1, LOW);
    digitalWrite(Led2, LOW);
    // Let's also make sure all Relays are set HIGH(OFF) when we start.
    RelayIn1_State = WriteDigitalPin(RelayIn1, HIGH);
    RelayIn2_State = WriteDigitalPin(RelayIn2, HIGH);
    RelayIn3_State = WriteDigitalPin(RelayIn3, HIGH);
    RelayIn4_State = WriteDigitalPin(RelayIn4, HIGH);

	// Read the input state on the 4 switch for their current state and store state
    Switch1_State = ReadDigitalPin(Switch1);
    Switch2_State = ReadDigitalPin(Switch2);
    Switch3_State = ReadDigitalPin(Switch3);
    Switch4_State = ReadDigitalPin(Switch4);	

    // Build IP Address and publish
    IPAddress myIp = WiFi.localIP();
    sprintf(myIpAddress, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);
    Particle.publish("IP", myIpAddress);

    WiFi.macAddress(MAC);
    //sprintf(MAC, "%02x:%02x:%02x:%02x:%02x:%02x", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
    //sprintf(MAC, "%d:%d", String(MAC[0], HEX), String(MAC[1], HEX)); // MAC[2], MAC[3], MAC[4], MAC[5]);
    //Particle.publish("MAC", MAC);

    // Expose all variables to the cloud. Note current maximum is 10.
    // https://api.particle.io/v1/devices/2e0048000a47343432313031/IP?access_token=***************
    Particle.variable("IP", myIpAddress);
    Particle.variable("SSID", SSID);
    Particle.variable("RelayIn1", RelayIn1_State);
    Particle.variable("RelayIn2", RelayIn2_State);
    Particle.variable("RelayIn3", RelayIn3_State);
    Particle.variable("RelayIn4", RelayIn4_State);
    Particle.variable("Switch1", Switch1_State);
    Particle.variable("Switch2", Switch2_State);
    Particle.variable("Switch3", Switch3_State);
    Particle.variable("Switch4", Switch4_State);
    Particle.variable("PrevSw1", Prev_Switch1_State);
    Particle.variable("PrevSw2", Prev_Switch2_State);
    Particle.variable("PrevSw3", Prev_Switch3_State);
    Particle.variable("PrevSw4", Prev_Switch4_State);
    
    // Enable could access to function
    Particle.function("WritePin", CloudWriteDigitalPin);
    // // https://api.particle.io/v1/devices/2e0048000a47343432313031/WritePin?access_token=***************?args=D4,HIGH
    
    // start listening for clients
    server.begin();

    // Make sure your Serial Terminal app is closed before powering your device
    Serial.begin(9600);
    // Now open your Serial Terminal, and hit any key to continue!
    while(!Serial.available()) Particle.process();
    Serial.print("IP Address: ");
    Serial.println(myIpAddress);
    Serial.print("subnetMask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("gateway IP Address: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("SSID: ");
    Serial.println(SSID);
    Serial.print("MAC: ");
    Serial.print(MAC[0], HEX);
    Serial.print(":");
    Serial.print(MAC[1],HEX);
    Serial.print(":");
    Serial.print(MAC[2],HEX);
    Serial.print(":");
    Serial.print(MAC[3],HEX);
    Serial.print(":");
    Serial.print(MAC[4],HEX);
    Serial.print(":");
    Serial.println(MAC[5],HEX);
    Serial.print("Switch3: ");
    Serial.println(Switch3);
    Serial.print("RelayIn3_State: ");
    Serial.println(RelayIn3_State);

    

}

void loop()
{
	bool success;

    Prev_Switch1_State = Switch1_State;
    Prev_Switch2_State = Switch2_State;
    Prev_Switch3_State = Switch3_State;
    Prev_Switch4_State = Switch4_State;

    Switch1_State = ReadDigitalPin(Switch1);
    Switch2_State = ReadDigitalPin(Switch2);
    Switch3_State = ReadDigitalPin(Switch3);
    Switch4_State = ReadDigitalPin(Switch4);	

	// Check if light switches have changed states "been flipped"
	RelayIn1_State = CheckSwitchStateChanged(Switch1, Switch1_State, Prev_Switch1_State, RelayIn1, RelayIn1_State);
	RelayIn2_State = CheckSwitchStateChanged(Switch2, Switch2_State, Prev_Switch2_State, RelayIn2, RelayIn2_State);
    RelayIn3_State = CheckSwitchStateChanged(Switch3, Switch3_State, Prev_Switch3_State, RelayIn3, RelayIn3_State);
    RelayIn4_State = CheckSwitchStateChanged(Switch4, Switch4_State, Prev_Switch4_State, RelayIn4, RelayIn4_State);

    // Check time sync every 24 hours with cloud
    if (millis() - lastSync > ONE_DAY_MILLIS) {
        // Request time synchronization from the Particle Cloud
        Particle.syncTime();
        lastSync = millis();
    }


//digitalWrite(Led1, LOW);
//digitalWrite(Led2, LOW);
//delay(500);
//digitalWrite(Led1, HIGH);
//digitalWrite(Led2, HIGH);
//delay(500);

  if (client.connected()) {
    // echo all available bytes back to the client
    while (client.available()) {
        server.write(client.read());
        server.print("IP Address: ");
        server.println(myIpAddress);
        server.print("SSID: ");
        server.println(SSID);
        server.print("MAC: ");
        server.print(MAC[0], HEX);
        server.print(":");
        server.print(MAC[1],HEX);
        server.print(":");
        server.print(MAC[2],HEX);
        server.print(":");
        server.print(MAC[3],HEX);
        server.print(":");
        server.print(MAC[4],HEX);
        server.print(":");
        server.println(MAC[5],HEX);
        server.print("Switch3: ");
        server.println(Switch3);
        server.print("RelayIn3_State: ");
        server.println(RelayIn3_State);
    }
  } else {
    // if no client is yet connected, check for a new connection
    client = server.available();
  }
delay(100);
}

// We're going to have a super cool function now that gets called when a matching API request is sent
// This is the ledToggle function we registered to the "led" Spark.function earlier.


int ledToggle(String command) {
    /* Spark.functions always take a string as an argument and return an integer.
    Since we can pass a string, it means that we can give the program commands on how the function should be used.
    In this case, telling the function "on" will turn the LED on and telling it "off" will turn the LED off.
    Then, the function returns a value to us to let us know what happened.
    In this case, it will return 1 for the LEDs turning on, 0 for the LEDs turning off,
    and -1 if we received a totally bogus command that didn't do anything to the LEDs.
    */
 
    if (command=="on") {
        digitalWrite(Led1,HIGH);
        digitalWrite(Led2,HIGH);
        return 1;
    }
    else if (command=="off") {
        digitalWrite(Led1,LOW);
        digitalWrite(Led2,LOW);
        return 0;
    }
    else {
        return -1;
    }
}
