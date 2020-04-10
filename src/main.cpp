#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WiFiManager.h>

#ifndef NDEBUG
#include <GDBStub.h>
#endif

#include "Application.h"
#include <chrono>

bool ready = false;
bool state = false;

m8r::Application* _application = nullptr;

void setup()
{
    delay(500);
    Serial.begin(115200);

#ifndef NDEBUG
    gdbstub_init();
#endif    

    _application = new m8r::Application(23);
}

void loop()
{
    _application->runOneIteration();

    // if (ready) {
    //     ready = false;
    //     state = !state;
    //     digitalWrite(LED_BUILTIN, state ? HIGH : LOW);

    //     if (state) {
    //         std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    //         uint64_t t = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    //         Serial.printf("******** now = %d\n", (int) t);
    //     }
    // }
}