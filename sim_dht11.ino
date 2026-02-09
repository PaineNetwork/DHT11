#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <HardwareSerial.h>

// Cấu hình OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C  // Địa chỉ I2C của OLED (0x3C thay vì 0x96)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Cấu hình DHT11
#define DHT_PIN 4
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// Cấu hình SIM A7680C
HardwareSerial sim(2);  // Sử dụng UART2 của ESP32
#define SIM_RX 16
#define SIM_TX 17

// Cấu hình hệ thống
#define PHONE_NUMBER "0964067845"
#define TEMP_THRESHOLD 30.0
#define CHECK_INTERVAL 5000  // Kiểm tra mỗi 5 giây
#define CALL_COOLDOWN 30000 // Chờ 30 giây giữa các cuộc gọi (test mode)

unsigned long lastCheck = 0;
unsigned long lastCall = 0;
bool alertActive = false;

void setup() {
  Serial.begin(115200);
  
  // Khởi tạo OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Khoi tao he thong...");
  display.display();
  
  // Khởi tạo DHT11
  dht.begin();
  
  // Khởi tạo SIM module
  sim.begin(115200, SERIAL_8N1, SIM_RX, SIM_TX);
  delay(3000);
  
  Serial.println("=== ESP32 Temperature Alert System ===");
  
  // Khởi tạo module SIM
  initSIM();
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("He thong san sang!");
  display.display();
  delay(2000);
}

void loop() {
  unsigned long currentTime = millis();
  
  // Kiểm tra nhiệt độ theo chu kỳ
  if (currentTime - lastCheck >= CHECK_INTERVAL) {
    lastCheck = currentTime;
    
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    // Kiểm tra dữ liệu hợp lệ
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Loi doc cam bien DHT11!");
      displayError();
      return;
    }
    
    // Hiển thị thông tin lên OLED
    displayInfo(temperature, humidity);
    
    // Kiểm tra cảnh báo nhiệt độ
    if (temperature > TEMP_THRESHOLD) {
      if (!alertActive) {
        alertActive = true;
        Serial.println("CANH BAO: Nhiet do vuot nguong!");
      }
      
      // Gọi điện nếu đã đủ thời gian chờ (kiểm tra mỗi lần nhiệt độ vượt ngưỡng)
      if (currentTime - lastCall >= CALL_COOLDOWN) {
        Serial.println("Thuc hien cuoc goi canh bao...");
        makeEmergencyCall(temperature);
        lastCall = currentTime;
      } else {
        unsigned long timeLeft = (CALL_COOLDOWN - (currentTime - lastCall)) / 1000;
        Serial.print("Cho them ");
        Serial.print(timeLeft);
        Serial.println(" giay truoc cuoc goi tiep theo");
      }
    } else {
      if (alertActive) {
        Serial.println("Nhiet do da tro ve binh thuong");
        alertActive = false;
      }
    }
  }
  
  // Xử lý phản hồi từ SIM module
  if (sim.available()) {
    Serial.write(sim.read());
  }
  if (Serial.available()) {
    sim.write(Serial.read());
  }
}

void displayInfo(float temp, float hum) {
  display.clearDisplay();
  
  // Tiêu đề
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("GIAM SAT NHIET DO");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  // Nhiệt độ
  display.setTextSize(2);
  display.setCursor(0, 15);
  display.print("T: ");
  display.print(temp, 1);
  display.println("C");
  
  // Độ ẩm
  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("Do am: ");
  display.print(hum, 1);
  display.println("%");
  
  // Trạng thái cảnh báo
  display.setCursor(0, 45);
  if (temp > TEMP_THRESHOLD) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Đảo màu
    display.println("!!! CANH BAO !!!");
    display.setTextColor(SSD1306_WHITE); // Trở lại màu bình thường
  } else {
    display.println("Binh thuong");
  }
  
  // Thời gian
  display.setCursor(0, 55);
  display.print("Time: ");
  display.print(millis() / 1000);
  display.println("s");
  
  display.display();
}

void displayError() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("LOI CAM BIEN DHT11!");
  display.setCursor(0, 35);
  display.println("Kiem tra ket noi...");
  display.display();
}

void initSIM() {
  Serial.println("Khoi tao module SIM...");
  
  sendAT("AT", 1000);
  sendAT("AT+CPIN?", 2000);
  sendAT("AT+CSQ", 1000);
  sendAT("AT+CREG?", 2000);
  sendAT("AT+CGATT?", 2000);
  
  Serial.println("Module SIM da san sang!");
}

void sendAT(String cmd, int timeout) {
  sim.println(cmd);
  Serial.print(">>> Gui lenh: ");
  Serial.println(cmd);
  
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (sim.available()) {
      Serial.write(sim.read());
    }
  }
  Serial.println();
}

void makeEmergencyCall(float temperature) {
  Serial.println(">>> BAT DAU GOI CANH BAO...");
  
  // Kiểm tra trạng thái mạng trước khi gọi
  Serial.println("Kiem tra trang thai mang...");
  sendAT("AT+CREG?", 2000);
  sendAT("AT+CSQ", 1000);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("DANG GOI CANH BAO!");
  display.setCursor(0, 25);
  display.print("Nhiet do: ");
  display.print(temperature, 1);
  display.println("C");
  display.setCursor(0, 40);
  display.print("Goi: ");
  display.println(PHONE_NUMBER);
  display.display();
  
  // Thực hiện cuộc gọi
  String dialCmd = "ATD" + String(PHONE_NUMBER) + ";";
  Serial.print("Gui lenh goi: ");
  Serial.println(dialCmd);
  
  sim.println(dialCmd);
  
  Serial.print("Goi den so: ");
  Serial.println(PHONE_NUMBER);
  Serial.print("Nhiet do hien tai: ");
  Serial.print(temperature);
  Serial.println("°C");
  
  // Đợi phản hồi trong 5 giây
  Serial.println("Doi phan hoi tu module SIM...");
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {
    if (sim.available()) {
      String response = sim.readString();
      Serial.print("SIM Response: ");
      Serial.println(response);
    }
  }
  
  // Gọi trong 15 giây
  Serial.println("Dang goi... (15 giay)");
  delay(15000);
  
  // Kết thúc cuộc gọi
  Serial.println("Ket thuc cuoc goi...");
  sendAT("ATH", 2000);
  
  Serial.println(">>> KET THUC CUOC GOI CANH BAO!");
  
  // Hiển thị thông báo hoàn thành
  display.clearDisplay();
  display.setCursor(0, 20);
  display.println("DA GOI CANH BAO!");
  display.setCursor(0, 35);
  display.println("Cho 5 phut truoc");
  display.setCursor(0, 45);
  display.println("cuoc goi tiep theo");
  display.display();
  delay(3000);
}
