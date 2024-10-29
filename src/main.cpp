#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "ESP8266WiFi.h"
#include "sprites.h"
#include "config.h"

Adafruit_SSD1306 *display;
int apCount=0; //the number of siscovered access points
int min_rssi = 0;//minimal RSSI value, used for APs clustering
int max_rssi = 0;//maximal RSSI value, used for APs clustering

int iteration = 0;//current iteration, or animation frame

//fake ap names if you want to hide real SSIDs
String fake_ap[]={"Home WiFi", "ExWiFi", "404", "Work", "Virus", "LOL", "Matrix", "127.0.0.1", "LAN", "Skynet",
"E=MC2", "Happy", "FBI", "Free", "Hacker", "Simon", "Sexy", "DIR", "D-Link", "TP-Link"};

// Class representing a single invader being
class Invader
{
  private:
    unsigned int m_x;//x coordinate
    unsigned int m_y;//y coordinate
    String m_name; //AP SSID
    unsigned int m_kind;//kind of character: 0-squid, 1-crab, 2-octopus, 3-UFO
    int m_speed; //x-speed
    bool m_state;//animation frame, each character has 2 states, so bool is enough
  public:
    Invader():
      m_x(0),
      m_y(0),
      m_name(""),
      m_kind(0),
      m_speed(0),
      m_state(false)
    {}

    Invader(int x, int y, int kind, String name):
      m_x(x),
      m_y(y),
      m_name(name),
      m_kind(kind),
      m_speed(0),
      m_state(false)
    {}

    int x()
    {return m_x;}

    int y()
    {return m_y;}

    int kind()
    {return m_kind;}

    String name()
    {return m_name;}

    void setX(int x){
      if(x>0)
        m_x=x;
    }

    void setY(int y){
      if(y>0)
        m_y=y;
    }

    void setKind(int kind){
      if(kind>=0)
        m_kind=kind;
    }

    void setName(String name){
      m_name=name;
    }

    //display a character on the screen
    void draw(){
      m_state = !m_state;//switching to the opposite animation frame
      m_x+=m_speed; //moving the character
      //avoiding screen edges
      if(m_x<8){
        m_x=8;
        m_speed=1;
      }
      if(m_x>SCREEN_WIDTH-16){
        m_x=SCREEN_WIDTH-16;
        m_speed=-1;
      }
      m_speed+=rand()%3-1;//changing randomly current speed
      //speed limits
      if (m_speed>2)
        m_speed = 2;
      if (m_speed<-2)
        m_speed = -2;
      //finally, draw the sprite
      display->drawBitmap(m_x,m_y,invader_bitmap[m_kind][m_state?1:0],16,8,SSD1306_WHITE);
    }

} invaders[MAX_INVADERS];

//Start OLED display
bool init_oled() {
  display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

  Wire.begin(OLED_SDA, OLED_SCL);
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    return false;
  }
  return true;
}

//update the scene
void update_oled() {
  display->clearDisplay();
  display->setTextWrap(false);//do not break text labels at the display edge 
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  
  //draw corner "SSID" bitmap
  display->fillRect(0,0,8,16,SSD1306_WHITE);
  display->drawBitmap(1,0,ssid_icon,8,16,SSD1306_BLACK);
  //draw "radar" circle animation
  if (iteration<32)
    display->drawCircle(display->width()/2,display->height(), iteration*4, SSD1306_WHITE);
  //draw "scan" indicator
  if (iteration>SCAN_ITERATION)
    display->drawBitmap(display->width()/2-4,display->height()-8,scan_bitmap[iteration%2],8,8,SSD1306_WHITE);
  //draw signal strength indicator
  for (int i=16;i<56;i+=8)
    display->fillRect(0,i+1,i/8-1,6,SSD1306_WHITE);

  //decide how much characters of AP names should be displayed
  //the more APs discovered - the shorter labels displayed
  int ap_label_len = 0;
  if (apCount==1)
    ap_label_len = 16;
  else if (apCount==2)
    ap_label_len = 8;
  else if (apCount==3)
    ap_label_len = 6;
  else if (apCount<6)
    ap_label_len = 5;
  else if (apCount<8)
    ap_label_len = 4;
  else 
    ap_label_len = 3;

  //if wireless networks were discovered
  if (apCount){
    for (int i=0; i<apCount; i++)
      if (i<MAX_INVADERS){//no more than first MAX_INVADERS APs are displayed 
        //distance to the center of "radar"
        int distance = (invaders[i].x()-display->width()/2)*(invaders[i].x()-display->width()/2)+
                    (invaders[i].y()-display->height())*(invaders[i].y()-display->height());
        //reveal only the invaders inside the "radar" circe
        if(distance<(iteration*4)*(iteration*4)){
          invaders[i].draw();
          //for UFO (free AP) draw also a laser cannon, so that it could be easily located
          if(invaders[i].kind()==3){
            display->drawBitmap(invaders[i].x(),display->height()-8,free_bitmap,8,8,SSD1306_WHITE);
            display->drawPixel(invaders[i].x()+4,display->height()-iteration%display->height(),SSD1306_WHITE);
          }
          //draw AP name label
          display->setCursor(invaders[i].x(), ((invaders[i].y()-16)/8>=RSSI_INTERVALS/2) ? 8:0);
          display->print(invaders[i].name().substring(0,ap_label_len));
        }
      }
  }
  display->display();
}

