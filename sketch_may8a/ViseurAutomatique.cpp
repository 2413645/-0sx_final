#include "ViseurAutomatique.h"
#include <Arduino.h>

ViseurAutomatique::ViseurAutomatique(int p1, int p2, int p3, int p4, float& distanceRef)
  : _stepper(AccelStepper::FULL4WIRE, p1, p3, p2, p4), _distance(distanceRef) {
  _stepper.setMaxSpeed(500);
  _stepper.setAcceleration(100);
}

void ViseurAutomatique::update() {
  _currentTime = millis();
  static bool firstTime = true;

  if (_etat == INACTIF) return;

  if (_distance >= _distanceMinSuivi && _distance <= _distanceMaxSuivi) {
    _etat = SUIVI;
  } else {
    _etat = REPOS;
    firstTime = true;
  }

  if (_etat == SUIVI) {

    if(firstTime){
      _stepper.disableOutputs();
      firstTime = false;
    }

    float distanceLim = constrain(_distance, _distanceMinSuivi, _distanceMaxSuivi);
    _angle = map(distanceLim, _distanceMinSuivi, _distanceMaxSuivi, _angleMin, _angleMax);
    
    if(_stepper.distanceToGo() == 0){
      _stepper.moveTo(_angleEnSteps(_angle));
    }
  }

  else if(_etat == REPOS){

    if(_stepper.distanceToGo() == 0){
      _stepper.disableOutputs();
    }
    _angle = (_distance > _distanceMinSuivi) ? _angleMax : _angleMin;
  }

  _stepper.run();
  
}

void ViseurAutomatique::setAngleMin(float angle) {
  _angleMin = angle;
}

void ViseurAutomatique::setAngleMax(float angle) {
  _angleMax = angle;
}

void ViseurAutomatique::setPasParTour(int steps) {
  _stepsPerRev = steps;
}

void ViseurAutomatique::setDistanceMinSuivi(float distanceMin) {
  _distanceMinSuivi = distanceMin;
}

void ViseurAutomatique::setDistanceMaxSuivi(float distanceMax) {
  _distanceMaxSuivi = distanceMax;
}

float ViseurAutomatique::getDistanceMinSuivi() {
  return _distanceMinSuivi;
}

float ViseurAutomatique::getDistanceMaxSuivi(){
  return _distanceMaxSuivi;
};

float ViseurAutomatique::getAngle() const {
  // long pos = _stepper.currentPosition();
  // return map(pos, _angleEnSteps(_angleMin), _angleEnSteps(_angleMax), _angleMin, _angleMax);
  return _angle;

}

void ViseurAutomatique::activer() {
  _etat = REPOS;
  _stepper.enableOutputs();
}

void ViseurAutomatique::desactiver() {
  _etat = INACTIF;
  _stepper.disableOutputs();
}

const char* ViseurAutomatique::getEtatTexte() const {
  switch (_etat) {
    case INACTIF: return "INACTIF";
    case SUIVI: return "SUIVI";
    case REPOS: return "REPOS";
  }
  return "";
}

long ViseurAutomatique::_angleEnSteps(float angle) const {
  return (long)(angle * (_stepsPerRev / _fullAngle));
}
