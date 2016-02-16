  //
// IOT Smart Switch variable initilization BEGIN

#include "IOT_Smart_Switch.h"

// First, let's create our "shorthand" for the pins used
int ic2_sda = D0;
int i2c_scl = D1;
int RelayIn1 = D2;
int RelayIn2 = D3;
int RelayIn3 = D4;
int RelayIn4 = D5;
int RelayIn[4] = { D2, D3, D4, D5 };
int RelayIn_States[4] = { HIGH, HIGH, HIGH, HIGH };
int Switch1 = A2;
int Switch2 = A3;
int Switch3 = A4;
int Switch4 = A5;
int Switch[4] = { A2, A3, A4, A5 };
int Switch_States[4] = { HIGH, HIGH, HIGH, HIGH };
int Prev_Switch_States[4] = { HIGH, HIGH, HIGH, HIGH };
int TimerStartTime[4] = { 0, 0, 0, 0 };
int TimerTime[4] = { 0, 0, 0, 0 };


int Led1 = D6;
int Led2 = D7;

// Save IP,SSID & MAC to variables
char myIpAddress[24];
String SSID = "";
byte MAC[6];

// Initialize time tracking stamps
#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
unsigned long currentSync = millis();
unsigned long lastMSecSync = currentSync;
unsigned long lastSecSync = currentSync;
unsigned long lastMinSync = currentSync;
unsigned long lastDaySync = currentSync;
int TimeZone = -5;

// Init RGB lcd color for background
const int colorR = 128;
const int colorG = 128;
const int colorB = 0;
//
// IOT Smart Switch variable initilization END


//
// RGB LCD initilazation BEGIN
#include "Grove_LCD_RGB_Backlight.h"
rgb_lcd lcd;
//
// RGB LCD initilazation END


//
// IOT Smart Switch Fuction initilazation BEGIN

//
// Function for Cloud/RestAPI to Change the state of RelayIn1,2,3,4 and set RelayIn#_State to proper state
// Argument syntax for command = 1|2|3|4
int CloudRelayInChange(String command) {
  bool value = 0;
  int RelayIn = 0;
  int RelayIn_State = 0;

  // Make sure command text is in upper case
  command.toUpperCase();

  Particle.publish("CloudRelayInChange", command);

  //Extract RelayIn# from Command and convert to integer
  int Relay = command.charAt(0) - '0';

  //Sanity check to see if the Relay number are within limits
  if (Relay< 1 || Relay >4) {
    return -1;
  }

  // Set the Pin number(RelayIn#) to RelayIn & set RelayIn_State
  switch (Relay) {
      case 1:   RelayIn = RelayIn1;
                RelayIn_State = RelayIn_States[0];
      break;
      case 2:   RelayIn = RelayIn2;
                RelayIn_State = RelayIn_States[1];
      break;
      case 3:   RelayIn = RelayIn3;
                RelayIn_State = RelayIn_States[2];
      break;
      case 4:   RelayIn = RelayIn4;
                RelayIn_State = RelayIn_States[2];
      break;
      default:  return -2;
  }

  Particle.publish("CloudRelayInChange", String::format("B - Switch:%i, RelayIn Pin:D%i, RelayIn_State:%i",Relay,RelayIn,RelayIn_State));

  // Check if RelayIn_State is 1(OFF) or 0(ON). Then with the opposit to the pin.
  if ( RelayIn_State == 0 ) {
    WriteDigitalPin(RelayIn, 1);
    RelayIn_State = 1;
    // Rest any timer values and stop Timer
    TimerStartTime[Relay-1] = 0;
    TimerTime[Relay-1] = 0;
  } else if ( RelayIn_State == 1 ) {
    WriteDigitalPin(RelayIn, 0);
    RelayIn_State = 0;
  } else {
    return -3;
  }

  // Set Global Variable for RelayIn#_State based on new state
  switch (Relay) {
      case 1:   RelayIn_States[0] = RelayIn_State;
      break;
      case 2:   RelayIn_States[1] = RelayIn_State;
      break;
      case 3:   RelayIn_States[2] = RelayIn_State;
      break;
      case 4:   RelayIn_States[3] = RelayIn_State;
      break;
      default:  return -4;
  }
  Particle.publish("CloudRelayInChange", String::format("C - Switch:%i, RelayIn Pin:D%i, RelayIn_State:%i",Relay,RelayIn,RelayIn_State));
}


