#include "Rfid.h"
#include <SPI.h>

/**
 * Good reference document on how RFID works
 * https://lastminuteengineers.com/how-rfid-works-rc522-arduino-tutorial/
*/

void Rfid::begin()
{
    SPI.begin();          // Init SPI bus
    m_mfrc522.PCD_Init(); // Init MFRC522 card
    delay(4);             // Add a short delay to allow hardware initialisation

    m_mfrc522.PCD_DumpVersionToSerial();

    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    memset(m_key.keyByte, 0xFF, MFRC522::MF_KEY_SIZE);

    DEBUG_RFID(Serial.print(F("Rfid::begin Using key (for A and B): "));
                printByteArray(m_key.keyByte, MFRC522::MF_KEY_SIZE);
                Serial.println());
}

void Rfid::writeRfid(const uint8_t* t_write_buffer, uint16_t t_write_buffer_size)
{
    // Set ourselves up for the write
    m_write_on_next_card = true;
    m_write_timer = millis();
    
    // Copy the write buffer over to a new buffer
    
    // TODO: Maybe we should just allocate a large char array to buffer this instead
    if (m_write_buffer) free(m_write_buffer); // Free any previous memory allocated to the pointer

    m_write_buffer = (uint8_t*)malloc(t_write_buffer_size * sizeof(uint8_t));
    memcpy(m_write_buffer, t_write_buffer, t_write_buffer_size * sizeof(uint8_t));
    m_write_buffer_size = t_write_buffer_size;
}

void Rfid::setWriteTimeout(const uint32_t t_length)
{
    DEBUG_RFID(Serial.print(F("Rfid::setWriteTimeout Setting timeout to ["));
                Serial.print(t_length);
                Serial.println(F("]")));

    m_write_timeout = t_length;
}

void Rfid::cancelWriteRfid()
{
    DEBUG_RFID(Serial.println(F("Rfid::cancelWriteRfid Cancelling write")));
    
    m_write_on_next_card = false;
    if (m_write_buffer) free(m_write_buffer);
    m_write_buffer = nullptr;
    m_write_buffer_size = 0;
}

void Rfid::handle(void (*read_callback)(const uint8_t*, const uint8_t*, const uint8_t), uint8_t* t_read_buffer, uint16_t t_buffer_size)
{
    // Check whether the write timer has expired 
    if (m_write_on_next_card && (millis() > (m_write_timeout + m_write_timer)))
    {
        Serial.println(F("Rfid::handleRfid Write timer expired. Cancelling write to card"));

        cancelWriteRfid(); // Clear everything out
    }

    // Check whether a card has been presented

    //TODO: improve the new card detection routine as per researched code
    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if (!m_mfrc522.PICC_IsNewCardPresent())
        return;

    // Select one of the cards
    if (!m_mfrc522.PICC_ReadCardSerial())
        return;

    Serial.println(F("Rfid::handleRfid New card detected"));

    MFRC522::PICC_Type piccType = m_mfrc522.PICC_GetType(m_mfrc522.uid.sak);

    // Show some details of the PICC (that is: the tag/card)
    DEBUG_RFID(Serial.print(F("Card UID: "));
                printByteArray(m_mfrc522.uid.uidByte, m_mfrc522.uid.size);
                Serial.println();
                Serial.print(F("PICC type: "));
                Serial.println(m_mfrc522.PICC_GetTypeName(piccType)));

    // Check for compatibility
    if (
            piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K)
    {
        Serial.println(F("Rfid::handleRfid This app only works with MIFARE Classic cards."));
        return;
    }

    if (m_write_on_next_card)
    {
        DEBUG_RFID(Serial.println(F("Rfid::handleRfid Writing to a card...")));

        writeBufferToCard(RFID_START_SECTOR, m_write_buffer, m_write_buffer_size);
        
        cancelWriteRfid(); // Clear everything out
    }
    else
    {
        DEBUG_RFID(Serial.println(F("Rfid::handleRfid Reading from a card...")));

        readBufferFromCard(RFID_START_SECTOR, t_read_buffer, t_buffer_size);
        
        read_callback(m_mfrc522.uid.uidByte, t_read_buffer, t_buffer_size);
    }

    // Halt PICC
    m_mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    m_mfrc522.PCD_StopCrypto1();

    DEBUG_RFID(Serial.println(F("Rfid::handleRfid Completed")));
}

