#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "EasyButton.h"
#include "RTClib.h"
#include "DHT.h"

#define MENU_BUTTON 8
#define SELECT_BUTTON 9
#define CHANGE_DATE_BUTTON 10
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET 4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define DHTPIN 2  
#define DHTTYPE DHT11
#define ANALOG_POT A5



EasyButton menuButton(MENU_BUTTON,40,true,true);
//         menuButton(PIN,DEBOUNCE,PULLUP,INVERT)
EasyButton selectButton(SELECT_BUTTON,40,true,true);
//         menuButton(PIN,DEBOUNCE,PULLUP,INVERT)
EasyButton confirmDateButton(CHANGE_DATE_BUTTON,40,true,true);
//         menuButton(PIN,DEBOUNCE,PULLUP,INVERT)

int val;
RTC_DS3231 rtc;
DHT dht(DHTPIN, DHTTYPE);
const byte isBigMonth[] = {1,3,5,7,8,10,12};
const byte isSmallMonth[] = {4,6,9,11};
const byte isFeb = 2;
const char *MenuItems[] = {"TEMP","DATE","RST DT","UNLOCK"};
const char *daysOfTheWeek[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
byte menuSelected = 0; //
byte currentColumn = 0; // only goes to 3 so 255 is more than enough
byte section = 0;
bool onMenu = true;

int lastTempReading;


void setup() 
{
	Serial.begin(9600);
  dht.begin();
  rtc.begin();
  pinMode(ANALOG_POT,INPUT);

  menuButton.begin();
  menuButton.enableInterrupt(menuDownISR);
  menuButton.onPressed(menuDown);

  selectButton.begin();
  selectButton.enableInterrupt(selectISR);
  selectButton.onPressed(select);

  confirmDateButton.begin();
  confirmDateButton.enableInterrupt(confirmChangeISR);
  confirmDateButton.onPressed(confirmChange);

	if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) 
	{ // Address for 128x64
		Serial.println(("SSD1306 allocation failed"));
		for(;;); // Don't proceed, loop forever
	}

	display.setTextSize(2);
  display.clearDisplay();
  lastTempReading = millis();
}


void menu()
{
  display.clearDisplay();
	display.setCursor(1,1); // Start at top-left corner
  display.fillRoundRect(0, currentColumn*16, SCREEN_WIDTH,16, 0, WHITE);

  for(int i = 0 ; i < sizeof(MenuItems)/4;i++) {
    if(i == currentColumn)
      display.setTextColor(BLACK); // Draw BLACK text
    else
      display.setTextColor(WHITE); // Draw WHITE text
      
    display.setCursor(1, ((16*i)+1));
    display.println(MenuItems[i]);
  }
}
void confirmChangeISR()
{
  confirmDateButton.read();
}
void confirmChange()
{
  if(!onMenu)
  {
    if(section == 3)
      section = 0;
    else
      section++;
  }
}
void selectISR()
{
  selectButton.read();
}
void select()
{
  onMenu = !onMenu;
}
void menuDownISR()
{
  menuButton.read();
}
void menuDown()
{
  if(onMenu)
  {
    if(currentColumn == 3)
      currentColumn = 0;
    else
      currentColumn++;
  }
}
void printTemp()
{
  if(millis() - lastTempReading >= 1000)
  {
    display.clearDisplay();
    display.setCursor(1,1);
    lastTempReading = millis();
    
    int h = dht.readHumidity();
    int t = dht.readTemperature();

    //display.clearDisplay();
    display.print("Humidity: ");
    display.print(h);
    display.println('%');
    display.println();
    display.print("Temp: ");
    display.print(t);
    display.print(" ");
    display.drawCircle(display.getCursorX()-4, display.getCursorY()+2, 2, WHITE);
    display.println('C');
  }

  display.display();
}
void printDate()
{
  display.clearDisplay();
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(1,1); // Start at one pixel away from edge of top-left corner

  DateTime now = rtc.now();

  //DATE
  display.print(insertChar(now.day()));
  display.print('-');
  display.print(insertChar(now.month()));
  display.print('-');
  display.print(now.year(), DEC);
  display.println();

  //TIME
  display.print(insertChar(now.hour()));
  display.print(':');
  display.print(insertChar(now.minute()));
  display.print(':');
  display.print(insertChar(now.second()));
  display.println("");

  //WEEKDAY
  display.println("Today is");
  display.println(daysOfTheWeek[now.dayOfTheWeek()]);
}
String insertChar(int val){
  if(val < 10){
    return String('0') + String(val);
  }
  return String(val);
}
void changeDate()
{
 bool confirmed = false;
  int year,month,date;

  while(!confirmed)
  {
    display.clearDisplay();
    display.setCursor(1, 1);
    val = analogRead(ANALOG_POT);
    switch(section)
    {
      case 0:
        display.setCursor(1, ((16*section)+1));
        val = map(val,0,1023,2000,2100);
        year = val;
      break;

      case 1:
        display.print("YEAR: ");
        display.println(year);
        //display.setCursor(1,((16*section)+1));
        val = map(val,0,1023,1,13);
        month = val;
      break;

      case 2:
        display.print("YEAR: ");
        display.println(year);
        display.print("MONTH: ");
        display.println(month);
        //display.setCursor(1,((16*section)+1));
        if (month == 2 && (year % 4 == 0 && year % 100 != 0) || year % 400 == 0) 
        {
          val = map(val,0,1023,1,30);//the pot is bad so we do 29+1
        }
        else if (month == 2)
        {
          val = map(val,0,1023,1,29);//the pot is bad so we do 28+1
        }
        else
        {
          for(int i = 0; i < 7;i++)
          {
            if(month == isBigMonth[i])
            {
              val = map(val,0,1023,1,31);
              break;
            }
            else if (month == isSmallMonth[i])
            {
              val = map(val,0,1023,1,30);
              break;
            }
          }
        }
        date = val;
      break;

      case 3:
        DateTime now = rtc.now();
        DateTime newDT = {year,month,date,(now.hour(),DEC),(now.minute() ,DEC), (now.second() ,DEC)};
        rtc.adjust(newDT);
        confirmed = true;
        onMenu = true;
      break;
    }
    if(!confirmed)
    display.println(val);

    display.display();
  }
}

void loop() 
{
  if(onMenu)
  menu();
  else
  {
    switch(currentColumn)
    {
      case 0:
        printTemp();
      break;
      case 1:
        printDate();
      break;
      case 2:
        changeDate();
      break;
      case 3:
      break;
    }
  }

  display.display();
}