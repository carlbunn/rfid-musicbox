#include <IPAddress.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h> 
#include "Sonos.h"
#include "Utility.h"

static const int s_default_timeout = 5000;

static const IPAddress SSDP_MULTICAST_ADDR(239, 255, 255, 250);

static const char *s_user_agent = "sonos_lib";

static const int s_soap_port = 1400;

static const char *s_content_type = "text/xml";

static const char *s_search_unicast_SSDP_template =
  "M-SEARCH * HTTP/1.1\r\n"
  "HOST: 239.255.255.250:1900\r\n"
  "MAN: \"SSDP:discover\"\r\n"
  "MX: 1\r\n"
  "ST: urn:schemas-upnp-org:device:ZonePlayer:1\r\n"
  "USER-AGENT: Arduino UPnP/2.0 Sonos Library/0.1\r\n";

/**
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
  <s:Body>
    <u:Play xmlns:u="urn:schemas-upnp-org:service:AVTransport:1">
      <InstanceID>0</InstanceID>
      <Speed>1</Speed>
    </u:Play>
  </s:Body>
</s:Envelope>
*/
static const char *s_sonos_play_transport_endpoint = "/MediaRenderer/AVTransport/Control";
static const char *s_sonos_play_action = "\"urn:schemas-upnp-org:service:AVTransport:1#Play\"";
static const char *s_sonos_play_payload = "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:Play xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\"><InstanceID>0</InstanceID><Speed>1</Speed></u:Play></s:Body></s:Envelope>";

static const char *s_sonos_pause_transport_endpoint = "/MediaRenderer/AVTransport/Control";
static const char *s_sonos_pause_action = "\"urn:schemas-upnp-org:service:AVTransport:1#Pause\"";
static const char *s_sonos_pause_payload = "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:Pause xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\"><InstanceID>0</InstanceID></u:Pause></s:Body></s:Envelope>";

/**
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
  <s:Body>
    <u:SetAVTransportURI xmlns:u="urn:schemas-upnp-org:service:AVTransport:1">
      <InstanceID>0</InstanceID>
      <CurrentURI>TRACK_ID?sid=SERVICE_ID</CurrentURI>
      <CurrentURIMetaData></CurrentURIMetaData>
    </u:SetAVTransportURI>
  </s:Body>
</s:Envelope>
*/
static const char *s_sonos_queue_transport_endpoint = "/MediaRenderer/AVTransport/Control";
static const char *s_sonos_queue_action = "\"urn:schemas-upnp-org:service:AVTransport:1#SetAVTransportURI\"";
static const char *s_sonos_queue_payload_1 = "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:SetAVTransportURI xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\"><InstanceID>0</InstanceID><CurrentURI>";
static const char *s_sonos_queue_payload_2 = "?sid=";
static const char *s_sonos_queue_payload_3 = "</CurrentURI><CurrentURIMetaData></CurrentURIMetaData></u:SetAVTransportURI></s:Body></s:Envelope>";


/**
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
  <s:Body>
    <u:SetAVTransportURI xmlns:u="urn:schemas-upnp-org:service:RenderingControl:1">
      <InstanceID>0</InstanceID>
      <CurrentURI>TRACK_ID?sid=SERVICE_ID</CurrentURI>
      <CurrentURIMetaData></CurrentURIMetaData>
    </u:SetAVTransportURI>
  </s:Body>
</s:Envelope>
*/
//TO BE FINISHED & CHECKED
//static const char *s_sonos_set_volume_transport_endpoint = "/MediaRenderer/RenderingControl/Control";
//static const char *s_sonos_set_volume_action = "\"urn:schemas-upnp-org:service:RenderingControl:1#SetVolume\"";
//static const char *s_sonos_set_volume_payload = "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:SetVolume xmlns:u=\"urn:schemas-upnp-org:service:RenderingControl:1\"><InstanceID>0</InstanceID><Channel>Master</Channel><DesiredVolume>%d</DesiredVolume></u:SetVolume></s:Body></s:Envelope>";


