#ifndef Sonos_h
#define Sonos_h

#include <IPAddress.h>
#include <WiFiUDP.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h> 

#ifdef DEBUG
    #define DEBUG_SONOS(x) x
#else 
    #define DEBUG_SONOS(x) do{}while(0)
#endif

#define NUM(a) (sizeof(a) / sizeof(*a))

struct SonosClient
{
    IPAddress ip;
    char location[255]{};
    char serial_num[20]{};
    char room_name[255]{};
    char display_name[255]{};
};


class Sonos
{
public:
    Sonos() {};
    void begin();
    void begin(unsigned int);
    long discover();
    long discover(unsigned long);
    void subscribe(WiFiClient&, IPAddress&);
    bool play();
    bool play(SonosClient*);
    bool pause();
    bool pause(SonosClient*);
    bool stop();
    bool queueUri(const uint16_t, const char*);
    bool queueUri(SonosClient*, const uint16_t, const char*);
    uint16_t getServiceID(const char*);
    uint16_t getServiceID(SonosClient*, const char*);
    bool setActiveClient(const char*);
    const SonosClient* getActiveClient();
    void printClients();
    uint8_t getClientCount();
    const SonosClient* getClient(const uint8_t);

private:
    WiFiUDP m_udp;
    WiFiClient m_wifi_client;
    HTTPClient m_http_client;
    SonosClient* m_active_client = nullptr;
    SonosClient m_sonos_clients[20];
    uint8_t m_sonos_client_count = 0;
    uint16_t m_ssdp_port = 1900;

    void getSonosDetails(SonosClient&);
    void fillBlankSonosDetails();
    bool addSonosClient(IPAddress&, const char*);
    bool sendRequest(IPAddress&, const int, const char*, const char*, const char*);
    bool sendRequest_End(IPAddress&, const int, const char*, const char*, const char*);
    static char *appendF(const char*, ...);
    bool decodeUri(char*);
    void printStream(WiFiClient*);
    bool readUntil(WiFiClient& client, const char *target, size_t targetLen, char* buffer, int buffer_len);
};

#endif
