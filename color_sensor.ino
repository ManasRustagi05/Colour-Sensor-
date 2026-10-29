#include <WiFi.h>
#include <WebServer.h>

// Wi-Fi Access Point Credentials
const char* ssid = "ESP32_Color_Sensor";
const char* password = "password123";

WebServer server(80);

// TCS3200 Pin Definitions
#define S0 22
#define S1 21
#define S2 19
#define S3 18
#define OUT 5

// Calibration values (Adjust these based on white and black surface readings)
int redMin = 20, redMax = 200;
int greenMin = 20, greenMax = 200;
int blueMin = 20, blueMax = 200;

// Read Pulse Width for selected filter
int getFilterValue(bool s2Val, bool s3Val) {
  digitalWrite(S2, s2Val);
  digitalWrite(S3, s3Val);
  // Measure pulse duration on OUT pin
  return pulseIn(OUT, LOW, 250000); 
}

// Read and map RGB values (0 - 255)
void getRGB(int &r, int &g, int &b) {
  // Red filter
  int rRaw = getFilterValue(LOW, LOW);
  r = constrain(map(rRaw, redMax, redMin, 0, 255), 0, 255);

  // Green filter
  int gRaw = getFilterValue(HIGH, HIGH);
  g = constrain(map(gRaw, greenMax, greenMin, 0, 255), 0, 255);

  // Blue filter
  int bRaw = getFilterValue(LOW, HIGH);
  b = constrain(map(bRaw, blueMax, blueMin, 0, 255), 0, 255);
}

// Serves the WebApp Interface
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP32 Color Sensor Dashboard</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; background: #121212; color: #fff; margin: 0; padding: 20px; }
        .card { background: #1e1e1e; max-width: 400px; margin: auto; padding: 20px; border-radius: 12px; box-shadow: 0 4px 10px rgba(0,0,0,0.5); }
        .color-box { width: 100%; height: 150px; border-radius: 8px; margin: 20px 0; border: 2px solid #333; transition: background 0.3s; }
        .rgb-values { font-size: 1.2rem; font-weight: bold; }
        .value { display: inline-block; width: 80px; margin: 5px; padding: 8px; border-radius: 6px; }
        .red { background: #d32f2f; }
        .green { background: #388e3c; }
        .blue { background: #1976d2; }
    </style>
</head>
<body>
    <div class="card">
        <h2>TCS3200 Live Color Sensor</h2>
        <div id="preview" class="color-box" style="background: rgb(0,0,0);"></div>
        <div class="rgb-values">
            <div class="value red">R: <span id="rVal">0</span></div>
            <div class="value green">G: <span id="gVal">0</span></div>
            <div class="value blue">B: <span id="bVal">0</span></div>
        </div>
        <p id="hexVal" style="font-family: monospace; font-size: 1.1rem;">#000000</p>
    </div>

    <script>
        function updateData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('rVal').innerText = data.r;
                    document.getElementById('gVal').innerText = data.g;
                    document.getElementById('bVal').innerText = data.b;
                    document.getElementById('hexVal').innerText = data.hex;
                    document.getElementById('preview').style.backgroundColor = `rgb(${data.r}, ${data.g}, ${data.b})`;
                })
                .catch(err => console.error(err));
        }
        setInterval(updateData, 500);
    </script>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

// Endpoint returning JSON color data
void handleData() {
  int r, g, b;
  getRGB(r, g, b);

  char hex[8];
  snprintf(hex, sizeof(hex), "#%02X%02X%02X", r, g, b);

  String json = "{\"r\":" + String(r) + ",\"g\":" + String(g) + ",\"b\":" + String(b) + ",\"hex\":\"" + String(hex) + "\"}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  // Pin Configuration
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(OUT, INPUT);

  // Set Output Frequency Scaling to 20%
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  // Start Wi-Fi Access Point
  WiFi.softAP(ssid, password);
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Web Server Routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Web Server Started!");
}

void loop() {
  server.handleClient();
}
