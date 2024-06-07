// Definition of Wi-Fi network name and password
#define WiFiname "DNA-Mokkula-2G-7dTp7c"
#define WiFipw "41951404706"

// Definition of the WebSocket server URL
#define NEPPI_SOCKET "ws://192.168.1.101:1880/ws/hex/pp"
#define NEPPI_SOCKET_2 "ws://192.168.1.101:1880/ws/testdevices"
const char* websockets_connection_string = NEPPI_SOCKET;
const char* websockets_connection_string_2 = NEPPI_SOCKET_2;
//

// Including necessary libraries
using namespace websockets;
// Creating a WebSocket client instance
WebsocketsClient client, client2;

// Callback function to handle incoming WebSocket messages(Getting message from NodeRed)
void onMessageCallback(WebsocketsMessage message) {
  // Print the received message to the serial monitor
  Serial.println("Message received:");
  Serial.println(message.data());
  JsonReceiveHandler(message.data());
  //messagehandler(message.data());
}

// Callback function to handle WebSocket events
void onEventsCallback(WebsocketsEvent event, String data) {
  // Checking the type of WebSocket event
  if (event == WebsocketsEvent::ConnectionOpened) {
    // If the connection is opened, print a message to the serial monitor
    Serial.println("Connection Opened");
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    // If the connection is closed, print a message to the serial monitor
    Serial.println("Connection Closed");
  } else if (event == WebsocketsEvent::GotPing) {
    // If a ping is received, print a message to the serial monitor
    Serial.println("Got a Ping!");
  } else if (event == WebsocketsEvent::GotPong) {
    // If a pong is received (response to a ping), do nothing (commented out)
    Serial.println("Got a Pong!");
  }
}

// Function to display setup screen on an OLED display
void setupscreen(void) {
  // Clear the OLED display
  display.clearDisplay();
  // Set text size and color
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  // Set cursor position for battery percentage display
  display.setCursor(50, 0);
  // Display battery percentage
  display.println("BATTERY: " + String(Vbattpr) + "%");
  // Set cursor position for connection status message
  display.setCursor(30, 20);  // Start at top-left corner
  // Display "Please wait" message
  display.println(F("Please wait"));
  // Set cursor position for connection status message
  display.setCursor(30, 30);
  // Display "Connecting..." message
  display.println(F("Connecting..."));
  // Update OLED display
  display.display();  //Do it after adding elements
}

// Function to setup Wi-Fi connection
void wirelesssetup() {
  // Attempt to connect to Wi-Fi network using defined network name and password
  WiFi.begin(WiFiname, WiFipw);
  // Retry connecting up to 3 times if unsuccessful
  for (int i = 0; i < 3 && WiFi.status() != WL_CONNECTED; i++) {
    // Display setup screen while attempting to connect
    setupscreen();
    // Delay before retrying connection
    delay(1000);
    // Record the time button is pressed
    buttonpresstime = millis();
  }
  // If Wi-Fi connection is still not established after retrying
  if (WiFi.status() != WL_CONNECTED) {
    // Print a message to the serial monitor indicating no Wi-Fi connection
    Serial.println("No WiFi!");
  }
  // Register callback functions for WebSocket events
  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);
  // Connect to WebSocket server
  client.connect(websockets_connection_string);
  // Send a ping to the WebSocket server
  client.ping();
  client2.onEvent(onEventsCallback);
  // Connect to WebSocket server
  client2.connect(websockets_connection_string_2);
  // Send a ping to the WebSocket server
  client2.ping();
}