//
// Function READs or Writes to Digital or Analog pins from the cloud
//
// Argument syntax: A1,Read or D4,Write,LOW
int CloudAccessPin(String command) {
  bool value = 0;

  // Make sure upper case
  command.toUpperCase();

  Particle.publish("CloudAccessPin", command);

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
    WriteDigitalPin(pinNumber, value);
    return 1;
  } else {
    return -3;
  }
}


//
// Function set Digital output HIGH or LOW
//
// Argument A1, HIGH or D4, LOW
int WriteDigitalPin(int pin, int state) {
  Particle.publish("WriteDigitalPin", String::format("A - Pin:%i, State:%i",pin,state) );

  // https://community.particle.io/t/pinmode-in-loop-for-analog-and-digital/7107/5
  // Change the pinMode to proper mode(OUPUT) for write operation.
  int mode = getPinMode(pin);
  if (mode != OUTPUT) {
      pinMode(pin, OUTPUT);
  }

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

  // Set pinMode back for how you found it.
  if (mode != OUTPUT) {
      pinMode(pin, INPUT);
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
      success = CloudRelayInChange(String(relayin-1));
      success = Particle.publish("CheckSwitchStateChanged", String::format("success:%i, light:%s, relayin:%i, relayin_state:%i, switch_pin:%i, current_switch_state:%i, previous_switch_state:%i", success, "ON", relayin, relayin_state, switch_pin, current_switch_state, previous_switch_state));
      return 0;
    } else {
      success = CloudRelayInChange(String(relayin-1));
      success = Particle.publish("CheckSwitchStateChanged", String::format("success:%i, light:%s, relayin:%i, relayin_state:%i, switch_pin:%i, current_switch_state:%i, previous_switch_state:%i", success, "OFF", relayin, relayin_state, switch_pin, current_switch_state, previous_switch_state));
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
  //Particle.publish("ReadDigitalPin", String::format("A - Pin:%i",pin) );
  return digitalRead(pin);
}

// You can't pass argument to the function being called by Timer.
// The work around is just create dedicated function for each swicth called Switch#TimerFunction
void Switch1TimerFunction(){
  Particle.publish("Switch1TimerFunction", "1");
  CloudRelayInChange("1");
}
void Switch2TimerFunction(){
  Particle.publish("Switch2TimerFunction", "2");
  CloudRelayInChange("2");
}
void Switch3TimerFunction(){
  Particle.publish("Switch3TimerFunction", "3");
  CloudRelayInChange("3");
}
void Switch4TimerFunction(){
  Particle.publish("Switch4TimerFunction", "4");
  CloudRelayInChange("4");
}


// You can't pass argument to the function being called by Timer.
// The work around is just create dedicated function for each swicth called Switch#TimerFunction
Timer TimerSwitch1(20000, Switch1TimerFunction, true);
Timer TimerSwitch2(30000, Switch2TimerFunction, true);
Timer TimerSwitch3(40000, Switch3TimerFunction, true);
Timer TimerSwitch4(50000, Switch4TimerFunction, true);

//
//  This function is used to set timer time and start timer.
int CloudSwitchTimer(int SwitchNum, int SwitchMins) {
  int SwitchMills = SwitchMins*60000;

  switch (SwitchNum) {
      case 1:   TimerSwitch1.changePeriod(SwitchMills);
                TimerTime[0] = SwitchMins*60;
                TimerSwitch1.reset();
                TimerStartTime[0] = Time.now();
      break;
      case 2:   TimerSwitch2.changePeriod(SwitchMills);
                TimerTime[1] = SwitchMins*60;
                TimerSwitch2.reset();
                TimerStartTime[1] = Time.now();
      break;
      case 3:   TimerSwitch3.changePeriod(SwitchMills);
                TimerTime[2] = SwitchMins*60;
                TimerSwitch3.reset();
                TimerStartTime[2] = Time.now();
      break;
      case 4:   TimerSwitch4.changePeriod(SwitchMills);
                //TimerTime[3] = SwitchMins*60;
                TimerSwitch4.reset();
                TimerStartTime[3] = Time.now();
      break;
      default:  return -1;
  }
}


//
// IOT Smart Switch Fuction initilazation END


//
// Webduino initilization BEGIN
#include "WebServer.h"

// no-cost stream operator as described at
// http://sundial.org/arduino/?page_id=119
template<class T>
inline Print &operator <<(Print &obj, T arg)
{ obj.print(arg); return obj; }


#define PREFIX ""

WebServer webserver(PREFIX, 80);

// commands are functions that get called by the webserver framework
// they can read any posted data from client, and they output to server

void debugCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  //server << "<!-- debugCmd ConnectionType=" << type << " -->\n";
  P(htmlHead) =
    "<html>\n"
    "<head>\n"
    "<title>Particle Photon Web Server</title>\n"
    "<style type=\"text/css\">\n"
    "BODY { font-family: sans-serif }\n"
    "H1 { font-size: 14pt; text-decoration: underline }\n"
    "P  { font-size: 10pt; }\n"
    "</style>\n"
    "</head>\n"
    "<body>\n";

  if (type == WebServer::POST) {
    server.httpFail();
    return;
  }

  server.httpSuccess();
  server.printP(htmlHead);

  if (type == WebServer::HEAD)
    return;

  int i;
  for (i = 0; i < 4; ++i) {
    server << "RelayIn " << i+1 << " Pin Number: " << RelayIn[i] << "<br>\n";
    server << "RelayIn " << i+1 << " State: " << RelayIn_States[i] << "<br>\n";
    server << "Switch " << i+1 << " Pin Number: " << Switch[i] << "<br>\n";
    server << "Switch " << i+1 << " State: " << Switch_States[i] << "<br>\n";
    server << "Previous Switch " << i+1 << " State: " << Prev_Switch_States[i] << "<br>\n";
    server << "Time.now() " << Time.now() << "<br>\n";
    server << "Timer " << i+1 << " Start Time: " << TimerStartTime[i] << "<br>\n";
    server << "Time.now() - TimerStartTime[i]: " << Time.now() - TimerTime[i] << "<br>\n";
    server << "Timer " << i+1 << " Time: " << TimerTime[i] << "<br>\n";
    server << "TimerStartTime[i]-(Time.now() - TimerTime[i]) " << i+1 << " Time: " << TimerStartTime[i]-(Time.now()-TimerTime[i]) << "<br>\n";
    server << "(TimerStartTime[i]-(Time.now() - TimerTime[i]))/60 " << i+1 << " Time: " << (TimerStartTime[i]-(Time.now()-TimerTime[i]))/60 << "<br>\n";
    server << "(TimerStartTime[i]-(Time.now() - TimerTime[i]))%60 " << i+1 << " Time: " << (TimerStartTime[i]-(Time.now()-TimerTime[i]))%60 << "<br>\n";
  }
  server << "myIpAddress: " << myIpAddress << "<br>\n";
  server << "ONE_DAY_MILLIS: " << ONE_DAY_MILLIS << "<br>\n";
  server << "currentSync: " << currentSync << "<br>\n";
  server << "lastMSecSync: " << lastMSecSync << "<br>\n";
  server << "lastSecSync: " << lastSecSync << "<br>\n";
  server << "lastMinSync: " << lastMinSync << "<br>\n";
  server << "lastDaySync: " << lastDaySync << "<br>\n";
  server << "TimeZone: " << TimeZone << "<br>\n";
  server << "colorR: " << colorR << "<br>\n";
  server << "colorG: " << colorG << "<br>\n";
  server << "colorB: " << colorB << "<br>\n";
}

void jsonCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  //server << "<!-- jsonCmd ConnectionType=" << type << " -->\n";
  if (type == WebServer::POST)
  {
    server.httpFail();
    return;
  }

  //server.httpSuccess(false, "application/json");
  server.httpSuccess("application/json");

  if (type == WebServer::HEAD)
    return;

  int i;
  server << "{ ";
  for (i = 0; i <= 8; ++i)
  {
    // ignore the pins we use to talk to the Ethernet chip
    int val = digitalRead(i);
    server << "\"d" << i << "\": " << val << ", ";
  }

  for (i = 0; i <= 5; ++i)
  {
    int val = analogRead(i);
    server << "\"a" << i << "\": " << val;
    if (i != 5)
      server << ", ";
  }

  server << " }";
}

void outputPins(WebServer &server, WebServer::ConnectionType type, bool addControls = false)
{
  P(htmlHead) =
    "<html>\n"
    "<head>\n"
    "<title>Particle Photon Web Server</title>\n"
    "<style type=\"text/css\">\n"
    "BODY { font-family: sans-serif }\n"
    "H1 { font-size: 14pt; text-decoration: underline }\n"
    "P  { font-size: 10pt; }\n"
    "</style>\n"
    "</head>\n"
    "<body>\n";

  int i;
  server.httpSuccess();
  server.printP(htmlHead);
  //server << "<!-- outputPins addControls=" << addControls << " ConnectionType=" << type << " -->\n";

  if (addControls) {
    server << "<form action='" PREFIX "/form' method='post'>\n";

    server << "<h1> Switch States - " << Time.format(Time.now(), TIME_FORMAT_DEFAULT) << " </h1> <p>\n";

    // Check if RelayIn_States are 1(OFF) or 0(ON). Then diplay ON/OFF details appropreiatly
    for (int i = 0; i < 4; i++) {
      server << "Switch " << i+1 << " is currently <B>";
      if ( RelayIn_States[i] == 0 ) {
        // Get time left on timer
        int LeftMins = (TimerStartTime[i]-(Time.now()-TimerTime[i]))/60;
        int LeftSecs = ((TimerStartTime[i]-(Time.now()-TimerTime[i]))%60);
        server << "ON</B>. <br>\nTurn Switch " << i+1;
        server << " <B>OFF</B> <input type=checkbox name=Switch" << i+1;
        server << "_State value=" << i+1 << "/><br>\nTimer Time Left: ";
        server << LeftMins << ":" << String::format("%02d", LeftSecs) << "<br>\n<hr>\n<br>\n";
        server << "<!-- addControls i=" << i << " -->\n";
        server << "<!-- addControls TimerStartTime=" << TimerStartTime[i] << " -->\n";
        server << "<!-- addControls TimerTime=" << TimerTime[i] << " -->\n";
        server << "<!-- addControls LeftMins=" << LeftMins << " -->\n";
        server << "<!-- addControls LeftSecs=" << LeftSecs << " -->\n";
      } else {
        server << "OFF</B>. <br>\nTurn Switch " << i+1;
        server << " <B>ON</B> <input type=checkbox name=Switch" << i+1;
        server << "_State value=" << i+1 << " /> <br>\nSwitch " << i+1;
        server << " Timer(Minutes): <input type=\"text\" name=\"Switch" << i+1;
        server << "_Timer\" value=0 /><br>\n<hr>\n<br>\n";
      }
    }
    server << "<INPUT TYPE=\"button\" onClick=\"history.go(0)\" VALUE=\"Refresh\"> <input type='submit' value='Submit'/></form>\n";
  }

  server << "</body></html>\n";
}

void formCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  //server << "<!-- formCmd ConnectionType=" << type << " -->\n";

  if (type == WebServer::POST)
  {
    bool repeat;
    char name[16], value[16];
    // Loop on all the http post form data
    do
    {
      repeat = server.readPOSTparam(name, 16, value, 16);

      int pin = strtoul(name + 1, NULL, 10);
      int val = strtoul(value, NULL, 10);
      //server << "<!-- formCmd - WebServer::POST name=" << name << "  value=" << value << " -->\n";
      //server << "<!-- formCmd - WebServer::POST pin=" << pin << "  val=" << val << " -->\n";
      //server << "<!-- formCmd - WebServer::POST repeat=" << repeat << " -->\n";

      // Check if name from the form data is Timer related
      if ( !strcmp(name,"Switch1_Timer") or !strcmp(name,"Switch2_Timer") or
           !strcmp(name,"Switch3_Timer") or !strcmp(name,"Switch4_Timer") ) {
        //server << "<!-- formCmd - WebServer::POST if matched Switch#_Timer -->\n";

        // If Timer val is greagter then 0 start appropreiate Timer
        if ( val > 0 ) {
          //server << "<!-- formCmd - WebServer::POST val=" << val << " start timer -->\n";

          // Call correct function based on val
          if ( !strcmp(name,"Switch1_Timer") ) {
            CloudSwitchTimer(1, val);
          } else if ( !strcmp(name,"Switch2_Timer") ) {
            CloudSwitchTimer(2, val);
          } else if ( !strcmp(name,"Switch3_Timer") ) {
            CloudSwitchTimer(3, val);
          } else if ( !strcmp(name,"Switch4_Timer") ) {
            CloudSwitchTimer(4, val);
          }
        }
      // Check if name from the form data is related related to changing switch state
      } else if ( !strcmp(name,"Switch1_State") or !strcmp(name,"Switch2_State") or
                  !strcmp(name,"Switch3_State") or !strcmp(name,"Switch4_State") ) {
        //server << "<!-- formCmd - WebServer::POST else if matched Switch#_state -->\n";
        CloudRelayInChange(String::format("%i",val));
      }
    } while (repeat);

    // httpSeeOther is a http 303 redirect back to the main /form
    server.httpSeeOther(PREFIX "/form");
  }
  else
    outputPins(server, type, true);
}

