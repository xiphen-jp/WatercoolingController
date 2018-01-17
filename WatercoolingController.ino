#include <OneWire.h>
#include <DallasTemperature.h>
OneWire oneWire(2);
DallasTemperature sensors(&oneWire);
DeviceAddress roomThermometer, waterThermometer;

// 増速開始温度差(℃)
const int temp_diff_min = 3;
// 全開温度差(℃)
const int temp_diff_max = 18;
// 調整温度差レンジ
const int temp_diff_range = temp_diff_max - temp_diff_min;

// 増速開始水温(℃)
const int temp_water_min = 35;
// 全開水温(℃)
const int temp_water_max = 45;
// 調整水温レンジ
const int temp_water_range = temp_water_max - temp_water_min;

// FAN1設定(1200rpm用)
const int fan1_min = 35; // 最低Duty比
const int fan1_p1 = 50; // ベジェ曲線 点1 Y座標
const int fan1_p2 = 60; // ベジェ曲線 点2 Y座標
const int fan1_p3 = 90; // ベジェ曲線 点3 Y座標

// FAN2設定(1500rpm用)
const int fan2_min = 30;
const int fan2_p1 = 25;
const int fan2_p2 = 50;
const int fan2_p3 = 75;

// ポンプ設定
const int pump_min = 50;
const int pump_p1 = 100;
const int pump_p2 = 90;
const int pump_p3 = 98;

// RPM init
/* RPM pins
D4     : Pump
D5,6,7 : Front(in)
D8     : Bottom(in)
A0     : Rear(out)
A1,2,3 : Top(out)
*/
const int rpm_pin[]{ 4, 5, 6, 7, 8, A0, A1, A2, A3 };
const int rpm_pin_num = sizeof(rpm_pin) / sizeof(rpm_pin[0]);
int rpm[rpm_pin_num];

// 使用率/温度
int cpu_usage;
int gpu_usage;

void setup()
{
  Serial.begin(9600);
  Serial.setTimeout(300);

  // Pin Mode
  /*
  |     | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
  |-----|---|---|---|---|---|---|---|---|
  |PortB|PB7|PB6|PB5|PB4|PB3|PB2|PB1|PB0|
  |Pin  |   |   |D13|D12|D11|D10|D9 |D8 |
  |-----|---|---|---|---|---|---|---|---|
  |PortC|PC7|PC6|PC5|PC4|PC3|PC2|PC1|PC0|
  |Pin  |   |   |A5 |A4 |A3 |A2 |A1 |A0 |
  |-----|---|---|---|---|---|---|---|---|
  |PortD|PD7|PD6|PD5|PD4|PD3|PD2|PD1|PD0|
  |Pin  |D7 |D6 |D5 |D4 |D3 |D2 |D1 |D0 |
  */
  DDRB  = B00100110; // OUTPUT: D13(LED) D10,D9(PWM)
  PORTB = B00000001; // INPUT_PULLUP: D8(RPM)
  DDRC  = B00000000;
  PORTC = B00111111; // INPUT_PULLUP: A5,A4(I2C), A3-0(RPM)
  DDRD  = B00001000; // OUTPUT: D3(PWM)
  PORTD = B11110001; // INPUT_PULLUP: D7-4(RPM) D0(Serial)

  // Timer1 (Phase Correct PWM 25kHz TOP=319)
  TCCR1A = B10100010;
  TCCR1B = B00010001;
  TCNT1  = 0;
  ICR1   = 319;
  OCR1A  = 319; // D9 (1200rpm Fan)
  OCR1B  = 319; // D10 (1500rpm Fan)

  // Timer2 (Phase Correct PWM 25kHz TOP=39)
  TCCR2A = B00100001;
  TCCR2B = B00001010;
  TCNT2  = 0;
  OCR2A  = 39;
  OCR2B  = 39; // D3 (Pump)

  // Thermometer 
  sensors.begin();
  sensors.setResolution(11);
  sensors.getAddress(roomThermometer, 0);
  sensors.getAddress(waterThermometer, 1);

  delay(1000);
}

int getRpm(int pin) {
  long time_cycle = pulseIn(pin, HIGH, 250000) + pulseIn(pin, LOW, 250000);
  if (time_cycle == 0) {
    return 0;
  }
  else {
    return 60000000 / (time_cycle * 2);
  }
}

float getRatio4thBezier(int p1, int p2, int p3, float t, int min = 0) {
  float p_y = 4 * pow(1 - t, 3) * t * p1 + 6 * pow(1 - t, 2) * pow(t, 2) * p2 + 4 * (1 - t) * pow(t, 3) * p3 + pow(t, 4) * 100;
  return ((100.0 - min) / 100) * p_y + min;
}

