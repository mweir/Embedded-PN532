#include "NDEFPushProtocol.h"

NDEFPushProtocol::NDEFPushProtocol(NFCLinkLayer *linkLayer) :
    _linkLayer(linkLayer)
{
}

NDEFPushProtocol::~NDEFPushProtocol() 
{
}

uint32_t NDEFPushProtocol::rxNDEFPayload(uint8_t *&data)
{
    uint32_t result = _linkLayer->openNPPServerLink();

    if(RESULT_OK(result)) //if connection is error-free
    {
       //Serial.println(F("CONNECTED."));
       result = _linkLayer->serverLinkRxData(data);
       if (RESULT_OK(result))
       {
           NPP_MESSAGE *nppMessage = (NPP_MESSAGE *)data; 
           nppMessage->numNDEFEntries = MODIFY_ENDIAN(nppMessage->numNDEFEntries);
           nppMessage->NDEFLength = MODIFY_ENDIAN(nppMessage->NDEFLength);
           
           if (nppMessage->version != NPP_SUPPORTED_VERSION) 
           {
              Serial.println(F("Recieved an NPP message of an unsupported version."));
              return NPP_UNSUPPORTED_VERSION;
           } 
           else if (nppMessage->numNDEFEntries != 1) 
           {
              Serial.println(F("Invalid number of NDEF entries"));
              return NPP_INVALID_NUM_ENTRIES;
           } 
           else if (nppMessage->actionCode != NPP_ACTION_CODE)
           {
              Serial.println(F("Invalid Action Code"));
              return NPP_INVALID_ACTION_CODE;
           }
           
           _linkLayer->closeNPPServerLink();
           
           //Serial.println(F("Returning NPP Message"));
           //Serial.print(F("Length: "));
           //Serial.println(nppMessage->NDEFLength, HEX);
           data = nppMessage->NDEFMessage;
           return nppMessage->NDEFLength;               
       }            
    }        
    return result;
}

uint32_t NDEFPushProtocol::pushPayload(uint8_t *NDEFMessage, uint32_t length)
{
    NPP_MESSAGE *nppMessage = (NPP_MESSAGE *) ALLOCATE_HEADER_SPACE(NDEFMessage, NPP_MESSAGE_HDR_LEN);
    
    nppMessage->version = NPP_SUPPORTED_VERSION;
    nppMessage->numNDEFEntries = MODIFY_ENDIAN((uint32_t)0x00000001);
    nppMessage->actionCode = NPP_ACTION_CODE;
    nppMessage->NDEFLength = MODIFY_ENDIAN(length);
 
    
    /*uint8_t *buf = (uint8_t *) nppMessage;
    Serial.println(F("NPP + NDEF Message")); 
    for (uint16_t i = 0; i < length + NPP_MESSAGE_HDR_LEN; ++i)
    {
        Serial.print(F("0x")); 
        Serial.print(buf[i], HEX);
        Serial.print(F(" "));
    }*/
    uint32_t result = _linkLayer->openNPPClientLink();
  
    if(RESULT_OK(result)) //if connection is error-free
    { 
        result =  _linkLayer->clientLinkTxData((uint8_t *)nppMessage, length + NPP_MESSAGE_HDR_LEN);
    } 
    
    return result;       
}