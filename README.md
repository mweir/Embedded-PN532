Embedded-PN532
==============

This is an open source NFC stack for the arduino microcontroller that allow peer-to-peer 
communication between an embedded platform and an Android NFC enabled device. The 
library implements the Ndef Push Protocol (NPP) and has been tested with an Android NFC-enabled phone
running Android 2.3.3.


Files
==============
NDEFPushProtocol - Implements the NDEF push protocol as described by http://static.googleusercontent.com/external_content/untrusted_dlcp/source.android.com/en/us/compatibility/ndef-push-protocol.pdf
NFCLinkLayer - Implements the NFC LinkLayer Protocol that is used by the NDEFPushProtocol. The Link Layer protocol
               can be found at http://www.nfc-forum.org/specs/spec_list/.
PN532 - Handle communication between the embedded device and the PN532 NFC reader.


Example
==============
The example is arduino program that send and recieves NDEF messages that contain text. When it recieves an 
NDEF message, it retrieves the text payload from the NDEF message and updates the NDEF message it sends to 
contain the recieved text payload. The program works with the example Android NFC program, stick notes 
(The code for stick notes can be found at http://nfc.android.com/).