float getRatio5thBezier(int p1, int p2, int p3, int p4, float t, int min = 0) {
  float p_y = 5 * pow(1 - t, 4) * t * p1 + 10 * pow(1 - t, 3) * pow(t, 2) * p2 + 10 * pow(1 - t, 2) * pow(t, 3) * p3 + 5 * (1 - t) * pow(t, 4) * p4 + pow(t, 5) * 100;
  return ((100.0 - min) / 100) * p_y + min;
}

void loop()
{
  unsigned long start = millis();
  //-------------------------------------------------------
  // 回転数取得
  for (size_t i = 0; i < rpm_pin_num; i++)
  {
    rpm[i] = getRpm(rpm_pin[i]);
  }
  //-------------------------------------------------------
  // 温度取得
  sensors.requestTemperatures();
  float temp_room = sensors.getTempC(roomThermometer);
  float temp_water = sensors.getTempC(waterThermometer);
  float temp_diff = temp_water - temp_room;

  //-------------------------------------------------------
  // 回転数調整

  // 使用率基準（ポンプのみ）
  float duty_usage_2B;

  if (cpu_usage < 0) {
    // 単独動作時は80%
    duty_usage_2B = 80;
  }
  else {
    // 使用率合計による補正
    int usage_sum = cpu_usage + gpu_usage;
    float usage_sum_ratio = (float) (constrain(usage_sum, 30, 150) - 30) / (150 - 30);
    duty_usage_2B = getRatio4thBezier(95, 80, 70, usage_sum_ratio, 50);

    // 単独使用率による補正
    if (cpu_usage >= 50 || gpu_usage >= 50) {
      float usage_max = (max(cpu_usage, gpu_usage) - 50) / 2 + 80;
      usage_max = min(usage_max, 100);
      duty_usage_2B = max(duty_usage_2B, usage_max);
    }
  }

  // 温度差基準
  float temp_diff_ratio = (float) (constrain(temp_diff, temp_diff_min, temp_diff_max) - temp_diff_min) / temp_diff_range;
  float duty_diff_1A = getRatio4thBezier(fan1_p1, fan1_p2, fan1_p3, temp_diff_ratio, fan1_min);
  float duty_diff_1B = getRatio4thBezier(fan2_p1, fan2_p2, fan2_p3, temp_diff_ratio, fan2_min);
  float duty_diff_2B = getRatio4thBezier(pump_p1, pump_p2, pump_p3, temp_diff_ratio, pump_min);

  // 水温基準
  float temp_water_ratio = (float) (constrain(temp_water, temp_water_min, temp_water_max) - temp_water_min) / temp_water_range;
  float duty_water_1A = getRatio4thBezier(fan1_p1, fan1_p2, fan1_p3, temp_water_ratio, fan1_min);
  float duty_water_1B = getRatio4thBezier(fan2_p1, fan2_p2, fan2_p3, temp_water_ratio, fan2_min);
  float duty_water_2B = getRatio4thBezier(pump_p1, pump_p2, pump_p3, temp_water_ratio, pump_min);
  
  // 最も大きい値を採用
  float duty_1A = max(duty_diff_1A, duty_water_1A);
  float duty_1B = max(duty_diff_1B, duty_water_1B);
  float duty_2B = max(duty_diff_1B, duty_water_1B);
  duty_2B = max(duty_2B, duty_usage_2B);

  OCR1A = duty_1A * 319 / 100;
  OCR1B = duty_1B * 319 / 100;
  OCR2B = duty_2B * 39 / 100;
  
  //-------------------------------------------------------
  // シリアル通信

  // 受信バッファをクリア
  while (Serial.available() > 0) {
    char temp = Serial.read();
  }

  // 送信
  // Prefix
  Serial.print("data:");
  // RPM
  for (size_t i = 0; i < rpm_pin_num; i++)
  {
    Serial.print(rpm[i]);
    Serial.print(",");
  }
  // 温度
  Serial.print(temp_room);
  Serial.print(",");
  Serial.print(temp_water);
  Serial.print(",");
  // Duty比
  Serial.print((int)duty_2B);
  Serial.print(",");
  Serial.print((int)duty_1A);
  Serial.print(",");
  Serial.print((int)duty_1B);
  Serial.print(",");
  // 使用率
  Serial.print(cpu_usage);
  Serial.print(",");
  Serial.print(gpu_usage);
  // 改行(終端)
  Serial.println();

  // 受信（タイムアウト300ms）
  String serial_header = Serial.readStringUntil(':');
  if (serial_header == NULL) {
    // タイムアウト時は単独動作判定
    cpu_usage = -1;
  }
  else {
    // パラメータを読む
    cpu_usage = Serial.readStringUntil(',').toInt();
    gpu_usage = Serial.readStringUntil(';').toInt();
  }

  //-------------------------------------------------------
  // 最小1秒間隔でループ
  unsigned int passed = millis() - start;
  if (passed <= 1000) {
    delay(1000 - passed);
  }
}
