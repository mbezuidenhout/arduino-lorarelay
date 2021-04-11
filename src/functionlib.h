#include <Arduino.h>

bool TimeReached(uint32_t timer);
void SetNextTimeInterval(uint32_t& timer, const uint32_t step);