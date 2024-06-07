#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels
#define OLED_RESET D7        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include "icons.h"

#define BUTTON_PIN_BITMASK 0x200000000

RTC_DATA_ATTR int usertempvalue = 24;  //stored in non-volatile memory
unsigned long buttonpresstime = millis();
unsigned long interrupt_time;
volatile unsigned long last_interrupt_time = 0;
volatile bool serviceCallButtonPressed = false;

String DeviceMac;
String MasterDevice="34:85:18:03:50:10";
uint32_t Vbatt = 0;
float Vbattf;
int Vbattpr;
bool showMessageSlave,showMessageMaster;

String room;
String fan_current_mode;
int tempvalue, StatusOfConditioner, resetTemp, SetStatusofConditioner,votedtemp,announcetemp;
int page = 0;

enum MenuOption { SET_TEMPERATURE,
                  SET_CUSTOM,};
MenuOption selectedOption = SET_CUSTOM;


void JsonReceiveHandler(String msg) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, msg);

  // Check for parsing errors
  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract the payload value
  const char* payload = doc["payload"];
  const char* fan_current_mode_char = doc["fanmode"];
  const char* voted_temp_char = doc["votedtemp"]; //show to master
  const char* announce_temp_char = doc["announcetemp"]; //show to slave

  // Print the payload value
  Serial.print("Payload value: ");
  tempvalue = atoi(payload);
  votedtemp=atoi(voted_temp_char);
  announcetemp=atoi(announce_temp_char);
  fan_current_mode = String(fan_current_mode_char);
  showMessageSlave = announcetemp > 0 ? true : false;
  showMessageMaster= votedtemp > 0 ? true : false;
  Serial.println(payload);
  Serial.println(fan_current_mode);
  Serial.print("Voted temperature: ");
  Serial.println(votedtemp);
  Serial.print("Announced temperature: ");
  Serial.println(announcetemp);
  Check_Mode();
}

#include "network.h"

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();  //This function retrieves the cause of the wake-up from the sleep mode.
  //It allows the programmer to identify why the microcontroller exited its sleep state and took control again.

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
    default: Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(A0, INPUT);         // ADC
  pinMode(D1, INPUT_PULLUP);  // increase button
  pinMode(D2, INPUT_PULLUP);  // center button
  pinMode(D3, INPUT_PULLUP);  //decrease button
  pinMode(D10, OUTPUT);
  digitalWrite(D10, HIGH);  //Vdd of screen
  attachInterrupt(digitalPinToInterrupt(D1), increase, FALLING);
  attachInterrupt(digitalPinToInterrupt(D2), pageChange, FALLING);
  attachInterrupt(digitalPinToInterrupt(D3), decrease, FALLING);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  print_wakeup_reason();
  esp_deep_sleep_enable_gpio_wakeup(BIT(D2), ESP_GPIO_WAKEUP_GPIO_LOW);  // When D2 goes low, wake esp up
  for (int i = 0; i < 16; i++) {
    Vbatt = Vbatt + analogReadMilliVolts(A0);  // ADC with correction
  }
  Vbattf = 2 * Vbatt / 16 / 1000.0;  // attenuation ratio 1/2, mV --> V
  Vbattpr = (Vbattf - 3.2) * 100;
  Serial.println(Vbattf, 3);
  delay(300);
  wirelesssetup();
  //The ESP32 sends the booting message, letting the websocket know its MAC address for identification
  DeviceMac = WiFi.macAddress();
  Serial.println(DeviceMac);
  // String bootmsg = DeviceMac + ";" + String(usertempvalue);
  client.send(DeviceMac);
  client2.send(DeviceMac);
}

