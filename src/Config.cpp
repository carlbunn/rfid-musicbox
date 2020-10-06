#include <Arduino.h>
#include <ESP_EEPROM.h>
#include "Config.h"

/**
 * Call at the start, to prep for execution
 *
 * Initialisation functio that preps the
 * class before configs can be read or written.
 * Doesn't recover any saved config so should
 * be followed by a call to readConfig().
 */
void ConfigClass::begin()
{
    // The library needs to know what size you need for your EEPROM variables
    EEPROM.begin(sizeof(ConfigStruct));
}

/**
 * Clear the current config back to default values
 *
 * Clears any current config that is set, and reverts
 * back to the class defaults. Immediately saves to
 * EEPROM.
 */
void ConfigClass::clearConfig()
{
    // Probably not the best solution, but let's just write
    // a blank struct over the top
    ConfigStruct temp;
    EEPROM.put(CONFIG_EEPROM_ADDRESS, temp);
}

/**
 * Wipes the EEPROM of any data
 *
 * Atempts to clear the EEPROM of any data
 * that might be stores there. Doesn't return
 * a result, but will log an error on the
 * serial port if something fails.
 */
void ConfigClass::wipeConfig()
{
    if (EEPROM.wipe())
    {
        DEBUG_CONFIG(Serial.println(F("ConfigClass::wipeConfig EEPROM wiped successfully")));
    }
    else
    {
        Serial.println(F("ConfigClass::wipeConfig error occured while wiping EEPROM"));
    }
}

/**
 * Reads any existing config from the EEPROM
 *
 * Reads the EEPROM and tries to recover any
 * previously saved config variables. Best to
 * only do this at the start of your programme.
 */
void ConfigClass::readConfig()
{
    // Read any old config from the EEPROM
    EEPROM.get(CONFIG_EEPROM_ADDRESS, stored_config);

    Serial.println(F("ConfigClass::readConfig successfully from EEPROM"));
}

/**
 * Writes any existing config to the EEPROM
 *
 * Writes the existing config variables to the
 * EEPROM, and attempts to commit them immediately
 * to memory. Doesn't return a result, but will
 * log any error to the Serial port.
 */
void ConfigClass::writeConfig()
{
    // Write the data to EEPROM - ignoring
    // anything that might be there already
    // (re-flash is guaranteed)
    EEPROM.commitReset();

    // Replace values in byte-array cache with modified data
    // no changes made to flash, all in local byte-array cache
    EEPROM.put(CONFIG_EEPROM_ADDRESS, stored_config);
    
    // Actually write the content of byte-array cache to
    // hardware flash. Flash write occurs if and only if one or more byte
    // in byte-array cache has been changed, but if so, ALL bytes are 
    // written to flash
    if (EEPROM.commit())
    {
        DEBUG_CONFIG(Serial.println(F("ConfigClass::writeConfig config written successfully to EEPROM")));
    }
    else
    {
        Serial.println(F("ConfigClass::writeConfig error writing config to EEPROM"));
    }
}

ConfigClass CONFIG;