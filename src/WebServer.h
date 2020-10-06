#ifndef WebServer_h
#define WebServer_h

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "Rfid.h"
#include "Sonos.h"

#ifdef DEBUG
    #define DEBUG_WEBSERVER(x) x
#else
    #define DEBUG_WEBSERVER(x) do{}while(0)
#endif

class WebServer
{
public:
    WebServer() {};
    void begin(const int, const char*, Rfid*, Sonos*);
    void handle();
    void handleWriteRequest();
    void handleWriteCancelRequest();
    void handleLocations();
    void handleName();

private:
    ESP8266WebServer m_web_server;
    Rfid* m_rfid;
    Sonos* m_sonos;
    char m_name[100];
    
    void handleRoot();
    void processWriteQuery(const char*, const char*, char*, uint8_t);
};

#endif
