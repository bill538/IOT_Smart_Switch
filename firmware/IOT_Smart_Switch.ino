//
// IOT Smart Switch variable initilization BEGIN
#include "IOT_Smart_Switch.h"

//
// SWITCHDATA
//   [#] = [0]=Empty, [1]=Switch1, [2]=Switch2, [3]=Switch3, [4]=Switch4
//   [#][0] = RelayIn Pin
//   [#][1] = RelayIn Pin Mode
//   [#][2] = RelayIn State
//   [#][3] = Switch Pin
//   [#][4] = Switch Mode
//   [#][5] = Switch State
//   [#][6] = Previous Switch State
//   [#][7] = TimerStartTime
//   [#][8] = TimerTime
//   [#][9] = CronDurationMins
//   [#][10] = CronStartTime
//   [#][11] = CronSec - seconds between (0-59) & -1=*
//   [#][12] = CronMin - minutes between (0-59) & -1=*
//   [#][13] = CronHour - hours between (0-23) & -1=*
//   [#][14] = CronDay - day of the month (1-31) & -1=*
//   [#][15] = CronMonth - month of the year (1-12) & -1=*
//   [#][16] = CronWeekDay - day of the week (0-6 with 0=Sunday) & -1=*
String WEBTITLE="Particle Photon IOT Smart Switch";
int SWITCHCOUNT = 5;
//int SWITCHCOUNT = sizeof(SWITCHDATA) / sizeof(int);
int SWITCHDATASIZE = 17;
//int SWITCHDATASIZE = sizeof(SWITCHDATA[0]) / sizeof(int);
int SWITCHDATA [5][17] { //initialize to zero
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {D2, OUTPUT, HIGH, A2, INPUT, HIGH, HIGH, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {D3, OUTPUT, HIGH, A3, INPUT, HIGH, HIGH, 0, 0, 1, 0, 30, 32, 12, 21, 2, 0},
  {D4, OUTPUT, HIGH, A4, INPUT, HIGH, HIGH, 0, 0, 2, 0, 40, -1, -1, -1, -1, -1},
  {D5, OUTPUT, HIGH, A5, INPUT, HIGH, HIGH, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

int ic2_sda = D0;
int i2c_scl = D1;
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
void SwitchCronFunction(){
  int i=0;
  for (i = 1; i < SWITCHCOUNT; ++i) {
    //   [#][9] = CronDurationMins
    //   [#][10] = CronStartTime
    //   [#][11] = CronSec - seconds between (0-59) & -1=*
    //   [#][12] = CronMin - minutes between (0-59) & -1=*
    //   [#][13] = CronHour - hours between (0-23) & -1=*
    //   [#][14] = CronDay - day of the month (1-31) & -1=*
    //   [#][15] = CronMonth - month of the year (1-12) & -1=*
    //   [#][16] = CronWeekDay - day of the week (0-6 with 0=Sunday) & -1=*    // Check if Cron Duration is non zero
    if ( SWITCHDATA[i][9] > 0 ) {
      time_t time = Time.now();

      // Check if CronStartTime is set and past the schduled duration
      if ( SWITCHDATA[i][10] > 0 ) {
        // Check if past Duration
        if ( ((time-SWITCHDATA[i][10])/60) > SWITCHDATA[i][9] ) {
          Particle.publish("SwitchCronFunction", String::format("Past Duration - A - sec:%i, min:%i, i:%i, [9]:%i, [10]:%i, [11]:%i, [12]:%1",Time.second(time),Time.minute(time),i,SWITCHDATA[i][9],SWITCHDATA[i][10],SWITCHDATA[i][11],SWITCHDATA[i][12]));
          SWITCHDATA[i][10] = 0;
        }
      } else {
        if ( Time.second(time) == SWITCHDATA[i][11] or SWITCHDATA[i][11] == -1 or
             Time.minute(time) == SWITCHDATA[i][12] or SWITCHDATA[i][12] == -1 or
             Time.hour(time) == SWITCHDATA[i][13] or SWITCHDATA[i][13] == -1 or
             Time.day(time) == SWITCHDATA[i][14] or SWITCHDATA[i][14] == -1 or
             Time.month(time) == SWITCHDATA[i][15] or SWITCHDATA[i][15] == -1 or
             Time.weekday(time)-1 == SWITCHDATA[i][16] or SWITCHDATA[i][16] == -1
           ) {
            Particle.publish("SwitchCronFunction", String::format("Matched - B - sec:%i, min:%i, i:%i, SWITCHDATA[i][10]:%i, SWITCHDATA[i][11]:%i",Time.second(time),Time.minute(time),i,SWITCHDATA[i][10],SWITCHDATA[i][11]));
            SWITCHDATA[i][10]=time;
        }
        if ( Time.second(time) == SWITCHDATA[i][10] ) {
          Particle.publish("SwitchCronFunction", String::format("Matched - C - sec:%i, min:%i, i:%i, SWITCHDATA[i][10]:%i, SWITCHDATA[i][11]:%i",Time.second(time),Time.minute(time),i,SWITCHDATA[i][10],SWITCHDATA[i][11]));
          SWITCHDATA[i][10]=time;
        }
      }
    }
  }
}


// You can't pass argument to the function being called by Timer.
// The work around is just create dedicated function for each swicth called Switch#TimerFunction
Timer TimerSwitch1(20000, Switch1TimerFunction, true);
Timer TimerSwitch2(30000, Switch2TimerFunction, true);
Timer TimerSwitch3(40000, Switch3TimerFunction, true);
Timer TimerSwitch4(50000, Switch4TimerFunction, true);
// Timer that checks if any switchs have crons enabled
Timer TimerSwitchCron(1000, SwitchCronFunction, false);
// Timer that updates the LCD display every second.
Timer TimerUpdateLCD(1000, UpdateLCD, false);

//
//  This function is used to set timer time and start timer.
int CloudSwitchTimer(int SwitchNum, int SwitchMins) {
  int SwitchMills = SwitchMins*60000;

  switch (SwitchNum) {
      case 1:   TimerSwitch1.changePeriod(SwitchMills);
                SWITCHDATA[1][8] = SwitchMins*60;
                TimerSwitch1.reset();
                SWITCHDATA[1][7] = Time.now();
      break;
      case 2:   TimerSwitch2.changePeriod(SwitchMills);
                SWITCHDATA[2][8] = SwitchMins*60;
                TimerSwitch2.reset();
                SWITCHDATA[2][7] = Time.now();
      break;
      case 3:   TimerSwitch3.changePeriod(SwitchMills);
                SWITCHDATA[3][8] = SwitchMins*60;
                TimerSwitch3.reset();
                SWITCHDATA[3][7] = Time.now();
      break;
      case 4:   TimerSwitch4.changePeriod(SwitchMills);
                SWITCHDATA[4][8] = SwitchMins*60;
                TimerSwitch4.reset();
                SWITCHDATA[4][7] = Time.now();
      break;
      default:  return -1;
  }
}

//
// Update the display every second
void UpdateLCD () {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.clear();
  lcd.setCursor(0, 0);

  // Get current time to update LCD display
  time_t time = Time.now();

  String secs = Time.format(time, "%S");
  if ( secs == "00" or secs == "01" or secs == "0" or secs == "1" ) {
    // Display IP for 2 seconds on LCD panel.
    lcd.print(myIpAddress);
  } else {
    // Time output sample: 01/01/15 01:08PM
    lcd.print(Time.format(time, "%m/%d/%Y %I:%M%p") );
  }
  lcd.setCursor(0, 1);
  lcd.print(secs);
  lcd.print(String::format("sec - %i,%i,%i,%i", SWITCHDATA[1][2], SWITCHDATA[2][2], SWITCHDATA[3][2], SWITCHDATA[4][2]));
}
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
      case 1:   RelayIn = SWITCHDATA[1][0];
                RelayIn_State = SWITCHDATA[1][2];
      break;
      case 2:   RelayIn = SWITCHDATA[2][0];
                RelayIn_State = SWITCHDATA[2][2];
      break;
      case 3:   RelayIn = SWITCHDATA[3][0];
                RelayIn_State = SWITCHDATA[3][2];
      break;
      case 4:   RelayIn = SWITCHDATA[4][0];
                RelayIn_State = SWITCHDATA[4][2];
      break;
      default:  return -2;
  }

  Particle.publish("CloudRelayInChange", String::format("B - Switch:%i, RelayIn Pin:D%i, RelayIn_State:%i",Relay,RelayIn,RelayIn_State));

  // Check if RelayIn_State is 1(OFF) or 0(ON). Then with the opposit to the pin.
  if ( RelayIn_State == 0 ) {
    WriteDigitalPin(RelayIn, 1);
    RelayIn_State = 1;
    // Rest any timer values and stop Timer for OFF Switch
    SWITCHDATA[Relay][7] = 0;
    //TimerTime[Relay] = 0;
    SWITCHDATA[Relay][8] = 0;
    switch (Relay) {
        case 1: TimerSwitch1.stop(); break;
        case 2: TimerSwitch2.stop(); break;
        case 3: TimerSwitch3.stop(); break;
        case 4: TimerSwitch4.stop(); break;
       default: return -3;
    }
  } else if ( RelayIn_State == 1 ) {
    WriteDigitalPin(RelayIn, 0);
    RelayIn_State = 0;
  } else {
    return -4;
  }

  // Set Global Variable for RelayIn#_State based on new state
  switch (Relay) {
      case 1:   SWITCHDATA[1][2] = RelayIn_State;
      break;
      case 2:   SWITCHDATA[2][2] = RelayIn_State;
      break;
      case 3:   SWITCHDATA[3][2] = RelayIn_State;
      break;
      case 4:   SWITCHDATA[4][2] = RelayIn_State;
      break;
      default:  return -5;
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
    "<html>\n<head>\n<title>WEBTITLE</title>\n</head>\n";

  if (type == WebServer::POST) {
    server.httpFail();
    return;
  }

  server.httpSuccess();
  server.printP(htmlHead);

  if (type == WebServer::HEAD)
    return;

  int i;
  int j;
  server << "SWITCHDATA:<br>\n";
  server << "<table border=\"1\">\n";
  server << "  <tr>\n";
  server << "    <th>Number</th>\n";
  server << "    <th>RelayIn Pin (0)</th>\n";
  server << "    <th>RelayIn Pin Mode (1)</th>\n";
  server << "    <th>RelayIn State (2)</th>\n";
  server << "    <th>Switch Pin (3)</th>\n";
  server << "    <th>Switch Pin Mode (4)</th>\n";
  server << "    <th>Switch State (5)</th>\n";
  server << "    <th>Previous Switch State (6)</th>\n";
  server << "    <th>Timer Start Time (7)</th>\n";
  server << "    <th>Timer Time (8)</th>\n";
  server << "    <th>Cron Duration in Minutes (9)</th>\n";
  server << "    <th>Cron Start Time (10)</th>\n";
  server << "    <th>Cron Secs (11)</th>\n";
  server << "    <th>Cron Mins (12)</th>\n";
  server << "    <th>Cron Hours (13)</th>\n";
  server << "    <th>Cron Day (14)</th>\n";
  server << "    <th>Cron Month (15)</th>\n";
  server << "    <th>Cron Week Day (16)</th>\n";
  server << "  </tr>\n";
  for (i = 0; i < SWITCHCOUNT; i++) {
    server << "  <tr>\n";
    server << "    <td>" << i << "</td>\n";
    for (j = 0; j < SWITCHDATASIZE; j++) {
      server << "    <td>" << SWITCHDATA[i][j] << "</td>\n";
    }
    server << "  </tr>\n";
  }
  server << "</table>";

  for (i = 1; i < SWITCHCOUNT; ++i) {
    server << "RelayIn " << i << " Pin Number: " << SWITCHDATA[i][0] << "<br>\n";
    server << "RelayIn " << i << " State: " << SWITCHDATA[i][2] << "<br>\n";
    server << "Switch " << i << " Pin Number: " << SWITCHDATA[i][3] << "<br>\n";
    server << "Switch " << i << " State: " << SWITCHDATA[i][5] << "<br>\n";
    server << "Previous Switch " << i << " State: " << SWITCHDATA[i][6] << "<br>\n";
    server << "Time.now() " << Time.now() << "<br>\n";
    server << "Timer " << i << " Start Time: " << SWITCHDATA[i][7] << "<br>\n";
    server << "Time.now() - TimerStartTime[i]: " << Time.now() - SWITCHDATA[i][8] << "<br>\n";
    server << "Timer " << i << " Time: " << SWITCHDATA[i][8] << "<br>\n";
    server << "TimerStartTime[i]-(Time.now() - TimerTime[i]) " << i << " Time: " << SWITCHDATA[i][7]-(Time.now()-SWITCHDATA[i][8]) << "<br>\n";
    server << "(TimerStartTime[i]-(Time.now() - TimerTime[i]))/60 " << i << " Time: " << (SWITCHDATA[i][7]-(Time.now()-SWITCHDATA[i][8]))/60 << "<br>\n";
    server << "(TimerStartTime[i]-(Time.now() - TimerTime[i]))%60 " << i << " Time: " << (SWITCHDATA[i][7]-(Time.now()-SWITCHDATA[i][8]))%60 << "<br>\n";
    server << "<hr>\n";
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
    "<script language=\"JavaScript\">\n"
    "<!--\n"
    "var AllArr = [\"Switch1_Timer\", \"Switch2_Timer\", \"Switch3_Timer\", \"Switch4_Timer\"];\n"
    "function isArray(value) {\n"
    "  if (value) {\n"
    "    if (typeof value === 'object') {\n"
    "      return (Object.prototype.toString.call(value) == '[object Array]')\n"
    "    }\n"
    "  }\n"
    "  return false;\n"
    "}\n"
    "function ChangeElementState(state,ListArr) {\n"
    "	state=!state\n"
    "	if (isArray(ListArr)) {\n"
    "    for (var i = 0, len = ListArr.length; i < len; ++i) {\n"
    "			 if (IOT.elements[ListArr[i]]) {"
    "        IOT.elements[ListArr[i]].disabled = state;\n"
    "      }"
    "    }\n"
    " } else {\n"
    "		IOT.elements[ListArr].disabled = state;\n"
    "	}\n"
    "}\n"
    "//-->\n"
    "</script>\n"
    "<style type=\"text/css\">\n"
    "BODY { font-family: sans-serif }\n"
    "H1 { font-size: 14pt; text-decoration: underline }\n"
    "P  { font-size: 10pt; }\n"
    "</style>\n"
    "</head>\n"
    "<body onload=ChangeElementState(false,AllArr);>\n";

  int i;
  server.httpSuccess();
  server.printP(htmlHead);
  //server << "<!-- outputPins addControls=" << addControls << " ConnectionType=" << type << " -->\n";

  if (addControls) {
    server << "<form name=\"IOT\" action='" PREFIX "/form' method='post'>\n";

    server << "<h1> Switch States - " << Time.format(Time.now(), TIME_FORMAT_DEFAULT) << " </h1> <p>\n";

    // Check if RelayIn State(SWITCHDATA[#][2]) are 1(OFF) or 0(ON). Then diplay ON/OFF details appropreiatly
    for (int i = 1; i < SWITCHCOUNT; i++) {
      server << "Switch " << i << " is currently <B>";
      if ( SWITCHDATA[i][2] == 0 ) {
        // Get time left on timer
        int LeftMins = (SWITCHDATA[i][7]-(Time.now()-SWITCHDATA[i][8]))/60;
        int LeftSecs = ((SWITCHDATA[i][7]-(Time.now()-SWITCHDATA[i][8]))%60);
        server << "ON</B>. <br>\n<label> Turn Switch " << i;
        server << " <B>OFF</B> <input type=checkbox onclick=\"ChangeElementState(this.checked,'Switch";
        server << i << "_Timer')\" name=Switch" << i;
        server << "_State value=" << i << "/></label><br>\nTimer Time Left: ";
        if ( SWITCHDATA[i][7] > 0 ) {
          server << LeftMins << ":" << String::format("%02d", LeftSecs) << "<br>\n<hr>\n<br>\n";
        } else {
          server << " <br>\n<hr>\n<br>\n";
        }
      } else {
        server << "OFF</B>. <br>\n<label> Turn Switch " << i;
        server << " <B>ON</B> <input type=checkbox onclick=\"ChangeElementState(this.checked,'Switch";
        server << i << "_Timer')\" name=Switch" << i;
        server << "_State value=" << i << " /> </label><br>\nSwitch " << i;
        server << " Timer(Minutes): <input type=\"number\" name=\"Switch" << i;
        server << "_Timer\" value=0 min=\"0\" max=\"71582\" /><br>\n<hr>\n<br>\n";
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

      //int pin = strtoul(name + 1, NULL, 10);
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
  int i=0;

  // Let's also make sure both LEDs are off when we start.
  pinMode(Led1, OUTPUT);
  pinMode(Led2, OUTPUT);
  digitalWrite(Led1, LOW);
  digitalWrite(Led2, LOW);

  // Loop to set the proper pin mode
  for (int i = 1; i < SWITCHCOUNT; i++) {
    //pinMode(SWITCHDATA[i][0], SWITCHDATA[i][1]);   // Set RealayIn# pinMode
    pinMode(SWITCHDATA[i][0], OUTPUT);               // Set RealayIn# pinMode
    // Default the the Output RelatIn# Pin to default states
    WriteDigitalPin(SWITCHDATA[i][0], SWITCHDATA[i][2]);
    // Set phyical switch # pin mode to proper SWITCHDATA[#][4]
    pinMode(SWITCHDATA[i][3], INPUT);                // Set Switch# pinMode
    //pinMode(SWITCHDATA[i][3], SWITCHDATA[i][4]);   // Set Switch# pinMode
  }

  // initialize Switch states at startup
  for (i = 1; i < SWITCHCOUNT; ++i) {
    // Read the four anlog inputs for a LOW or HIGH state
    // Switch_State[#] = ReadDigitalPin(SWITCHDATA[1][3]);
    SWITCHDATA[i][5] = ReadDigitalPin(SWITCHDATA[i][3]);
    // Prev_Switch_State = Switch_State;
    SWITCHDATA[i][6] = SWITCHDATA[i][5];
  }

  // Publish devi's IP
  // Build IP Address and publish
  IPAddress myIp = WiFi.localIP();
  sprintf(myIpAddress, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);
  Particle.publish("IP", myIpAddress);

  // Enable could access to function
  // https://api.particle.io/v1/devices/2e0048000a47343432313031/WritePin?access_token=***************?args=D4,HIGH
  Particle.function("AccessPin", CloudAccessPin);
  Particle.function("RelayInChg", CloudRelayInChange);

  // Export key variables to the cloud for access
  // https://api.particle.io/v1/devices/2e0048000a47343432313031/IP?access_token=***************
  Particle.variable("IP", myIpAddress);
  Particle.variable("SSID", SSID);
  for (i = 1; i < SWITCHCOUNT; ++i) {
    Particle.variable(String::format("Switch%i",i), SWITCHDATA[i][5]);
    Particle.variable(String::format("RelayIn%i",i), SWITCHDATA[i][2]);
    Particle.variable(String::format("PrevSw1%i",i), SWITCHDATA[i][6]);
  }

  // Set time zone to Eastern USA daylight saving time
  Time.zone(TimeZone);

  //
  // Webduino setup BEGIN
  webserver.begin();
  webserver.setDefaultCommand(&defaultCmd);     // Web path /
  webserver.addCommand("json", &jsonCmd);       // Web path /json
  webserver.addCommand("form", &formCmd);       // Web path /form
  webserver.addCommand("debug", &debugCmd);     // Web path /debug
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
  //
  // Enable LCD update Timer
  TimerUpdateLCD.start();
  //
  // Enable CronTimer
  TimerSwitchCron.start();
}

void loop() {
  //
  // Webserver process incoming connections one at a time forever
  webserver.processConnection();

  // Get the current Millis for this loop
  currentSync = millis();

  //
  // Run below code every 100 mili seconds 1/10th second
  if ( (currentSync - lastMSecSync) > 100 ) {
    // Loop on each switch
    int i=0;
    for (i = 1; i < SWITCHCOUNT; ++i) {
      // At the start of loop copy Switch$_State over to Prev_Switch#_State so we
      // can tell if State has changed from previous loop
      // Prev_Switch_State = Switch_State;
      SWITCHDATA[i][6] = SWITCHDATA[i][5];
      // Read the four anlog inputs for a LOW or HIGH state
      // Switch_State[#] = ReadDigitalPin(SWITCHDATA[1][3]);
      SWITCHDATA[i][5] = ReadDigitalPin(SWITCHDATA[i][3]);
      // Check if light switches have changed states "been flipped"
      // RelayIn_State = CheckSwitchStateChanged(switch_pin,   current_switch_state, prev_switch_state, relayin,         relayin_state)
      SWITCHDATA[i][2] = CheckSwitchStateChanged(SWITCHDATA[i][3], SWITCHDATA[i][5], SWITCHDATA[i][6], SWITCHDATA[i][0], SWITCHDATA[i][2]);
    }
    lastMSecSync = currentSync;
  }

  //
  // Run below code every 100 mili second ( 1/10th second )
  //if ( (currentSync - lastSecSync) > 1000 ) {
  //  lastSecSync = currentSync;
  //}

  //
  // Run below code every 60000 mili seconds 60 second
  //if ( (currentSync - lastMinSync) > 60000 ) {
  //  lastMinSync = currentSync;
  //}

  //
  // Run below code every 1 day ( 24 hours )
  if ( (currentSync - lastDaySync) > ONE_DAY_MILLIS ) {
    // Sync time with cloud once a day
    // Request time synchronization from the Particle Cloud
    Particle.syncTime();
    Particle.publish("syncTime", String::format("Time synced - currentSync:%i, lastDaySync:%i",currentSync,lastDaySync));
    lastDaySync = currentSync;
  }
}
