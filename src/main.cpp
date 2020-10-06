#include <Arduino.h>
#include <WiFiManager.h>
#include "Config.h"
#include "Sonos.h"
#include "Rfid.h"
#include "WebServer.h"

#define MAIN_START_RFID
#define MAIN_START_SONOS
#define MAIN_START_WEB

// Defining instances to be used later
// in the setup and loop
Sonos g_sonos;
WiFiManager g_wifi_manager;
Rfid g_rfid_instance;
WebServer g_web_server;

// Using this as a bit of a hack to start/stop
// processing RFID requests
bool g_lock;

// We only support spotify at the moment, so
// we don't really store this value anywhere
// except here
const char* g_service_name = "spotify";
uint16_t g_service_id = 0;

// Currently using this to manage ongoing discovery
// of the sonos clients in the loop
unsigned long target_time = 0L;
const unsigned long DISCOVER_PERIOD = 2*60*1000UL; // 2 minutes

/**
 * Check and save any change of location
 *
 * Checks if the audio destination for playing has
 * changed, and if so it attempts to save that config
 * in the defined storage environment.
 */
void checkLocationChange()
{
    const SonosClient* client = g_sonos.getActiveClient();

    if (strcmp(CONFIG.stored_config.last_sonos_serial, client->serial_num) != 0)
    {
        memset(CONFIG.stored_config.last_sonos_serial, 0, sizeof(CONFIG.stored_config.last_sonos_serial));
        memcpy(CONFIG.stored_config.last_sonos_serial, client->serial_num, sizeof(client->serial_num));
        
        Serial.print(F("main::checkLocationChange saving Location change: "));Serial.println(CONFIG.stored_config.last_sonos_serial);

        // Write out the config to save it
        CONFIG.writeConfig();

        Serial.print(F("main::checkLocationChange Location change saved: "));Serial.println(CONFIG.stored_config.last_sonos_serial);
    }
}

/**
 * Process the callback from any RFID tag that's read
 *
 * Main callback passed to the RFID class, which in
 * turn processes the data provided that's read.
 */
void readRFIDCallback(const uint8_t* t_card_uid, const uint8_t* t_read_buffer, uint8_t t_buffer_size)
{
    // Just make sure that we have something to process
    if (!t_read_buffer)
    {
        Serial.print(F("main::readRFIDCallback no valid buffer provided/found"));
        return;
    }

    Serial.print(F("main::readRFIDCallback called ["));Serial.print((char*)t_read_buffer);Serial.println(F("]"));

    // Split the buffer up to the first space character
    // Commands should be one of:
    //   <COMMAND>
    //   <COMMAND> <ARGUMENT>
    char* command = strtok((char*)t_read_buffer, " ");
    
    if ((!g_lock) && strcmp(command, "PLAY") == 0)
    {
        // PLAY: expect next argument is a uri
        char* argument = strtok(NULL, " ");
        if (argument != NULL)
        {
            Serial.print(F("main::readRFIDCallback PLAY command ["));Serial.print(argument);Serial.println(F("]"));
            g_sonos.queueUri(g_service_id, argument);
            g_sonos.play();
        }
    }
    else if ((!g_lock) && strcmp(command, "LOCATION") == 0)
    {
        // LOCATION: expect next number is a Sonos Serial# for a location
        char* argument = strtok(NULL, " ");
        if (argument != NULL)
        {
            Serial.print(F("main::readRFIDCallback LOCATION command ["));Serial.print(argument);Serial.println(F("]"));
            g_sonos.setActiveClient(argument);

            // Check & save any change in the active client
            checkLocationChange();
        }
    }
    else if ((!g_lock) && strcmp(command, "STOP") == 0)
    {
        // STOP: no further arguments
        Serial.println(F("main::readRFIDCallback STOP command"));
        g_sonos.stop();
    }
    else if (strcmp(command, "LOCK") == 0)
    {
        // LOCK: no further arguments
        g_lock = !g_lock;
        Serial.print(F("main::readRFIDCallback LOCK command ["));Serial.print(g_lock ? F("LOCKED") : F("UNLOCKED"));Serial.println(F("]"));
    }
}

/**
 * Setup wifi access using Captive Portal
 *
 * Connects to a wifi network if details already
 * provided, otherwise it sets up a Captive Portal
 * for the user to enter ssid details.
 */
void setupWifi()
{
  // Setup wifi manager
  g_wifi_manager.setTimeout(CONFIG_WIFI_AP_TIMEOUT);
  
  if (!g_wifi_manager.autoConnect(CONFIG_WIFI_AP_NAME, CONFIG_WIFI_AP_PWD))
  {
    Serial.println(F("main::SetupWifi Failed to connect to WiFi and hit timeout"));
    
    // Reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
  
  Serial.print(F("main::SetupWifi connected, IP address: "));Serial.println(WiFi.localIP());
}

/**
 * Main setup function that configures elements
 * before we start the main loop.
 */
void setup()
{
    delay(100); // Seems to make flashing more reliable

    // Set up the serial port
    Serial.begin(CONFIG_SERIAL_SPEED);
    Serial.println();
    Serial.println(F("main::Setup starting..."));

    // Setup Wifi manager
    setupWifi();

    // Read the saved config
    CONFIG.begin();
    CONFIG.readConfig();

#ifdef MAIN_START_RFID
    // Prep the MFRC522 Card reader interface
    g_rfid_instance.begin();
#endif

#ifdef MAIN_START_SONOS
    // Discover Sonos players & set the active clients (based on config)
    g_sonos.begin();
    g_sonos.discover();

    g_service_id = g_sonos.getServiceID(g_service_name);

    // If there is a previously configured location
    // then set the location
    if (strcmp(CONFIG.stored_config.last_sonos_serial, "") != 0)
    {
        Serial.print(F("main::Setup using last Sonos Serial ["));Serial.print(CONFIG.stored_config.last_sonos_serial);Serial.println(F("]"));
        g_sonos.setActiveClient(CONFIG.stored_config.last_sonos_serial);
    }
#endif

#ifdef MAIN_START_WEB
    // Start the webserver
    g_web_server.begin(CONFIG_WEB_PORT, CONFIG_WEB_NAME, &g_rfid_instance, &g_sonos);
#endif

    // Start unlocked, but should probably read this from
    // a config so we don't use a power reset to unlock the device
    g_lock = false;

    Serial.println(F("main::Setup ...completed"));
}

/**
 * Main loop function
 */
void loop()
{
#ifdef MAIN_START_RFID
    // Check if we need to handle any RFID events
    uint8_t buffer[255];
    g_rfid_instance.handle(readRFIDCallback, buffer, sizeof(buffer));
#endif

#ifdef MAIN_START_WEB
    // Check for any Web requests
    g_web_server.handle();
#endif

#ifdef MAIN_START_SONOS
    // Check if there has been a change to the Active location
    // that we need to store for next time. This should really
    // never trigger as we also catch it in the RFID location
    // change area of the callback, but is here just in case
    checkLocationChange();

    // Run discovery of new Sonos clients every period
    if ((millis() - target_time) >= DISCOVER_PERIOD)
    {
        Serial.println(F("main::Loop running discovery for clients"));
        target_time += DISCOVER_PERIOD;
        g_sonos.discover();
    }
#endif
}