void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  //server << "<!-- defaultCmd ConnectionType=" << type << " -->\n";
  outputPins(server, type, false);
}
//
// Webduino initilization END


//
// Main Functions
//
void setup() {
  // Here's the pin Mode configuration
  //           D0,        D1       D2   3   4   5   6   7   8   9  10  11  12  13  14  15
  //modes[16] { "OUTPUT", "OUTPUT", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""}
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

  // Set the RelayIn# output to HIGH when we start
  WriteDigitalPin(RelayIn1, HIGH);
  WriteDigitalPin(RelayIn2, HIGH);
  WriteDigitalPin(RelayIn3, HIGH);
  WriteDigitalPin(RelayIn4, HIGH);

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
  Particle.variable("RelayIn1", RelayIn_States[0]);
  Particle.variable("RelayIn2", RelayIn_States[1]);
  Particle.variable("RelayIn3", RelayIn_States[2]);
  Particle.variable("RelayIn4", RelayIn_States[3]);
  Particle.variable("Switch1", Switch_States[0]);
  Particle.variable("Switch2", Switch_States[1]);
  Particle.variable("Switch3", Switch_States[2]);
  Particle.variable("Switch4", Switch_States[3]);
  Particle.variable("PrevSw1", Prev_Switch_States[0]);
  Particle.variable("PrevSw2", Prev_Switch_States[1]);
  Particle.variable("PrevSw3", Prev_Switch_States[2]);
  Particle.variable("PrevSw4", Prev_Switch_States[3]);

  // Set time zone to Eastern USA daylight saving time
  Time.zone(TimeZone);

  //
  // Webduino setup BEGIN
  webserver.begin();

  webserver.setDefaultCommand(&defaultCmd);
  webserver.addCommand("json", &jsonCmd);
  webserver.addCommand("form", &formCmd);
  webserver.addCommand("debug", &debugCmd);
  //
  // Webduino setup END

  // Set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Set LCD background color
  lcd.setRGB(colorR, colorG, colorB);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hello, world!");
  lcd.setCursor(0, 1);
  lcd.print("Setup Complete!!");

  delay(2000);
}

