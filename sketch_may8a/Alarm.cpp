#include "Alarm.h"
#include <Arduino.h>

Alarm::Alarm(int rPin, int gPin, int bPin, int buzzerPin, float* distancePtr) {
  _rPin = rPin;
  _gPin = gPin;
  _bPin = bPin;
  _buzzerPin = buzzerPin;
  _distance = distancePtr;
  pinMode(_rPin, OUTPUT);
  pinMode(_gPin, OUTPUT);
  pinMode(_bPin, OUTPUT);
  pinMode(_buzzerPin, OUTPUT);
  digitalWrite(_buzzerPin, LOW);
}

void Alarm::setColourA(int r, int g, int b) {
  _rA = r; _gA = g; _bA = b;
}

void Alarm::setColourB(int r, int g, int b) {
  _rB = r; _gB = g; _bB = b;
}

void Alarm::setVariationTiming(unsigned long ms) {
  _variationRate = ms;
}

void Alarm::setDistance(float d) {
  _distanceTrigger = d;
}

void Alarm::setTimeout(unsigned long ms) {
  _timeoutDelay = ms;
}

void Alarm::turnOn() {
  _manualOn = true;
}

void Alarm::turnOff() {
  _manualOff = true;
}

void Alarm::test() {
  _testing = true;
  _testStartTime = millis();
  analogWrite(_rPin, 255);
  analogWrite(_gPin, 0);
  analogWrite(_bPin, 0);
  digitalWrite(_buzzerPin, HIGH);
}

AlarmState Alarm::getState() const {
  return _state;
}

void Alarm::update() {
  _currentTime = millis();

  if (_testing) {
    if (_currentTime - _testStartTime >= 3000) {
      analogWrite(_rPin, 0);
      analogWrite(_gPin, 0);
      analogWrite(_bPin, 0);
      digitalWrite(_buzzerPin, LOW);
      _testing = false;
      _state = OFF;
    }
    return;
  }

  if (_manualOn) {
    _state = ON;
    _lastDetectedTime = _currentTime;
    _manualOn = false;
  } else if (_manualOff) {
    _state = OFF;
    analogWrite(_rPin, 0);
    analogWrite(_gPin, 0);
    analogWrite(_bPin, 0);
    digitalWrite(_buzzerPin, LOW);
    _manualOff = false;
    return;
  }

  if (_state == OFF && *_distance <= _distanceTrigger) {
    _state = ON;
    _lastDetectedTime = _currentTime;
  } else if (_state == ON) {
    if (*_distance > _distanceTrigger) {
      if (_currentTime - _lastDetectedTime >= _timeoutDelay) {
        _state = OFF;
        analogWrite(_rPin, 0);
        analogWrite(_gPin, 0);
        analogWrite(_bPin, 0);
        digitalWrite(_buzzerPin, LOW);
        return;
      }
    } else {
      _lastDetectedTime = _currentTime;
    }

    if (_currentTime - _lastUpdate >= _variationRate) {
      _toggleColor = !_toggleColor;

      if (_toggleColor) {
        analogWrite(_rPin, _rA);
        analogWrite(_gPin, _gA);
        analogWrite(_bPin, _bA);
      } else {
        analogWrite(_rPin, _rB);
        analogWrite(_gPin, _gB);
        analogWrite(_bPin, _bB);
      }
      digitalWrite(_buzzerPin, HIGH);
      _lastUpdate = _currentTime;
    }
  }
}
