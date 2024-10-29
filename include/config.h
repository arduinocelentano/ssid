#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1   // Reset pin (-1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // If not work please try 0x3D
// SDA and SCL might be connected to non-standard pins on some boarrds
#define OLED_SDA D5         // SDA pin - check out your board
#define OLED_SCL D6         // SCL pin - check out your board

#define RSSI_INTERVALS 5 //number of AP clusters, i.e. rows of characters
#define SCAN_ITERATION 100 //the number of iteration after which networks scan should be done
#define MAX_INVADERS 20 //maximum number of sprites on the screen

#define SHOW_FAKE_AP_NAMES false //set true if you want to hide real SSIDs
#define SHOW_INTRO true //show intro scene
#define SHOW_LEGEND true //show legend scene