/**
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
  <s:Body>
    <u:ListAvailableServices xmlns:u="urn:schemas-upnp-org:service:MusicServices:1">
      <InstanceID>0</InstanceID>
    </u:ListAvailableServices>
  </s:Body>
</s:Envelope>
*/
static const char *s_sonos_get_services_transport_endpoint = "/MusicServices/Control";
static const char *s_sonos_get_services_action = "\"urn:schemas-upnp-org:service:MusicServices:1#ListAvailableServices\"";
static const char *s_sonos_get_services_payload = "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:ListAvailableServices xmlns:u=\"urn:schemas-upnp-org:service:MusicServices:1\"><InstanceID>0</InstanceID></u:ListAvailableServices></s:Body></s:Envelope>";


void KMP(WiFiClient* t_x, const char* t_needle, int t_stream_len, int t_needle_len); // x, y, m, n


void Sonos::begin()
{
    m_udp.begin(m_ssdp_port);
}

void Sonos::begin(unsigned int t_ssdp_port)
{
    m_ssdp_port = t_ssdp_port;
    begin();
}

long Sonos::discover()
{
    return discover(s_default_timeout);
}

long Sonos::discover(unsigned long t_time_out)
{
    long num_new_devices = 0;
    IPAddress sonosIP;
    unsigned long search_start = millis();
    
    DEBUG_SONOS(Serial.println(F("Sonos::discover Sending M-SEARCH multicast")));

    m_udp.beginPacketMulticast(SSDP_MULTICAST_ADDR, m_ssdp_port, WiFi.localIP());
    m_udp.write(s_search_unicast_SSDP_template, strlen(s_search_unicast_SSDP_template));
    m_udp.endPacket();

    do
    {
        int packet_size = m_udp.parsePacket();

        // If we received a packet
        if (packet_size)
        {
            char packet_buffer[255];
            IPAddress ip;

            ip = m_udp.remoteIP();
            
            DEBUG_SONOS(Serial.print(F("Sonos::discover Received packet of size "));
                        Serial.println(packet_size);
                        Serial.print(F("Sonos::discover From "));
                        Serial.print(ip);
                        Serial.print(F(", port "));
                        Serial.println(m_udp.remotePort()););

            // Read the packet into packet_buffer
            int len = m_udp.read(packet_buffer, sizeof(packet_buffer) * sizeof(*packet_buffer));
            if (len > 0)
            {
                packet_buffer[len] = 0;
            }
            
            DEBUG_SONOS(Serial.println(F("Sonos::discover Contents:"));
                        Serial.println(packet_buffer));
            
            char* token;
            token = strtok(packet_buffer, "\n");

            while (token != NULL)
            {
                // TODO: fix this up to give a better way of grabbing the location
                if (strncmp(token, "LOCATION:", 9) == 0)
                {
                    if (addSonosClient(ip, &token[10]))
                    {
                        num_new_devices += 1;
                    }
                }

                token = strtok(NULL, "\n");
            }
            
        }
    } while ((millis() - search_start) < t_time_out);

    // Fill in the details for any found servers
    // Note, we can't do this above as we discover services
    // I believe it's due to conflicts with our single thread
    // and so it seems best to wait until we've finished discovery
    fillBlankSonosDetails();

    Serial.print(F("Sonos::discover Completed and found "));Serial.print(num_new_devices);Serial.println(F(" new devices"));

    // Set an active client if none has been set
    if ((!m_active_client) && num_new_devices > 0)
    {
        // Just set it to the first one in the list
        setActiveClient(m_sonos_clients[0].serial_num);
    }
    
    return num_new_devices;
}

uint16_t Sonos::getServiceID(const char* t_service_name)
{
    if (m_active_client && m_active_client->ip)
    {
        return getServiceID(m_active_client, t_service_name);
    } else {
        return -1;
    }
}

uint16_t Sonos::getServiceID(SonosClient* t_client, const char* t_service_name)
{
    DEBUG_SONOS(Serial.print(F("Sonos::getServiceID Getting service ID for ["));
                Serial.print(t_service_name);
                Serial.println(F("]")));

    uint16_t service_id = -1;

    if (sendRequest(t_client->ip, s_soap_port, s_sonos_get_services_transport_endpoint, s_sonos_get_services_action, s_sonos_get_services_payload))
    {
        while (m_http_client.getStream().find("&lt;Service ", 12))
        {
            char buffer[255];
            
            readUntil(m_http_client.getStream(), "&lt;/Service", 12, buffer, 255);

            if (stristr(buffer, t_service_name))
            {
                int start = 0;
                char dest[5];

                char* value = stristr(buffer, "id=&quot;");
                start = value - buffer + 9;
                value = stristr(value + 9, "&quot;");

                memset(dest, '\0', sizeof(dest));
                strncpy(dest, buffer + start, value - buffer - start);

                service_id = atoi(dest);
            }
        }

        // End the session
        m_http_client.end();
    }

    DEBUG_SONOS(Serial.print(F("Sonos::getServiceID matched service id ["));
                Serial.print(service_id);
                Serial.println(F("]")));

    return service_id;
}

