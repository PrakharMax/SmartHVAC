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

uint32_t Vbatt = 0;
float Vbattf;
int Vbattpr;

String room;
String fan_current_mode;
int tempvalue, StatusOfConditioner, resetTemp, SetStatusofConditioner;
int page = 0;

enum MenuOption { SET_TEMPERATURE,
                  SET_MODE,
                  SET_FANSPEED };
MenuOption selectedOption = SET_TEMPERATURE;

enum Option { FAN_MODE,
              DRY_MODE };
Option currentOption = FAN_MODE;

enum FanSpeed { HIGHER,
                MED,
                LOWER };
FanSpeed selectedFanSpeed = HIGHER;


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

  // Print the payload value
  Serial.print("Payload value: ");
  tempvalue = atoi(payload);
  fan_current_mode = String(fan_current_mode_char);
  Serial.println(payload);
  Serial.println(fan_current_mode);
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
  // String bootmsg = DeviceMac + ";" + String(usertempvalue);
  client.send("Hi");
  client2.send("Moi");
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
    if (page == 2) {
      if (usertempvalue > tempvalue) {
        updateNodeRed(usertempvalue, true, 0, false, false, true,false,"");
      } else {
        updateNodeRed(usertempvalue, true, 0, false, false, false,false,"");
      }
    } else if (page == 3) {
      updateNodeRed(usertempvalue, false, SetStatusofConditioner, true, false, false,false,"");
    } else if (page == 4) {
      if(selectedFanSpeed==HIGHER){
        updateNodeRed(usertempvalue, false, SetStatusofConditioner, false, false, false,true,"Hig");
      }
      else if(selectedFanSpeed==MED){
        updateNodeRed(usertempvalue, false, SetStatusofConditioner, false, false, false,true,"Mid");
      }
      else{
        updateNodeRed(usertempvalue, false, SetStatusofConditioner, false, false, false,true,"Low");
      }
    }
    serviceCallButtonPressed = false;
    page = 0;
    tempvalue = 0;
    delay(3000);
    client.send("Hi");
  }
  if (digitalRead(D2) == 0 && (millis() - buttonpresstime) >= 4000) {
    display.clearDisplay();
    display.setCursor(10, 20);
    display.setTextSize(1);
    display.println("Turning off! Bye");
    updateNodeRed(0, false, 0, false, true, false,false,"");
    
    display.display();       // Update the display after printing the message
    delay(2000);             // Wait for 2 seconds to allow message to be seen
    display.clearDisplay();  // Clear the display before deep sleep
    display.display();       // Update the display to clear any remaining content
    client.close();
    client2.close();
    esp_deep_sleep_start();
  }
  roomtempview();
  client.poll();
  client2.poll();
  delay(1000);
  //Serial.print(digitalRead(D2));
}

void page0() {
  if (tempvalue != 0) {
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
    display.setCursor(50, 55);
    display.println(" Menu ");
    display.setTextColor(SSD1306_WHITE);
  } else {
    display.setCursor(0, 20);
    display.setTextSize(1);
    display.println("Waiting for service");
    display.println("response!");
  }
}

void page1() {
  display.setTextSize(1);
  display.setCursor(15, 20);
  if (selectedOption == SET_TEMPERATURE) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }
  display.print("1) Set Temperature");
  display.setCursor(15, 30);
  if (selectedOption == SET_MODE) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }
  display.print("2) Set Mode       ");
  display.setCursor(15, 40);
  if (selectedOption == SET_FANSPEED) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }
  display.print("3) Set Fan Speed  ");
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(1, 55);
  display.println(" BACK " + String(char(178)) + "   " + String(char(239)) + "  " + String(char(178)) + " TOGGLE ");
}
void page2() {
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

void page3() {
  display.setCursor(1, 55);
  display.println(" BACK " + String(char(178)) + "   " + String(char(239)) + "  " + String(char(178)) + " TOGGLE ");
  display.setCursor(15, 20);
  display.setTextSize(1);
  if (currentOption == FAN_MODE) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }
  display.print("  1) Fan Mode  ");
  display.setCursor(15, 35);
  if (currentOption == DRY_MODE) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }
  display.print("  2) Dry Mode  ");
}

