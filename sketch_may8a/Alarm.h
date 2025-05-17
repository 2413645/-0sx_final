#pragma once

enum AlarmState {
  OFF,
  WATCHING,
  ON,
  TESTING
};

class Alarm {
public:
  Alarm(int rPin, int gPin, int bPin, int buzzerPin, float* distancePtr);
  void update();
  void setColourA(int r, int g, int b);
  void setColourB(int r, int g, int b);
  void setVariationTiming(unsigned long ms);
  void setDistance(float d);
  void setTimeout(unsigned long ms);
  void turnOff();
  void turnOn();
  void test();
  AlarmState getState() const;

private:
  int _rPin, _gPin, _bPin, _buzzerPin;
  float* _distance;
  AlarmState _state = OFF;

  
  int _rB, _gB, _bB;
  int _rA, _gA, _bA;
  bool _manualOn = false;
  bool _manualOff = false;
  bool _toggleColor = false;
  bool _testing = false;

  unsigned long _currentTime = 0;
  unsigned long _variationRate = 500;
  unsigned long _timeoutDelay = 3000;
  unsigned long _lastUpdate = 0;
  unsigned long _lastDetectedTime = 0;
  unsigned long _testStartTime = 0;
  float _distanceTrigger = 10.0;
};
