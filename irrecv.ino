/*
 * IRrecord: record and play back IR signals as a minimal 
 * An IR detector/demodulator must be connected to the input RECV_PIN.
 * An IR LED must be connected to the output PWM pin 3.
 * A button must be connected to the input BUTTON_PIN; this is the
 * send button.
 * A visible LED can be connected to STATUS_PIN to provide status.
 *
 * The logic is:
 * If the button is pressed, send the IR code.
 * If an IR code is received, record it.
 *
 * Version 0.11 September, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 */

#include <IRremote.h>

int RECV_PIN = 11;
int STATUS_PIN = 13;

IRrecv irrecv(RECV_PIN);
IRsend irsend;

decode_results results;

void setup()
{
  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver
  pinMode(STATUS_PIN, OUTPUT);
}

// Storage for the recorded code
unsigned int rawCodes[RAWBUF]; // The durations if raw
unsigned int rawbinCodes[RAWBUF]; // The durations if raw
uint64_t rawValue = 0;
int codeLen = 0; // The length of the code
bool buttonState = LOW;

unsigned int openOrClose[48] = {1,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,0,0,0,0,0,0,1,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
unsigned int upTemp[48] = {1,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

#define NEC_HDR_MARK    9000
#define NEC_HDR_SPACE   4500
#define NEC_BIT_MARK     560
#define NEC_ONE_SPACE   1690
#define NEC_ZERO_SPACE   560
#define NEC_RPT_SPACE   2250

bool decodeNEC (decode_results *results)
{
  int   offset = 1;  // Index in to results; Skip first entry!?

  // Check header "mark"
  if (!MATCH_MARK(results->rawbuf[offset], NEC_HDR_MARK))  return false ;
  offset++;

  // Check header "space"
  if (!MATCH_SPACE(results->rawbuf[offset], NEC_HDR_SPACE))  return false ;
  offset++;

  // Build the data
  for (int i = 0;  i < (results->rawlen - 3)/2;  i++) {
    // Check data "mark"
    if (!MATCH_MARK(results->rawbuf[offset], NEC_BIT_MARK))  return false ;
    offset++;
    // Suppend this bit
    if (MATCH_SPACE(results->rawbuf[offset], NEC_ONE_SPACE )) {  
      rawValue = (rawValue << 1) | 1 ;
      rawbinCodes[i] = 1; 
    } else if (MATCH_SPACE(results->rawbuf[offset], NEC_ZERO_SPACE)) {
      rawValue = (rawValue << 1) | 0 ;
      rawbinCodes[i] = 0;
    } else                                                            
      return false;
    offset++;
  }

  return true;
}

// Stores the code for later playback
// Most of this code is just logging
void storeCode(decode_results *results) {
  //if (results->rawlen > 90)
  //  buttonState = HIGH;

  int codeType = results->decode_type;
  if (codeType == NEC) {
    Serial.print("Received NEC: ");
  } 
  else if (codeType == SONY) {
    Serial.print("Received SONY: ");
  } 
  else if (codeType == PANASONIC) {
    Serial.print("Received PANASONIC: ");
  }
  else if (codeType == JVC) {
    Serial.print("Received JVC: ");
  }
  else if (codeType == SANYO) {
    Serial.print("Received SANYO: ");
  }
  else if (codeType == UNKNOWN) {
    Serial.print("Received UNKNOWN: ");
  }
  else {
    Serial.print("Unexpected codeType ");
    Serial.print(codeType, DEC);
  }
  Serial.println("");
  
  codeLen = results->rawlen - 1;
  for (int i = 1; i <= codeLen; i++) {
    if (i % 2) {
      // Mark
      rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK - MARK_EXCESS;
      //Serial.print(" m");
    } 
    else {
      // Space
      rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK + MARK_EXCESS;
      //Serial.print(" s");
    }
    //Serial.print(rawCodes[i - 1], DEC);
  }
  //Serial.println("");
  decodeNEC(results);

 #if 1 
  for (int i = 0; i < (codeLen - 3)/2; i++) {
    Serial.print(rawbinCodes[i]);
  }
  Serial.println("");
 #endif
}

void sendNEC(unsigned int data[], int len)
{
  // Set IR carrier frequency
  irsend.enableIROut(38);

  // Header
  irsend.mark(NEC_HDR_MARK);
  irsend.space(NEC_HDR_SPACE);

  // Data
  for (int i = 0; i < len; i++) {
    if (data[i]) {
      irsend.mark(NEC_BIT_MARK);
      irsend.space(NEC_ONE_SPACE);
    } else {
      irsend.mark(NEC_BIT_MARK);
      irsend.space(NEC_ZERO_SPACE);
    }
  }

  // Footer
  irsend.mark(NEC_BIT_MARK);
  irsend.space(0);  // Always end with the LED off
}

void sendCode() {
  //irsend.sendRaw(rawCodes, codeLen, 38);
  sendNEC(openOrClose, 48);
  //sendNEC(rawbinCodes, (codeLen-3)/2);
  buttonState = LOW;
  sendNEC(openOrClose, 48);
}

String comdata = "";//声明字符串变量

void loop() {
  // check if data has been sent from the computer:
  /*if (Serial.available()) {
    char brightness = Serial.read();
    Serial.println(brightness);
    //buttonState = HIGH;
    if (brightness == '1') {
      sendNEC(openOrClose, 48);
      sendNEC(openOrClose, 48);
    }
    if (brightness == '2') {
      sendNEC(upTemp, 48);
      sendNEC(upTemp, 48);
    }
  }*/

  while (Serial.available() > 0) {
    comdata += char(Serial.read());
    delay(2);
  }

  int temp = 0;
  if (comdata.length() > 0) {
    Serial.println(comdata);
    if (comdata == "26")
      Serial.println(11111);
    temp = comdata.toInt() - 18;
    comdata = "";
    for (int i = 0; i < 4; i++) {
      if (temp&1 << i)
        upTemp[28+i] = 1;
      else
        upTemp[28+i] = 0;
    }
    for (int i = 0; i < 48; i++) {
      Serial.print(upTemp[i]);
    }
    Serial.println("");
    sendNEC(upTemp, 48);
    sendNEC(upTemp, 48);
  }

  if (buttonState) {
    Serial.println("Pressed, sending");
    sendCode();
    //delay(2000); // Wait a bit between retransmissions
  } 
  else if (irrecv.decode(&results)) {
    //Serial.println("Pressed, recving");
#if 0
    Serial.println(results.rawlen);
    for (int i = 0; i < results.rawlen; ++i)
    {
      Serial.print(results.rawbuf[i], DEC);
      Serial.print(" ");
    }
    Serial.println("");
#endif
    storeCode(&results);
    irrecv.resume(); // resume receiver
  }
}


