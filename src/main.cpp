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
uint32_t flashSize = 0;
uint32_t fileReadIndex = 0;
int blockSize = 1024;

unsigned long ledTime = 0;
int slideIndex = 0;

AudioGeneratorWAV *wav;
AudioFileSourceFunction *file;
AudioOutputI2SNoDAC *audioOutput;

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

void loop() {
  return;
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

  if (!flashReady) {
    flashReady = SerialFlash.begin(FLASH_CS);
  } else if (wav->isRunning()) {
      if (!wav->loop()) wav->stop();
  } else if (!SerialFlash.ready()) {
    Serial.println("WAitin for cartridge");
  } else if (flashSize == 0) {
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
  } else {
    SerialFlash.opendir();
    Serial.println("Reading FS");

    float hz = 440.f;

    uint32_t bytes_per_sec = 8000U * 1 * 16U / 8;

    file = new AudioFileSourceFunction(blockSize / bytes_per_sec);
    file->addAudioGenerators([&](const float time) {
      float v = sin(TWO_PI * hz * time);  // generate sine wave
      v *= fmod(time, 1.f);               // change linear
      v *= 0.5;                           // scale
      return v;
    });

    byte data[blockSize];
    uint32_t sizeI = sizeof(data);

    if ((fileReadIndex + sizeI) > flashSize) {
        sizeI = flashSize - fileReadIndex;
    }

    if (sizeI == 0) {
      fileReadIndex = flashSize;
      return;
    }

    SerialFlash.read(fileReadIndex, data, sizeI);

    file->read(data, sizeI);
    wav->begin(file, audioOutput);

    fileReadIndex += sizeI;
  }

    // motorLoopEndstop.loop();
    // cartridgeEndstop.loop();
}