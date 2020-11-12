#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <Arduino.h>
#include <string>

#include "common.hpp";

class Configuration
{
public:
  static std::string getWifiSsid()
  {
    return "WiFiSSID";
  }

  static std::string getWifiPassword()
  {
    return "WiFiPassword";
  }

  static std::string getMqttHost()
  {
    return "mqtt.org";
  }

  constexpr static uint16_t getMqttPort()
  {
    return 1883;
  }

  static std::string getMqttUser()
  {
    return "mqttUser";
  }

  static std::string getMqttPassword()
  {
    return "mqttPassword";
  }

  static pin_t getDoorSensorPin(GarageDoor garageDoor)
  {
    switch (garageDoor)
    {
    case EAST:
      return D1;
    case WEST:
      return D5;
    }
  }

  constexpr static voltage_t getDoorSensorVoltageOpen()
  {
    return HIGH;
  }

  constexpr static voltage_t getDoorSensorVoltageClosed()
  {
    return LOW;
  }

  static pin_t getDoorSensorStatusLedPin(GarageDoor garageDoor)
  {
    switch (garageDoor)
    {
    case EAST:
      return D0;
    case WEST:
      return D4;
    }
  }

  constexpr static voltage_t getDoorSensorStatusLedVoltageOn()
  {
    return LOW;
  }

  constexpr static voltage_t getDoorSensorStatusLedVoltageOff()
  {
    return HIGH;
  }

  static pin_t getDoorOpenerPin(GarageDoor garageDoor)
  {
    switch (garageDoor)
    {
    case EAST:
      return D2;
    case WEST:
      return D6;
    }
  }

  constexpr static voltage_t getDoorOpenerVoltageOn()
  {
    return HIGH;
  }

  constexpr static voltage_t getDoorOpenerVoltageOff()
  {
    return LOW;
  }

  constexpr static millis_t getDoorOpenerTriggerMillis()
  {
    return 1500;
  }

  constexpr static millis_t getDoorOpenerTriggerCooldownMillis()
  {
    return 5000;
  }

  static std::string getMqttStateTopic(GarageDoor garageDoor)
  {
    switch (garageDoor)
    {
    case EAST:
      return "garage-door/east/state";
    case WEST:
      return "garage-door/west/state";
    }
  }

  constexpr static millis_t getMqttStatePublishCooldownMillis()
  {
    return 5000;
  }

  constexpr static millis_t getMqttStateRepublishMillis()
  {
    return 30000;
  }

  static std::string getMqttSetStateTopic(GarageDoor garageDoor)
  {
    switch (garageDoor)
    {
    case EAST:
      return "garage-door/east/state/set";
    case WEST:
      return "garage-door/west/state/set";
    }
  }

  static std::string getMqttPayloadOpen()
  {
    return "open";
  }

  static std::string getMqttPayloadClosed()
  {
    return "closed";
  }

  static std::string getMqttPayloadOnline()
  {
    return "online";
  }

  static std::string getMqttPayloadOffline()
  {
    return "offline";
  }

  static std::string getMqttStatusTopic()
  {
    return "garage-door/status";
  }

private:
  Configuration() {}
};

#endif // !CONFIG_HPP