bool Sonos::stop()
{
    return pause();
}

bool Sonos::pause()
{
    if (m_active_client && m_active_client->ip)
    {
        return pause(m_active_client);
    } else {
        return false;
    }
}

bool Sonos::pause(SonosClient* t_client)
{
    DEBUG_SONOS(Serial.print(F("Sonos::pause Pausing/Stopping ["));
                Serial.print(t_client->room_name);
                Serial.print(":");
                Serial.print(t_client->serial_num);
                Serial.print(":");
                Serial.print(t_client->ip);
                Serial.println(F("]")));

    return sendRequest_End(t_client->ip, s_soap_port, s_sonos_pause_transport_endpoint, s_sonos_pause_action, s_sonos_pause_payload);
}

bool Sonos::play()
{
    if (m_active_client && m_active_client->ip)
    {
        return play(m_active_client);
    } else {
        return false;
    }
}

bool Sonos::play(SonosClient* t_client)
{
    DEBUG_SONOS(Serial.print(F("Sonos::play Playing ["));
                Serial.print(t_client->room_name);
                Serial.print(":");
                Serial.print(t_client->serial_num);
                Serial.print(":");
                Serial.print(t_client->ip);
                Serial.println(F("]")));

    return sendRequest_End(t_client->ip, s_soap_port, s_sonos_play_transport_endpoint, s_sonos_play_action, s_sonos_play_payload);
}

bool Sonos::queueUri(const uint16_t t_service_id, const char* t_uri)
{
    if (m_active_client && m_active_client->ip)
    {
        return queueUri(m_active_client, t_service_id, t_uri);
    } else {
        return false;
    }
}

bool Sonos::queueUri(SonosClient* t_client, const uint16_t t_service_id, const char* t_uri)
{
    DEBUG_SONOS(Serial.print(F("Sonos::queueUri Queuing track ["));
                Serial.print(t_client->room_name);
                Serial.print(":");
                Serial.print(t_client->serial_num);
                Serial.print(":");
                Serial.print(t_client->ip);
                Serial.println(F("]"));
                Serial.print(F("Sonos::queueUri Queuing track ["));
                Serial.print(t_uri);
                Serial.println(F("]")));

    char buffer[1024];
    uint8_t uri_len = strlen(t_uri);
    char service_id[5];
    itoa(t_service_id, service_id, 10);
    uint8_t id_len = strlen(service_id);

    // TODO: rewrite this mess
    // Should just be able to use strlen of the buffer since it's null terminated
    // I also need to ensure we don't overrun the 1024 buffer which we might do at the moment
    memset(buffer, 0, 1024);
    memcpy(buffer, s_sonos_queue_payload_1, 244 * sizeof(char));
    memcpy(buffer + (244 * sizeof(char)), t_uri, uri_len * sizeof(char));
    memcpy(buffer + (244 * sizeof(char)) + (uri_len * sizeof(char)), s_sonos_queue_payload_2, 5 * sizeof(char));
    memcpy(buffer + (244 * sizeof(char)) + (uri_len * sizeof(char)) + (5 * sizeof(char)), service_id, id_len * sizeof(char));
    memcpy(buffer + (244 * sizeof(char)) + (uri_len * sizeof(char)) + (5 * sizeof(char)) + (id_len * sizeof(char)), s_sonos_queue_payload_3, 98 * sizeof(char));

    bool ret_val = sendRequest_End(t_client->ip, s_soap_port, s_sonos_queue_transport_endpoint, s_sonos_queue_action, buffer);

    return ret_val;
}

bool Sonos::sendRequest_End(IPAddress& t_ip_address, const int t_port, const char* t_transport_endpoint, const char* t_action, const char* t_payload)
{
    bool ret = sendRequest(t_ip_address, t_port, t_transport_endpoint, t_action, t_payload);
    
    // End the request
    m_http_client.end();

    return ret;
}

