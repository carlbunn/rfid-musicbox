#ifndef ConfigClass_h
#define ConfigClass_h

#ifdef DEBUG
    #define DEBUG_CONFIG(x) x
#else
    #define DEBUG_CONFIG(x) do{}while(0)
#endif

static const char* CONFIG_WIFI_AP_NAME     = "musicbox_setup";
static const char* CONFIG_WIFI_AP_PWD      = "musicbox_setup";
static const int CONFIG_WIFI_AP_TIMEOUT    = 180;

static const int CONFIG_EEPROM_ADDRESS     = 0;

static const long CONFIG_SERIAL_SPEED      = 74880; // 74880 gives us some additional boot info

static const char* CONFIG_WEB_NAME         = "musicbox";
static const int CONFIG_WEB_PORT           = 80;

typedef struct {
    char last_sonos_serial[20] = "";
} ConfigStruct;

class ConfigClass
{
public:
    ConfigClass() {};
    ConfigStruct stored_config;

    void begin();
    void clearConfig(); // Use sparingly
    void readConfig();
    void writeConfig(); // Use sparingly
    void wipeConfig(); // Use sparingly
};

extern ConfigClass CONFIG;

#endif