//process AP scan results
void scanResults(int networksFound){
  iteration=0;//reset current iteration
  apCount = networksFound;
  
  //if networks were discovered
  if (apCount){
    //calculate min and max RSSI
    min_rssi = WiFi.RSSI(0);
    max_rssi = WiFi.RSSI(0);
    for (int i = 0; i < apCount; ++i) {
      if(WiFi.RSSI(i)>max_rssi)
        max_rssi = WiFi.RSSI(i);
      if(WiFi.RSSI(i)<min_rssi)
        min_rssi = WiFi.RSSI(i);
    }

    //clustering APs to separate rows by RSSI
    for (int i = 0; i < apCount; ++i){
      int row = abs( (RSSI_INTERVALS-1) * (WiFi.RSSI(i)-min_rssi) / (max_rssi - min_rssi) );
      //confifuring the corresponding invader
      if (i<MAX_INVADERS){
        invaders[i].setY(16+row*8);
        invaders[i].setX(rand()%8*16);
        if (SHOW_FAKE_AP_NAMES)
          invaders[i].setName(fake_ap[rand()%20]);
        else
          invaders[i].setName(WiFi.SSID(i));
        //for unencrypted networks UFO sprite will be displayed 
        if (WiFi.encryptionType(i)==ENC_TYPE_NONE)
          invaders[i].setKind(3);
        //otherwise, the sprite will depend on the row
        else
          invaders[i].setKind((row+1)/2);
      }
    }
  }
}

//asynchronous networks scan 
void scanNetworks()
{
  WiFi.scanNetworksAsync(scanResults, true);
}

void display_transition(){
  for(int i=0;i<128;i++){
    display->drawCircle(display->width()/2,display->height(), i, SSD1306_BLACK);
    display->drawCircle(display->width()/2-1,display->height(), i, SSD1306_BLACK);
    display->drawCircle(display->width()/2,display->height()-1, i*2, SSD1306_BLACK);
    display->drawCircle(display->width()/2,display->height()-2, i*3, SSD1306_BLACK);
    display->display();
  }
}

