// PN532 library by adafruit/ladyada
// MIT license

// authenticateBlock, readMemoryBlock, writeMemoryBlock contributed
// by Seeed Technology Inc (www.seeedstudio.com)

#include "PN532.h"

#define PN532DEBUG 1

uint8_t pn532ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
uint8_t pn532response_firmwarevers[] = {0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03};

#define COMMAND_RESPONSE_SIZE 3 
#define TS_GET_DATA_IN_MAX_SIZE  262 + 3

uint8_t pn532_packetbuffer[TS_GET_DATA_IN_MAX_SIZE];

PN532::PN532(uint8_t clk, uint8_t miso, uint8_t mosi, uint8_t ss) 
{
    _clk = clk;
    _miso = miso;
    _mosi = mosi;
    _ss = ss;

    pinMode(_ss, OUTPUT);
    pinMode(_clk, OUTPUT);
    pinMode(_mosi, OUTPUT);
    pinMode(_miso, INPUT);
}

void PN532::initializeReader() 
{
    digitalWrite(_ss, LOW);

    delay(1000);

    // not exactly sure why but we have to send a dummy command to get synced up
    pn532_packetbuffer[0] = PN532_FIRMWAREVERSION;
    sendCommandCheckAck(pn532_packetbuffer, 1);

    // ignore response!
}


uint32_t PN532::getFirmwareVersion(void) 
{
    uint32_t version;

    pn532_packetbuffer[0] = PN532_FIRMWAREVERSION;

    if (IS_ERROR(sendCommandCheckAck(pn532_packetbuffer, 1))) 
    {
        return 0;
    }

    // read response Packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    if (IS_ERROR(readspicommand(PN532_FIRMWAREVERSION, response)))
    {
       return 0;
    }
    
    //response->printResponse();
    
    version = response->data[0];
    version <<= 8;
    version |= response->data[1];
    version <<= 8;
    version |= response->data[2];
    version <<= 8;
    version |= response->data[3];

    return version;
}


// default timeout of one second
uint32_t PN532::sendCommandCheckAck(uint8_t *cmd, 
                                    uint8_t cmdlen, 
                                    uint16_t timeout, 
                                    boolean debug) 
{
    uint16_t timer = 0;
    
    // write the command
    spiwritecommand(cmd, cmdlen, debug);
    
    // Wait for chip to say its ready!
    while (readspistatus() != PN532_SPI_READY) 
    {
        if (timeout != 0) 
        {
            timer+=10;
            if (timer > timeout) 
            {
                return SEND_COMMAND_TX_TIMEOUT_ERROR;
            }
        }
        delay(10);
    }
    
    // read acknowledgement
    if (!spi_readack(debug)) {
        return SEND_COMMAND_RX_ACK_ERROR;
    }
    
    timer = 0;
    
    // Wait for chip to say its ready!
    while (readspistatus() != PN532_SPI_READY) 
    {
        if (timeout != 0) 
        {
            timer+=10;
            if (timer > timeout) 
            {
                return SEND_COMMAND_RX_TIMEOUT_ERROR;
            }
        }
        delay(10);
    }

    return RESULT_SUCCESS; // ack'd command
}

uint32_t PN532::SAMConfig(void) 
{
    pn532_packetbuffer[0] = PN532_SAMCONFIGURATION;
    pn532_packetbuffer[1] = 0x01; // normal mode;
    pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
    pn532_packetbuffer[3] = 0x01; // use IRQ pin!
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 4);

    if (IS_ERROR(result)) 
    {
        return result;
    }

    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    return readspicommand(PN532_SAMCONFIGURATION, response);
}

uint32_t PN532::configurePeerAsInitiator(uint8_t baudrate) 
{
    // Does not support 106
    if (baudrate != NFC_READER_CFG_BAUDRATE_201_KPS && baudrate != NFC_READER_CFG_BAUDRATE_424_KPS) 
    {
       return CONFIGURE_HARDWARE_ERROR;      
    }
    
    pn532_packetbuffer[0] = PN532_INJUMPFORDEP;
    pn532_packetbuffer[1] = 0x01; //Active Mode
    pn532_packetbuffer[2] = baudrate; // Use 1 or 2. //0 i.e 106kps is not supported yet
    pn532_packetbuffer[3] = 0x01; //Indicates Optional Payload is present

    //Polling request payload
    pn532_packetbuffer[4] = 0x00; 
    pn532_packetbuffer[5] = 0xFF; 
    pn532_packetbuffer[6] = 0xFF; 
    pn532_packetbuffer[7] = 0x00; 
    pn532_packetbuffer[8] = 0x00; 
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 9);

    if (IS_ERROR(result)) 
    {
        return result;
    }

    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    result = readspicommand(PN532_INJUMPFORDEP,  response);
    if (IS_ERROR(result))
    {
       return result;    
    }
    
    if (response->data[0] != 0x00)
    {
       return (GEN_ERROR | response->data[0]);
    }
    
    return RESULT_SUCCESS; //No error
}


