#include <functional>
#include <map>
#include <string>
#include <utility>

#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "common.hpp"
#include "config.hpp"

class DoorSensor
{
public:
  DoorSensor(GarageDoor garageDoor_)
      : garageDoor(garageDoor_) {}

  void setup()
  {
    pinMode(Configuration::getDoorSensorPin(garageDoor), INPUT);
  }

  void loop()
  {
    voltage_t value = digitalRead(Configuration::getDoorSensorPin(garageDoor));
    switch (value)
    {
    case Configuration::getDoorSensorVoltageOpen():
      currentState = DOOR_OPEN;
      break;
    case Configuration::getDoorSensorVoltageClosed():
      currentState = DOOR_CLOSED;
      break;
    default:
      currentState = NONE;
      break;
    }
  }

  DoorState getState() const
  {
    return currentState;
  }

private:
  const GarageDoor garageDoor;
  DoorState currentState = NONE;
};

class WifiClient
{
public:
  WifiClient()
      : client() {}

  void setup()
  {
    ensureConnected();
    client.status();
  }

  void loop()
  {
    ensureConnected();
  }

  WiFiClient &getClient()
  {
    return client;
  }

private:
  void ensureConnected()
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(Configuration::getWifiSsid().c_str(),
               Configuration::getWifiPassword().c_str());
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(50);
    }
  }

private:
  WiFiClient client;
};

class MqttClient
{
public:
  using MqttMessageHandler = std::function<void(const std::string &)>;

  MqttClient(WifiClient &wifiClient_)
      : wifiClient(wifiClient_), client()
  {
    instance = this;
  }

  void setup()
  {
    client.setServer(Configuration::getMqttHost().c_str(),
                     Configuration::getMqttPort());
    client.setClient(wifiClient.getClient());
    client.setCallback(&MqttClient::callback);

    ensureConnected();
  }

  void loop()
  {
    ensureConnected();
    client.loop();
  }

  bool publish(const std::string &topic, const std::string &payload)
  {
    return client.publish(topic.c_str(), payload.c_str(), true);
  }

  void subscribe(const std::string &topic,
                 MqttMessageHandler handler)
  {
    subscriptions.insert({topic, handler});
    client.subscribe(topic.c_str());
  }

private:
  void ensureConnected()
  {
    bool wasConnected = client.connected();
    while (!client.connected())
    {
      client.connect("",
                     Configuration::getMqttUser().c_str(),
                     Configuration::getMqttPassword().c_str(),
                     Configuration::getMqttStatusTopic().c_str(),
                     MQTTQOS0,
                     true,
                     Configuration::getMqttPayloadOffline().c_str(),
                     true);
      delay(50);
    }
    if (!wasConnected)
    {
      publish(Configuration::getMqttStatusTopic(),
              Configuration::getMqttPayloadOnline());

      for (const auto &subscription : subscriptions)
      {
        client.subscribe(subscription.first.c_str());
      }
    }
  }

  static void callback(char *topic, byte *payload, unsigned int length)
  {
    instance->handleMessage(topic, payload, length);
  }

  void handleMessage(char *topic_c, byte *payload_c, unsigned int length)
  {
    std::string topic(topic_c);
    std::string payload(reinterpret_cast<char *>(payload_c), length);
    for (const auto &subscription : subscriptions)
    {
      if (subscription.first == topic)
      {
        subscription.second(payload);
      }
    }
  }

private:
  PubSubClient client;
  WifiClient wifiClient;
  std::multimap<std::string, MqttMessageHandler> subscriptions;

  static MqttClient *instance;
};
MqttClient *MqttClient::instance = nullptr;

class DoorStatePublisher
{
public:
  DoorStatePublisher(GarageDoor garageDoor_,
                     const DoorSensor &doorSensor_,
                     MqttClient &mqttClient_)
      : garageDoor(garageDoor_),
        doorSensor(doorSensor_),
        mqttClient(mqttClient_) {}

  void setup()
  {
    pinMode(Configuration::getDoorSensorStatusLedPin(garageDoor), OUTPUT);
  }

  void loop()
  {
    if (isInCooldown())
    {
      return;
    }

    DoorState currentState = doorSensor.getState();
    if (currentState != lastKnownState)
    {
      updateStatusLed(currentState);
      bool successful = publishState(currentState);

      if (successful)
      {
        lastKnownState = currentState;
      }
    }
    else if (shouldRepublishState())
    {
      publishState(currentState);
    }
  }

private:
  bool isInCooldown()
  {
    return millis() < lastPublishMillis +
                          Configuration::getMqttStatePublishCooldownMillis();
  }

  void updateStatusLed(DoorState doorState)
  {
    voltage_t ledOutput;
    switch (doorState)
    {
    case DOOR_OPEN:
      ledOutput = Configuration::getDoorSensorStatusLedVoltageOff();
      break;
    case DOOR_CLOSED:
      ledOutput = Configuration::getDoorSensorStatusLedVoltageOn();
      break;
    }

    digitalWrite(Configuration::getDoorSensorStatusLedPin(garageDoor),
                 ledOutput);
  }