Rfid::RfidIfaceReturn Rfid::writeBufferToCard(uint8_t t_starting_sector, const uint8_t* t_buffer, uint16_t t_buffer_size)
{
    Rfid::RfidIfaceReturn ret_val = RfidIfaceReturn::OK;

    DEBUG_RFID(Serial.println(F("Rfid::writeBufferToCard Writing string to a card")));

    for (auto i = 0; i <= ceil(t_buffer_size / RFID_BLOCK_SIZE); i++)
    {
        uint8_t block_content[RFID_BLOCK_SIZE];
        uint8_t block_size = (t_buffer_size < ((i + 1) * RFID_BLOCK_SIZE)) ? (t_buffer_size - (i * RFID_BLOCK_SIZE)) : RFID_BLOCK_SIZE;

        memset(block_content, 0, RFID_BLOCK_SIZE);
        memcpy(block_content, &t_buffer[i * RFID_BLOCK_SIZE], block_size);

        DEBUG_RFID(Serial.print(F("Rfid::writeBufferToCard Writing sector["));
                    Serial.print(t_starting_sector + (uint8_t)(i / 3));
                    Serial.print(F("] block["));
                    Serial.print(i % 3);
                    Serial.print(F("] ["));
                    printByteArray(block_content, block_size);
                    Serial.println(F("]")));
        
        if (writeRfidBlock(t_starting_sector + (uint8_t)(i / 3), i % 3, block_content, sizeof(block_content)) == RfidIfaceReturn::OK)
        {
            DEBUG_RFID(Serial.println(F("Rfid::writeBufferToCard Written buffer to a card")));
        }
        else
        {
            DEBUG_RFID(Serial.println(F("Rfid::writeBufferToCard Error while writing a card")));
        }
    }
    return ret_val;
}

Rfid::RfidIfaceReturn Rfid::readBufferFromCard(uint8_t t_starting_sector, uint8_t* t_return_buffer, uint16_t t_buffer_size)
{
    Rfid::RfidIfaceReturn ret_val = RfidIfaceReturn::OK;

    DEBUG_RFID(Serial.println(F("Rfid::readBufferFromCard Reading string from a card")));

    memset(t_return_buffer, 0, t_buffer_size); // Clear the buffer to start
  
    for (uint8_t i = 0; i < (t_buffer_size / RFID_BLOCK_SIZE); i++)
    {
        uint8_t read_buffer[RFID_BLOCK_SIZE + 2]; // Block is 16 bytes, but we need a buffer of 18 (as defined in docs)

        DEBUG_RFID(Serial.print(F("Rfid::readBufferFromCard Reading sector["));
                    Serial.print(t_starting_sector + (uint8_t)(i / 3));
                    Serial.print(F("] block["));
                    Serial.print(i % 3);
                    Serial.println("]"));
    
        ret_val = readRfidBlock(t_starting_sector + (uint8_t)(i / 3), i % 3, read_buffer, sizeof(read_buffer) / sizeof(uint8_t));

        if (ret_val == RfidIfaceReturn::OK)
        {
            memcpy(t_return_buffer + (i * RFID_BLOCK_SIZE), read_buffer, (((i + 1) * RFID_BLOCK_SIZE) > t_buffer_size) ? t_buffer_size : RFID_BLOCK_SIZE);
            DEBUG_RFID(Serial.println(F("Rfid::readBufferFromCard Read string from a card")));
        }
        else
        {
            DEBUG_RFID(Serial.println(F("Rfid::readBufferFromCard Error occured while reading from a card")));
        }
  }

  return ret_val;
}

Rfid::RfidIfaceReturn Rfid::readRfidBlock(uint8_t t_sector, uint8_t t_relative_block, uint8_t *t_output_buffer, uint8_t t_buffer_size)
{
    if (t_relative_block > 3)
    {
        DEBUG_RFID(Serial.println(F("Rfid::readRfidBlock Invalid block number")));
        return RfidIfaceReturn::INVALID_BLOCK_NUMBER;
    }

    uint8_t absolute_block = (t_sector * 4) + t_relative_block; // Translate the relative block in to an absolute block number
  
    MFRC522::StatusCode status;
  
    // Authenticate using key A
    DEBUG_RFID(Serial.println(F("Rfid::readRfidBlock Authenticating using key A...")));
    status = (MFRC522::StatusCode) m_mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, absolute_block, &m_key, &(m_mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
        DEBUG_RFID(Serial.print(F("Rfid::readRfidBlock PCD_Authenticate() failed: "));
                    Serial.println(m_mfrc522.GetStatusCodeName(status)));
        return RfidIfaceReturn::AUTHENTICATION_FAILURE;
    }

    DEBUG_RFID(Serial.print(F("Rfid::readRfidBlock Authenticated")));

    // Read the bytes in the available block in to the buffer
    status = m_mfrc522.MIFARE_Read(absolute_block, t_output_buffer, &t_buffer_size); // &t_buffer_size is a pointer to the t_buffer_size variable; MIFARE_Read requires a pointer instead of just a number
    if (status != MFRC522::STATUS_OK)
    {
        DEBUG_RFID(Serial.print(F("Rfid::readRfidBlock MIFARE_read() failed: "));
                    Serial.println(m_mfrc522.GetStatusCodeName(status)));

        return RfidIfaceReturn::READ_FAILURE;
    }
  
    DEBUG_RFID(Serial.print(F("Rfid::readRfidBlock Data in block "));
                Serial.print(absolute_block);
                Serial.println(F(":"));
                printByteArray(t_output_buffer, 16);
                Serial.println());
  
    return RfidIfaceReturn::OK;
}

