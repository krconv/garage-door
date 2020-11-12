#ifndef COMMON_HPP
#define COMMON_HPP

#include <stdint.h>

using pin_t = uint64_t;
using voltage_t = int;
using millis_t = unsigned long;

#ifndef D0
#define D0 16
#define D1 5
#define D2 4
#define D3 0
#define D4 2
#define D5 14
#define D6 12
#define D7 13
#define D8 15
#define D9 3
#define D10 1
#endif

enum GarageDoor
{
  EAST = 0,
  WEST = 1
};

enum DoorState
{
  NONE = 0,
  DOOR_OPEN = 1,
  DOOR_CLOSED = 2
};

#endif /* !COMMON_HPP */
