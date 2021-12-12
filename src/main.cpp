#include <Arduino.h>
#include "config.h"
#include "endstop.h"
#include <SerialFlash.h>
#include "AudioFileSourceSvetlyachok.h"
#include <AudioOutputI2SNoDAC.h>
#include <AudioGeneratorWAV.h>

#define AUDIO_GENERATOR AudioGeneratorWAV

bool ledIsOn = false;
bool flashReady = false;
bool fsRead = false;
bool slidesRead = false;
uint32_t flashSize = 0;
uint32_t fileReadIndex = 0;
int blockSize = 1024;
int pagesPerBlock = 4;
int pageSize = blockSize / pagesPerBlock;

unsigned long ledTime = 0;
int slideIndex = 0;
const int slidesCount = 18;

AUDIO_GENERATOR *audioGenerator;
AudioOutputI2SNoDAC *audioOutput;
uint32_t soundsAddresses[slidesCount + 1];

byte prevByte = 0xFF;

// Endstop motorLoopEndstop(MOTOR_LOOP_BTN, INPUT, 10);
// Endstop cartridgeEndstop(CARTIDGE_ENDSTOP, INPUT, 200);
//TMRpcm tmrpcm;

void toggleMotor(bool on) {

}

void onMotorLoop(uint8_t val) {
  Serial.print("Motor went around ");
  Serial.println(val);
  slideIndex++;
}

void onCartridgeTrigger(uint8_t val) {
  Serial.print("Cartridge ticked ");
  Serial.println(val);
  digitalWrite(LIGHTS_PIN, HIGH);
  slideIndex = 0;
  toggleMotor(false);
}

void setup() {
  Serial.begin(SERIAL_SPEED);

  Serial.println("---------== BEGIN ==------------- ");

  pinMode(LED_BUILTIN, OUTPUT);
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);

  audioLogger = &Serial;
  audioOutput = new AudioOutputI2SNoDAC();
  audioGenerator = new AUDIO_GENERATOR();

  //audioGenerator->SetBufferSize(blockSize);

  // motorLoopEndstop.registerOnTtriggered(onMotorLoop);
  // cartridgeEndstop.registerOnTtriggered(onCartridgeTrigger);

  // pinMode(MOTOR_PIN, OUTPUT);
  // pinMode(LIGHTS_PIN, OUTPUT);

  // digitalWrite(MOTOR_PIN, HIGH);
  // digitalWrite(LIGHTS_PIN, HIGH);
  // delay(500);
  // digitalWrite(MOTOR_PIN, LOW);
  // digitalWrite(LIGHTS_PIN, LOW);
}

uint32_t c_uint32(uint8_t *b){
  uint32_t u;
  u = b[3];
  u = (u  << 8) + b[2];
  u = (u  << 8) + b[1];
  u = (u  << 8) + b[0];
  return u;
}

uint32_t c_uint32be(uint8_t *b){
  uint32_t u;
  u = b[0];
  u = (u  << 8) + b[1];
  u = (u  << 8) + b[2];
  u = (u  << 8) + b[3];
  return u;
}

void loop() {
  //return;
  // if (ledTime < millis()) {
  //   if (ledIsOn) {
  //     digitalWrite(LED_BUILTIN, HIGH);
  //     //digitalWrite(MOTOR_PIN, HIGH);
  //     //digitalWrite(LIGHTS_PIN, HIGH);
  //   } else {
  //     digitalWrite(LED_BUILTIN, LOW);
  //     digitalWrite(MOTOR_PIN, LOW);
  //     digitalWrite(LIGHTS_PIN, LOW);
  //     Serial.println("Led off");
  //   }
  //   ledIsOn = !ledIsOn;
  //   ledTime = millis() + 3000;
  // }

  digitalWrite(LED_BUILTIN, LOW);

  if (!flashReady) {
    flashReady = SerialFlash.begin(FLASH_CS);
  } else if (audioGenerator->isRunning()) {
    if (!audioGenerator->loop()) {
      audioGenerator->stop();
    }
  } else if (!SerialFlash.ready()) {
    Serial.println("WAitin for cartridge");
  } else if (flashSize == 0) { // Read cartridge size and serial
    uint8_t id[5];
    uint8_t sn[8];         // 25%

    SerialFlash.readID(id);
    SerialFlash.readSerialNumber(sn);

    flashSize = 1UL << id[2];
    
    Serial.print("Cartrige inserted ");

    Serial.printf("ID: %02X %02X %d\n", id[0], id[1], id[2]);
    Serial.printf("Serial Number: %02X %02X %02X %02X %02X %02X %02X %02X\n", sn[0], sn[1], sn[2], sn[3], sn[4], sn[5], sn[6], sn[7]);
    Serial.printf("Capacity: %d\n", flashSize);
    Serial.println();
  } else if (fileReadIndex == 0) { // Read cartridge header
    char data[blockSize];
    uint32_t sizeI = sizeof(data);

    SerialFlash.read(fileReadIndex, data, sizeI);

    
    Serial.println("Cartridge header:");

    for (uint32_t i=0; i < sizeI; i++) {
      if (i % 16 == 0) Serial.println();

      Serial.printf("%02X ", data[i]);
      //Serial.print(data[i]);
    }

    Serial.println("\n-----------------------------------");

    fileReadIndex += sizeI;
  } else if(!slidesRead && fileReadIndex < flashSize){ // Read slides addresses
    SerialFlash.read(fileReadIndex, soundsAddresses, sizeof(soundsAddresses));

    for (uint32_t i=0; i <= slidesCount; i++) {
      Serial.printf("%d: %d\n", i, soundsAddresses[i]);
    }

    slidesRead = true;
    
  } else if (slideIndex < slidesCount && fileReadIndex < flashSize) {
    Serial.printf("Showing slide %d ", slideIndex);
    fileReadIndex = soundsAddresses[slideIndex + 1];
    uint32_t endAddress = slideIndex + 2 < slidesCount ? soundsAddresses[slideIndex + 2] : flashSize;

    Serial.printf("WAW %d: %d - %d\n", slideIndex, fileReadIndex, endAddress);

    AudioFileSourceSvetlyachok *file = new AudioFileSourceSvetlyachok(fileReadIndex, endAddress);
    audioGenerator->begin(file, audioOutput);
    delay(200);

    slideIndex++;
  }

    // motorLoopEndstop.loop();
    // cartridgeEndstop.loop();
}