uint32_t PN532::initiatorTxRxData(uint8_t *DataOut, 
                           uint32_t dataSize, 
                           uint8_t *DataIn,
                           boolean debug)
{
    pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 0x01; //Target 01

    for(uint8_t iter=(2+0);iter<(2+dataSize);iter++)
    {
        pn532_packetbuffer[iter] = DataOut[iter-2]; //pack the data to send to target
    }
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, dataSize+2);

    if (IS_ERROR(result))
    {
        return result;
    }
    
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    if (!readspicommand(PN532_INDATAEXCHANGE, response)) 
    {
       return false;    
    }
    
    if (debug) 
    { 
       response->printResponse();   
    }
    
    if (response->data[0] != 0x00)
    {
       return (GEN_ERROR | response->data[0]);
    }
    return RESULT_SUCCESS; //No error
}

uint32_t PN532::configurePeerAsTarget(uint8_t type) 
{
    static const uint8_t npp_client[44] =      { PN532_TGINITASTARGET, 
                             0x00,
                             0x00, 0x00, //SENS_RES
                             0x00, 0x00, 0x00, //NFCID1
                             0x00, //SEL_RES

                             0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // POL_RES
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                           
                             0x00, 0x00,
                            
                             0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //NFCID3t: Change this to desired value

                             0x06, 0x46,  0x66, 0x6D, 0x01, 0x01, 0x10, 0x00
                             };
    
    static const uint8_t npp_server[44] =      { PN532_TGINITASTARGET, 
                             0x01,
                             0x00, 0x00, //SENS_RES
                             0x00, 0x00, 0x00, //NFCID1
                             0x40, //SEL_RES

                             0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6, 0xC9, 0x89, // POL_RES
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                           
                             0xFF, 0xFF,
                            
                             0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6, 0xC9, 0x89, 0x00, 0x00, //NFCID3t: Change this to desired value

                             0x06, 0x46,  0x66, 0x6D, 0x01, 0x01, 0x10, 0x00
                             };  
                             
    if (type == NPP_CLIENT)  
    {                                              
       for(uint8_t iter = 0;iter < 44;iter++)
       {
          pn532_packetbuffer[iter] = npp_client[iter];
       }
    } 
    else if (type == NPP_SERVER)  
    {
       for(uint8_t iter = 0;iter < 44;iter++)
       {
          pn532_packetbuffer[iter] = npp_server[iter];
       }
    }
    
    uint32_t result;
    result = sendCommandCheckAck(pn532_packetbuffer, 44);
   
    if (IS_ERROR(result))
    {       
        return result;
    }
    
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    return readspicommand(PN532_TGINITASTARGET, response);
}

uint32_t PN532::getTargetStatus(uint8_t *DataIn)
{
    pn532_packetbuffer[0] = PN532_TGTARGETSTATUS;
    
    if (IS_ERROR(sendCommandCheckAck(pn532_packetbuffer, 1))) {
        return 0;
    }
    
    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    if (RESULT_OK(readspicommand(PN532_TGTARGETSTATUS, response)))
    {
       memcpy(DataIn, response->data, response->data_len);
       return response->data_len;
    } 
    
    return 0;
}

uint32_t PN532::targetRxData(uint8_t *DataIn, boolean debug)
{
    ///////////////////////////////////// Receiving from Initiator ///////////////////////////
    pn532_packetbuffer[0] = PN532_TGGETDATA;
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 1, 1000, debug);
    if (IS_ERROR(result)) {
        //Serial.println(F("SendCommandCheck Ack Failed"));
        return NFC_READER_COMMAND_FAILURE;
    }
    
    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    
    result = readspicommand(PN532_TGGETDATA, response, debug);
    
    if (IS_ERROR(result)) 
    {
       return NFC_READER_RESPONSE_FAILURE;    
    }
 
    if (response->data[0] == 0x00)
    {
       uint32_t ret_len = response->data_len - 1;
       memcpy(DataIn, &(response->data[1]), ret_len);
       return ret_len;
    }
    
    return (GEN_ERROR | response->data[0]);
}