void loop() {
  if ((millis() - buttonpresstime) / 1000 >= 60) {  // screen active time
    display.clearDisplay();
    display.display();
    client.close();
    client2.close();
    esp_deep_sleep_start();
  }
  if (serviceCallButtonPressed) {
    if(page==1)
    {
      updateNodeRed(usertempvalue,true,false,true);
    }
    if (page == 2) {
      updateNodeRed(usertempvalue,true,false,false);
    } 
    serviceCallButtonPressed = false;
    page = 0;
    tempvalue = 0;
    showMessageSlave=false;
    showMessageMaster=false;   
    delay(3000);
    client.send("Hi");
  }
  if(DeviceMac==MasterDevice)
{
  if (digitalRead(D2) == 0 && (millis() - buttonpresstime) >= 4000) {
    display.clearDisplay();
    display.setCursor(10, 20);
    display.setTextSize(1);
    display.println("Turning off! Bye");
    updateNodeRed(0, false, true, false);
    
    display.display();       // Update the display after printing the message
    delay(2000);             // Wait for 2 seconds to allow message to be seen
    display.clearDisplay();  // Clear the display before deep sleep
    display.display();       // Update the display to clear any remaining content
    client.close();
    client2.close();
    esp_deep_sleep_start();
  }
}
  roomtempview();
  client.poll();
  client2.poll();
  delay(1000);
  Serial.println("bb");
}

void page0() {
  if (tempvalue != 0) {
     Serial.println("dd");
    if(showMessageSlave && DeviceMac!=MasterDevice){
       Serial.println("ee");
    display.setCursor(20, 20);
    display.setTextSize(1);
    char message[50]; // Allocate a character array to hold the formatted message
    sprintf(message, "Temperature is set to %d %cC", announcetemp, 248);

    display.println(message);
    display.setCursor(50, 55);
    display.println(" OK ");
    }
    else{
    display.fillRect(0, 20, 128, 20, SSD1306_BLACK);
    if (StatusOfConditioner == 0) {
      display.setCursor(30, 20);
    } else if (StatusOfConditioner == 1) {
      display.drawBitmap(95, 15, cooling_logo, 25, 25, 2);  // SSD1306_WHITE);
      display.setCursor(95, 45);
      display.println("Cool");
      display.setCursor(0, 20);
    } else if (StatusOfConditioner == 2) {
      display.drawBitmap(95, 15, dry_logo, 25, 25, 2);  // SSD1306_WHITE);
      display.setCursor(95, 45);
      display.println("Dry");
      display.setCursor(0, 20);
    } else if (StatusOfConditioner == 3) {
      display.drawBitmap(95, 15, fan_logo, 25, 25, 2);  // SSD1306_WHITE);
      display.setCursor(95, 45);
      display.println("Fan");
      display.setCursor(0, 20);
    } else if (StatusOfConditioner == 4) {
      display.drawBitmap(95, 15, heating_logo, 25, 25, 2);  // SSD1306_WHITE);
      display.setCursor(95, 45);
      display.println("Heat");
      display.setCursor(0, 20);
    }
    display.setTextSize(3);
    display.println(String(tempvalue) + String(char(247)) + "C");
    Serial.println(String(tempvalue) + String(char(247)) + "C");
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(20, 55);
    display.println(" SET TEMPERATURE ");
    display.setTextColor(SSD1306_WHITE);
  }
   
  } else {
    display.setCursor(0, 20);
    display.setTextSize(1);
    display.println("Waiting for service");
    display.println("response!");
  }
}

void page1() {
  display.setTextSize(1);
  if(showMessageMaster)
  {
  display.setCursor(15, 20);
  if (selectedOption == SET_TEMPERATURE) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }
  char message[50]; // Allocate a character array to hold the formatted message

// Format the message with sprintf
   sprintf(message, " -> Set to %d %cC", votedtemp, 248);


// Print the message
  display.println(message);
  }
  display.setCursor(15, 30);
  if (selectedOption == SET_CUSTOM) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }
  display.print("-> Custom Setting");
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(1, 55);
  display.println(" BACK " + String(char(178)) + "   " + String(char(239)) + "  " + String(char(178)) + " TOGGLE ");
}
void page2() { //Page 1 for slave
  display.setCursor(2, 55);
  display.println(" COOLER " + String(char(178)) + " " + String(char(239)) + " " + String(char(178)) + " WARMER ");
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setCursor(0, 10);
  if (tempvalue != 0) {
    display.println("    Current: " + String(tempvalue) + String(char(247)) + "C    ");
  } else {
    display.println(" Changes won't apply ");
  }
  display.println("                     ");
  //display.setCursor(10, 20);
  display.setTextSize(3);
  display.println("  " + String(usertempvalue) + String(char(247)) + "C ");
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}

