#include "Arduino.h"
#include "NFCReader.h"
#ifndef NFC_LINK_LAYER_H
#define NFC_LINK_LAYER_H

#define SYMM_PTYPE                    0x00
#define PAX_PTYPE                     0x01
#define AGGREGATE_PTYPE               0x02
#define UNNUMBERED_INFORMATION_PTYPE  0x03
#define CONNECT_PTYPE                 0x04
#define DISCONNECT_PTYPE              0x05
#define CONNECTION_COMPLETE_PTYPE     0x06
#define DISCONNECTED_MODE_PTYPE       0x07
#define FRAME_REJECT_PTYPE            0X08
#define INFORMATION_PTYPE             0x0C
#define RECEIVE_READY_TYPE            0x0D
#define RECEIVE_NOT_READY_TYPE        0x0E

#define SERVICE_NAME_PARAM_TYPE      0x06

#define CONNECT_SERVICE_NAME_LEN     0x0F
#define CONNECT_SERVICE_NAME         "com.android.npp"

#define CONNECT_SERVER_PDU_LEN       CONNECT_SERVICE_NAME_LEN + 4


struct PARAMETER_DESCR {
   union {
      uint8_t type;
      uint8_t sequence;
   };
   uint8_t length;
   uint8_t data[0];
};

struct PDU {
public:
   uint8_t field[2]; 
   PARAMETER_DESCR  params;


   PDU();
   
   uint8_t getDSAP();
   uint8_t getSSAP();
   uint8_t getPTYPE();

   void setDSAP(uint8_t DSAP);
   void setSSAP(uint8_t SSAP);
   void setPTYPE(uint8_t PTYPE);
   bool isConnectClientRequest();
};

class NFCLinkLayer {
public:
   NFCLinkLayer(NFCReader *nfcReader);
   ~NFCLinkLayer();
   
   uint32_t openNPPServerLink(boolean debug = false);
   uint32_t closeNPPServerLink();
      
   uint32_t openNPPClientLink(boolean debug = false);
   uint32_t closeNPPClientLink();
   
   uint32_t serverLinkRxData(uint8_t *&Data, boolean debug = false);
   uint32_t clientLinkTxData(uint8_t *nppMessage, uint32_t len, boolean debug = false);
private:
   NFCReader *_nfcReader;
   uint8_t DSAP;
   uint8_t SSAP;
};


#endif