#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>

// WiFi and MQTT Configurations
#define mqtt_port 1883
#define gas_topic "gas_level"
#define water_level_topic "water_level"
#define servoTopic "servo"

const char* ssid = "home sweet home";
const char* password = "anakbaik";
const char* mqtt_server = "test.mosquitto.org";

Servo servo;

// Ultrasonic Sensor
const int trigPin = 13;
const int echoPin = 12;
#define MAX_DISTANCE 200

// MQ9 Gas Sensor
#define MQ9_PIN A0

WiFiClient wifiClient;
PubSubClient client(wifiClient);

unsigned long previousMillis = 0;
const long interval = 1000; // Interval to publish readings

// Connect to WiFi
void setup_wifi() {
    delay(10);
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

// Reconnect to MQTT Server
void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);

        if (client.connect(clientId.c_str())) {
            Serial.println("connected");
            client.subscribe(servoTopic); // Subscribe to servo topic
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

// MQTT Callback
void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived on topic: ");
    Serial.println(topic);

    if (String(topic) == servoTopic) {
        if (payload[0] == '1') {
            Serial.println("Servo: 90 degrees");
            servo.write(180);
        } else if (payload[0] == '0') {
            Serial.println("Servo: 0 degrees");
            servo.write(0);
        }
    }
}

// Function to read distance from the ultrasonic sensor
float readDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    float distance = (duration * 0.034) / 2;

    // Cap distance at MAX_DISTANCE
    if (distance > MAX_DISTANCE) {
        distance = MAX_DISTANCE;
    }

    return distance;
}

// Function to calculate distance percentage
int calculateDistancePercentage(float distance) {
    int percentage = ((MAX_DISTANCE - distance) / MAX_DISTANCE) * 100;

    // Ensure percentage is within bounds
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;

    return percentage;
}

// Read general gas level from MQ9 sensor
int readGasLevel() {
    return analogRead(MQ9_PIN);
}

// Setup Function
void setup() {
    Serial.begin(115200);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(MQ9_PIN, INPUT);

    servo.attach(0);
    servo.write(0); // Initialize servo at 0 degrees

    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

// Main Loop
void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        // Read gas level and water level
        int gasLevel = readGasLevel();
        float waterLevel = readDistance();
        int waterLevelPercentage = calculateDistancePercentage(waterLevel);

        // Publish gas level and water level
        char gasLevelString[8];
        char waterLevelString[8];

        dtostrf(gasLevel, 1, 2, gasLevelString);
        dtostrf(waterLevelPercentage, 1, 2, waterLevelString);

        client.publish(gas_topic, gasLevelString);
        client.publish(water_level_topic, waterLevelString);

        // Print values to Serial Monitor
        Serial.print("General Gas Level: ");
        Serial.println(gasLevel);
        Serial.print("Water Level Percentage: ");
        Serial.print(waterLevelPercentage);
        Serial.println(" %");
    }
}