  bool publishState(DoorState doorState)
  {
    std::string payload;
    switch (doorState)
    {
    case DOOR_OPEN:
      payload = Configuration::getMqttPayloadOpen();
      break;
    case DOOR_CLOSED:
      payload = Configuration::getMqttPayloadClosed();
      break;
    }

    bool successful = mqttClient
                          .publish(Configuration::getMqttStateTopic(garageDoor),
                                   payload);
    if (successful)
    {
      lastPublishMillis = millis();
    }
    return successful;
  }

  bool
  shouldRepublishState()
  {
    return millis() > lastPublishMillis +
                          Configuration::getMqttStateRepublishMillis();
  }

private:
  const GarageDoor garageDoor;
  const DoorSensor &doorSensor;
  MqttClient &mqttClient;

  DoorState lastKnownState = NONE;
  millis_t lastPublishMillis = 0;
};

class DoorOpener
{
public:
  DoorOpener(GarageDoor garageDoor_,
             const DoorSensor &doorSensor_,
             MqttClient &mqttClient_)
      : garageDoor(garageDoor_),
        doorSensor(doorSensor_),
        mqttClient(mqttClient_) {}

  void setup()
  {
    pinMode(Configuration::getDoorOpenerPin(garageDoor), OUTPUT);

    mqttClient.subscribe(Configuration::getMqttSetStateTopic(garageDoor),
                         std::bind(&DoorOpener::handleMessage,
                                   this,
                                   std::placeholders::_1));
  }

  void loop()
  {

    if (isTriggeringDoor)
    {
      if (shouldStopTriggeringDoor())
      {
        stopTriggeringDoor();
      }
      return;
    }
    else if (isInCooldown())
    {
      return;
    }

    if (nextRequestedState != NONE &&
        nextRequestedState != doorSensor.getState())
    {
      startTriggeringDoor();
      nextRequestedState = NONE;
    }
  }

private:
  bool shouldStopTriggeringDoor()
  {
    return millis() > lastTriggeredMillis +
                          Configuration::getDoorOpenerTriggerMillis();
  }

  void startTriggeringDoor()
  {
    digitalWrite(Configuration::getDoorOpenerPin(garageDoor),
                 Configuration::getDoorOpenerVoltageOn());
    isTriggeringDoor = true;
    lastTriggeredMillis = millis();
  }

  void stopTriggeringDoor()
  {
    digitalWrite(Configuration::getDoorOpenerPin(garageDoor),
                 Configuration::getDoorOpenerVoltageOff());
    isTriggeringDoor = false;
  }

  bool isInCooldown()
  {
    return millis() < lastTriggeredMillis +
                          Configuration::getDoorOpenerTriggerCooldownMillis();
  }

  void handleMessage(const std::string &payload)
  {
    if (payload == Configuration::getMqttPayloadOpen())
    {
      nextRequestedState = DOOR_OPEN;
    }
    else if (payload == Configuration::getMqttPayloadClosed())
    {
      nextRequestedState = DOOR_CLOSED;
    }
  }

private:
  const GarageDoor garageDoor;
  const DoorSensor &doorSensor;
  MqttClient &mqttClient;

  bool isTriggeringDoor = false;
  unsigned long lastTriggeredMillis = 0;
  DoorState nextRequestedState = NONE;
};

class OtaUpdater
{
public:
  OtaUpdater() {}

  void setup()
  {
    ArduinoOTA.setHostname("garage-door-controller");
    ArduinoOTA.begin(true);
  }

  void loop()
  {
    ArduinoOTA.handle();
  }
};

struct _State
{
  WifiClient wifiClient;
  OtaUpdater otaUpdater;

  MqttClient mqttClient;

  DoorSensor eastDoorSensor;
  DoorStatePublisher eastDoorPublisher;
  DoorOpener eastDoorOpener;

  DoorSensor westDoorSensor;
  DoorStatePublisher westDoorPublisher;
  DoorOpener westDoorOpener;

  _State()
      : wifiClient(),
        otaUpdater(),
        mqttClient(wifiClient),
        eastDoorSensor(EAST),
        eastDoorPublisher(EAST, eastDoorSensor, mqttClient),
        eastDoorOpener(EAST, eastDoorSensor, mqttClient),
        westDoorSensor(WEST),
        westDoorPublisher(WEST, westDoorSensor, mqttClient),
        westDoorOpener(WEST, westDoorSensor, mqttClient) {}
} State;

void setup()
{
  State.wifiClient.setup();
  State.otaUpdater.setup();
  State.mqttClient.setup();

  State.eastDoorSensor.setup();
  State.eastDoorPublisher.setup();
  State.eastDoorOpener.setup();

  State.westDoorSensor.setup();
  State.westDoorPublisher.setup();
  State.westDoorOpener.setup();
}

void loop()
{
  State.wifiClient.loop();
  State.otaUpdater.loop();
  State.mqttClient.loop();

  State.eastDoorSensor.loop();
  State.eastDoorPublisher.loop();
  State.eastDoorOpener.loop();

  State.westDoorSensor.loop();
  State.westDoorPublisher.loop();
  State.westDoorOpener.loop();

  delay(25);
}
