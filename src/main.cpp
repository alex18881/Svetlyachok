#include <Arduino.h>
#include "config.h"
#include "endstop.h"
#include <SerialFlash.h>
//#include "TMRpcm.h"

bool ledIsOn = false;
bool flashReady = false;
bool fsRead = false;
unsigned long ledTime = 0;
int slideIndex = 0;

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

  // motorLoopEndstop.registerOnTtriggered(onMotorLoop);
  // cartridgeEndstop.registerOnTtriggered(onCartridgeTrigger);

  // // tmrpcm.speakerPin = SPEAKER_PIN;
  // // tmrpcm.setVolume(3);

  pinMode(LED_BUILTIN, OUTPUT);
  // pinMode(MOTOR_PIN, OUTPUT);
  // pinMode(LIGHTS_PIN, OUTPUT);

  // digitalWrite(MOTOR_PIN, HIGH);
  // digitalWrite(LIGHTS_PIN, HIGH);
  // delay(500);
  // digitalWrite(MOTOR_PIN, LOW);
  // digitalWrite(LIGHTS_PIN, LOW);
}

void loop() {
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
  } else if(!fsRead && SerialFlash.ready()) {
    uint8_t id[5];
    uint8_t sn[8];
    SerialFlash.readID(id);
    SerialFlash.readSerialNumber(sn);
    Serial.print("Cartrige inserted ");

    Serial.printf("ID: %02X %02X %d\n", id[0], id[1], id[2]);
    Serial.printf("Serial Number: %02X %02X %02X %02X %02X %02X %02X %02X\n", sn[0], sn[1], sn[2], sn[3], sn[4], sn[5], sn[6], sn[7]);

    SerialFlash.opendir();
    Serial.println("Reading FS");

    uint32_t sig[2];

    SerialFlash.read(0, sig, 8);

    Serial.printf("Max FS size %08X %08X\n", sig[0], sig[1]);

    char fileName[256];

    SerialFlash.read(0, fileName, sizeof(fileName));

    Serial.println(fileName);

    //uint32_t fileSize;

    // while (SerialFlash.readdir(fileName, sizeof(fileName), fileSize)) {
    //   Serial.print("/");
    //   Serial.print(fileName);
    //   Serial.printf("(%d)", fileSize);
    //   Serial.println();
    // }

    Serial.println("Reading FS done");

    fsRead = true;
  }

  // motorLoopEndstop.loop();
  // cartridgeEndstop.loop();
}