Rfid::RfidIfaceReturn Rfid::writeRfidBlock(uint8_t t_sector, uint8_t t_relativeBlock, const uint8_t *t_write_buffer, uint8_t t_buffer_size)
{
    MFRC522::PICC_Type piccType = m_mfrc522.PICC_GetType(m_mfrc522.uid.sak);

    if (t_relativeBlock > 3)
    {
        DEBUG_RFID(Serial.println(F("Rfid::writeRfidBlock Invalid block number")));

        return RfidIfaceReturn::INVALID_BLOCK_NUMBER;
    }
  
    uint8_t absolute_block = (t_sector * 4) + t_relativeBlock;
    if (absolute_block > 2 && (absolute_block + 1) % 4 == 0)
    {
        DEBUG_RFID(Serial.print(F("Rfid::writeRfidBlock "));
                    Serial.print(absolute_block);
                    Serial.println(F(" is a trailer block:")));
        return RfidIfaceReturn::TRAILER_BLOCK_WRITE_ERROR; // Block number is a trailer block (modulo 4); quit and send error code 2
    }
    
    DEBUG_RFID(Serial.print(F("Rfid::writeRfidBlock "));
                Serial.print(absolute_block);
                Serial.println(F(" is a data block:")));

    // Authenticate using key A
    DEBUG_RFID(Serial.println(F("Rfid::writeRfidBlock Authenticating using key A...")));
    MFRC522::StatusCode status = m_mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, absolute_block, &m_key, &(m_mfrc522.uid));
    
    if (status != MFRC522::STATUS_OK)
    {
        DEBUG_RFID(Serial.print(F("Rfid::writeRfidBlock PCD_Authenticate() failed: "));
                    Serial.println(m_mfrc522.GetStatusCodeName(status)));

        return RfidIfaceReturn::AUTHENTICATION_FAILURE;
    }

    DEBUG_RFID(Serial.print(F("Rfid::writeRfidBlock Authenticated")));

    uint8_t block_size = RFID_BLOCK_SIZE;
    if(piccType == MFRC522::PICC_TYPE_MIFARE_UL)
    {
        block_size = RFID_BLOCK_SIZE_ULTRA;
    }

    // Copy the supplied buffer in to the actual write buffer
    byte write_buffer[block_size];

    // Mifare Ultras need to be written in chunks of 4 bytes instead of 16 across the block
    for(uint8_t i = 0; i < (RFID_BLOCK_SIZE / block_size); i++)
    {
        memset(write_buffer, 0, block_size * sizeof(uint8_t));
        memcpy(write_buffer, t_write_buffer + (i * block_size), (((i + 1) * block_size > t_buffer_size) ? (t_buffer_size % block_size) : block_size) * sizeof(uint8_t));
        
        // Write block
        DEBUG_RFID(Serial.print(F("Rfid::writeRfidBlock Writing data to block: "));
                    printByteArray(write_buffer, block_size));

        status = m_mfrc522.MIFARE_Write(absolute_block, write_buffer, block_size);

        if (status != MFRC522::STATUS_OK)
        {
            DEBUG_RFID(Serial.print(F("Rfid::writeRfidBlock MIFARE_Write() failed: "));
                        Serial.println(m_mfrc522.GetStatusCodeName(status)));

            return RfidIfaceReturn::WRITE_FAILURE;
        }
    }

    DEBUG_RFID(Serial.println(F("Rfid::writeRfidBlock Block was written")));
    return RfidIfaceReturn::OK;
}

// Helper routine to print a byte array as hex values to Serial
void Rfid::printByteArray(const uint8_t *t_buffer, const uint8_t t_buffer_size)
{
    for (uint8_t i = 0; i < t_buffer_size; i++) {
        Serial.print(t_buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(t_buffer[i], HEX);
    }
}
