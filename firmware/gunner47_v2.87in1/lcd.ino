#if USE_OLED_DISPLAY

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/Org_01.h>

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
static const unsigned char PROGMEM bell_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };
 
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void lcd_display_init(void)
{
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_I2C_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  //  display.display();
  //  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

// 18pt font is 21x21 pixels with 3 pixels blank space on both sides
// Hence X= -3 is the leftmost point for fonts
#define TEXT_ZERO_X (-3)
#define TEXT_ZERO_Y (20)

void updateLCD(bool time_blink)
{
  time_t t = localTimeZone.toLocal(timeClient.getEpochTime());
  static uint32_t time_old = 0;
  uint32_t time_now = second(t); // update it once a second
  uint32_t time_to_alarm;

  if (time_now != time_old)
  {    
    printTimeLCD(t, time_blink);
    time_old = time_now;
    uint8_t thisDay = dayOfWeek(t);
    if (thisDay == 1) thisDay = 8; // в библиотеке Time воскресенье - это 1; приводим к диапазону [0..6], где воскресенье - это 6
    thisDay -= 2;
    //LOG.printf_P(PSTR("alarm: %d %d %d\n"), thisDay, alarms[thisDay].State, alarms[thisDay].Time);

    uint32_t time_minutes = 60*hour(t) + minute(t);
    uint8_t nextDay = thisDay + 1;
    if(nextDay == 7)
    {
      nextDay = 0;
    }
    // today alarm
    if(alarms[thisDay].State)
    {
      time_to_alarm = alarms[thisDay].Time - time_minutes;
      if(time_to_alarm <= 480)
      {
        LOG.printf_P(PSTR("today: minutes to alarm: %d\n"), time_to_alarm);
        printAlarmStatus(time_to_alarm, true);
      }
    }
    // check the next day
    else if(alarms[nextDay].State)
    {
      uint32_t minutes_left_today = 24*60 - time_minutes;
      time_to_alarm = minutes_left_today + alarms[nextDay].Time;
      if(time_to_alarm <= 480)
      {
        LOG.printf_P(PSTR("tomorrow: minutes to alarm: %d\n"), time_to_alarm);
        printAlarmStatus(time_to_alarm, true);
      }
    }
    else
    {
        printAlarmStatus(0, false);
    }
    display.display();
  }
}

void printTimeLCD(time_t t, bool time_blink)
{
  char LCDTime[]   = "00000";
  static bool flag = false;

  display.setFont(&FreeMono18pt7b);
  display.setTextColor(SSD1306_WHITE);
  display.fillRect(0, 0, 98, TEXT_ZERO_Y+1, SSD1306_BLACK);
  display.setCursor(TEXT_ZERO_X, TEXT_ZERO_Y);
  if(flag)
    sprintf(LCDTime, "%02d:%02d", hour(t), minute(t));
  else
    sprintf(LCDTime, "%02d %02d", hour(t), minute(t));
  if(!time_blink | flag)
    display.print(LCDTime);
  flag = !flag;
}

void printAlarmStatus(uint32_t time_to_alarm, bool alarm_enabled)
{
  static bool lcd_is_clean = false;
  uint32_t hours_to_alarm = time_to_alarm/60;
  uint32_t quaters_to_alarm = time_to_alarm/15 - hours_to_alarm *4;

  if(time_to_alarm%15 > 0)
{
    quaters_to_alarm++;
  }

  //LOG.printf_P(PSTR("Hours: %d quaters: %d delta: %d\n"), hours_to_alarm, quaters_to_alarm, time_to_alarm%15);
  display.setFont(&Org_01);  
  display.setCursor(105, 6);
  display.println(F("sync"));

  //clear the bar
  display.fillRect(0, 25, 8*14, 7, SSD1306_BLACK);

  // show alarm indicator only if alarm is within 8 hours and is enabled
  if (alarm_enabled && hours_to_alarm <= 8)
  {
    // draw alarm empty lines
    for(int i = 1; i <= 8; i++)
    {
      display.fillRect(8*14 - i*14, 31,
                       12, 1,
                       SSD1306_WHITE);
    }
    // draw full hours
    for(int i = 1; i <= hours_to_alarm; i++)
      {
        display.fillRect(8*14 - i*14, 25,
                         12, 7,
                         SSD1306_WHITE);
      }
    // draw quaters
    for(int i = 1; i <= quaters_to_alarm; i++)
    {
      display.fillRect(8*14 - hours_to_alarm*14 - i*3 - 2, 25,
                       3, 7,
                       SSD1306_WHITE);
    }

    display.drawBitmap(
      (display.width()  - LOGO_WIDTH ),
      (display.height() - LOGO_HEIGHT),
      bell_bmp, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
    lcd_is_clean = false;
    }
    else
    {
    if (!lcd_is_clean)
    {
      //clear
      display.fillRect(
        (display.width()  - LOGO_WIDTH ),
        (display.height() - LOGO_HEIGHT),
        LOGO_WIDTH, LOGO_HEIGHT, SSD1306_BLACK);
      lcd_is_clean = true;
    }
    // print ip
    display.setCursor(0, 31);
    display.print("IP: ");  display.print(WiFi.localIP());
    display.print(" MODE: "); display.print(currentMode);
  }
}
#endif
