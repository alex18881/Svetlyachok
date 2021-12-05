#include <Arduino.h>
#include "config.h"
#include "endstop.h"
#include <SerialFlash.h>
#include "AudioFileSourceFunction.h"
#include "AudioOutputI2SNoDAC.h"
#include "AudioGeneratorWAV.h"

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

AudioGeneratorWAV *wav;
AudioFileSourceFunction *file;
AudioOutputI2SNoDAC *audioOutput;
uint16_t channels = 1U;
uint32_t sample_per_sec = 4410U;
uint32_t bits_per_sample = 8;
uint32_t bytes_per_sec = sample_per_sec * channels * bits_per_sample / 8;
uint32_t soundsAddresses[slidesCount + 1];

// Endstop motorLoopEndstop(MOTOR_LOOP_BTN, INPUT, 10);
// Endstop cartridgeEndstop(CARTIDGE_ENDSTOP, INPUT, 200);
//TMRpcm tmrpcm;

float sine_wave(const float time) {
  char data[1];

  SerialFlash.read(fileReadIndex++, data, 1);

  return (float)data[0] / 1024.f;
}

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

  audioOutput = new AudioOutputI2SNoDAC(SPEAKER_PIN);
  wav = new AudioGeneratorWAV();

  wav->SetBufferSize(blockSize);

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
  } else if (wav->isRunning()) {
    if (!wav->loop()) wav->stop();
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
    uint32_t sizeI = endAddress - fileReadIndex;

    if ((fileReadIndex + sizeI) > flashSize) {
      sizeI = flashSize - fileReadIndex;
    }

    if (sizeI == 0 || fileReadIndex >= flashSize) {
      fileReadIndex = flashSize;
      return;
    }

    float seconds = (float)sizeI / (float)bytes_per_sec;
    Serial.printf("WAW %db, %fs, %dsamp/s, %db/s\n", sizeI, seconds, sample_per_sec, bits_per_sample);

    file = new AudioFileSourceFunction(seconds, channels, sample_per_sec, bits_per_sample);
    file->addAudioGenerators(sine_wave);
    wav->begin(file, audioOutput);

    slideIndex++;
  }

    // motorLoopEndstop.loop();
    // cartridgeEndstop.loop();
}