uint32_t PN532::targetTxData(uint8_t *DataOut, uint32_t dataSize, boolean debug)
{
    ///////////////////////////////////// Sending to Initiator ///////////////////////////
    pn532_packetbuffer[0] = PN532_TGSETDATA;
    uint8_t commandBufferSize = (1 + dataSize);
    for(uint8_t iter=(1+0);iter < commandBufferSize; ++iter)
    {
        pn532_packetbuffer[iter] = DataOut[iter-1]; //pack the data to send to target
    }
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, commandBufferSize, 1000, debug);
    if (IS_ERROR(result)) {
        Serial.println(F("TX_Target Command Failed."));
        return result;
    }
    
    
    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    
    result = readspicommand(PN532_TGSETDATA, response);
    if (IS_ERROR(result)) 
    {
       return result;    
    }
    
    if (debug)
    {
       response->printResponse();
    }
    
    if (response->data[0] != 0x00)
    {
       return (GEN_ERROR | response->data[0]);
    }
    return RESULT_SUCCESS; //No error
}  
    

uint32_t PN532::authenticateBlock(uint8_t cardnumber /*1 or 2*/,
                                  uint32_t cid /*Card NUID*/, 
                                  uint8_t blockaddress /*0 to 63*/,
                                  uint8_t authtype/*Either KEY_A or KEY_B */, 
                                  uint8_t * keys) 
{
    pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
    pn532_packetbuffer[1] = cardnumber;  // either card 1 or 2 (tested for card 1)
    if(authtype == KEY_A)
    {
        pn532_packetbuffer[2] = PN532_AUTH_WITH_KEYA;
    }
    else
    {
        pn532_packetbuffer[2] = PN532_AUTH_WITH_KEYB;
    }
    pn532_packetbuffer[3] = blockaddress; //This address can be 0-63 for MIFARE 1K card

    pn532_packetbuffer[4] = keys[0];
    pn532_packetbuffer[5] = keys[1];
    pn532_packetbuffer[6] = keys[2];
    pn532_packetbuffer[7] = keys[3];
    pn532_packetbuffer[8] = keys[4];
    pn532_packetbuffer[9] = keys[5];

    pn532_packetbuffer[10] = ((cid >> 24) & 0xFF);
    pn532_packetbuffer[11] = ((cid >> 16) & 0xFF);
    pn532_packetbuffer[12] = ((cid >> 8) & 0xFF);
    pn532_packetbuffer[13] = ((cid >> 0) & 0xFF);
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 14);

    if (IS_ERROR(result)) 
    {
        return result;
    }
    
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    result = readspicommand(PN532_INDATAEXCHANGE, response);
    if (IS_ERROR(result)) 
    {
       return result;    
    }


    if((response->data[0] == 0x41) && (response->data[1] == 0x00))
    {
  	    return RESULT_SUCCESS;
    }

    return (GEN_ERROR | response->data[1]);
}

uint32_t PN532::readMemoryBlock(uint8_t cardnumber /*1 or 2*/,
                                uint8_t blockaddress /*0 to 63*/, 
                                uint8_t * block) 
{
    pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
    pn532_packetbuffer[1] = cardnumber;  // either card 1 or 2 (tested for card 1)
    pn532_packetbuffer[2] = PN532_MIFARE_READ;
    pn532_packetbuffer[3] = blockaddress; //This address can be 0-63 for MIFARE 1K card
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 4);

    if (IS_ERROR(result)) {
        return result;
    }

    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    result = readspicommand(PN532_INDATAEXCHANGE, response);
    if (IS_ERROR(result)) 
    {
       return result;    
    }

    if((response->data[0] == 0x41) && (response->data[1] == 0x00))
    {
  	    return RESULT_SUCCESS; //read successful
    }

    return (GEN_ERROR | response->data[1]);
}

//Do not write to Sector Trailer Block unless you know what you are doing.
uint32_t PN532::writeMemoryBlock(uint8_t cardnumber /*1 or 2*/,
                                 uint8_t blockaddress /*0 to 63*/, 
                                 uint8_t * block) 
{
    pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
    pn532_packetbuffer[1] = cardnumber;  // either card 1 or 2 (tested for card 1)
    pn532_packetbuffer[2] = PN532_MIFARE_WRITE;
    pn532_packetbuffer[3] = blockaddress;

    for(uint8_t i =0; i <16; i++)
    {
        pn532_packetbuffer[4+i] = block[i];
    }
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 20);

    if (IS_ERROR(result))
    {
        return result;
    }
    
    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    result = readspicommand(PN532_INDATAEXCHANGE, response);
    if (IS_ERROR(result)) 
    {
       return result;    
    }

    if((response->data[0] == 0x41) && (response->data[1] == 0x00))
    {
  	    return RESULT_SUCCESS; //read successful
    }

    return (GEN_ERROR | response->data[1]);
}

