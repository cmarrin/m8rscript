#include "Application.h"
#include <chrono>

bool ready = false;
bool state = false;

void ICACHE_RAM_ATTR onTimerISR(){
    ready = true;
    timer1_write(5000000);
}

m8r::Application* _application = nullptr;

void setup()
{
    Serial.begin(74880);
    pinMode(LED_BUILTIN, OUTPUT);

    _application = new m8r::Application(23);

    m8r::system()->printf(m8r::ROMString("\n*** m8rscript v%d.%d - %s\n\n"), m8r::MajorVersion, m8r::MinorVersion, __TIMESTAMP__);
    m8r::system()->printf(m8r::ROMString("Free heap: %d\n"), m8r::Mallocator::shared()->freeSize());

    timer1_attachInterrupt(onTimerISR);
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
    timer1_write(5000000); //120000 us
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