bool Sonos::sendRequest(IPAddress& t_ip_address, const int t_port, const char* t_transport_endpoint, const char* t_action, const char* t_payload)
{
    DEBUG_SONOS(Serial.println(F("Sonos::sendRequest started")));

    m_http_client.begin(m_wifi_client, t_ip_address.toString(), t_port, t_transport_endpoint, false);
    m_http_client.setUserAgent(s_user_agent);
    m_http_client.setReuse(false);
    m_http_client.addHeader(F("Content-type"), s_content_type);
    m_http_client.addHeader(F("SOAPACTION"), t_action);
    
    int http_response_code = m_http_client.POST(t_payload);
    
    if (http_response_code > 0)
    {
        DEBUG_SONOS(Serial.print(F("Sonos::sendRequest response code: "));
                    Serial.println(http_response_code));
    } else {
        DEBUG_SONOS(Serial.print(F("Sonos::sendRequest error on sending POST ["));
                    Serial.print(http_response_code);
                    Serial.print(F("] ["));
                    Serial.print(m_http_client.errorToString(http_response_code));
                    Serial.println(F("]")));
    }

    if (http_response_code == HTTP_CODE_OK)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void Sonos::printStream(WiFiClient* stream)
{
    // create buffer for read
    uint8_t buffer[128] = { 0 };

    // Get available data size
    unsigned int size = stream->available();

    if (size)
    {
        // Read up to 128 bytes
        int c = stream->readBytes(buffer, ((size > sizeof(buffer)) ? sizeof(buffer) : size));

        Serial.write(buffer, c);
    }
}

bool Sonos::addSonosClient(IPAddress& t_ip, const char* t_location)
{
    bool found = false;
    bool added = false;
  
    for (auto i = 0; i < m_sonos_client_count; i++)
    {
        if (t_ip == m_sonos_clients[i].ip)
        {
            found = true;
            break;
        }
    }

    if ((!found) && (m_sonos_client_count < NUM(m_sonos_clients)))
    {
        m_sonos_clients[m_sonos_client_count].ip = t_ip;

        memset(m_sonos_clients[m_sonos_client_count].location, 0, NUM(m_sonos_clients[m_sonos_client_count].location));
        strncpy(m_sonos_clients[m_sonos_client_count].location, t_location, NUM(m_sonos_clients[m_sonos_client_count].location) - 1);
        
        m_sonos_client_count++;
        added = true;
    }

    return added;
}

void Sonos::printClients()
{
    Serial.print(F("Sonos::printClients starting client list"));

    for (auto i = 0; i < m_sonos_client_count; i++)
    {
        Serial.print(F("Client #"));Serial.print(i + 1);Serial.print(F(" of "));Serial.println(m_sonos_client_count);
        Serial.print(F("IP: "));Serial.println((IPAddress)m_sonos_clients[i].ip);
        Serial.print(F("Location: "));Serial.println(m_sonos_clients[i].location);
        Serial.print(F("RoomName: "));Serial.println(m_sonos_clients[i].room_name);
        Serial.print(F("DisplayName: "));Serial.println(m_sonos_clients[i].display_name);
        Serial.print(F("SerialNum: "));Serial.println(m_sonos_clients[i].serial_num);
    }

    Serial.println(F("Sonos::printClients finished client list"));
}

void Sonos::fillBlankSonosDetails()
{
    DEBUG_SONOS(Serial.println(F("Sonos::fillBlankSonosDetails starting client list")));
    
    for (auto i = 0; i < m_sonos_client_count; i++)
    {
        if (!m_sonos_clients[i].serial_num[0])
        {
            getSonosDetails(m_sonos_clients[i]);

            DEBUG_SONOS(Serial.print(F("Client #"));
                        Serial.print(i);
                        Serial.print(F(": "));
                        Serial.println(m_sonos_clients[i].serial_num));
        }
    }

    DEBUG_SONOS(Serial.println(F("Sonos::fillBlankSonosDetails finished client list")));
}

void Sonos::getSonosDetails(SonosClient& t_client)
{
    DEBUG_SONOS(Serial.print(F("Sonos::getSonosDetails IP:"));
                Serial.println(t_client.ip));

    m_http_client.begin(m_wifi_client, t_client.location);
    m_http_client.setUserAgent(s_user_agent);
    m_http_client.setReuse(false);

    int http_response_code = m_http_client.GET();

    if (http_response_code > 0)
    {
        long index_start;
        long index_end;
        
        String response = m_http_client.getString();  //Get the response to the request

        index_start = response.indexOf(F("<roomName>"));
        index_end = response.indexOf(F("</roomName>"), index_start);
        memset(t_client.room_name, '\0', sizeof(t_client.room_name));
        strncpy(t_client.room_name, response.substring(index_start + 10, index_end).c_str(), sizeof(t_client.room_name) - 1);

        index_start = response.indexOf(F("<displayName>"));
        index_end = response.indexOf(F("</displayName>"), index_start);
        memset(t_client.display_name, '\0', sizeof(t_client.display_name));
        strncpy(t_client.display_name, response.substring(index_start + 13, index_end).c_str(), sizeof(t_client.display_name) - 1);

        index_start = response.indexOf(F("<serialNum>"));
        index_end = response.indexOf(F("</serialNum>"));
        memset(t_client.serial_num, '\0', sizeof(t_client.serial_num));
        strncpy(t_client.serial_num, response.substring(index_start + 11, index_end).c_str(), sizeof(t_client.serial_num) - 1);
    }
    else
    {
        DEBUG_SONOS(Serial.print(F("Sonos::sendRequest error on sending GET ["));
                    Serial.print(http_response_code);
                    Serial.print(F("] ["));
                    Serial.print(m_http_client.errorToString(http_response_code));
                    Serial.println(F("]")));
    }

    m_http_client.end();
}

const SonosClient* Sonos::getActiveClient()
{
    return m_active_client;
}

bool Sonos::setActiveClient(const char* t_serial_num)
{
    DEBUG_SONOS(Serial.print(F("Sonos::setActiveClient Setting active client ["));
                Serial.print(t_serial_num);
                Serial.println(F("]")));
    
    bool found_client;
    
    for (auto i = 0; i < m_sonos_client_count; i++)
    {
        if (strcmp(m_sonos_clients[i].serial_num, t_serial_num) == 0)
        {
            m_active_client = &m_sonos_clients[i];
            found_client = true;
            
            DEBUG_SONOS(Serial.print(F("Sonos::setActiveClient Found active client ["));
                        Serial.print(m_active_client->room_name);
                        Serial.print(":");
                        Serial.print(m_active_client->serial_num);
                        Serial.print(":");
                        Serial.print(m_active_client->ip);
                        Serial.println(F("]")));
            break;
        }
    }

    DEBUG_SONOS(if (!found_client)
                {
                    Serial.print(F("Sonos::setActiveClient Could not find client ["));
                    Serial.print(t_serial_num);
                    Serial.println(F("]"));
                });

    return found_client;
}

/*bool Sonos::decodeUri(char *p)
{
    int n = strlen(p);

    while (*p)
    {
        if (*p != '%')
        {
            p++;
            n--;
            continue;
        }

        if (!isxdigit(p[1]) || !isxdigit(p[2])) return false;

        *p = 0;
        if (p[1] <= '9')
            *p = (p[1] - '0') << 4;
        else if (p[1] <= 'F')
            *p = (p[1] - 'A' + 10) << 4;
        else
            *p = (p[1] - 'a' + 10) << 4;

        if (p[2] <= '9')
            *p |= (p[2] - '0');
        else if (p[2] <= 'F')
            *p |= (p[2] - 'A' + 10);
        else
            *p |= (p[2] - 'a' + 10);

        n -= 3;
        p++;
        memcpy(p, p + 2, n + 1); // +1 to include the '\0'
    }

    return true;
}*/

uint8_t Sonos::getClientCount()
{
    return m_sonos_client_count;
}

const SonosClient* Sonos::getClient(const uint8_t id)
{
    return &m_sonos_clients[id];
}

// Not mine - copied from Arduino forums
bool Sonos::readUntil(WiFiClient& client, const char *target, size_t targetLen, char* buffer, int buffer_len)
{
    size_t index = 0;  // maximum target string length is 64k bytes!
    int c;
    int i = 0;

    if(*target == 0)
        return true;   // return true if target is a null string
    
    while(((c = client.read()) > 0) && (i < (buffer_len - 1)))
    {
        buffer[i++] = c;

        if(c != target[index])
            index = 0; // reset index if any char does not match

        if(c == target[index])
        {
            if(++index >= targetLen)
            { // return true if all chars in the target match
                return true;
            }
        }
    }

    return false;
}