uint32_t PN532::readPassiveTargetID(uint8_t cardbaudrate) 
{
    uint32_t cid;

    pn532_packetbuffer[0] = PN532_INLISTPASSIVETARGET;
    pn532_packetbuffer[1] = 1;  // max 1 cards at once (we can set this to 2 later)
    pn532_packetbuffer[2] = cardbaudrate;

    if (IS_ERROR(sendCommandCheckAck(pn532_packetbuffer, 3)))
    {
        return 0;  // no cards read
    }

    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    if (IS_ERROR(readspicommand(PN532_INDATAEXCHANGE, response))) 
    {
       return 0;    
    }
    
    // check some basic stuff
    Serial.print(F("Found ")); 
    Serial.print(response->data[2], DEC); 
    Serial.println(F(" tags"));
    
    if (response->data[2] != 1) 
    {
        return 0;
    }
    
    uint16_t sens_res = response->data[4];
    sens_res <<= 8;
    sens_res |= response->data[5];
    
    Serial.print(F("Sens Response: 0x"));  
    Serial.println(sens_res, HEX);
    Serial.print(F("Sel Response: 0x"));  
    Serial.println(response->data[6], HEX);
    
    cid = 0;
    for (uint8_t i = 0; i < response->data[7]; i++) 
    {
        cid <<= 8;
        cid |= response->data[8 + i];
        Serial.print(F(" 0x")); 
        Serial.print(response->data[8 + i], HEX);
    }

    return cid;
}


inline boolean PN532::isTargetReleasedError(uint32_t result)
{
   return result == (GEN_ERROR | TARGET_RELEASED_ERROR);
}

/************** high level SPI */


boolean PN532::spi_readack(boolean debug) 
{
    uint8_t ackbuff[6];

    readspidata(ackbuff, 6, debug);

    return (0 == strncmp((char *)ackbuff, (char *)pn532ack, 6));
}

/************** mid level SPI */

uint8_t PN532::readspistatus(void) 
{
    digitalWrite(_ss, LOW);
    delay(2);
    spiwrite(PN532_SPI_STATREAD);
    // read uint8_t
    uint8_t x = spiread();

    digitalWrite(_ss, HIGH);
    return x;
}

uint32_t PN532::readspicommand(uint8_t cmdCode, PN532_CMD_RESPONSE *response, boolean debug) 
{

    uint8_t calc_checksum = 0;
    uint8_t ret_checksum;
    
    digitalWrite(_ss, LOW);
    delay(2);
    spiwrite(PN532_SPI_DATAREAD);
    
    response->header[0] = response->header[1] = 0xAA;
    
    uint32_t retVal = RESULT_SUCCESS;
    
    do 
    {
       response->header[0] = response->header[1];
       delay(1);
       response->header[1] = spiread();
    } while (response->header[0] != 0x00 || response->header[1] != 0xFF);

    delay(1);
    response->len = spiread();  
    
    delay(1);
    response->len_chksum = spiread();  
    
    delay(1);
    response->direction = spiread(); 
    calc_checksum += response->direction;
    
    delay(1);
    response->responseCode = spiread(); 
    
    calc_checksum += response->responseCode;
    
    retVal = response->verifyResponse(cmdCode) ? RESULT_SUCCESS : INVALID_RESPONSE;  
    
    if (RESULT_OK(retVal)) 
    {  
        // Readjust the len to account only for the data
        // Removing the Direction and response byte from the data length parameter
        response->data_len = response->len - 2;
        
        for (uint8_t i = 0; i < response->data_len; ++i) 
        {
            delay(1); 
            response->data[i] = spiread();
            calc_checksum +=  response->data[i];
         }
         
         delay(1); 
         ret_checksum = spiread();
        
         if (((uint8_t)(calc_checksum + ret_checksum)) != 0x00) 
         {
            Serial.println(F("Invalid Checksum recievied."));
            retVal = INVALID_CHECKSUM_RX;
         }
        
         delay(1); 
         uint8_t postamble = spiread();
         
         
         if (RESULT_OK(retVal) && postamble != 0x00) 
         {
             retVal = INVALID_POSTAMBLE;
             Serial.println(F("Invalid Postamble."));
         }
       
    }
    
    digitalWrite(_ss, HIGH); 
    if (debug)
    {
      response->printResponse();
      
      Serial.print(F("Calculated Checksum: 0x"));
      Serial.print(calc_checksum, HEX);
      Serial.print(F(" Returned Checksum: 0x") );
      Serial.println(ret_checksum, HEX);
      Serial.println();    
    }

    return retVal;
}