void page4() {
  display.setTextSize(1);
  display.setCursor(15, 20);
  if (selectedFanSpeed == HIGHER) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }
  display.print(" 1) HIGH ");
  display.setCursor(15, 30);
  if (selectedFanSpeed == MED) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }
  display.print(" 2) MEDIUM ");
  display.setCursor(15, 40);
  if (selectedFanSpeed == LOWER) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }
  display.print(" 3) LOW ");
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(1, 55);
  display.println(" BACK " + String(char(178)) + "   " + String(char(239)) + "  " + String(char(178)) + " TOGGLE ");
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
    page0();
  } else if (page == 1) {
    page1();
  } else if (page == 2) {
    page2();
  } else if (page == 3) {
    page3();
  } else if(page==4) {
    page4();
  }
  display.display();
}

void updateNodeRed(int Updated_User_Temp,
                   bool Is_Temp_Updated,
                   int SetStatusConditioner,
                   bool Is_Mode_Updated,
                   bool Is_Turn_Off,
                   bool Is_Heat_Required,
                   bool Is_Fan_Speed_Set,
                   String Fan_Speed_Set) {
  StaticJsonDocument<200> doc;
  doc["DeviceID"] = DeviceMac;
  doc["Updated_User_Temp"] = Updated_User_Temp;
  doc["Is_Temp_Updated"] = Is_Temp_Updated;
  doc["SetStatusConditioner"] = SetStatusConditioner;
  doc["Is_Mode_Updated"] = Is_Mode_Updated;
  doc["Is_Turn_Off"] = Is_Turn_Off;
  doc["Is_Heat_Required"] = Is_Heat_Required;
  doc["Is_Fan_Speed_Set"] = Is_Fan_Speed_Set;
  doc["Fan_Speed_Set"] = Fan_Speed_Set;

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
  if ((page == 3 || page == 4) && interrupt_time - last_interrupt_time > 300 && millis() > 800) {
    //currentOption = (currentOption == FAN_MODE) ? DRY_MODE : FAN_MODE;
    page = 1;
  }
  buttonpresstime = millis();
  last_interrupt_time = interrupt_time;
}

void increase() {
  interrupt_time = millis();
  if (page == 1 && interrupt_time - last_interrupt_time > 300 && millis() > 800) {
    //selectedOption = (selectedOption == SET_TEMPERATURE) ? SET_MODE : SET_TEMPERATURE;
    if (selectedOption == SET_TEMPERATURE) {
      selectedOption = SET_MODE;
    } else if (selectedOption == SET_MODE) {
      selectedOption = SET_FANSPEED;
    } else {
      selectedOption = SET_TEMPERATURE;
    }
  }
  if (interrupt_time - last_interrupt_time > 300 && millis() > 800 && usertempvalue < 28 && page == 2) {
    usertempvalue += 1;
    // delay(200);
  }
  if (page == 3 && interrupt_time - last_interrupt_time > 300 && millis() > 800) {
    currentOption = (currentOption == FAN_MODE) ? DRY_MODE : FAN_MODE;
  }
  if (page == 4 && interrupt_time - last_interrupt_time > 300 && millis() > 800) {
    //selectedOption = (selectedOption == SET_TEMPERATURE) ? SET_MODE : SET_TEMPERATURE;
    if (selectedFanSpeed == HIGHER) {
      selectedFanSpeed = MED;
    } else if (selectedFanSpeed == MED) {
      selectedFanSpeed = LOWER;
    } else {
      selectedFanSpeed = HIGHER;
    }
  }
  buttonpresstime = millis();
  last_interrupt_time = interrupt_time;
}

void pageChange() {
 // if (tempvalue != 0) {
    interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 300 && millis() > 800) {
      if (page == 0) {
        page = 1;
      } else if (page == 1) {
        if (selectedOption == SET_TEMPERATURE) {
          page = 2;  // Assign function for Set Temperature
        } else if (selectedOption == SET_MODE) {
          page = 3;  // Assign function for Set Mode
        } else {
          page = 4;
        }
      } else if (page == 2) {
        serviceCallButtonPressed = true;
      } else if (page == 3) {
        SetStatusofConditioner = (currentOption == FAN_MODE) ? 3 : 2;
        serviceCallButtonPressed = true;
      } else if (page == 4) {
        serviceCallButtonPressed = true;
      }
    }
    buttonpresstime = millis();
    last_interrupt_time = interrupt_time;
  }
//}

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
