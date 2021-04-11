/*
  functionlib.ino - Support functions for the project

  Copyright (C) 2021  Marius Bezuidenhout

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*********************************************************************************************\
 * Sleep aware time scheduler functions borrowed from ESPEasy
\*********************************************************************************************/

#include <functionlib.h>

inline int32_t TimeDifference(uint32_t prev, uint32_t next)
{
  return ((int32_t) (next - prev));
}

int32_t TimePassedSince(uint32_t timestamp)
{
  // Compute the number of milliSeconds passed since timestamp given.
  // Note: value can be negative if the timestamp has not yet been reached.
  return TimeDifference(timestamp, millis());
}

bool TimeReached(uint32_t timer)
{
  // Check if a certain timeout has been reached.
  const long passed = TimePassedSince(timer);
  return (passed >= 0);
}

void SetNextTimeInterval(uint32_t& timer, const uint32_t step)
{
  timer += step;
  const long passed = TimePassedSince(timer);
  if (passed < 0) { return; }   // Event has not yet happened, which is fine.
  if (static_cast<unsigned long>(passed) > step) {
    // No need to keep running behind, start again.
    timer = millis() + step;
    return;
  }
  // Try to get in sync again.
  timer = millis() + (step - passed);
}

int32_t TimePassedSinceUsec(uint32_t timestamp)
{
  return TimeDifference(timestamp, micros());
}

bool TimeReachedUsec(uint32_t timer)
{
  // Check if a certain timeout has been reached.
  const long passed = TimePassedSinceUsec(timer);
  return (passed >= 0);
}