void PN532::readspidata(uint8_t* buff, uint32_t n, boolean debug) 
{
    digitalWrite(_ss, LOW);
    delay(2);
    spiwrite(PN532_SPI_DATAREAD);
    
    if (debug) 
    {
        Serial.print(F("Reading: "));
    }
    
    for (uint8_t i=0; i<n; i++) 
    {
        delay(1);
        buff[i] = spiread();
        
        if (debug)
        {
            Serial.print(F(" 0x"));
            Serial.print(buff[i], HEX);
        }
    }

    if (debug)
    {
        Serial.println();
    }

    digitalWrite(_ss, HIGH);
}

void PN532::spiwritecommand(uint8_t* cmd, uint8_t cmdlen, boolean debug) 
{
    uint8_t checksum;
    cmdlen++;

    if (debug) 
    {
      Serial.print(F("\nSending: "));
    }
    digitalWrite(_ss, LOW);
    delay(2);     // or whatever the delay is for waking up the board
    spiwrite(PN532_SPI_DATAWRITE);

    checksum = PN532_PREAMBLE + PN532_PREAMBLE + PN532_STARTCODE2;
    spiwrite(PN532_PREAMBLE);
    spiwrite(PN532_PREAMBLE);
    spiwrite(PN532_STARTCODE2);

    spiwrite(cmdlen);
    uint8_t cmdlen_1=~cmdlen + 1;
    spiwrite(cmdlen_1);

    spiwrite(PN532_HOSTTOPN532);
    checksum += PN532_HOSTTOPN532;

    if (debug) 
    {
        Serial.print(F(" 0x")); Serial.print(PN532_PREAMBLE, HEX);
        Serial.print(F(" 0x")); Serial.print(PN532_PREAMBLE, HEX);
        Serial.print(F(" 0x")); Serial.print(PN532_STARTCODE2, HEX);
        Serial.print(F(" 0x")); Serial.print(cmdlen, HEX);
        Serial.print(F(" 0x")); Serial.print(cmdlen_1, HEX);
        Serial.print(F(" 0x")); Serial.print(PN532_HOSTTOPN532, HEX);
    }

    for (uint8_t i=0; i<cmdlen-1; i++) {
        spiwrite(cmd[i]);
        checksum += cmd[i];
        if (debug) 
        {
          Serial.print(F(" 0x")); Serial.print(cmd[i], HEX);
        }
    }
    uint8_t checksum_1=~checksum;
    spiwrite(checksum_1);
    spiwrite(PN532_POSTAMBLE);
    digitalWrite(_ss, HIGH);

    if (debug) 
    {
      Serial.print(F(" 0x")); Serial.print(checksum_1, HEX);
      Serial.print(F(" 0x")); Serial.print(PN532_POSTAMBLE, HEX);
      Serial.println();
    }
} 
/************** low level SPI */

void PN532::spiwrite(uint8_t c) 
{
    int8_t i;
    digitalWrite(_clk, HIGH);

    for (i=0; i<8; i++) 
    {
        digitalWrite(_clk, LOW);
        if (c & _BV(i)) 
        {
            digitalWrite(_mosi, HIGH);
        } 
        else 
        {
            digitalWrite(_mosi, LOW);
        }
        digitalWrite(_clk, HIGH);
    }
}

uint8_t PN532::spiread(void) 
{
    int8_t i, x;
    x = 0;
    digitalWrite(_clk, HIGH);

    for (i=0; i<8; i++) 
    {
        if (digitalRead(_miso)) 
        {
            x |= _BV(i);
        }
        digitalWrite(_clk, LOW);
        digitalWrite(_clk, HIGH);
    }
    return x;
}

boolean PN532_CMD_RESPONSE::verifyResponse(uint32_t cmdCode)
{
    return ( header[0] == 0x00 && 
             header[1] == 0xFF &&
            ((uint8_t)(len + len_chksum)) == 0x00 && 
            direction == 0xD5 && 
            (cmdCode + 1) == responseCode);
}

void PN532_CMD_RESPONSE::printResponse() 
{
    Serial.println(F("Response"));
    Serial.print(F("Len: 0x"));
    Serial.println(len, HEX);
    Serial.print(F("Direction: 0x"));
    Serial.println(direction, HEX);
    Serial.print(F("Response Command: 0x"));
    Serial.println(responseCode, HEX);
    
    Serial.println("Data: ");
    
    for (uint8_t i = 0; i < data_len; ++i) 
    {
        Serial.print(F("0x"));
        Serial.print(data[i], HEX);
        Serial.print(F(" "));
    }
    
    Serial.println();
}
