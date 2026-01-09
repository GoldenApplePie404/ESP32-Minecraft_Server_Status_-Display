// --- 库 ---
#include <Arduino.h>
#include <WiFi.h>        
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> 
#include <Preferences.h> 

// --- 配置 ---
Preferences preferences;

// --- 默认配置值 (如果 Preferences 中没有值，则使用这些默认值) ---
const char* DEFAULT_SSID = "your_ssid"; // 替换为你的默认 WiFi 名称
const char* DEFAULT_PASSWORD = "your_password"; // 替换为你的默认 WiFi 密码
const char* DEFAULT_API_KEY = "your_api_key"; // 默认 API 密钥
const char* DEFAULT_SERVER_IP = "server_ip"; // 默认服务器 IP
const char* DEFAULT_REMOTE_SERVICES_BASE_URL = "mcsmanager_url"; // 默认 MCSManager 面板基础 URL
const char* DEFAULT_MC_SERVER_API_URL_BASE = "http://mcstatus.goldenapplepie.xyz/api/?ip="; // 默认 MC 状态 API 基础 URL

// --- 用于存储配置的变量 ---
String ssid = "";
String password = "";
String api_key = "";
String server_ip = "";
String remote_services_base_url = ""; 
const char* MC_SERVER_API_URL_BASE = DEFAULT_MC_SERVER_API_URL_BASE; 

// --- OLED 配置 ---
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1 
#define OLED_SDA_PIN   5  // I2C SDA 引脚
#define OLED_SCL_PIN   4  // I2C SCL 引脚
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- 定时 ---
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 5000; // 更新间隔 5 秒
bool displayServicesData = true; 

// --- 函数声明 ---
void connectToWiFi();
void fetchAndDisplayRemoteServicesData();
void fetchAndDisplayMinecraftStatus();
void displayMinecraftStatus(String status, int online, int maxPlayers);
void displayRemoteServicesData(float cpu, float mem, int running, int total);
void displayError(String errorLine1, String errorLine2 = "");
void loadConfiguration(); 
void saveConfiguration(); 
void printConfiguration(); 
void printHelp(); 

void setup() {
  Serial.begin(115200);
  delay(1000);

  // --- 初始化 Preferences ---
  preferences.begin("config", false); 

  // --- 加载配置 ---
  loadConfiguration();
  printConfiguration(); 

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  // 初始化 OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); 
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Initializing...");
  display.display();

  connectToWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    display.clearDisplay();
    display.setCursor(0, 30);
    display.println("WiFi Disconnected");
    display.display();
    connectToWiFi(); 
  }

  unsigned long currentTime = millis();
  if (currentTime - lastUpdate >= updateInterval) {
    lastUpdate = currentTime;

    if (displayServicesData) {
      Serial.println("Fetching Remote Services Data...");
      fetchAndDisplayRemoteServicesData(); 
    } else {
      Serial.println("Fetching Minecraft Server Status...");
      fetchAndDisplayMinecraftStatus();
    }
    displayServicesData = !displayServicesData; 
  }

  // --- 添加串口命令来修改配置 ---
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim(); 

    if (command.startsWith("set_ssid ")) {
        String newSSID = command.substring(9); 
        ssid = newSSID;
        Serial.println("New SSID set: " + ssid);
        saveConfiguration(); 
    } else if (command.startsWith("set_password ")) {
        String newPassword = command.substring(13); 
        password = newPassword;
        Serial.println("New password set.");
        saveConfiguration();
    } else if (command.startsWith("set_apikey ")) {
        String newAPIKey = command.substring(11); 
        api_key = newAPIKey;
        Serial.println("New API Key set.");
        saveConfiguration();
    } else if (command.startsWith("set_serverip ")) {
        String newServerIP = command.substring(13); 
        server_ip = newServerIP;
        Serial.println("New Server IP set: " + server_ip);
        saveConfiguration();
    } else if (command.startsWith("set_remote_base_url ")) { 
        String newBaseURL = command.substring(20); 
        remote_services_base_url = newBaseURL;
        Serial.println("New Remote Services Base URL set: " + remote_services_base_url);
        saveConfiguration();
    } else if (command == "print_config") {
        printConfiguration();
    } else if (command == "reconnect_wifi") {
        WiFi.disconnect();
        connectToWiFi();
    } else if (command == "help") { 
        printHelp();
    }
    
  }

}

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid.c_str()); 
  WiFi.begin(ssid.c_str(), password.c_str());

  int attempts = 0;
  const int maxAttempts = 20; 
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("WiFi Connected!");
    display.setCursor(0, 25);
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.display();
    delay(2000); 
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi.");
    display.clearDisplay();
    display.setCursor(0, 20);
    display.println("WiFi Connection");
    display.setCursor(0, 35);
    display.println("FAILED");
    display.display();
    // while(true) { delay(1000); }
  }
}

