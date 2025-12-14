#include <Arduino.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

#define I2C_ADDR 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2

#define SETTING_BUTTON 13
#define UP_BUTTON 9
#define OK_BUTTON 4
#define CANCEL_BUTTON 2

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_ROWS);
RTC_DS1307 rtc;

// ===== 按鈕類別（原本不動）=====
class Button
{
private:
  const int pin;
  const unsigned long deboundceMs;
  int last_btn_state;
  unsigned long last_time_change;

public:
  Button(int pin)
      : pin(pin), deboundceMs(30), last_btn_state(HIGH), last_time_change(0) {}

  bool isPressed()
  {
    unsigned long now = millis();
    if (now - last_time_change > deboundceMs)
    {
      int now_state = digitalRead(pin);
      if (now_state != last_btn_state)
      {
        last_btn_state = now_state;
        last_time_change = now;
        if (now_state == LOW)
          return true;
      }
    }
    return false;
  }

  void begin()
  {
    pinMode(pin, INPUT_PULLUP);
  }
};

// ===== 宣告區 =====
String number_prefix(int, unsigned int);
String blanking(int, unsigned int, bool, int);
void show_time(bool);
void init_time();

Button setting_btn(13);
Button up_btn(9);
Button ok_btn(4);
Button cancel_btn(2);

bool is_setting = false;
int now_setting_digit = 1;

bool is_blanking = false;
const unsigned int blanking_delay_ms = 600;
unsigned long last_blanking_time = 0;

uint16_t year;
uint8_t month;
uint8_t day;
uint8_t hour;
uint8_t minute;
uint8_t second;

void setup()
{
  Serial.begin(9600);
  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    delay(1000);
    abort();
  }

  lcd.init();
  lcd.backlight();

  setting_btn.begin();
  up_btn.begin();
  ok_btn.begin();
  cancel_btn.begin();
}

void loop()
{
  if (setting_btn.isPressed() && !is_setting)
  {
    is_setting = true;
    now_setting_digit = 1;
    init_time();
    return;
  }

  if (is_setting)
  {
    // 取消鍵功能
    if (cancel_btn.isPressed())
    {
      is_setting = false;
      now_setting_digit = 1;
      return;
    }

    if (up_btn.isPressed())
    {
      switch (now_setting_digit)
      {
      case 1:
        year++;
        break;
      case 2:
        month = month % 12 + 1;
        break;
      case 3:
        day = day % 31 + 1;
        break;
      case 4:
        hour = (hour + 1) % 24;
        break;
      case 5:
        minute = (minute + 1) % 60;
        break;
      case 6:
        second = (second + 1) % 60;
        break;
      }
    }

    if (ok_btn.isPressed())
    {
      if (now_setting_digit < 6)
      {
        now_setting_digit++;
        return;
      }

      rtc.adjust(DateTime(year, month, day, hour, minute, second));
      is_setting = false;
      now_setting_digit = 1;
    }
  }

  show_time(is_setting);
}

void show_time(bool is_setting)
{
  DateTime now = rtc.now();

  lcd.setCursor(0, 0);
  lcd.print("Day:");
  lcd.print(blanking(is_setting ? year : now.year(), 4, is_setting, 1));
  lcd.print("/");
  lcd.print(blanking(is_setting ? month : now.month(), 2, is_setting, 2));
  lcd.print("/");
  lcd.print(blanking(is_setting ? day : now.day(), 2, is_setting, 3));

  lcd.setCursor(0, 1);
  lcd.print("Time:");
  lcd.print(blanking(is_setting ? hour : now.hour(), 2, is_setting, 4));
  lcd.print(":");
  lcd.print(blanking(is_setting ? minute : now.minute(), 2, is_setting, 5));
  lcd.print(":");
  lcd.print(blanking(is_setting ? second : now.second(), 2, is_setting, 6));
}

String blanking(int number, unsigned int digit, bool is_setting, int setting_digit)
{
  unsigned long now = millis();

  if (!is_setting || now_setting_digit != setting_digit)
  {
    is_blanking = true;
  }
  else if (last_blanking_time + blanking_delay_ms < now)
  {
    last_blanking_time = now;
    is_blanking = !is_blanking;
  }

  if (is_blanking)
  {
    return number_prefix(number, digit);
  }
  else
  {
    String s = "";
    for (unsigned int i = 0; i < digit; i++)
      s += " ";
    return s;
  }
}

String number_prefix(int number, unsigned int width)
{
  String numStr = String(number);
  while (numStr.length() < width)
    numStr = "0" + numStr;
  return numStr;
}

void init_time()
{
  DateTime now = rtc.now();
  year = now.year();
  month = now.month();
  day = now.day();
  hour = now.hour();
  minute = now.minute();
  second = now.second();
}
