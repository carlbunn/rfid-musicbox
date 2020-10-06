#ifndef Rfid_h
#define Rfid_h

#include <MFRC522.h>

#define SS_PIN                      D4
#define RST_PIN                     D3

#define RFID_BLOCK_SIZE             16
#define RFID_BLOCK_SIZE_ULTRA       4
#define RFID_START_SECTOR           1   // Need to change how this works

#ifdef DEBUG
    #define DEBUG_RFID(x) x
#else 
    #define DEBUG_RFID(x) do{}while(0)
#endif

class Rfid
{
public:
    Rfid() :
        m_mfrc522(SS_PIN, RST_PIN)
        {};
    void begin();
    void handle(void (*callback)(const uint8_t*, const uint8_t*, const uint8_t), uint8_t*, uint16_t);
    void writeRfid(const uint8_t*, uint16_t);
    void cancelWriteRfid();
    void setWriteTimeout(const uint32_t);
  
private:
    enum RfidIfaceReturn
    {
        OK,
        INVALID_BLOCK_NUMBER,
        TRAILER_BLOCK_WRITE_ERROR,
        READ_FAILURE,
        WRITE_FAILURE,
        AUTHENTICATION_FAILURE
    };
    MFRC522 m_mfrc522;
    MFRC522::MIFARE_Key m_key;

    uint32_t m_write_timeout =  (10 * 1000); // 10 Seconds
    uint32_t m_write_timer = 0;
    bool m_write_on_next_card = false;
    uint8_t* m_write_buffer = nullptr;
    uint16_t m_write_buffer_size;

    Rfid::RfidIfaceReturn writeBufferToCard(uint8_t, const uint8_t*, const uint16_t);
    Rfid::RfidIfaceReturn readBufferFromCard(uint8_t, uint8_t*, const uint16_t);
    static void printByteArray(const uint8_t*, const uint8_t);
    Rfid::RfidIfaceReturn writeRfidBlock(uint8_t, uint8_t, const uint8_t*, uint8_t) ;
    Rfid::RfidIfaceReturn readRfidBlock(uint8_t, uint8_t, uint8_t*, uint8_t);
    std::vector<std::string> splitString(const std::string&, int);
};

#endif