void roomtempview(void) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  display.setCursor(0, 0);              // Start at top-left corner
  if (WiFi.status() != WL_CONNECTED) {
    display.println("Offline");
  } else {
    display.println("Room: TUAS 3555");
  }
  display.setCursor(105, 0);
  display.println(String(Vbattpr) + "%");
  if (page == 0) {
     Serial.println("ff");
    page0();
  } else if (page == 1) {
    if(DeviceMac==MasterDevice)
    {
      page1();
    }
    else{
      Serial.println("gg");
      page=2;
    }
  } else if (page == 2) {
    page2();
  } 
  display.display();
}

void updateNodeRed(int Updated_User_Temp,
                   bool Is_Temp_Updated,
                   bool Is_Turn_Off,
                   bool Set_Voted
                   ) {
  StaticJsonDocument<200> doc;
  doc["DeviceID"] = DeviceMac;
  doc["Updated_User_Temp"] = Updated_User_Temp;
  doc["Is_Temp_Updated"] = Is_Temp_Updated;
  doc["Is_Turn_Off"]=Is_Turn_Off;
  doc["Set_Voted"] = Set_Voted;

  // Serialize JSON to string
  String output;
  serializeJson(doc, output);
  client2.send(output);
}

void decrease() {
  interrupt_time = millis();
  if (page == 1 && interrupt_time - last_interrupt_time > 300 && millis() > 800) {
    //selectedOption = (selectedOption == SET_MODE) ? SET_TEMPERATURE : SET_MODE;
    page = 0;
  }
  if (interrupt_time - last_interrupt_time > 300 && millis() > 800 && usertempvalue > 17 && page == 2) {
    usertempvalue -= 1;
    // delay(200);  // Debouncing delay
  }
  buttonpresstime = millis();
  last_interrupt_time = interrupt_time;
}

void increase() {
  interrupt_time = millis();
  if (page == 1 && interrupt_time - last_interrupt_time > 300 && millis() > 800) {
     if(showMessageMaster)
     {
      selectedOption = (selectedOption == SET_TEMPERATURE) ? SET_CUSTOM : SET_TEMPERATURE;
     }
    else{
      selectedOption=SET_CUSTOM;
    }
  }
  if (interrupt_time - last_interrupt_time > 300 && millis() > 800 && usertempvalue < 28 && page == 2) {
    usertempvalue += 1;
    // delay(200);
  }
  buttonpresstime = millis();
  last_interrupt_time = interrupt_time;
}

void pageChange() {
 // if (tempvalue != 0) {
    interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 300 && millis() > 800) {
      if (page == 0) {
        if(DeviceMac==MasterDevice)
        {
          page = 1;
        }
        else{
          if(showMessageSlave)
          {
            showMessageSlave=false;
          }
          else{
            page=2;
          }
        }
      } else if (page == 1) {
        if (selectedOption == SET_TEMPERATURE) {
          //page = 2;  // Assign function for Set Temperature
          serviceCallButtonPressed=true;
        } else {
          page=2; //through master
        } 
      } else if (page == 2) {
        serviceCallButtonPressed = true;
    }
    buttonpresstime = millis();
    last_interrupt_time = interrupt_time;
  }
}
void Check_Mode() {
  if (fan_current_mode == "Cool") {
    StatusOfConditioner = 1;
  } else if (fan_current_mode == "Dry") {
    StatusOfConditioner = 2;
  } else if (fan_current_mode == "Fan") {
    StatusOfConditioner = 3;
  } else if (fan_current_mode == "Heat") {
    StatusOfConditioner = 4;
  } else {
    // Default case if none of the above conditions match
    StatusOfConditioner = 0;  // or any other default value you want
  }
}

