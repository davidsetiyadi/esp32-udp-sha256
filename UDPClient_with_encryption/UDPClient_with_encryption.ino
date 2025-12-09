#include "WiFi.h"
#include "AsyncUDP.h"
#include <MD5Builder.h>

const char *ssid = "Wifi";
const char *password = "pass";
#include <ArduinoJson.h>

AsyncUDP udp;
#include <SHA256.h> 
String SECRET_KEY = "my_secret_passw";
// Rolling code storage
double Lastunixtime = 0;

String sha256Hash(String input) {
  SHA256 sha;
  sha.reset();
  sha.update((const uint8_t*)input.c_str(), input.length());
  uint8_t hash[32];
  sha.finalize(hash, sizeof(hash));

  String output = "";
  for (int i = 0; i < 32; i++) {
    if (hash[i] < 16) output += "0";
    output += String(hash[i], HEX);
  }
  return output;
}

void handlePacket(String data) {

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, data);

  if (error) {
    Serial.print("JSON Error: ");
    Serial.println(error.c_str());
    return;
  }
  // Extract first object and signature
  JsonObject obj = doc[0];
  String signature = doc[1]["signature"].as<String>();

  // Re-create original message (without signature)
  String jsonPayload;
  serializeJson(doc[0], jsonPayload);

 
  String unix = doc[0]["unixtime"].as<String>();
  double unixtime = doc[0]["unixtime"].as<double>();
  Serial.println("Unix Time: " + unix );
  // -- Rolling code check --
  if (unixtime <= Lastunixtime) {
    Serial.println("⛔ Replay attack detected!");
    return;
  }

  // Compute SHA256
  String calculatedHash = sha256Hash(jsonPayload + SECRET_KEY);

  Serial.println("\n📩 Received Data:");
  Serial.println(jsonPayload);

  Serial.println("\n🔑 Signature Received:");
  Serial.println(signature);

  Serial.println("\n🧮 Calculated SHA256:");
  Serial.println(calculatedHash);
  if (signature.equalsIgnoreCase(calculatedHash)) {
    Lastunixtime = unixtime;
    Serial.println("\n✅ Signature VERIFIED!");
  } else {
    Serial.println("\n❌ INVALID SIGNATURE!");
  }

}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed");
    while (1) {
      delay(1000);
    }
  }
  // if (udp.connect(IPAddress(192, 168, 1, 255), 2002)) {
     // Listen on port 2002
  if (udp.listen(2002)) {
    Serial.println("UDP connected");
    udp.onPacket([](AsyncUDPPacket packet) {
      Serial.print("UDP Packet Type: ");
      Serial.print(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast" : "Unicast");
      Serial.print(", From: ");
      Serial.print(packet.remoteIP());
      Serial.print(":");
      Serial.print(packet.remotePort());
      Serial.print(", To: ");
      Serial.print(packet.localIP());
      Serial.print(":");
      Serial.print(packet.localPort());
      Serial.print(", Length: ");
      Serial.print(packet.length());
      Serial.print(", Data: ");
      // MD5Builder md;

      // md.begin();
      // md.add(password);
      // md.calculate();
      // String result = md.toString();

      // if (!md5.equalsIgnoreCase(result)) {
      //   Serial.println("Odd - failing MD5 on String");
      // } else {
      //   Serial.println("OK!");
      // }

       // FIX: Convert packet bytes to String properly
      String payload((char*)packet.data(), packet.length());

      Serial.println("RAW PAYLOAD:");
      //Serial.println(payload);

      // Process JSON
      handlePacket(payload);
      // handlePacket(packet.data());

      // Serial.write(packet.data(), packet.length());
      Serial.println();
      //reply to the client
      packet.printf("Got %u bytes of data", packet.length());
    });
    //Send unicast
    udp.print("Hello Server!");
  }
}

void loop() {
  delay(1000);
  //Send broadcast on port 2255
  udp.broadcastTo("Anyone here 12?", 2255);
}