void fetchAndDisplayRemoteServicesData() {
  if (WiFi.status() != WL_CONNECTED) {
    displayError("No WiFi");
    return;
  }

  HTTPClient http;
  // 动态构建完整 URL
  String url = remote_services_base_url + "/api/service/remote_services_system?apikey=" + api_key;

  http.begin(url);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println("API Response: " + payload);

    JsonDocument doc; 
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("JSON Deserialize Error: ");
      Serial.println(error.c_str());
      displayError("JSON Parse Error");
      http.end();
      return;
    }

    int status = doc["status"];
    if (status == 200) {
      JsonObject systemData = doc["data"][0]["system"];
      float cpuUsage = systemData["cpuUsage"];
      float memUsage = systemData["memUsage"];
      int running = doc["data"][0]["instance"]["running"];
      int total = doc["data"][0]["instance"]["total"];

      // 调用显示函数
      displayRemoteServicesData(cpuUsage * 100, memUsage * 100, running, total);
    } else {
      Serial.printf("API returned error status: %d\n", status);
      displayError("API Error", "Status: " + String(status));
    }
  } else {
    Serial.printf("HTTP GET Error: %d\n", httpResponseCode);
    displayError("HTTP Request Failed", "Code: " + String(httpResponseCode));
  }
  http.end();
}

void fetchAndDisplayMinecraftStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    displayError("No WiFi");
    return;
  }

  HTTPClient http;
  String url = String(MC_SERVER_API_URL_BASE) + server_ip; 

  http.begin(url);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println("MC API Response: " + payload);

    JsonDocument doc; 
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("JSON Deserialize Error: ");
      Serial.println(error.c_str());
      displayError("JSON Parse Error");
      http.end();
      return;
    }

    int code = doc["code"];
    if (code == 200) {
      int online = doc["data"]["players"]["online"];
      int max = doc["data"]["players"]["max"];
      displayMinecraftStatus("Online", online, max);
    } else if (code == 204) {
      displayMinecraftStatus("Offline", 0, 0);
    } else {
      displayMinecraftStatus("Unknown", 0, 0);
    }
  } else {
    Serial.printf("HTTP GET Error: %d\n", httpResponseCode);
    displayError("HTTP Request Failed", "Code: " + String(httpResponseCode));
  }
  http.end();
}

void displayMinecraftStatus(String status, int online, int maxPlayers) {
  display.clearDisplay(); 
  display.setCursor(0, 5);
  display.println("MC Server Status");
  display.println("------------------");
  display.setCursor(0, 30);
  display.print("Server: ");
  display.println(status);
  display.setCursor(0, 45);
  display.print("Players: ");
  display.print(online);
  display.print("/");
  display.println(maxPlayers);
  display.display();
}

void displayRemoteServicesData(float cpu, float mem, int running, int total) {
  display.clearDisplay(); 
  display.setCursor(0, 5);
  display.println("MCSManager Data"); // 标题
  display.println("------------------");
  display.setCursor(0, 25);
  display.printf("CPU: %.2f%%\n", cpu); // 表示为MCSManager面板占用的CPU情况
  display.setCursor(0, 40);
  display.printf("Mem: %.2f%%\n", mem); // 表示为MCSManager面板占用的内存情况
  display.setCursor(0, 55);
  display.printf("Instance: %d/%d", running, total); // 表示为MCSManager面板运行的实例与总实例的情况
  display.display();
}

void displayError(String errorLine1, String errorLine2) {
  display.clearDisplay();
  display.setCursor(0, 20);
  display.println(errorLine1);
  if (errorLine2 != "") {
    display.setCursor(0, 35);
    display.println(errorLine2);
  }
  display.display();
  delay(2000); 
}

// --- 新增函数 ---

void loadConfiguration() {
    // 从 Preferences 加载配置，如果不存在则使用默认值
    ssid = preferences.getString("ssid", DEFAULT_SSID);
    password = preferences.getString("password", DEFAULT_PASSWORD);
    api_key = preferences.getString("api_key", DEFAULT_API_KEY);
    server_ip = preferences.getString("server_ip", DEFAULT_SERVER_IP);
    remote_services_base_url = preferences.getString("remote_base_url", DEFAULT_REMOTE_SERVICES_BASE_URL); 
}

void saveConfiguration() {
    // 保存配置到 Preferences
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.putString("api_key", api_key);
    preferences.putString("server_ip", server_ip);
    preferences.putString("remote_base_url", remote_services_base_url); 
    Serial.println("Configuration saved to Preferences.");
}

void printConfiguration() {
    Serial.println("--- Current Configuration ---");
    Serial.println("SSID: " + ssid);
    Serial.println("Server IP: " + server_ip);
    Serial.println("Remote Services Base URL: " + remote_services_base_url); 
    Serial.println("MC Server API URL Base: " + String(MC_SERVER_API_URL_BASE));
    Serial.println("---------------------------");
}

// 新增：打印帮助信息
void printHelp() {
    Serial.println("\n--- Available Commands ---");
    Serial.println("help                    - 显示此帮助信息");
    Serial.println("set_ssid <ssid>         - 设置 WiFi 名称 (SSID)");
    Serial.println("set_password <password> - 设置 WiFi 密码");
    Serial.println("set_apikey <key>        - 设置 API 密钥");
    Serial.println("set_serverip <ip>       - 设置 MC 服务器 IP");
    Serial.println("set_remote_base_url <url> - 设置 MCSManager 面板基础 URL (例如: http://panel.example.com)");
    Serial.println("print_config            - 打印当前配置");
    Serial.println("reconnect_wifi          - 断开并重新连接 WiFi");
    Serial.println("---------------------------\n");
}