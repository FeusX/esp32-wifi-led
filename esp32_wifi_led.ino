/*
        If you want more information, visit esp32 documents on https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html
*/
#include <WiFi.h>
#include <WebServer.h>

const char *ssid = ""; // your wi-fi has to be 2.4 GHz
const char *password = "";

WebServer server(80);

byte leds[] = {4, 16, 17, 5, 18, 19, 21};
byte leds_size = sizeof(leds) / sizeof(leds[0]);
unsigned int resolution = 10;

bool fade_enabled = true;
bool sine_enabled = false;
bool chaser_enabled = false;
int delay_val = 5;
uint16_t brightness = pow(2, resolution) - 1;
uint16_t sine_step = 0;

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  unsigned long start_time = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start_time < 15000)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi.");
  Serial.println(WiFi.localIP());

  for (byte i = 0; i < leds_size; i++)
  {
    pinMode(leds[i], OUTPUT);
    ledcAttach(leds[i], 5000, resolution);
  }

  server.on("/", handleRoot);
  server.on("/speed", handleSpeed);
  server.on("/effect", handleEffect);
  server.begin();
}

void loop()
{
  server.handleClient();
  if (fade_enabled) fade();
  if (sine_enabled) sine_wave();
  if (chaser_enabled) chaser();
}

void fade()
{
  for (uint16_t j = 0; j <= brightness; j++)
  {
    for (uint8_t i = 0; i < leds_size; i++)
    {
      ledcWrite(leds[i], j);
    }
    delay(delay_val);
  }

  delay(200);

  for (uint16_t j = brightness; j > 0; j--)
  {
    for (uint8_t i = 0; i < leds_size; i++)
    {
      ledcWrite(leds[i], j);
    }
    delay(delay_val);
  }

  delay(200);
}

void sine_wave()
{
  for (byte i = 0; i < leds_size; i++)
  {
    float phase = (i - sine_step) * 0.4;
    int sine_brightness = (sin(phase) + 1.0) * (brightness / 2);
    ledcWrite(leds[i], sine_brightness);
  }
  sine_step++;
  delay(50);
}

void chaser()
{
  for (byte i = 0; i < leds_size; i++)
  {
    ledcWrite(leds[i], brightness);
    delay(delay_val);
    ledcWrite(leds[i], 0);
  }
  for (int k = leds_size - 2; k > 0; k--)
  {
    ledcWrite(leds[k], brightness);
    delay(delay_val);
    ledcWrite(leds[k], 0);
  }
}

void handleRoot()
{
  String html = "<html><body>";
  html += "<h1>ESP32 LED Control</h1>";
  html += "<p>Current Mode: <strong>";
  if (fade_enabled) html += "Fade";
  else if (sine_enabled) html += "Sine";
  else if (chaser_enabled) html += "Chaser";
  else html += "None";
  html += "</strong></p>";
  html += "<a href=\"/effect?mode=fade\"><button>Fade</button></a><br><br>";
  html += "<a href=\"/effect?mode=sine\"><button>Sine</button></a><br><br>";
  html += "<a href=\"/effect?mode=chaser\"><button>Chaser</button></a><br><br>";
  html += "<form action=\"/speed\" method=\"get\">";
  html += "Fade/Chaser Delay (1-50 ms): <input type=\"number\" name=\"value\" min=\"1\" max=\"50\">";
  html += "<input type=\"submit\" value=\"Set Speed\">";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleEffect()
{
  if (!server.hasArg("mode"))
  {
    server.send(400, "text/plain", "Missing mode parameter");
    return;
  }

  String mode = server.arg("mode");
  fade_enabled = false;
  sine_enabled = false;
  chaser_enabled = false;

  if (mode == "fade") fade_enabled = true;
  else if (mode == "sine") sine_enabled = true;
  else if (mode == "chaser") chaser_enabled = true;
  else
  {
    server.send(400, "text/plain", "Invalid mode");
    return;
  }

  server.sendHeader("Location", "/");
  server.send(303);
}

void handleSpeed()
{
  if (server.hasArg("value"))
  {
    int val = server.arg("value").toInt();
    if (val >= 1 && val <= 50)
    {
      delay_val = val;
    }
  }
  server.sendHeader("Location", "/");
  server.send(303);
}
