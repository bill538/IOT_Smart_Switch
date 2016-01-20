//
// RGB LCD initilazation
#include "Grove_LCD_RGB_Backlight.h"
rgb_lcd lcd;



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

// Initialize time tracking stamps
#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
unsigned long lastDaySync = millis();
unsigned long lastSecSync = lastDaySync;
unsigned long lastMSecSync = lastDaySync;
unsigned long currentSync = lastDaySync;

// Init rge lcd rgb color
const int colorR = 128;
const int colorG = 128;
const int colorB = 0;


//
// Function for Cloud/RestAPI to Change the state of RelayIn1,2,3,4 and set RelayIn#_State to proper state
//
// Argument syntax: 1 or 3
int CloudRelayInChange(String command) {
  bool value = 0;
  int RelayIn = 0;
  int RelayIn_State = 0;

  // Make sure upper case
  command.toUpperCase();

  Particle.publish("CloudRelayInChange Command", command);

  //Extract RelayIn# from Command and convert to integer
  int Relay = command.charAt(0) - '0';

  //Sanity check to see if the Relay number are within limits
  if (Relay< 1 || Relay >4) {
    return -1;
  }

  // Set the Pin number(RelayIn#) to RelayIn & set RelayIn_State
  switch (Relay) {
      case 1:   RelayIn = RelayIn1;
                RelayIn_State = RelayIn1_State;
      break;
      case 2:   RelayIn = RelayIn2;
                RelayIn_State = RelayIn2_State;
     break;
      case 3:   RelayIn = RelayIn3;
                RelayIn_State = RelayIn3_State;
      break;
      case 4:   RelayIn = RelayIn4;
                RelayIn_State = RelayIn4_State;
      break;
      default:  return -2;
  }

  Particle.publish("CloudRelayIn RelayIn - A", String::format("RelayIn:%i, RelayIn_State:%i",RelayIn,RelayIn_State));

  // Check if RelayIn_State is 1(OFF) or 0(ON). Then with the opposit to the pin.
  if ( RelayIn_State == 0 ) {
    digitalWrite(RelayIn, 1);
    RelayIn_State = 1;
  } else if ( RelayIn_State == 1 ) {
    digitalWrite(RelayIn, 0);
    RelayIn_State = 0;
  } else {
    return -3;
  }

  // Set Global Variable for RelayIn#_State based on new state
  switch (Relay) {
      case 1:   RelayIn1_State = RelayIn_State;
      break;
      case 2:   RelayIn2_State = RelayIn_State;
      break;
      case 3:   RelayIn3_State = RelayIn_State;
      break;
      case 4:   RelayIn4_State = RelayIn_State;
      break;
      default:  return -4;
  }
  Particle.publish("CloudRelayIn RelayIn - B", String::format("RelayIn:%i, RelayIn_State:%i",RelayIn,RelayIn_State));
}


//
// Function READs or Writes to Digital or Analog pins from the cloud
//
// Argument syntax: A1,Read or D4,Write,LOW
int CloudAccessPin(String command) {
  bool value = 0;

  // Make sure upper case
  command.toUpperCase();

  //Particle.publish("CloudAccessPin Command", command);
  Particle.publish("CloudAccessPin Command", command);
  Particle.publish("CloudAccessPin RelayIn3_State", String(RelayIn3_State));
  //Particle.publish("CloudAccessPin Command", command);

  //convert ascii to integer
  int pinNumber = command.charAt(1) - '0';

  //Sanity check to see if the pin numbers are within limits
  if (pinNumber< 0 || pinNumber >7) {
    return -1;
  }

  // Add 10 to Analog pin number
  if(command.startsWith("A")) {
    pinNumber = pinNumber+10;
  }

  //Particle.publish("Pin", String(pinNumber) );
  //Particle.publish("String", String(command.substring(3,7)) );
  //Particle.publish("String", String(command.substring(9,12)) );
  //Particle.publish("String", String(command.substring(9,13)) );

  // Check if READ or Write operation
  if ( command.substring(3,7) == "READ" ) {
    pinMode(pinNumber, INPUT);
    return digitalRead(pinNumber);
  } else if ( command.substring(3,8) == "WRITE" ) {
    if ( command.substring(9,13) == "HIGH" ) {
      value = 1;
    } else if ( command.substring(9,12) == "LOW" ) {
      value = 0;
    } else {
      return -2;
    }

    pinMode(pinNumber, OUTPUT);
    digitalWrite(pinNumber, value);
    return 1;
  } else {
    return -3;
  }
}


//
// Function set Digital output HIGH or LOW
//
// Argument A1,HIGH or D4,LOW
int WriteDigitalPin(int pin, int state) {
  Particle.publish("Pin - WriteDigital", String(pin) );

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
// Function to Read inputs as digital with HIGH or LOW
// Note: Assume pinmode has been set
//
int ReadDigitalPin(int pin) {
  //Particle.publish("Pin - ReadDigital", String(pin) );
  return digitalRead(pin);
}

//
// Main Functions
//

//
// Setup loop that is run only once at bootup
void setup() {
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

  // Publish devi's IP
  // Build IP Address and publish
  IPAddress myIp = WiFi.localIP();
  sprintf(myIpAddress, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);
  Particle.publish("IP", myIpAddress);

  // Enable could access to function
  // https://api.particle.io/v1/devices/2e0048000a47343432313031/WritePin?access_token=***************?args=D4,HIGH
  Particle.function("AccessPin", CloudAccessPin);
  Particle.function("RelayInChg", CloudRelayInChange);

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

  // Set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Set LCD background color
  lcd.setRGB(colorR, colorG, colorB);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hello, world!");
  lcd.setCursor(0, 1);
  lcd.print("Setup Complete!!");
  delay(1000);
}

//
// Infinty loop that runs after setup() function
void loop() {
  // Get the current Millis for this loop
  currentSync = millis();

  //Particle.publish("LOOP", String(Switch3_State) );
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

  // Update the display every second when the lastSync is > 1000 Millis ie one second
  // This is needed so the loop() can run faster to detect switch has been flipped
  if ( (currentSync - lastSecSync) > 1000) {
    // set the cursor to column 0, line 1
    // (note: line 1 is the second row, since counting begins with 0):
    lcd.clear();
    lcd.setCursor(0, 0);
    //lcd.print(myIpAddress);
    lcd.print(String::format("1:%i 2:%i 3:%i 4:%i", RelayIn1_State, RelayIn2_State, RelayIn3_State, RelayIn4_State));
    lcd.setCursor(0, 1);
    // print the number of seconds since reset:
    lcd.print(String::format("%i %i %i", currentSync/1000, lastSecSync/1000, currentSync - lastSecSync ));
    lastSecSync = currentSync;
  }

  // Sync time with cloud once a day
  if ( (currentSync - lastDaySync) > ONE_DAY_MILLIS) {
    // Request time synchronization from the Particle Cloud
    Particle.syncTime();
    lastDaySync = currentSync;
  }

  // Add a delay so loop only checks if physical switch state changes 10 times in one second.
  delay(100);
}
