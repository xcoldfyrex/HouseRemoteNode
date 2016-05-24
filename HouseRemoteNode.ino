#include <Arduino.h>
#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>
#include "printf.h"

#define SERVER_ADDRESS 1
#define CLIENT_ADDRESS 2

RH_NRF24 driver;
RHReliableDatagram manager(driver, CLIENT_ADDRESS);

#define PAYLOAD_LEN 64

struct payload_t {
    unsigned long id = 32767;
    char type[3];
    short source;
    char message[13] = "00000000000";
};

payload_t payload;

int incomingByte = 0;
int redPin = 3; // pins that the cathodes of LED are attached to
int greenPin = 5;
int bluePin = 6;

uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
uint8_t data[] = "32767";


void setup(void) {

    printf_begin();
    Serial.begin(115200);
    Serial.println("init");
    if (!manager.init()) {
      Serial.println("radio init failed");
    } else {
      Serial.println("radio init ok");
    }
}

void loop(void) {
  uint8_t len = sizeof(buf);
  uint8_t from;
  if (manager.recvfromAckTimeout(buf, &len, 2000, &from)) {
    memcpy(payload.type, buf+8, 2);
    memcpy(payload.message,buf+12, 8);
    //printf("type %s message %s \n", payload.type,payload.message);
    process_payload();
    if (!manager.sendtoWait(data, sizeof(data), from))
      Serial.println("sendtoWait failed");
  }
}

void process_payload() {
    char R[3] = "00";
    char G[3] = "00";
    char B[3] = "00";
    char W[3] = "00";

    // message type 0x00
    if (strcmp(payload.type, "00") == 0) {
        printf("GOT RESET COMMAND 0x00..\n\r");
        delay(500);
        asm volatile ("  jmp 0");
    }

    // message type 0x01
    if (strcmp(payload.type, "01") == 0) {
        payload.message[8] = 0;
        //printf("type %s message %s \n", payload.type,payload.message);
        if (strlen(payload.message) != 8) {
            invalidPayload(payload.type);
        } else {
            memcpy(R, &payload.message[0], 2);
            memcpy(G, &payload.message[2], 2);
            memcpy(B, &payload.message[4], 2);
            memcpy(W, &payload.message[6], 2);
            setColor((int)strtol(R, NULL, 16), (int)strtol(G, NULL, 16), (int)strtol(B, NULL, 16), (int)strtol(W, NULL, 16));
            return;
        }
    }

    // message type 0x02
    if (strcmp(payload.type, "02")) {
        //printf("PWM Poll request\n\r");
        //printf("R:%s G:%s B:%s: W:%s\n\r",R, G, B, W);
        //char buf[64];
        //sprintf(buf,"%s%s%s%s%s", payload.id, R ,G ,B ,W);
        //writeRadio(buf);
    }

    //message type 0x03
    if (strcmp(payload.type, "03")) {
        //payload.type = 0xFF;
        sprintf(payload.message, "%03x%03x%03x%03x", analogAverage(0), analogAverage(1), analogAverage(2), analogAverage(3));
        //writeRadio(payload);
    }
}

void invalidPayload(char *message) {
    printf("Invalid payload for message type %s\n\r", message);
}

void setColor(int red, int green, int blue, int white) {
    //return;
    if (red < 0 || red > 255 ) {
      red = 254;
      Serial.println("Invalid red");
    }

    if (green < 0 || green > 255 ) {
      green = 254;
      Serial.println("Invalid green");
    }

    if (blue < 0 || blue > 255 ) {
      blue = 254;
      Serial.println("Invalid blue");
    }

    if (white < 0 || white > 255 ) {
      white = 254;
      Serial.println("Invalid white");
    }

    printf("Setting PWM to R:%u G:%u B:%u W:%u\n\r",red, green, blue, white);
    analogWrite(redPin, red);
    analogWrite(greenPin, green);
    analogWrite(bluePin, blue);
    return;
}

int analogAverage(int pin) { //get average value of 100 samples
    int x = 0;
    double avg = 0;
    while (x <= 50) {
        avg+=analogRead(pin);
        x++;
    }
    avg = (avg / 50);
    return (int) avg;
}