//displaying intro scene animation
void intro()
{
  for (int w=0; w<256; w++){
    display->setTextSize(2);
    display->setTextColor(SSD1306_WHITE);
    display->clearDisplay();
    display->setCursor(32,16);
    display->println("Insert");
    display->setCursor(44,32);
    display->println("c in");
    display->setTextSize(1);
    display->setCursor(36,48);
    display->println("to continue");
    display->fillRoundRect(61-w%8/2,36,w%8,10,4,SSD1306_WHITE);
    display->display();
  }
  display_transition();

  for (int y=64; y>24; y--){
    display->clearDisplay();
    display->drawBitmap(0,y,splash_bitmap_array[0],80,40,SSD1306_WHITE);
    display->display();
  }
  display->setTextWrap(false);//do not break text labels at the display edge
  display->clearDisplay();
  display->drawBitmap(0,24,splash_bitmap_array[0],80,40,SSD1306_WHITE);
  display->display();
  delay(500);
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  String text = "SSID";
  for (unsigned int c=0; c<text.length(); c++)
    for(int x=128; x>=72; x--){
      display->setCursor(x+1, 16+c*8);
      display->setTextColor(SSD1306_BLACK);
      display->println(text[c]);
      display->setCursor(x, 16+c*8);
      display->setTextColor(SSD1306_WHITE);
      display->println(text[c]);
      display->display();
    }
  delay(500);
  for (int i=0;i<3;i++)
    for (int j=0;j<136;j++){
      display->clearDisplay();
      display->drawBitmap(0,24,splash_bitmap_array[0],80,40,SSD1306_WHITE);
      display->fillRect(61,50,3,3,rand()%2?SSD1306_BLACK:SSD1306_WHITE);
      display->fillRect(51,50,3,3,rand()%2?SSD1306_BLACK:SSD1306_WHITE);
      display->setTextSize(1);
      display->setTextColor(SSD1306_WHITE);
      display->setCursor(72, 16);
      display->println("S");
      display->setCursor(72, 24);
      display->println("S");
      display->setCursor(72, 32);
      display->println("I");
      display->setCursor(72, 40);
      display->println("D");
      display->drawCircle(8,24,j,SSD1306_WHITE);
      display->drawCircle(8,24,j*2,SSD1306_WHITE);
      display->display();
  }

  display->drawBitmap(0,24,splash_bitmap_array[0],80,40,SSD1306_WHITE);
  display->setCursor(72, 16);
  display->println("Silly");
  display->display();
  delay(500);
  display->setCursor(72, 24);
  display->println("Space");
  display->display();
  delay(500);
  display->setCursor(72, 32);
  display->println("Invaders");
  display->display();
  delay(500);
  display->setCursor(72, 40);
  display->println("Dashboard");
  display->display();
  delay(500);

  for (int i=0;i<8;i++){
    display->clearDisplay();
    display->drawBitmap(0,24,splash_bitmap_array[i],80,40,SSD1306_WHITE);
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(72, 16);
    display->println("Silly");
    display->setCursor(72, 24);
    display->println("Space");
    display->setCursor(72, 32);
    display->println("Invaders");
    display->setCursor(72, 40);
    display->println("Dashboard");
    display->display();
    delay(500);
  }
  delay(3000);
  display_transition();
}

void legend()
{
  display->setTextWrap(false);//do not break text labels at the display edge
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  display->clearDisplay();
  display->setCursor(8, 0);
  String text = "_____________LEGEND_";
  for (unsigned int i=0;i<text.length();i++){
    display->print(text[i]);
    display->display();
    delay(50);
  }
  delay(1000);
  display->drawBitmap(0,16,invader_bitmap[0][0],16,8,SSD1306_WHITE);
  display->setCursor(16, 16);
  display->println("<-weak signal AP");
  display->display();
  delay(1000);
  display->drawBitmap(0,26,invader_bitmap[1][0],16,8,SSD1306_WHITE);
  display->setCursor(16, 26);
  display->println("<-medium signal AP");
  display->display();
  delay(1000);
  display->drawBitmap(0,36,invader_bitmap[2][0],16,8,SSD1306_WHITE);
  display->setCursor(16, 36);
  display->println("<-strong signal AP");
  display->display();
  delay(1000);
  display->drawBitmap(0,46,invader_bitmap[3][0],16,8,SSD1306_WHITE);
  display->setCursor(16, 46);
  display->println("<-free AP");
  display->display();
  delay(1000);
  display->drawBitmap(4,56,free_bitmap,8,8,SSD1306_WHITE);
  display->setCursor(16, 56);
  display->println("<-free AP pointer");
  display->display();
  delay(1000);
  display->display();
  display->setCursor(6, 8);
  display->println("Home<-AP name");
  display->display();
  delay(5000);
  display_transition();
}

void setup() {
  Serial.begin(9600);
  init_oled();
  WiFi.mode(WIFI_STA);//station mode
  iteration = SCAN_ITERATION;//set current iteration
  if(SHOW_INTRO)
    intro();//play intro scene
  if(SHOW_LEGEND)
    legend();//show usage legend
  scanNetworks();//perform initial scan 
}

void loop() {
  //periodically rescan networks
  if(++iteration==SCAN_ITERATION){
    scanNetworks();
  }
  update_oled();
  delay(100);//delay between animation frames
}
