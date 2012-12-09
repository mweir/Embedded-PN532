#include "Arduino.h"
#include "NFCLinkLayer.h"

#ifndef NPP_H
#define NPP_H

#define NPP_SUPPORTED_VERSION 0x01

#define MODIFY_ENDIAN(x) ((((x) >> 24) & 0xFF)         /* move byte 3 to byte 0 */ \
                                   | (((x) << 24) & 0xFF000000) /* move byte 0 to byte 3 */ \
                                   | (((x) << 8)  & 0xFF0000)   /* move byte 1 to byte 2 */ \
                                   | (((x) >> 8)  & 0xFF00))    /* move byte 2 to byte 1 */         


#define NDEF_MESSAGE_BEGIN_FLAG          0x80
#define NDEF_MESSAGE_END_FLAG            0x40
#define NDEF_MESSAGE_CHUNK_FLAG          0X20
#define NDEF_MESSAGE_SHORT_RECORD        0X10
#define NDEF_MESSAGE_ID_LENGTH_PRESENT   0X08
#define NDEF_MESSAGE_TYPENAME_FORMAT     0x07


#define TYPE_FORMAT_EMPTY             0x00
#define TYPE_FORMAT_NFC_FORUM_TYPE    0x01
#define TYPE_FORMAT_MEDIA_TYPE        0x02
#define TYPE_FORMAT_ABSOLUTE_URI      0x03
#define TYPE_FORMAT_NFC_FORUM_EXTERNAL_TYPE    0x04
#define TYPE_FORMAT_UNKNOWN_TYPE               0x05
#define TYPE_FORMAT_UNCHANGED_TYPE             0x06
#define TYPE_FORMAT_RESERVED_TYPE              0x07  


#define NPP_ACTION_CODE            0x01
#define NFC_FORUM_TEXT_TYPE        0x54                   


enum NDEFState 
{
    NDEF_TYPE_LEN,
    NDEF_PAYLOAD_LEN,
    NDEF_ID_LEN,
    NDEF_TYPE,
    NDEF_ID,
    NDEF_PAYLOAD,
    NDEF_FINISHED    
};


#define NPP_MESSAGE_HDR_LEN   0x0A

struct NPP_MESSAGE 
{
   uint8_t version;
   uint32_t numNDEFEntries;
   uint8_t actionCode;
   uint32_t NDEFLength;
   uint8_t NDEFMessage[0];   
};


/*struct TEXT_PAYLOAD
{
   uint8_t textType;  // ASCII, UTF-8, ect.
   uint8_t locale[2];
   uint8_t text[0]; 
};
*/

class NDEFPushProtocol 
{

public:
   NDEFPushProtocol(NFCLinkLayer *);  
   ~NDEFPushProtocol();
   
   uint32_t openRxConnection(uint32_t timeout);
   uint32_t closeRxConnection();
   
   uint32_t rxNDEFPayload(uint8_t *&NDEFMessage);
   uint32_t pushPayload(uint8_t *NDEFMessage, uint32_t length); 
private:
   NFCLinkLayer *_linkLayer;
};

#endif