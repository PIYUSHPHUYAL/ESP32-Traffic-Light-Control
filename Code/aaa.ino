#include <WiFi.h>
#include <WebServer.h>

// WiFi credentials
const char* ssid = "Piyush";
const char* password = "howdareyou#@123";

// GPIO Pins for traffic lights
#define RED_LED 15
#define YELLOW_LED 4
#define GREEN_LED 2

// Current state tracking
struct LightState {
  bool red;
  bool yellow;
  bool green;
} currentState = {false, false, false};

// Create WebServer object on port 80
WebServer server(80);

// HTML webpage
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Traffic Light Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            margin: 0;
            padding: 20px;
            background-color: #f0f0f0;
        }
        .container {
            max-width: 600px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            margin-bottom: 30px;
        }
        .light-container {
            display: flex;
            flex-direction: column;
            gap: 20px;
            margin-bottom: 30px;
        }
        .light {
            width: 100px;
            height: 100px;
            border-radius: 50%;
            margin: 0 auto;
            border: 5px solid #333;
            transition: all 0.3s ease;
        }
        .red { background-color: #ff000033; }
        .yellow { background-color: #ffff0033; }
        .green { background-color: #00ff0033; }
        .active.red { background-color: #ff0000; box-shadow: 0 0 20px #ff0000; }
        .active.yellow { background-color: #ffff00; box-shadow: 0 0 20px #ffff00; }
        .active.green { background-color: #00ff00; box-shadow: 0 0 20px #00ff00; }
        .controls {
            display: flex;
            gap: 10px;
            justify-content: center;
            flex-wrap: wrap;
        }
        button {
            padding: 15px 30px;
            font-size: 16px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            transition: all 0.3s ease;
            color: white;
            background-color: #444;
        }
        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 2px 8px rgba(0,0,0,0.2);
        }
        #status {
            margin-top: 20px;
            padding: 10px;
            border-radius: 5px;
            display: none;
        }
        .success { background-color: #d4edda; color: #155724; }
        .error { background-color: #f8d7da; color: #721c24; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Traffic Light Control</h1>
        
        <div class="light-container">
            <div class="light red" id="redLight"></div>
            <div class="light yellow" id="yellowLight"></div>
            <div class="light green" id="greenLight"></div>
        </div>
        
        <div class="controls">
            <button onclick="controlLight('red')">Toggle Red</button>
            <button onclick="controlLight('yellow')">Toggle Yellow</button>
            <button onclick="controlLight('green')">Toggle Green</button>
            <button onclick="controlLight('off')">All Off</button>
        </div>
        
        <div id="status"></div>
    </div>

    <script>
        let lightStates = {
            red: false,
            yellow: false,
            green: false
        };

        function showStatus(message, isError = false) {
            const status = document.getElementById('status');
            status.textContent = message;
            status.style.display = 'block';
            status.className = isError ? 'error' : 'success';
            setTimeout(() => status.style.display = 'none', 3000);
        }

        function updateLightVisual(color, state) {
            const light = document.getElementById(color + 'Light');
            if (state) {
                light.classList.add('active');
            } else {
                light.classList.remove('active');
            }
        }

        async function controlLight(color) {
            try {
                const response = await fetch(`/control?light=${color}`);
                if (!response.ok) throw new Error('Network response was not ok');
                
                const result = await response.json();
                
                if (color === 'off') {
                    Object.keys(lightStates).forEach(key => {
                        lightStates[key] = false;
                        updateLightVisual(key, false);
                    });
                } else {
                    lightStates[color] = result[color];
                    updateLightVisual(color, result[color]);
                }
                
                showStatus('Light control successful');
            } catch (error) {
                console.error('Error:', error);
                showStatus('Failed to control light', true);
            }
        }

        // Initial state request
        fetch('/state')
            .then(response => response.json())
            .then(state => {
                Object.keys(state).forEach(color => {
                    lightStates[color] = state[color];
                    updateLightVisual(color, state[color]);
                });
            })
            .catch(error => console.error('Error fetching initial state:', error));
    </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleState() {
  String json = "{\"red\":" + String(currentState.red ? "true" : "false") + 
                ",\"yellow\":" + String(currentState.yellow ? "true" : "false") + 
                ",\"green\":" + String(currentState.green ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void handleControl() {
  String response = "{";
  
  if (server.hasArg("light")) {
    String light = server.arg("light");
    
    if (light == "off") {
      // Turn off all lights
      digitalWrite(RED_LED, LOW);
      digitalWrite(YELLOW_LED, LOW);
      digitalWrite(GREEN_LED, LOW);
      currentState = {false, false, false};
      response = "{\"red\":false,\"yellow\":false,\"green\":false}";
    } else {
      // Toggle the requested light
      if (light == "red") {
        currentState.red = !currentState.red;
        digitalWrite(RED_LED, currentState.red);
        response += "\"red\":" + String(currentState.red ? "true" : "false");
      } else if (light == "yellow") {
        currentState.yellow = !currentState.yellow;
        digitalWrite(YELLOW_LED, currentState.yellow);
        response += "\"yellow\":" + String(currentState.yellow ? "true" : "false");
      } else if (light == "green") {
        currentState.green = !currentState.green;
        digitalWrite(GREEN_LED, currentState.green);
        response += "\"green\":" + String(currentState.green ? "true" : "false");
      }
    }
  }
  
  response += "}";
  server.send(200, "application/json", response);
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Set GPIO pins as outputs and initialize to OFF
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/state", HTTP_GET, handleState);
  server.on("/control", HTTP_GET, handleControl);

  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();  // Handle client requests
  delay(2);  // Small delay to prevent watchdog timer issues
}