#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "WebServer.h"
#include "WebContent.h"
#include "Rfid.h"
#include "Sonos.h"

void WebServer::begin(const int t_port, const char* t_name, Rfid* t_rfid, Sonos* t_sonos)
{
    Serial.println(F("WebServer::begin Starting webserver"));

    m_rfid = t_rfid;
    m_sonos = t_sonos;

    memset(m_name, '\0', sizeof(m_name));
    strncpy(m_name, t_name, sizeof(m_name));

    m_web_server.on(F("/"), HTTP_GET, std::bind(&WebServer::handleRoot, this));
    m_web_server.on(F("/write"), HTTP_GET, std::bind(&WebServer::handleWriteRequest, this));
    m_web_server.on(F("/writecancel"), HTTP_GET, std::bind(&WebServer::handleWriteCancelRequest, this));
    m_web_server.on(F("/locations"), HTTP_GET, std::bind(&WebServer::handleLocations, this));
    m_web_server.on(F("/name"), HTTP_GET, std::bind(&WebServer::handleName, this));

    m_web_server.begin(t_port);
  
    if (!MDNS.begin(t_name))
    {
        Serial.println(F("WebServer::begin Error starting MDNS responder"));
    }
    else
    {
        MDNS.addService(F("http"), F("tcp"), t_port);
    }
  
    Serial.println(F("WebServer::begin Started webserver"));
}

void WebServer::handleRoot()
{
    DEBUG_WEBSERVER(Serial.println(F("WebServer::handleRoot")));

    m_web_server.sendHeader(F("Connection"), F("close"));
    m_web_server.send_P(200, "text/html", HTML);
}

void WebServer::handleWriteRequest()
{
    DEBUG_WEBSERVER(Serial.println(F("WebServer::handleWriteRequest")));

    m_web_server.sendHeader(F("Connection"), F("close"));

    if (m_web_server.arg(F("type")) == "")
    {
        DEBUG_WEBSERVER(Serial.println(F("WebServer::handleWriteRequest No parameter defined: type")));
        m_web_server.send(500, "text/html");
    }
    else
    {
        DEBUG_WEBSERVER(Serial.print(F("WebServer::handleWriteRequest Processing request: type["));
                        Serial.print(m_web_server.arg(F("type")));
                        Serial.println(F("]")));

        char buffer[255];
        bool valid = false;

        if ((m_web_server.arg("type") == F("PLAY")) && m_web_server.hasArg("url"))
        {
            valid = true;
            processWriteQuery("PLAY", m_web_server.arg(F("url")).c_str(), buffer, sizeof(buffer));
        }
        else if ((m_web_server.arg("type") == F("LOCATION")) && m_web_server.hasArg("location"))
        {
            valid = true;
            processWriteQuery("LOCATION", m_web_server.arg(F("location")).c_str(), buffer, sizeof(buffer));
        }
        else if (m_web_server.arg("type") == F("STOP"))
        {
            valid = true;
            processWriteQuery("STOP", "", buffer, sizeof(buffer));
        }
        else if (m_web_server.arg("type") == F("LOCK"))
        {
            valid = true;
            processWriteQuery("LOCK", "", buffer, sizeof(buffer));
        }

        if (valid)
        {
            DEBUG_WEBSERVER(Serial.print(F("WebServer::handleWriteRequest Sending to RFID ["));
                            Serial.print(buffer);
                            Serial.println(F("]")));
            
            m_rfid->writeRfid((uint8_t*)buffer, strlen(buffer));

            m_web_server.send(200, F("text/html"), "");
        }
        else
        {
            DEBUG_WEBSERVER(Serial.print(F("WebServer::handleWriteRequest Invalid request ["));
                            Serial.print(m_web_server.uri());
                            Serial.println(F("]")));

            m_web_server.send(500, F("text/html"), "");
        }
    }
}

void WebServer::handleWriteCancelRequest()
{
    DEBUG_WEBSERVER(Serial.println(F("WebServer::handleWriteCancelRequest")));

    m_rfid->cancelWriteRfid();

    m_web_server.send(200, F("text/html"), "");
}

void WebServer::handleLocations()
{
    DEBUG_WEBSERVER(Serial.println(F("WebServer::handleLocations")));

    // Run discovery to check for new clients
    m_sonos->discover();

    m_web_server.sendHeader(F("Connection"), F("close"));
    m_web_server.chunkedResponseModeStart(200, F("text/json"));

    // Create JSON
    m_web_server.sendContent(F("{\r\n"));

    for (uint8_t i = 0 ; i < m_sonos->getClientCount() ; i++)
    {
        if (i > 0) m_web_server.sendContent(F(",\r\n"));

        m_web_server.sendContent(F("\""));
        m_web_server.sendContent(m_sonos->getClient(i)->serial_num);
        m_web_server.sendContent(F("\": \""));
        m_web_server.sendContent(m_sonos->getClient(i)->room_name);
        m_web_server.sendContent(F("\""));
    }

    m_web_server.sendContent(F("\r\n}"));
    m_web_server.chunkedResponseFinalize();
}

void WebServer::handleName()
{
    DEBUG_WEBSERVER(Serial.println(F("WebServer::handleName")));

    m_web_server.send(200, F("text/plain"), m_name);
}

void WebServer::handle()
{
    m_web_server.handleClient();
    MDNS.update();
}

void WebServer::processWriteQuery(const char* t_type, const char* t_arg, char* t_buffer, uint8_t t_buffer_length)
{
    uint8_t len = strlen(t_type);
    uint8_t arg_len = strlen(t_arg);

    memset(t_buffer, 0, t_buffer_length);
    memcpy(t_buffer, t_type, (len > (t_buffer_length - 1)) ? (t_buffer_length - 1) : len);

    if (arg_len > (uint8_t)0)
    {
        memcpy(t_buffer + len, " ", ((len + 1) > (t_buffer_length - 1)) ? 0 : 1);
        memcpy(t_buffer + len + 1, t_arg, (arg_len > (t_buffer_length - len - 1)) ? (t_buffer_length - len - 1) : arg_len);
    }
}