void loop() {
  //
  // Webduino loop END
  // process incoming connections one at a time forever
  webserver.processConnection();
  //
  // Webduino loop END

  // Get the current Millis for this loop
  currentSync = millis();

  //
  // Run below code every 100 mili seconds 1/10th second
  if ( (currentSync - lastMSecSync) > 100 ) {
    // At the start of loop copy Switch$_State over to Prev_Switch#_State so we
    // can tell if State has changed from previous loop
    Prev_Switch_States[0] = Switch_States[0];
    Prev_Switch_States[1] = Switch_States[1];
    Prev_Switch_States[2] = Switch_States[2];
    Prev_Switch_States[3] = Switch_States[3];

    // Read the four anlog inputs for a LOW or HIGH state
    Switch_States[0] = ReadDigitalPin(Switch1);
    Switch_States[1] = ReadDigitalPin(Switch2);
    Switch_States[2] = ReadDigitalPin(Switch3);
    Switch_States[3] = ReadDigitalPin(Switch4);

    // Check if light switches have changed states "been flipped"
    RelayIn_States[0] = CheckSwitchStateChanged(Switch1, Switch_States[0], Prev_Switch_States[0], RelayIn1, RelayIn_States[0]);
    RelayIn_States[1] = CheckSwitchStateChanged(Switch2, Switch_States[1], Prev_Switch_States[1], RelayIn2, RelayIn_States[1]);
    RelayIn_States[2] = CheckSwitchStateChanged(Switch3, Switch_States[2], Prev_Switch_States[2], RelayIn3, RelayIn_States[2]);
    RelayIn_States[3] = CheckSwitchStateChanged(Switch4, Switch_States[3], Prev_Switch_States[3], RelayIn4, RelayIn_States[3]);

    lastMSecSync = currentSync;
  }


  //
  // Run below code every 100 mili second ( 1/10th second )
  if ( (currentSync - lastSecSync) > 1000 ) {
    // Update the display every second when the lastSync is > 1000 Millis ie one second

    // set the cursor to column 0, line 1
    // (note: line 1 is the second row, since counting begins with 0):
    lcd.clear();
    lcd.setCursor(0, 0);

    // Get current time to update LCD display
    time_t time = Time.now();

    String secs = Time.format(time, "%S");
    if ( secs == "00" or secs == "01" ) {
      // Display IP for 2 seconds on LCD panel.
      lcd.print(myIpAddress);
    } else {
      // Time output sample: 01/01/15 01:08PM
      lcd.print(Time.format(time, "%m/%d/%Y %I:%M%p") );
    }
    lcd.setCursor(0, 1);
    lcd.print(secs);
    lcd.print(String::format("sec - %i,%i,%i,%i", RelayIn_States[0], RelayIn_States[1], RelayIn_States[2], RelayIn_States[3]));

    // print the number of seconds since reset:
    //lcd.print(String::format("%i %i %i", currentSync/1000, lastSecSync/1000, currentSync - lastSecSync ));
    lastSecSync = currentSync;
  }

  //
  // Run below code every 60000 mili seconds 60 second
  if ( (currentSync - lastMinSync) > 60000 ) {
    lastMinSync = currentSync;
  }

  //
  // Run below code every 1 day ( 24 hours )
  if ( (currentSync - lastDaySync) > ONE_DAY_MILLIS ) {
    // Sync time with cloud once a day
    // Request time synchronization from the Particle Cloud
    Particle.syncTime();
    lastDaySync = currentSync;
  }
}
