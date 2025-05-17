
#pragma once

#include <AccelStepper.h>

enum EtatViseur {
  INACTIF,
  SUIVI,
  REPOS
};

class ViseurAutomatique {
public:
  ViseurAutomatique(int p1, int p2, int p3, int p4, float& distanceRef);
  void update();
  void setAngleMin(float angle);
  void setAngleMax(float angle);
  void setPasParTour(int steps);
  void setDistanceMinSuivi(float distanceMin);
  void setDistanceMaxSuivi(float distanceMax);
  float getDistanceMinSuivi();
  float getDistanceMaxSuivi();
  float getAngle() const;
  void activer();
  void desactiver();
  const char* getEtatTexte() const;

private:
  AccelStepper _stepper;
  unsigned long _currentTime = 0;
  float _angle;
  float& _distance;
  float _fullAngle = 360;
  float _angleMin = 0.0;
  float _angleMax = 180.0;
  int _stepsPerRev = 2048;
  float _distanceMinSuivi = 30.0;
  float _distanceMaxSuivi = 60.0;
  EtatViseur _etat = INACTIF;

  void _inactifState(unsigned long cT);
  void _suiviState(unsigned long cT);
  void _reposState(unsigned long cT);
  long _angleEnSteps(float angle) const;
};
