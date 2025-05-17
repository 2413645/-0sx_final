#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>
#include <LedControl.h>
#include "Alarm.h"
#include "ViseurAutomatique.h"
#include <WiFiEspAT.h>
#include <PubSubClient.h>

#if HAS_SECRETS
#include "arduino_secrets.h"
const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;
#else
const char ssid[] = "TechniquesInformatique-Etudiant";
const char pass[] = "shawi123";
#endif

WiFiClient espClient;
PubSubClient Client(espClient);
char mqttTopic[] = "etd/36/data";

unsigned long lastMqttSend1 = 0;
unsigned long lastMqttSend2 = 0;
const unsigned long MQTT_SEND_INTERVAL1 = 2500;
const unsigned long MQTT_SEND_INTERVAL2 = 1100;
unsigned long lastMqttAttempt = 0;

const char* NUM_ETUDIANT = "2413645";
LiquidCrystal_I2C lcd(0x27, 16, 2);
unsigned long lastLcdUpdate = 0;
const unsigned long lcdUpdateInterval = 100;

int potentiometrePin = A0;

LedControl matrix = LedControl(30, 32, 34, 1);
unsigned long symboleExpire = 0;
bool symboleActif = false;
byte* symboleActuel = nullptr;

byte symboleCheck[8] = {
  B00000000, B00000001, B00000010, B00000100,
  B10001000, B01010000, B00100000, B00000000
};

byte symboleX[8] = {
  B10000001, B01000010, B00100100, B00011000,
  B00011000, B00100100, B01000010, B10000001
};

byte symboleInterdit[8] = {
  B00111100, B01000010, B10100001, B10010001,
  B10001001, B10000101, B01000010, B00111100
};

const int trigPin = 6;
const int echoPin = 7;
float distance = 0;
unsigned long lastDistanceRead = 0;
const unsigned long distanceInterval = 50;

unsigned long lastSerialTime = 0;
const unsigned long serialInterval = 100;

Alarm alarme(2, 5, 3, 4, &distance);
ViseurAutomatique viseur(31, 33, 35, 37, distance);
unsigned long _currentTime = 0;

int limiteAlarme = 15;
int limInf = 30;
int limSup = 60;

String ligne1;
String ligne2;

void initialiserLCD() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(NUM_ETUDIANT);
  lcd.setCursor(0, 1);
  lcd.print("Labo final");
  delay(2000);
  lcd.clear();
}

void initialiserMatrix() {
  matrix.shutdown(0, false);
  matrix.setIntensity(0, 5);
  matrix.clearDisplay(0);
}

void afficherSymbole(byte symbole[8]) {
  symboleActif = true;
  symboleActuel = symbole;
  symboleExpire = millis() + 3000;
  for (int i = 0; i < 8; i++) matrix.setRow(0, i, symbole[i]);
}

void tacheEffacerSymbole() {
  if (symboleActif && millis() > symboleExpire) {
    for (int i = 0; i < 8; i++) matrix.setRow(0, i, 0);
    symboleActif = false;
  }
}

long lireDistanceCM() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duree = pulseIn(echoPin, HIGH, 30000);
  long d = duree * 0.034 / 2;
  if (d < 2 || d > 400) return distance;
  return d;
}

void tacheLireDistance() {
  if (millis() - lastDistanceRead >= distanceInterval) {
    distance = lireDistanceCM();
    lastDistanceRead = millis();
  }
}

void tacheAffichageLCD() {
  if (millis() - lastLcdUpdate >= lcdUpdateInterval) {

    ligne1 = "Dist: " + (String)distance + " cm  ";
    ligne2 = "Obj :";

    if (distance < viseur.getDistanceMinSuivi()) {
      ligne2 += "Trop pret";
    } else if (distance < viseur.getDistanceMaxSuivi()) {

      int angle = (int)viseur.getAngle();
      ligne2 += (String)angle + " deg";
    } else {
      ligne2 += "Trop loin";
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(ligne1);
    lcd.setCursor(0, 1);
    lcd.print(ligne2);


    lastLcdUpdate = millis();
  }
}

void tacheCommande() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    bool cmdOK = false;
    bool erreurLimites = false;

    if (cmd.equalsIgnoreCase("g_dist")) {
      long mesure = lireDistanceCM();
      Serial.println(mesure);
      afficherSymbole(symboleCheck);
      return;
    } else if (cmd.startsWith("cfg;alm;")) {
      int val = cmd.substring(8).toInt();
      if (val > 0) {
        alarme.setDistance(val);
        cmdOK = true;
      }
    } else if (cmd.startsWith("cfg;lim_inf;")) {
      int val = cmd.substring(12).toInt();
      if (val < limSup) {
        viseur.setDistanceMinSuivi(val);
        cmdOK = true;
      } else erreurLimites = true;
    } else if (cmd.startsWith("cfg;lim_sup;")) {
      int val = cmd.substring(12).toInt();
      if (val > limInf) {
        viseur.setDistanceMaxSuivi(val);
        cmdOK = true;
      } else erreurLimites = true;
    }

    if (erreurLimites) afficherSymbole(symboleInterdit);
    else if (!cmdOK) afficherSymbole(symboleX);
    else afficherSymbole(symboleCheck);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  initialiserLCD();
  initialiserMatrix();



  alarme.setColourA(0, 0, 255);
  alarme.setColourB(255, 0, 0);
  alarme.setVariationTiming(250);
  alarme.setTimeout(3000);
  alarme.setDistance(15);

  viseur.setAngleMin(10);
  viseur.setAngleMax(170);
  viseur.setPasParTour(2048);
  viseur.setDistanceMinSuivi(30);
  viseur.setDistanceMaxSuivi(60);
  viseur.activer();

  Serial1.begin(115200);
  WiFi.init(Serial1);
  WiFi.setPersistent();
  WiFi.endAP();
  WiFi.disconnect();

  Serial.print("Connexion au WiFi ");
  Serial.println(ssid);
  int status = WiFi.begin(ssid, pass);

  if (status == WL_CONNECTED) {
    Serial.println("WiFi connecté !");
    IPAddress ip = WiFi.localIP();
    Serial.print("Adresse IP : ");
    Serial.println(ip);
  } else {
    Serial.println("Connexion WiFi échouée.");
  }

  Client.setServer("arduino.nb.shawinigan.info", 1883);
  Client.setCallback(mqttEvent);
}

void loop() {
  _currentTime = millis();

  tacheLireDistance();
  viseur.update();
  alarme.update();
  tacheAffichageLCD();
  tacheCommande();
  tacheEffacerSymbole();
  if (!Client.connected()) reconnectMQTT();
  Client.loop();

  if (millis() - lastMqttSend1 > MQTT_SEND_INTERVAL1) {
    sendMQTTData1();
    lastMqttSend1 = millis();
  }
  if (millis() - lastMqttSend2 > MQTT_SEND_INTERVAL2) {
    sendMQTTData2();
    lastMqttSend2 = millis();
  }
}

void reconnectMQTT() {
  if (millis() - lastMqttAttempt < 5000) return;

  lastMqttAttempt = millis();

  if (!Client.connected()) {
    Serial.print("Connexion MQTT...");

    // 🔑 Utilise un nom de client unique
    if (Client.connect("arduinoClient36", "etdshawi", "shawi123")) {
      Serial.println("Connecté");
      Client.subscribe("etd/36/#");  // ou tout autre sujet utile
    } else {
      Serial.print("Échec : ");
      Serial.println(Client.state());
    }
  }
}


void sendMQTTData1() {
  if (!Client.connected()) {
    Serial.println("MQTT non connecté, envoi annulé.");
    return;
  }

  static char message[200];
  char distStr[8], angleStr[8];
  char szPot[8];
  int motor = (viseur.getAngle() > 10) ? 1 : 0;
 int potValue = map(analogRead(potentiometrePin), 0, 1023,0, 100);
  dtostrf(distance, 4, 1, distStr);            // ex: "42.3"
  dtostrf(viseur.getAngle(), 4, 1, angleStr);  // ex: "50.5"
  dtostrf(potValue, 4, 1, szPot);

  sprintf(message,
          "{\"number\":\"2413645\",\"name\":\"Audry noupoue\", \"uptime\":%lu, \"dist\":%s, \"angle\":%s,  \"pot\":%s}",
          millis() / 1000, distStr, angleStr, szPot);

  Serial.print("Message envoyé : ");
  Serial.println(message);

  Client.publish("etd/36/data", message);  // assure-toi que le topic est correct
}

void sendMQTTData2() {
  if (!Client.connected()) {
    Serial.println("MQTT non connecté, envoi annulé.");
    return;
  }

  static char message[128];

  static char line1Str[17];
  static char line2Str[17];

  ligne1.toCharArray(line1Str, sizeof(line1Str));
  ligne2.toCharArray(line2Str, sizeof(line2Str));

  // Former le message JSON uniquement avec line1 et line2
  snprintf(message, sizeof(message),
           "{\"line1\":\"%s\", \"line2\":\"%s\"}", line1Str, line2Str);

  Serial.print("Message LCD envoyé : ");
  Serial.println(message);

  Client.publish("etd/36/data", message);
}

void mqttEvent(char* topic, byte* payload, unsigned int length) {

  const char* pretopic = "etd/36/";  // Configurer pour vos besoins
  const int pretopicLen = 7;         // Configurer pour vos besoins

  Serial.print("Message recu [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strncmp(topic, pretopic, pretopicLen) != 0) {
    Serial.print("Sujet non reconnu!");
    return;
  }

  // Décale le pointeur pour ignorer "prof/"
  char* specificTopic = topic + pretopicLen;

  topicManagement(specificTopic, payload, length);
}

void topicManagement(char* topic, byte* payload, unsigned int length) {
  // Assumer que le topic est déjà trimmé à "prof/"

  if (strcmp(topic, "color") == 0) {

    char* position = strchr((char*)payload, '#');
    char colorStr[7];

    strncpy(colorStr, position + 1, 6);
    colorStr[6] = '\0';

    SetLedColour(colorStr);

    return;
  }

  else if (strcmp(topic, "motor") == 0) {

    char* position = strchr((char*)payload, ':');

    bool motor = (position[1] == '1');

    if (motor) {
      viseur.activer();
    } else {
      viseur.desactiver();
    }

    return;
  }
}

void SetLedColour(const char* hexColor) {
  // Assurez-vous que la chaîne hexColor commence par '#' et a une longueur de 7 caractères (ex: #FF5733)
  if (strlen(hexColor) == 6) {
    // Extraction des valeurs hexadécimales pour rouge, vert et bleu
    long number = strtol(hexColor, NULL, 16);  // Convertit hex à long

    int red = number >> 16;            // Décale de 16 bits pour obtenir le rouge
    int green = (number >> 8) & 0xFF;  // Décale de 8 bits et masque pour obtenir le vert
    int blue = number & 0xFF;          // Masque pour obtenir le bleu

    // Définissez les couleurs sur les broches de la DEL
    alarme.setColourB(red, green, blue);

    Serial.print("Couleur : ");
    Serial.print(red);
    Serial.print(", ");
    Serial.print(green);
    Serial.print(", ");
    Serial.println(blue);
  } else {
    // Gestion d'erreur si le format n'est pas correct
    Serial.println("Erreur: Format de couleur non valide.");
  }
}

