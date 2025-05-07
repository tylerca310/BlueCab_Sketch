
#include <ArduinoBLE.h>
#include <Arduino.h>
#include <Keypad.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>
//#include <ATT.h>  4/20/25 does not like this "No such file or directory"
//#include <NimBLEDevice.h>   // does not work

#define MAX_CONNECTIONS 6

//#include <Arduino_APDS9960.h>
// #include <BLEDevice.h>
// #include <BLEProperty.h>
// #include <BLEService.h>
// #include <BLECharacteristic.h>

//************************** TOP OF KEYPAD **************************************************
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns

char keys[ROWS][COLS] = {
  {'1','2','3','t'},  // THROTTLE
  {'4','5','6','f'},  // F1
  {'7','8','9','g'},  // F2
  {'r','0','e','l'}   // REVERSE, FORWARD, LOCO #
};

//  4/1/25 CONFIRMED THAT IT WORKS
//crossed wires
byte rowPins[ROWS] = {17, 16, 12, 14}; // kr1 white GPIO17, kr2 grey GPIO16, kr3 purple GPIO12, kr4 blue GPIO14
byte colPins[COLS] = {32, 27, 25, 26}; // kc1 brown GPIO32, kc2 red GPIO27, kc3 orange GPI25, kc4 yellow GPIO26

//Create an object of keypad
Keypad membrane_keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );// ORIG
//********************************** BOTTOM KEYPAD ********************************************

//-------------------------------declare LCD constructor-----------------------------------
//U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 18, /* data=*/ 23, /* CS=*/ 5, /* reset=*/ 22);
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* LCD E purple GPIO18 (spi clock) */18, /* LCD R/W Green GPIO 23 (SPI MOSI) */23, /* LCD RS White GPIO 5 (SPI CS) */5, /* LCD RST Blue GPIO22 */22); //BLK, PSB, GND -Black - GND VCC Red 5V BLA Orange 3.3V
//-------------------------------bottom LCD display-----------------------------------
//------------------------------ SPEED COMMAND for PRIMARY LOCO NUMBER--------------------
//  5 hex pairs required for command 
//  element 1 never changes
//  elements 2 never changes 
//  element 3 is the loco number - 7/12/24 allowable numbers 1-127 default loco 3
//  element 5 is actual speed value
//  6/16/24 speed commands  
byte speed[] = {0x02,0x03,0x03,0x3F,0x00 }; // only need 5 bytes. the base command defaults to off

//------------------------------SPEED COMMAND FOR EXTENDED LOCO NUMBER---------------------
//  7/16/24 extended speed commands >= 128 to <= 9999 
//  6 hex pairs required for command 
//  element 1 never changes
//  elements 2 never changes 
//  element 3 is the loco number for left hex pair for default loco #128
//  element 4 is the loco number for right hex pair
//  element 5 is actual speed value
byte speed_e[] = {0x02,0x04,0xC0,0x80,0x3F,0x00 }; // only need 6 bytes. the base command defaults to off

//  6/16/24 PRIMARY ADDRESS: these are legacy commands and for information only
//byte stop[] =     {0x02,0x03,0x03,0x3F};//,0x01,0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x43,0xFF,0xFF }; // 5/14/24
//byte speed1_r[] = {0x02,0x03,0x03,0x3F,0x02,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
//byte speed1_f[] = {0x02,0x03,0x03,0x3F,0x82,0x03,0x00,0x00,0x30,0xDA,0x6E,0x04,0x01,0x00,0x00,0x00,0x01,0x00 };
//byte speed1_f[] = {0X02,0x03,0x03,0x3F,0x82};//,0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x43,0xFF,0xFF };// 5/14/24
//byte speed1_r[] = {0X02,0x03,0x03,0x3F,0x02};//,0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x43,0xFF,0xFF };// 5/14/24
//byte speed2_f[] = {0x02,0x03,0x03,0x3F,0x83 };//,0x03,0x00,0x00,0x30,0xDA,0x6E,0x04,0x01,0x00,0x00,0x00,0x02,0x00 };
//byte speed2_r[] = {0x02,0x03,0x03,0x3F,0x03};//,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
//byte speed4_f[] = {0x02,0x03,0x03,0x3F,0x85};//,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
//byte speed4_r[] = {0x02,0x03,0x03,0x3F,0x05,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
//byte speed7_f[] = {0x02,0x03,0x03,0x3F,0x88,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
//byte speed7_r[] = {0x02,0x03,0x03,0x3F,0x08,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

//---------------------------- FUNCTION GROUP 1 -PRIMARY ADDRESS--------------------------------------
//  this is the base function where elements 3 and 4 are modified on the fly.
//  elements 1 & 2 are fixed 
//  element 3 is the loco number and by default is set to #3. 
//  element 4 is the function hex command and by default is set to 80, which is all functions are off
byte function_group_1[] = {0x02,0x02,0x03,0x80 }; //  blunami only need 4 hex pairs
//byte function_group_1[] = {0x02,0x02,0x03,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x43,0xFF,0xFF };  //  OFF

/*  these are legacy commands and are not used (5/31/24)
byte lighton[] =  {0x02,0x02,0x03,0x90,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };//5/22/24 F0
byte lighton_f[] =  {0x02,0x02,0x03,0x60,0x60,0x00,0x00,0x00,0xA0,0x0C,0x16,0x6B,0x01,0x00,0x00,0x00,0x31,0x00 };
byte lighton_r[] =  {0x02,0x02,0x03,0x40,0x40,0x00,0x00,0x00,0xA0,0x0C,0x16,0x6B,0x01,0x00,0x00,0x00,0x31,0x00 };
byte lightoff[] = {0x02,0x02,0x03,0x84,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4F,0x99 };
byte bellon[] = {0x02,0x02,0x03,0x81,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00 }; //F1
byte belloff[] = {0x02,0x02,0x03,0x80,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00 };  //using nmra binar
byte longhornon[] = {0x02,0x02,0x03,0x82,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00 }; //  F2
byte longhornoff[] = {0x02,0x02,0x03,0x80,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00 };
byte shorthornon[] = {0x02,0x02,0x03,0x84,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00 };  //  F3
byte shorthornoff[] = {0x02,0x02,0x03,0x80,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00 };
byte beacon_on[] = {0x02,0x02,0x03,0x88,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00 };  // F4 BEACON
*/
//---------------------------- FUNCTION GROUP 1 -EXTENDED ADDRESS--------------------------------------
//  7/16/24
//  this is the base function where elements 3, 4 and 5 are modified on the fly.
//  elements 1 & 2 are fixed 
//  element 3 is the loco number LEFT HEX, default is set to #128.
//  element 4 is the loco number RIGHT HEX, default is set to #128.
//  element 5 is the function hex command and by default is set to 80, which is all functions are off
//  7-16-24 appears that the function commands are the same as PRIMARY
byte function_group_1_e[] = {0x02,0x03,0xC0,0x80,0x80 }; //  blunami only need 5 hex pairs

// ---------------------------FUNCTION GROUP 2A -PRIMARY ADDRESS---------------------------------
//  element values are the same as function group 1
byte function_group_2a[] = {0x02,0x02,0x03,0xB0 };  // intialized to off
//byte function_group_2a[] = {0x02,0x02,0x03,0xB0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x43,0xFF,0xFF }; //  OFF
//byte mute[] = {0x02,0x02,0x03,0xB8,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00};  //  F8
//byte unmute[] = {0x02,0x02,0x03,0xB0,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00};

// ---------------------------FUNCTION GROUP 2A -EXTENDED ADDRESS--------------------------------
//  7/16/24
//  element values are the same as function group 2A
//  element values are the same as function group 1 EXTENDED
//  element 3 is the loco number LEFT HEX, default is set to #128.
//  element 4 is the loco number RIGHT HEX, default is set to #128.
//  element 5 is the command
byte function_group_2a_e[] = {0x02,0x03,0xC0,0x80,0xB0 };  // intialized to off

//---------------------------- FUNCTION GROUP 2B --PRIMARY---------------------------------------
//  element values are the same as function group 1
byte function_group_2b[] = {0x02,0x02,0x03,0xA0 };  // intialized to off
//byte function_group_2b[] = {0x02,0x02,0x03,0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x43,0xFF,0xFF };

//---------------------------- FUNCTION GROUP 2B --EXTENDED---------------------------------------
//  7/16/24
//  element values are the same as function group 2B
//  element 3 is the loco number LEFT HEX, default is set to #128.
//  element 4 is the loco number RIGHT HEX, default is set to #128.
byte function_group_2b_e[] = {0x02,0x03,0xC0,0x80,0xA0 };  // intialized to off

//----------------------------- FUNCTION GROUP 3 ----PRIMARY-------------------------------------
// this function was created to change the direction lights at speed zero
// it uses the same hex structure as the other functions, i.e. 020203xx
// However the actual 4th element hex byte does not use the 1st 3 digits to identify which function it is.
// (ie 100, 101). This byte starts with a 0 (01100000, 01000000).
// by default it is set to forward 0x60
byte function_group_3[] = {0x02,0x02,0x03,0x60 }; //  forward-reverse light. initialized to forward

//----------------------------- FUNCTION GROUP 3 --EXTENDED-ADDRESS---------------------------------
//  7/16/24
//  similar to the other functions where 2 hex pairs are used for the loco address
//  elements 3 & 4 are loco number
byte function_group_3_e[] = {0x02,0x03,0xC0,0x80,0x60 }; //  forward-reverse light. initialized to forward

//-----------------------------RAIL-MOTOR-VOLTAGE----------------------------------------------------
//  this is a read only characteristic
//  when this is sent, it produces the correct value when it is read
//  refer to rail-voltage FUNCTION
byte rail_mtr_volts[] = {0x0A,0x01,0x01,0x00,0x00,0x00,0x00 };  // sending this command activates the correct volts

//------------------------------CV COMMANDS--------------------------------------------------------
//  this byte is used to send DCC commands so that CV29, CV 1, CV 17, & CV18 values can be discovered
//  in order to detemine/calculate the loco number for the decoder that is being quiered.
//  command is sent to mr. blue and then immediately read to obtain the values.
//  from CV 29 we can determine if loco # is primary or extended
byte cv_29[] = {0x03,0x04,0x00,0x74,0x1D,0x01 };  //  CV 29 - configuration data 1
byte cv_1[] =  {0x03,0x04,0x00,0x74,0x01,0x01 };  //  CV 1 - primary loco #
byte cv_17[] =  {0x03,0x04,0x00,0x74,0x11,0x01 };  // CV 17 - extended loco # left hex pair
byte cv_18[] =  {0x03,0x04,0x00,0x74,0x12,0x01 };  // CV 18 - extended loco # right hex pair

//---------------------------------- DCC START BYTE ----------------------------------
// after connection, this value is in the dcc characteristic. The same value also appears when sniffing
// have no idea what it does. maybe when read, it makes the bluanami happy.
//  this is the value 01 10 53 6F 75 6E 64 54 72 61 78 78 20 42 6C 75 6E 61
// 12/3/24 the byte name is blue_dcc and it contains 18 hex pairs
byte blue_dcc[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

//--------------------------------- BOTTOM DCC START BYTE ------------------------------------------
//----------------------------------BOTTOM OF SPEED AND FUNCTION COMMAND---------------------------------------

//-----------------------------------------------------------------------------------------------
BLEDevice Blunami;
BLEDevice Blucab_Blunami[6];
BLEDevice central;    //  1/14/25 not sure why we did this
BLECharacteristic BlunamiCommand[6]; //  defines main commands, store in array
BLECharacteristic motorvoltage[6];   //  defines motor voltage, store in array

String Blunami_UUID = "F688FD00-DA9A-2E8F-D003-EBCDCC259AF9";

int max_scans = 100;

int in=0;                 // tracks if blunami has been found and already in list
int i = 0;                //  tracks the index of the matrix
int max_blunamis = 6;     // max devices
int num_blunamis = 0;     //  number of available blunamis
int num_blunamis_connect = 0; // this tracks the number of blunamis that are connected
char num_blunamis_char;   //  char represenation of the "int num_blunamis"

//--------------------------------TOP OF SURVEY STRUCT #1--------------------------
//  this is the struct that stores the blunamis found that are available
struct Blunami_matrix
{ // open struct
  int Blunami_index;
  String Blunami_address;  // peripheral device address
  String Blunami_name;     //  this is the name that we want
} ; // close struct
//  create structure variable
Blunami_matrix Blunamis[6]; // Blunamis[] is a struc array (matrix) of int Blunami_index, 
                            //  String Blunami_address, String Blunami_local-name and String Blunami_name
//--------------------------------BOTTOM SURVEY STRUCT #1------------------------------
//--------------------------------  TOP SORTED CONNECT STRUCT #2-------------------------------
//  this struct resorts the data using the selection order.
struct Blunami_matrix2
{ // open struct
  int Blunami_index2;
  String Blunami_address2;  // peripheral device address
  String Blunami_name2;     //  this is the name that we want
} ; // close struct
//  create structure variable
Blunami_matrix2 Blunamis_sorted[6];
//------------------------------ BOTTOM SORTED STRUCT #2-----------------------------------
//
int Blunami_attributes[6] = {0,0,0,0,0,0}; //  array to store the individual blunami attributes
int Blunami_services[6] = {0,0,0,0,0,0};   //  array to store the individual blunami services
String(Blunami_Address[6]);   //  string array to store the individual address
String(Blunami_name[6]);      //  string array to store the individual name
int Blunami_index[6];       //  int array to store index
int loco_number[6] = {3,3,3,3,3,3};   //  int array to store the individual loco numbers
                      //  7/12/24 primary loco numbers 1-127. decoder default is 3 from soundtraxx.
                      // 7/16/24 extended loco numbers >128-9999. 
int line;
int rssi;

//  keyboard inputs
char t;                         //  throttle command
char f;                         //  function  group 1
char g;                         //  function  group 2
char l;                         //  loco number 
char d;                         //  direction command
char activecommand = ' ';     //  if keypress is t, f, g, r, , e, l store it.  
char activecommand_previous = ' ';
char speed_value = '0';         //  0-9 number entered for speed. 
char old_speed_value = '0';     //  stores previous speed command 
char function_value = ' ';      //  0-9 number entered for function

//  FUNCTION GROUP 1 VARIABLES
String f0 = "1";              //  function 0 - headlight - turn on at startup?
String f1 = "0";              //  function 1 - bell
String f2 = "0";              //  function 2 - airhorn -long      
String f3 = "0";              //  function 3 - airhorn  - short horn
String f4 = "0";              //  function 4 - loco beacon  
String f5 = "0";              //  function 5 - battery off
String f6 = "0";              //  function 6 - FX3 - not connected
String f7 = "0";              //  function 7 - dimlight
String f8 = "0";              //  function 8 - mute
String f9 = "0";              //  function 9 - grade crossing
String f10 = "0";             //  not configured in initial BlueCabtm -7/1/24
String f11 = "0";             //  not configured in initial BlueCabtm - 7/1/24
String f12 = "0";             //  not configured in initial BlueCabtm - 7/1/24

char keypress = ' ';          //  this stores the keypress character
char currentkeypress = ' ';   //  if one of the functons keys is pressed then store that value
char activekeypress = ' ';    //  as the name implies
char oldkeypress = ' ';       //  save value of current keypress
char loco_direction = '1';    //  intialize as forward. 0 for reverse
char value;                   //  stores the 0-9 keypress for functions      

//  --------------------multi loco connect variables--------------------------------  
int connect_keypress;
String upperlimit;        //  used in converting the blunami int index into a character 
char avail_blunamis [2];  //   char array that tracks the number of blunamis.
int case1_selected = 0;   //  in select function, this monitors if case 1 has been entered
int case2_selected = 0;   //  in select function, this monitors if case 2 has been entered
int case3_selected = 0;   //  in select function, this monitors if case 3 has been entered
int case4_selected = 0;   //  in select function, this monitors if case 4 has been entered
int case5_selected = 0;   //  in select function, this monitors if case 4 has been entered
int case6_selected = 0;   //  in select function, this monitors if case 4 has been entered
int connect_order[6] = {0,0,0,0,0,0};   //  array to track the loco selection order
int blunami_connected[6] = {0,0,0,0,0,0};   //  integer array that tracks the connected status of each blunami
                                            //  only allows a maximum of 6 connected locos. 
//-------------------------------------------------------------------------------
int key_id = 1;         //  this identifies which number key is pressed during selection process
int num_keypress=0;
int first_keypress = 0; //  this tracks the 1st time that a keypress ocurs. used for selecting a loco
int order = 0;          // this is the order of the loco selected
int attribute_lap=0;    // tracks number of tries to discover attributes

// define millis() parameters ---
unsigned long startMillis;
unsigned long tickle_millis; 
unsigned long minute_millis;
unsigned long volt_millis;
unsigned long return_millis;
unsigned long setup_millis;
unsigned long startmillis;

//  millis delays
unsigned long setup_delay = 30;      //  used in void setup  (orig was 25)
int while_blunami_delay = 45;
int speed_delay = 0;                //  time between consecutive speed commands
int function_delay = 0;
unsigned long command_delay = 12;   //  gives mr blue time to react
int attribute_delay = 15;           //  used during identifying attributes (start 20)
int cv_delay = 100;                 //  delay during reading and writing CV values
int motor_start_delay = 5;          //  at times when motor is 1st energized there is a delay. see if this helps

//  keypad parameters
int debounce_delay = 25; // this was trial and error. used in lcd test (orig 20)  1/6/25 previous 50, 90
int keypad_delay = debounce_delay+5; // make this slightly larger than debouce
int keypad_hold = 0;

int lap =0;
int motor_start = 0;    //  this tracks the motor state. used to help motors start in unison the 1st time
int k;  // used globally in the peripheral and connect FOR loops
int connect_attempt=1;    //  this tracks the number of times we have tried to connect
float loop_lap = 0;       //  laps in the void loop
int loco_index = 0;
int previous_loco_index = 0;  //  when direction this becomes the opposite of Loco_index

int actual_blunami_connected = 0;   //  this tracks the actual number of blunamis connected 
int reconnect=0;    //  this is updated during the disconnect event handler 
//---------------------------- bottom multi loco connect variables -----------------------------

//----------------function command byte for all loco # ------------------------------------
String hex_command = ("10000000");      // required in the hex calculator function. not used in sketch
// at connect, the light is turned on so fg1_current_command = 10010000
byte fg1_current_command = 0b10010000;  // function group 1 current binary byte command (f0-f4)
//byte fg1_old_command = 0b10000000;      // this is the previous command of fg1 current
byte fg1_old_command = 0b10010000;      // this is the previous command of fg1 current
//byte fg1_current_command = 0b10000000;  // light off at start up
byte fg2a_current_command = 0b10110000; // function group 2a current binary byte command (f5-f8)
byte fg2a_old_command = 0b10110000;     // this is the previous command of fg2a current

byte fg2b_current_command = 0b10100000; // function group 2b current binary byte command (f9-f12)
byte fg2b_old_command = 0b10100000;     // this is previous command of fg2b current

//----------------bottom function command byte-------------------------------------------
//----------------extended loco number variables-------------------------------------------
int loco_hex_pair_length;
int loco_left_hex_pair;  //  this is the integer value of left hex pair
int loco_right_hex_pair; //  this is the integer value of right hex pair
String loco_hex_pair;
String loco_hex_pair_4digits;
String left_hex_pair_edited;

String b0;    //  bit 0  of right hex pair
String b1;    //  bit 1  of right hex pair
String b2;    //  bit 2  of left hex pair
String b3;    //  this is the left most bit that is replaced with bluerail's value
int bit0;
int bit1;
int bit2;
int bit3;
//-------------------bottom extended loco number------------------------------------

int speed_step = 0;  //  speed step is defined in speed routine

float decoder_voltage[6] = {0,0,0,0,0,0};    //  array to store values, this is read from blunami
                            //  if value below 12, assume battery power, then F5 does not latch

// these are used in the  IF CONNECTED AND WHILE CONNECTED. 
// in the statement, the Blucab_Blunami[0].connected, the index (in square brackets) is saved as a variable and is
// updated depending upon the number connected. 
int blue_1;   //  blunami #1 selected to be connected 1st
int blue_2;   //  blunami #2
int blue_3;   //  blunami #3
int blue_4;
int blue_5;
int blue_6;
       
//****************************************Top setup()***********************************************
void setup()
{ // open setup void

#undef CONFIG_BTDM_CONTROLLER_BLE_MAX_CONN    //  4/20/25 DOES NOT WORK
#define CONFIG_BTDM_CONTROLLER_BLE_MAX_CONN 6


    // Start Serial/UART
  Serial.begin(115200);
  while (!Serial);     // comment out if mr nano powered by battery, which will be the norm
 
  u8g2.begin();   //  intialize LCD display
  u8g2.clearBuffer(); // clear lcd screen

        u8g2.setFont(u8g2_font_micro_tr);
  //              (col,row)
        u8g2.setCursor(37,6);
        u8g2.print("BlueCab Setup");  //  display for 5 sec
        u8g2.sendBuffer();    //  this sends the commands to the LCD

  membrane_keypad.setDebounceTime(debounce_delay);  //  keypad debounce function, in keypad library
  membrane_keypad.setHoldTime(keypad_hold);

// start BLE
  if (!BLE.begin()) { // open if !BLE
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  } // Close if !BLE

//  11/24/24 added these out of a whim
    BLE.setAppearance(0xFC94);     //  for generic Apple device
//  BLE.setAppearance(0xFCA0);     //  for generic Apple device
//  BLE.setAppearance(0xFCB2);     //  for generic Apple device
//  BLEDescriptor DCCLabelDescriptor("2901", "DCC");
//  BLEDescriptor RailVltsLabelDescriptor("2901", "RailVlts");

  BLEService BLUNAMI("F688FD00-DA9A-2E8F-D003-EBCDCC259AF9"); //  define blunami service
//  BLEService DCC("F688FD00-DA9A-2E8F-D003-EBCDCC259AF9");   //  DCC, not sure how to do it
//  BLEService RailVlts("F688FD00-DA9A-2E8F-D003-EBCDCC259AF9");    // RAIL VOLT, not sure how to do it
//  BLECharCharacteristic("F688FD14-DA9A-2E8F-D003-EBCDCC259AF9", BLEBroadcast);    //  RailVlts
//  BLECharCharacteristic("F688FD1D-DA9A-2E8F-D003-EBCDCC259AF9", BLEBroadcast);    //  DCC

//---------------------------- bottom of whim--------------------------------

  BLE.setLocalName("BLUECAB");  
  startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command
  
  //BLE.central();
  BLEDevice central = BLE.central();

  BLE.setConnectionInterval(0x0006, 0x0200);   //  in units of 1.25ms. 10 - 30  //  4/18/25 try this makes no difference
//  BLE.setConnectionInterval(0x0006, 0x0012);   //  in units of 1.25ms. 10 - 30  //  disabled 4/18/25
//  4 = 0x0004, 24 = 0x0018 must use hex  min 5(4), max 35(28)
//  11/12/24 set interval from 9(7) - 11 (9) target is 10 per sniffing
//  11/14/24 apple wants min >=15 and max <= min+15 or min 15(12), max 30(24)*****
//  11/15/24 try 08 to 80 - does not like
//  11/21/24 try 4 to 18 - did not seem to like this
//  11/21/24 0012 to 0024 - range between sniffed nano and bluerail. seems to work the best****
//  11/22/24 disabling makes it worse, it appears.
//  11/22/24 try narrow band 8-12 - can't tell if it is any better. takes longer to connect
//  11/22/24 changed 6-24 seems to work the same as 12-24.
//  11/26/24 TRY SETTING TO SPECS OF 7.5(6) TO 4000 (3200)  SEEMS TO WORK  BETTER
//  11/26/24  sniffing shows nano connect interval = 6(7.5). Bluerail 24(30). set range 6(7.5)-26(32.5)
//  11/29/24 messed around and 6-9 seems to work the best

BLE.poll();   //  11/30/24 not sure if this will work here since we are doing stuff in setup
  
//  event handler seems distracting. don't know where connect/disconnect occurs. may revisit
//  but it does confirm the connection!!!
BLE.setEventHandler(BLEConnected, connecthandler);
BLE.setEventHandler(BLEDisconnected, disconnecthandler);
//---------------------------------------- LCD TITLE PAGE---------------------------------------
//  LCD display title page
  lcd_title_page();   //  display for 5 sec
  startMillis = millis(); while (millis() <= (startMillis + 5000)) {} //  delay command 

  //  begin displaying setup steps on LCD
        u8g2.clearBuffer(); //  must clear 1st 
        u8g2.setFont(u8g2_font_micro_tr);
  //              (col,row)
        u8g2.setCursor(37,6);
        u8g2.print("BlueCab Setup");  //  display for 5 sec
        u8g2.sendBuffer();    //  this sends the commands to the LCD

  startMillis = millis(); while (millis() <= (startMillis + 5000)) {} //  delay command 
 
//---------------------------------- BOTTOM LCD TITLE---------------------------------------------

//********************************* SURVEY FOR AVAILABLE BLUNAMIS **************************************
Serial.println("\t\tScanning for Locomotives..");   //  to serial monitor
 //  begin displaying setup steps on LCD
//  alert on LCD that the scanning has started by calling function
lcd_loco_scanning();    //  lcd command to display "scanning"

blunami_survey();                 //   <-------------search for blunamis (line 660)   
//******************************** BOTTOM SEARCH FOR AVAILABLE BLUNAMIS ***********************************

//****************************** SURVEY OF AVAILABLE BLUNAMI ********************************************
blunami_list();                   //  <--------------display blunami search (line 725)

//  this lists the number of available blunamis and is saved globally in "int num_blunamis"
//  the int num_blunamis has to be converted to a character for the selection
//  upperlimit = String(num_blunamis);  //  convert integer to string
//  upperlimit.toCharArray(avail_blunamis,2);   //  convert char array to a character
//***************************** BOTTOM LIST OF AVAILABLE BLUNAMI*****************************************

//******************************* SELECT THE BLUNAMIS TO CONNECT*************************************
blunami_select();           //    <--------------------- select locomotive (line 765) 
//****************************** BOTTOM SELECT BLUNAMIS TO CONNECT**********************************

//********************************* TOP find_peripheral() ********************************
find_peripheral();          //    <----------------------- find peripherals (line 925)
//  THIS FUNCTION STARTS THE CONNECTION PROCESS
//  inside of find_peripheral, we call the connect functon, "Blunami_connect();" 
//  inside of "Blunami_connect();, we connect and discover the attributes and characteristics
//*********************************BOTTOM find_peripheral() ******************************

//  if here we should be connected to all. lets check
k=num_blunamis_connect-1; //  reset k
//connection_status_all(440,k);
startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command 
//************************************** UPDATE BLUNAMI_CONNECTED ARRAY*************************
// the int "blunami_connected" array has to be updated for all potential 6 connections
//  the array is updated as the blunamis are connected and disconnected. 

//  calculate the actual number of blunamis connected
for (int q=0;q<=num_blunamis_connect-1;q++ ){
  actual_blunami_connected = actual_blunami_connected+blunami_connected[q];
}
//Serial.println();
Serial.print("\t\t(@450) actual blunamis online:  ");Serial.println(actual_blunami_connected);
//Serial.print("\t\t(@450) num blunami connect: ");Serial.println(num_blunamis_connect);
//connected_status(430);  //  print out connection and updated array "blunami_connected[k]""
k=num_blunamis_connect-1; //  reset k

connection_status_all(460,k);
startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command
//connected_status(198);  //  print out connection 

//*************************************BOTTOM BLUNAMI_CONNECTED ARRAY**************************************
//*********************************** BUILD IF and WHILE CONDITION ************************************
// we need to figure out how to make a dynamic IF and WHILE CONNECTED that will change depending upon the number
// of blunamis online. While not elegant, I came up with this solution. The limit of connected blunamis will be 6
// if one blunami is online the index k = 0; 2 online k = 0,1,  3 online k = 0,1,2, etc. 
// the IF and WHILE contains the following tests:
// Blucab_Blunami[blue_1].connected()&&Blucab_Blunami[blue_2].connected()&&Blucab_Blunami[blue_3].connected()
// &&Blucab_Blunami[blue_4].connected()&&Blucab_Blunami[blue_5].connected()&&Blucab_Blunami[blue_6].connected()
//So we will use a switch case to accomplish.

switch (actual_blunami_connected) {
      case 1:   //  1 connected. (set all the non connected ones to 0)
        blue_1 = 0;blue_2=0;blue_3=0;blue_4=0;blue_5=0;blue_6=0;
        break;
      case 2:   //  2 connected
        blue_1 = 0;blue_2=1;blue_3=0;blue_4=0;blue_5=0;blue_6=0;
        break;
      case 3:   //  3 connected
        blue_1 = 0;blue_2=1;blue_3=2;blue_4=0;blue_5=0;blue_6=0;
        break;
      case 4:   //  5 connected
        blue_1 = 0;blue_2=1;blue_3=2;blue_4=3;blue_5=0;blue_6=0;
        break;
      case 5:   //  4 connected
        blue_1 = 0;blue_2=1;blue_3=2;blue_4=3;blue_5=4;blue_6=0;
        break;
      case 6:   //  4 connected
        blue_1 = 0;blue_2=1;blue_3=2;blue_4=3;blue_5=4;blue_6=5;
        break;
    }
//************************************* BOTTOM IF and WHILE CONDITION ************************************
//********************************** top turn off trailing unit light*************************
//  turn off trailing units light if there is a consist
    function_group_3[2] = loco_number[0];        //  update lead loco number which is index 0
  //loco_extended_address();    //  calculate extended number hex pairs  removed 1/17/25
    function_group_3_e[2] = loco_left_hex_pair;    // extended left loco #     1/17/25 this needs to be used
    function_group_3_e[3] = loco_right_hex_pair;   // xtended right loco #     1/17/25 this needs to be used

//    delay for 2 sec before we turn the lights off
    startMillis = millis(); while (millis() <= (startMillis + 2000)) {} //  delay command 
    //  only turn trailing units off if we have more than 1 loco
    if(actual_blunami_connected >=2 ){    
      trailing_unit_functions_2();
    }

//**************************************** bottom top turn off trailing unit light*************************
//********************************LCD DISPLAY LOCOMOTIVE CONNECTION SUMMARY *******************************
// after the locos have been connected, display the results on the LCD
// this will alsbe accessed through menu commands

    lcd_loco_summary_page();

//******************************* BOTTOM LCD DISPLAY LOCOMOTIVE CONNECTION SUMMARY *******************************

}  // bottom  setup void
//***********************************Bottom setup()*******************************************************
//
//***********************************Top loop()*****************************************************

void loop()
{     // top void loop
  BLE.poll();   //  this polls for connect and disconnect events
//-------------------------------- top if connected ------------------------------------  
    if(Blucab_Blunami[blue_1].connected()&&Blucab_Blunami[blue_2].connected()&&Blucab_Blunami[blue_3].connected()
        &&Blucab_Blunami[blue_4].connected()&&Blucab_Blunami[blue_5].connected()&&Blucab_Blunami[blue_6].connected())
    {   // ------- top of if connected -----------------------------------
      //  cell these again even though it was done in the connect function. probably not needed
      for(int l=0;l<=actual_blunami_connected-1;l++) {    //  top for cv      
        loco_number[l] = cv_loco_number(l); //  call this function to read the CV numbers and calculate the loco number
        startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command
        start_rail_volts();
        startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command
        decoder_voltage[l] = rail_voltage(l);   
      }     // bottom for cv
  
//--------disable if on battery power--------------------------------
      Serial.println();
      Serial.println("Press a keypad function to start");
      Serial.println();
      Serial.println();
      Serial.println();

//      lcd_display();        // display LCD main screen. 3/6/25 disabled
//      lcd_loco_summary_page();

  //  initialize delays    
      minute_millis=millis();
      tickle_millis = millis();
      volt_millis = millis();
//*********************************** TOP WHILE CONNECTED ************************************
//  THIS IS WHERE ALL COMMANDS ARE EXECUTED
      while(Blucab_Blunami[blue_1].connected()&&Blucab_Blunami[blue_2].connected()&&Blucab_Blunami[blue_3].connected()
        &&Blucab_Blunami[blue_4].connected()&&Blucab_Blunami[blue_5].connected()&&Blucab_Blunami[blue_6].connected())
      {    // ---------------- TOP OF WHILE CONNECTED LOOP --------------------------
          BLE.poll();    // not sure this is needed here. put back in 1/4/25

//        this function records and stores the keyboard keystrokes
//        and then creates all function group (1, 2a, 2b) command bytes
//        all commands are global
//        keypressfunction();      //  this uses the computer keyboard for input (line 357)
          keypad_keypress();      // call keypad keypress function  (line 1872)

  //      delay slightly to let keypad catch up.  1/17/25 appears to make no difference
          startMillis = millis(); while (millis() <= (startMillis + keypad_delay)) {} //  delay command    

//------------------------------- top main keypad commands -----------------------------------
  //      these are the main commands     
  //      send direction command, forward (E) or reverse (R) (line 1672)    

          switch (activecommand) {    //  switch case active command
              case 'e':    //  reverse
                direction_command();
                activecommand = 't';  // reset active command to prevent infinite loop
              break;
              case 'f':   //  function binary byte
                function_command_byte();  //  this builds the binary function byte                
                function_command();       //  execute function for each blunami   (1600)
              break;
              case 'l':   //  locomotive - scan, available, list, connect, disconnect
                //  3/6/25 this displays the loco connected and the voltages on the LCD
                //  going forward we may do other things
              //  lcd_loco_summary_page();
              break;
              case 'r':   //  forward
                direction_command();
                activecommand = 't';  // reset active command to prevent infinite loop
              break;
              case 't':   //  throttle
                  speed_command(); // call speed function 
              break;
          }   //  bottom switch case   active command
//---------------------- bottom keypad commands -------------------------------

//-------------------update screen and LCD-after function calls.------
  //        only process commands if there is a key press. 
            if (membrane_keypad.getState()==PRESSED && keypress != NO_KEY){ 
  
               Serial.print ("\t\t\t\t  active command(@580):  ");
               Serial.println(activecommand);
               Serial.print ("\t\t\t\t  key press(@585):  ");
               Serial.println(keypress);

        //     Blunami.connected(); //  update connection for display

               lcd_display();        //  call the LCD output (2110)
//            this function prints the results of keypress function()    
               keypress_results();    // update output (line 388)

            } //  bottom of NO key.
 //------------------bottom update screen and LCD-after function calls.------
 
  //      12/3/24 every 10 sec we want to update the voltages. we did this in previous sketches
          if (millis() >= (volt_millis +10000)) {    //   this tickles blunami every 10 sec   
            for(int l=0;l<=actual_blunami_connected-1;l++){
                start_rail_volts();
                startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command         
                decoder_voltage[l] = rail_voltage(l);      
            }
            volt_millis = millis();
          }

      }//************************* bottom while connected ****************************

  } // ****************** bottom if connected************************************

  else{   //  IF HERE not connected so start over
    Serial.println("\t\tBlunamis disconnected at 600");
    Serial.println("\t\tReconnecting...");
    //k=k-1;    //  reset index
    //Serial.print("\t\tK:  ");Serial.println(k);
    
  //  BLE.scanForUuid(Blunami_UUID);
  //  startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command 
  //  Blunami = BLE.available();
    startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command 
  //  find_peripheral();
  //  Blunami_connect(k);
  //find_peripheral(); // 1/16/25 turn off trying to reconnect
  
  }   //  bottom of else connected
  
} // bottom void loop
//***************************************Bottom VOID loop()****************************************************

//*************************** TOP BLUNAMI SURVEY****************************************************
void blunami_survey()
  { // open Blunami_survey
//  this function surveys for available blunamis
//  this needs to scan continuosly as is the case with bluerail until locos are found
  
    int j;
    int i;
    int in;

survey_label:

// max scan for Blunamis
  for (j = 1; j <= max_scans; j++)
    {// open for j<= max_scans
    in = 0;
//  Serial.print("  Blunami_survey() lap ");
//  Serial.println(j);

    BLE.scanForUuid(Blunami_UUID);
    Blunami = BLE.available();
    
//  12-16-24 this delay is very important without it does not find the locos
//  delay started at 500ms and progressively decreased it to 12ms. seems to work but
//  increased it to 20ms to be safe.
    startMillis = millis(); while (millis() <= (startMillis + 20)) {} //  delay command

//  using a while loop instead of "IF" exits the function so it is no good

    if(Blunami.address() != "00:00:00:00:00:00") // if there is a good address
      { // open if good address

      for (i=0; i<=(num_blunamis); i++)
        {  //open for i
        if (Blunami.address() == Blunamis[i-1].Blunami_address)
          {  // open if address
          in = 1;  // already in matrix
          //  Serial.print(Blunamis[i-1].Blunami_address);
          //  Serial.print(" Is already in the Blunami matrix");
          }  // close if address
        }  //close for i

      if (in==0)
        {  //open if in == 0 --> not in
    //    Serial.print(Blunami.address());
    //    Serial.print(" is not in the Blunami matrix");

          num_blunamis = num_blunamis + 1; // increment the Blunami index
        // Add into the matrix
          Blunamis[num_blunamis - 1].Blunami_index = num_blunamis;         
          Blunamis[num_blunamis - 1].Blunami_name = Blunami.localName();   //  this is the name that we want
          Blunamis[num_blunamis - 1].Blunami_address = Blunami.address(); 
        } // close if in
      } // close if good address


    } // for j<= max_scans

  if (num_blunamis==0){goto survey_label;}   //  restart scan if no blunamis

  BLE.stopScan();   // stop scanning after finding blunamis

  } // close Blunami_survey
//************************************BOTTOM BLUNAMI SURVEY**********************************************

//********************************TOP blunami_list ********************************************
void blunami_list()
{ // top blunami lste
  //  this functions prints out the available blumamis
  int i;
  //clearscreen();
  Serial.println();
  Serial.println("\t\tAvailable Locomotives");
  Serial.println("\t\tNo.\tName\t\tAddress\t\t\tIndex");

    for (i = 0; i <= (num_blunamis - 1); i++)
    { // open for i
        Serial.print("\t\t");
        Serial.print(Blunamis[i].Blunami_index);
        Serial.print("\t");
        Serial.print(Blunamis[i].Blunami_name); //  THIS IS ALL WE CARE ABOUT
        Serial.print("\t");
        Serial.print(Blunamis[i].Blunami_address);Serial.print("\t  ");
        Serial.println(i);
      //  Serial.println();
    } // close for i
    num_blunamis = i;
  //  num_blunamis_char = num_blunamis; //  convert to character for switch statement
  //  Serial.println();
    //Serial.print("\t\t# of locomotives (@474): ");
    //Serial.println(num_blunamis);  

    // this lists the number of available blunamis and is saved globally in int num_blunamis
    //  the int num_blunamis has to be converted to a character
    upperlimit = String(num_blunamis);  //  convert integer to string
    upperlimit.toCharArray(avail_blunamis,2);   //  convert char array to a character
    Serial.println("\t\t----------------------------------------------------------------");
    Serial.println("\t\tSelect Locomotive(s) - Press Number, then");
    Serial.println("\t\t\t\t       Press [F] to connect, [R] to rescan");
    Serial.println("\t\t----------------------------------------------------------------");
    //Serial.println();

// display available locos on LCD
  lcd_loco_list();
// display LCD instruction footer
  lcd_footer();

} // bottom blunami_list
//*********************************** BOTTOM BLUNAMI_LIST *******************************************************
//********************************* BLUNAMI_SELECT **********************************************
void blunami_select(){ 

//  the available locos have already been listed in a left side table. 
//  here we just select which ones we want to connect
//  to and then display the results in a right side table.
int f_keypress = 0;    // this tracks the number of times that the "F" key is pressed
int order;
  while(true){  //  infinite loop
//while (keypress !='e'||keypress !='r'){  //  loop until "f" is pressed
    //  r = reverse e = forward on keypad
//    delay(150);   //  11/4/24 consider using millis
    startMillis = millis(); while (millis() <= (startMillis + 150)) {} //  delay command

    keypress = membrane_keypad.getKey();// Read the key pressed
  //activekeypress = keypress;
  
    if (membrane_keypad.getState()==PRESSED && keypress != NO_KEY){
  
      if (keypress > '0' && keypress <=avail_blunamis[0]){
        switch (keypress) {        
          case  '1':    //   loco #1 selected
            if(case1_selected==0){  // this prevents duplicate entries
              Serial.print("\t\t");Serial.print(keypress);Serial.print("\t"); 
              Serial.print(Blunamis[0].Blunami_name);Serial.print("\t");
              Serial.print(Blunamis[0].Blunami_address);Serial.print("\t  ");
              Serial.println(Blunamis[0].Blunami_index-1);
            //  if(num_keypress==1){first_keypress = 1;} //  first keypress is updated when num_keypress=1
              num_keypress = num_keypress+1;  //  track the number of keypresses
              num_blunamis_connect = num_blunamis_connect +1; // track the # of blunamis selected
              key_id = 1;       //  this is the key pressed, in this case #1
              case1_selected = 1;}
            break;
          case '2':   //  loco #2 selected 
            if(case2_selected==0){  // this prevents the same entry
              Serial.print("\t\t");Serial.print(keypress);Serial.print("\t");
              Serial.print(Blunamis[1].Blunami_name);Serial.print("\t");
              Serial.print(Blunamis[1].Blunami_address);Serial.print("\t  ");
              Serial.println(Blunamis[1].Blunami_index-1);
             // if(num_keypress==1){first_keypress = 1;} //  first keypress is updated when num_keypress=1
              num_keypress = num_keypress+1;
              num_blunamis_connect = num_blunamis_connect +1; // track the # of blunamis selected
              key_id = 2;  //  this is the key pressed, in this case #2
              case2_selected = 1;}
           break; 
          case '3':   //  loco #3 selected 
            if(case3_selected==0){  // this prevents the same entry
              Serial.print("\t\t");Serial.print(keypress);Serial.print("\t"); 
              Serial.print(Blunamis[2].Blunami_name);Serial.print("\t");
              Serial.print(Blunamis[2].Blunami_address);Serial.print("\t  ");
              Serial.println(Blunamis[2].Blunami_index-1);
            // if(num_keypress==1){first_keypress = 1;} //  first keypress is updated when num_keypress=1
             num_keypress = num_keypress+1;
             num_blunamis_connect = num_blunamis_connect +1; // track the # of blunamis selected
             key_id = 3;  //  this is the key pressed, in this case #3
             case3_selected = 1;}
           break;
          case '4':   //  loco #4 selected 
           if(case4_selected==0){  // this prevents the same entry
             Serial.print("\t\t");Serial.print(keypress);Serial.print("\t"); 
             Serial.print(Blunamis[3].Blunami_name);Serial.print("\t");
             Serial.print(Blunamis[3].Blunami_address);Serial.print("\t  ");
             Serial.println(Blunamis[3].Blunami_index-1);
            // if(num_keypress==1){first_keypress = 1;} //  first keypress is updated when num_keypress=1
             num_keypress = num_keypress+1;
             num_blunamis_connect = num_blunamis_connect +1; // track the # of blunamis selected
             key_id = 4;  //  this is the key pressed, in this case #4
             case4_selected = 1;}
            break; 
          case '5':   //  loco #5 selected 
           if(case5_selected==0){  // this prevents the same entry
             Serial.print("\t\t");Serial.print(keypress);Serial.print("\t"); 
             Serial.print(Blunamis[4].Blunami_name);Serial.print("\t");
             Serial.print(Blunamis[4].Blunami_address);Serial.print("\t  ");
             Serial.println(Blunamis[4].Blunami_index-1);
            // if(num_keypress==1){first_keypress = 1;} //  first keypress is updated when num_keypress=1
             num_keypress = num_keypress+1;
             num_blunamis_connect = num_blunamis_connect +1; // track the # of blunamis selected
             key_id = 5;  //  this is the key pressed, in this case #5
             case5_selected = 1;}
            break; 
          case '6':   //  loco #6 selected 
           if(case5_selected==0){  // this prevents the same entry
             Serial.print("\t\t");Serial.print(keypress);Serial.print("\t"); 
             Serial.print(Blunamis[5].Blunami_name);Serial.print("\t");
             Serial.print(Blunamis[5].Blunami_address);Serial.print("\t  ");
             Serial.println(Blunamis[5].Blunami_index-1);
            // if(num_keypress==1){first_keypress = 1;} //  first keypress is updated when num_keypress=1
             num_keypress = num_keypress+1;
             num_blunamis_connect = num_blunamis_connect +1; // track the # of blunamis selected
             key_id = 6;  //  this is the key pressed, in this case #6
             case6_selected = 1;}
            break;
        }   //  bottom switch  keypress
        //     -------------------- LCD DISPLAY SELECTED LOCOS----------------------------
        //  send current selection to the LCD
        // pass "key_id" which is the number of the key pressed (1-6) and "num_keypress" which
        //  is the order that the key has been pressed. this is used to change the LCD row and basically acts
        //  like a line feed
          lcd_loco_selected(key_id,num_keypress);
        //  ---------------------- BOTTOM LCD DISPLAY SELETED LOCOS----------------------------

        //  this tracks the order of the key pressed and saves it into array connect_order
        //  key_id is the keypressed.
          connect_order[num_keypress-1] = key_id-1;   //  num_keypress-1  is the index    

      }    //  bottom if keypress

//---------------------------------TOP CONNECT ----------------------------------       
        if(keypress =='0' ){      // keypress '0' is to confirm the selection

          f_keypress = f_keypress + 1;    //  update the number of times the "F" key is pressed
            
            Serial.println("\t\tFinal connect order");
            Serial.println("\t\tNo.\tName\t\tAddress\t\t\tIndex");
/*
          for (order=0;order<=num_keypress-1;order++ ){   //  order is global           
              Serial.print("\t\t");Serial.print(connect_order[order]+1);  // 
              Serial.print("\t");Serial.print(Blunamis[connect_order[order]].Blunami_name);    //  name
              Serial.print("\t");Serial.print(Blunamis[connect_order[order]].Blunami_address); //ADDRESS
              Serial.print("\t  ");Serial.println(connect_order[order]);   //  index
          } //  bottom for
*/
        //  re sort the list
          order=0;
          for (int sort=0;sort<=num_keypress-1;sort++){
                Blunamis_sorted[sort].Blunami_index2 = sort;
                Blunami_index[sort] = Blunamis_sorted[sort].Blunami_index2;   //  save index in array
                Blunamis_sorted[sort].Blunami_name2 = Blunamis[connect_order[order]].Blunami_name;
                Blunami_name[sort] = Blunamis_sorted[sort].Blunami_name2;  //  save the name in a string
                Blunamis_sorted[sort].Blunami_address2 = Blunamis[connect_order[order]].Blunami_address;
                Blunami_Address[sort] = Blunamis_sorted[sort].Blunami_address2;  //  save the address in a string
                order=order+1;
          }

  //    we need to print a header and footer for the selected sorted locos
          lcd_header();
          lcd_connect_footer();

  //    print out resorted matrix
          for (int sorted=0;sorted<=num_keypress-1;sorted++){
              Serial.print("\t\t");Serial.print(Blunamis_sorted[sorted].Blunami_index2+1);  // NO.
              Serial.print("\t");Serial.print(Blunamis_sorted[sorted].Blunami_name2);    //  name 2

              Serial.print("\t");Serial.print(Blunamis_sorted[sorted].Blunami_address2); //ADDRESS 2
              Serial.print("\t  ");Serial.println(Blunamis_sorted[sorted].Blunami_index2);   //  index 2

//            LCD DISPLAY FINAL LOCO ORDER AND CONNECTION STATUS.              
              lcd_loco_connected(sorted);
          }
          Serial.println();

        }    // bottom keypress = '0'     

        if(keypress == 'e'){     //  F key,-------------->  CONNECT, BREAK
              break;  //  exit then connect by calling PERIPHERAL FUNCTION
        }    //  bottom F key    CONNECT
//---------------------------------- BOTTOM CONNECT ---------------------------------------
//---------------------------------- TOP RESCAN --------------------------------    
        if(keypress =='r'){         //  R key       RESCAN
            keypress = '0';
            first_keypress = 0;
            case1_selected = 0;
            case2_selected = 0;
            case3_selected = 0;
            case4_selected = 0;
            case5_selected = 0;
            case6_selected = 0;
            num_blunamis =0;
            num_blunamis_connect = 0; // track the # of blunamis selected
            num_keypress = 0;
            f_keypress = 0;

            lcd_rescan_loco();    //  display reScan on LCD screen
            // delay screen
            startMillis = millis(); while (millis() <= (startMillis + 1000)) {} //  delay 1 sec 
            Serial.println("\t\t\tRe-scanning");  //  RESCAN
            blunami_survey();
            blunami_list();
            blunami_select(); 
            int connect_order[6] = {0,0,0,0,0,0};        // reset array  
            
          }   //  bottom  keypress 'r' RESCAN
//-------------------------------- BOTTOM RESCAN --------------------------------------------

    }   //  bottom no key

}   //  bottom while infinite loop

}   //  bottom blunami select

//**********************************BOTTOM BLUNAMI_SELECT **********************************************

//**************************** find peripheral ************************************
void find_peripheral()
{   //  top find peripheral function
    // k is a global variable used in peripheral and connect for the for loop
//  startMillis = millis(); while (millis() <= (startMillis + while_blunami_delay)) {} //  delay command (45)
  int peripheral_lap=0;
//  BLE.scan();   //  11/27/24 not sure if this is necessary or of any value
  print_line() ;   // prints line in sketch
//  Serial.println("\t\t(@925) Peripheral function TOP");

  for ( k=0;k<=num_blunamis_connect-1;k++) // we are looking at the # selected locos and not available ones
    {  // Open for (k = 0; k <= num_blunamis; k++) 
//       Serial.println("\t\t(@825) In peripheral for loop");
      
      while (!Blucab_Blunami[k]) {// open while !peripheral      
        startMillis = millis(); while (millis() <= (startMillis + 6)) {} //  connection delay
      //  BLE.scanForUuid(Blunami_UUID);  // Scan for Blunami UUID        
        BLE.scanForAddress(Blunami_Address[k]);
        startMillis = millis(); while (millis() <= (startMillis + 12)) {} //  connection delay   
        Blucab_Blunami[k] = BLE.available();    //  are we available?
        startMillis = millis(); while (millis() <= (startMillis + while_blunami_delay)) {} //  delay command      
      //  Blucab_Blunami[k].connect();    //  12/3/24 lets try connecting here. not sure any better
        peripheral_lap = peripheral_lap+1;    //  see how many laps it takes
      }   //  bottom while

  //   11/24/24 these were initialized in SELECT function
  //    Blunami_name[k] = Blunamis_sorted[k].Blunami_name2;         //  save the name in a string
  //    Blunami_Address[k] = Blunamis_sorted[k].Blunami_address2;   //  save the address in a string
  //    Blunami_index[k] = Blunamis_sorted[k].Blunami_index2;       //  save index in array

  //  print out what we found 
  //    Serial.print("\t(@900) # peripheral laps:  ");Serial.println(peripheral_lap);  //  laps tp find peripheral
      Serial.print("\t\t(@930) Name:  ");Serial.print(Blunami_name[k]);
      Serial.print("\tAddress:  ");Serial.print(Blunami_Address[k]);       
      Serial.print("\tAvailable:  ");Serial.print(Blucab_Blunami[k]);
      Serial.print("\tIndex:  ");Serial.print(Blunami_index[k]);
      Serial.print("\tK:  ");Serial.println(k);
   
//-------------------CALL CONNECT FUNCTION HERE ---------------------------------------------------
//  if here we know there is a peripheral, so call the function

//          Serial.println("\t\t(@930) calling connect (from peripheral)");
          print_line();   // prints line in sketch
          BLE.stopScan();        
          Blunami_connect(k);   //  call connect function
 
//-------------------- BOTTOM CALL CONNECT FUNCTION -------------------------------------------------- 
 
    }   //  bottom for

}   //  bottom find peripheral function 

//******************************* bottom find peripheral **********************************************
//**************************************Top Blunami_connect()***************************************************
void Blunami_connect(int k)
//  this function  was modified 11/19/24. removed while loops
  
{  // top Blunami_connect()
  //  the index is passed to the function
  int attribute_time_lap=0;
  unsigned long connect_start_millis = millis();
  unsigned long connect_end_millis = millis();   
  unsigned long total_millis = millis()  ;
  int connect_lap = 0;      //  track the number of laps to connect. should only be 1
  //  K is a global variable that tracks the decoder connection index 
 

  connect_label:    //  this is used with the goto statement
//  print_line(); // this prints a line

  Serial.println("\t\t(@1060) TOP connect function");

  // 11/23/24 for nano 33 BLE, this connect status helps the connection process. go figure
  //  seems to like checking for both
//  connection_status_all(980,k);   //  check connection 11/23/24 to solve misconnect issue
//  connected_status1(870,k);   //  1 blunami at a time   

  //----------------------------------- CONNECT---------------------------------------------
    Serial.print("\t\t(@1065) name:  ");Serial.print(Blunami_name[k]);
    Serial.print("\tConnecting... ");
//  ----------------------------TOP LCD CONNECTING FUNCTION---------------------------------- 
    lcd_connecting();   //  display the connecting status on the LCD
//  ----------------------------- BOTTOM LCD CONNECTING---------------------------------- 

    if (Blucab_Blunami[k]){      
        BLE.scanForAddress(Blunami_Address[k]);
      //  startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command        
        Blucab_Blunami[k].connect();
        Blucab_Blunami[k].connect();    //  12/2/24 DOUBLE CONNECTS SEEM TO IMPROVE THE SITUATION       
    }
    if(Blucab_Blunami[k].connected()){    //  confirm connection
        Serial.println("Connected") ;
        blunami_connected[k] = 1;    //  update integer array for connect status        
    }
    else{Serial.println("\tNot connected");
        blunami_connected[k] = 0;   //  update integer array for connect status 
        connect_lap = connect_lap + 1;
  //      Serial.print("\t(@990) connect laps:  ");Serial.println(connect_lap);
        }

    if(connect_lap > 5){
        Serial.println("\tConnect laps exceeded, reconnect");
        Blunami_attributes[k] = 0;
        blunami_connected[k] = 0;  
        attribute_lap = 0;
        connect_lap = 0;
        Blucab_Blunami[k].disconnect();
        //   add this delay because the disconnect event does not immediatley occur
        startMillis = millis(); while (millis() <= (startMillis + 75)) {} //  delay command
        goto connect_label;     //  call function
    }   //  bottom if connect_lap > 3
//------------------------------ bottom connect -----------------------------

  //----------------------DISCOVER ATTRIBUTES-------------------------- 
    attribute_label:    //  goto here if we want to discover attributes again 

    connect_start_millis = millis(); connect_end_millis = millis(); total_millis = millis(); 
    Serial.print("\t\t(@1100) name:  ");Serial.print(Blunami_name[k]);
  //  Serial.print("\tDiscover Atributes... ");

     Serial.print("\tDiscover Atributes... ");   
    if(Blucab_Blunami[k])
    {
        Blucab_Blunami[k].discoverAttributes();
        Serial.println("@1111 Attributes found"); 
        startMillis = millis(); while (millis() <= (startMillis + attribute_delay)){}
        Blunami_attributes[k] = 1;      
    }   //  bottom if
    else{   //  if here no attributes found
        Blunami_attributes[k] = 0;
        Serial.println();
        Serial.println("(@1118) Attributes not found, reconnect");    
        blunami_connected[k] = 0;
        attribute_lap = attribute_lap+1;    //  tracking the number of laps to discover attributes
      //  Blucab_Blunami[k].disconnect();
      //   add this delay because the disconnect event does not immediatley occur
        startMillis = millis(); while (millis() <= (startMillis + 75)) {} //  delay command
        goto connect_label;     //  recall function
    }   //  bottom else
    
//    Serial.print("\t(@1030) # attribute laps: ");Serial.println(attribute_lap);

//  test for the number of max ATTRIBUTE laps = 3
    if(attribute_lap > 2){   //  11/30/24 PULLED VALUE OUT OF MY ASS
      attribute_lap=0;
      Serial.println("\t(@1132) Attribute laps exceeded, reconnect");
      blunami_connected[k] = 0;   //    update int array
      Blunami_attributes[k] = 0;  //    update int array
      Blucab_Blunami[k].disconnect();
      //   add this delay because the disconnect event does not immediatley occur
      startMillis = millis(); while (millis() <= (startMillis + 75)) {} //  delay command
      goto connect_label;     //  recall the function function

    }
//------------------------ BOTTOM DISCOVER ATTRIBUTES--------------------------

//-----------------CHECK IF THERE A CONNECTION AND IF DISCOVER ATTRIBUTES WAS SUCCESSFUL------------------------
//  if no connection or no attributes, start over
    if(blunami_connected[k] == 0 || Blunami_attributes[k] == 0){
      Serial.println("\t(@1146) reconnecting, no connection and/or attributes not found"); 
      attribute_lap=0;
      attribute_time_lap = 0;
      goto connect_label;            
      }
//--------BOTTOM CHECK IF THERE A CONNECTION AND IF DISCOVER ATTRIBUTES WAS SUCCESSFUL-------------------

  //   connection_status_all(1085,k);   //  check connection to maintain it
  //    connected_status1(930,k);       //  1 blunami at a time
//---------------------------- CHECK IF CONNECTION LOST THEN RECONNECT---------------------------------
    //  sometimes the connection is lost by the time we get here. so lets check it and
    //  reconnect if it is lost
    if(!Blucab_Blunami[k].connected()){
      Serial.println("\t(@1160) lost connection before return, so reconnect");  
      attribute_lap=0;
      attribute_time_lap = 0;
      blunami_connected[k] = 0;   //    update int array    
      Blucab_Blunami[k].disconnect();    //  11/30/24 added to see if it helps
      goto connect_label;   //  call connect function again
    }
//------------------------- BOTTOM IF CONNECTION LOST THEN RECONNECT ----------------------------

//------------------------ define characteristics-----------------------------------
//   Define Blunami "DCC Characteristic"
    BlunamiCommand[k] = Blucab_Blunami[k].characteristic("f688fd1d-da9a-2e8f-d003-ebcdcc259af9");
//   Define Blunami "MOTOR/TRACK VOLTAGE Characteristic"    
    motorvoltage[k] = Blucab_Blunami[k].characteristic("f688fd14-da9a-2e8f-d003-ebcdcc259af9");
//------------------------ bottom define characteristics-----------------------------------
//-------------------------------------- read initial value stored in blunami ---------------------
//  have no idea what this value does or represents. when sniffing with light blue & wireshark
//  it shows up when initially reading the DCC characteristic
// this is the hex string 01 10 53 6F 75 6E 64 54 72 61 78 78 20 42 6C 75 6E 61  that is read
//  I printed out the bit 2 pair only and it produces the correct value
    BlunamiCommand[k].readValue(blue_dcc,18);
    int blue_dcc_result = blue_dcc[1];    //  print bit 1, should be 10
//    Serial.print("\t(@1090) dcc_blue bit 2:  ");Serial.println(blue_dcc_result,HEX);

//------------------------------- bottom read initial value in blunami --------------------
//*************************** CHECK OVERALL CONNECT DURATION*********************** 
//  if the entire process takes more than 7 sec, start over 
//  this should not happen under normal conditions
    total_millis = millis();
    if((total_millis-connect_start_millis)>7000){
        Serial.println("\t(@1190) Exceeded MAX CONNECT time limit, re-connect");      
        blunami_connected[k] = 0;   //    update int array
        Blunami_attributes[k] = 0;
        attribute_lap=0;  //  reset lap counter
        attribute_time_lap = 0;
      //  Blucab_Blunami[k].disconnect();
      //   add this delay because the disconnect event does not immediatley occur
      //  startMillis = millis(); while (millis() <= (startMillis + 75)) {} //  delay command
        goto connect_label;     //  recall function
    } 

//******************* BOTTOM CONNECT DURATION**************************** 
  
//************************* TOP SOUND HORN ****************************
// -------------------- DO SOME HOUSEKEEPING AND CONFIRM CONNECTION WITH HORN-------------------
//  1.  loco_number = cv_loco_number();     //get loco number
//  2.  start_rail_volts();                 //  send command to intialize rail voltage
//  3.  decoder_voltage = rail_voltage();   //  this function calculates and returns the decoder voltage.
//    loco_number[k] = 0;       //  make sure it is zero    this makes no difference
//    decoder_voltage[k] = 0;   //  make sure it is zero
//  check if we are connected and attirbutes have been found
    if(Blucab_Blunami[k].connected() && Blunami_attributes[k] == 1){    //  makes calcs if connected
    //    print_line();
        Serial.println("\t\t(@1200) CONFIRM CONNECTION");
        Serial.print("\t\tName:  ");Serial.print(Blunami_name[k]);
        Serial.print("\tIndex:  ");Serial.print(k);
        loco_number[k] = cv_loco_number(k); //  call this function to read the CV numbers and calculate the loco number
        startMillis = millis(); while (millis() <= (startMillis + cv_delay)) {} //  delay command
        start_rail_volts();
        startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command
        decoder_voltage[k] = rail_voltage(k);
      //  startMillis = millis(); while (millis() <= (startMillis + cv_delay)) {} //  delay command
      //  start_rail_volts();
      //  startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command
      //  decoder_voltage[k] = rail_voltage(k);
        startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command        
        Serial.print("\tLoco number:  ");Serial.print(loco_number[k]);        
        Serial.print("\tVoltage:  ");Serial.println(decoder_voltage[k], 1); 
        startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command
        confirm_connect(k);   //  THIS SOUNDS THE HORN
        //  add 1 sec delay to see if it solves the weird looping problem
      //  startMillis = millis(); while (millis() <= (startMillis + 1000)) {} //  delay command
        print_line();
        
    }   //  bottom if connected
    else{   //  we have disconnected for some reason, so reconnect
        Serial.println("\t(@1235) Disconnect while in SUMMARY CONNECT");
        blunami_connected[k] = 0;   //    update int array
        Blunami_attributes[k] = 0;
        attribute_lap=0;  //  reset lap counter
        attribute_time_lap = 0;
      //   add this delay because the disconnect event does not immediatley occur
      //  startMillis = millis(); while (millis() <= (startMillis + 100)) {} //  delay command
        goto connect_label;     //  recall function

    }
//**************************************** BOTTOM SOUND HORN **********************************

//  ----------------------------TOP LCD CONNECTED FUNCTION---------------------------------- 
    lcd_connected();    //  display blunami connected
//  ----------------------------BOTTOM LCD CONNECTED FUNCTION---------------------------------- 


//    Serial.println("\t(@1175) leaving connect function");
//    print_line();   //   this prints a line
 
    attribute_lap=0;    // tracks number of tries to discover attributes, RESET
    attribute_time_lap = 0;

  //  BLE.stopScan();  // added 11/24/24 DOES NOT LIKE IT

//  put 5 sec delay between connections to slow things down
//  also trying this to see if more than 3 blunamis can be connected
//  Do not delay for last blunami
    if(k<num_blunamis_connect-1){
      startMillis = millis(); while (millis() <= (startMillis + 5000)) {} //  delay command
    }
    else{   //  2 sec for final loco
      startMillis = millis(); while (millis() <= (startMillis + 2000)) {} //  delay command
    }
    


  return; 

} // bottom Blunami_connect()

//***************************Bottom Blunami_connect()**************************************************************

//************************FUNCTION- DISPLAY RSSI AND IF CONNECTED ********************
void show_rssi(int line, int rssi, int connect){

//  This function displays the signal strength and connection status
//  The function is called - show_rssi(21, rssi, connect) where 21 represents the targeted line number
//  must pass the rssi and connect. This is the only way it works

Serial.print ("Line: ");
Serial.print(line);
Serial.print("    RSSI: ");
Serial.print(rssi);
delay(100);
Serial.print("\tConnected:   ");
delay(100);
Serial.println(connect);

return;
}
//************************ end function show rssi and connection**************************
//*******************************KEYPRESS FUNCTION****************************************
void keypressfunction(){

//  this uses the computer keyboard for input  
// ---------------------------- RECORD KEYPRESS --------------------------------

  if (Serial.available() > 0) {   //  record keypress  
    keypress = Serial.read();   // read the incoming byte:
  //  delay(command_delay);
    currentkeypress = keypress;
    
  }   //bottom of if serial available 

//--------------------------- PROCESS  KEYPRESS ----------------------------------
//  if any of the top level commands, throttle, F1, F2, loco #, forward, reverse, are issued
//  save the keypress as active command. 
//  1/17/25 sort letter keypress to see if it speeds up response
//  makes no difference
  if (keypress =='e'||keypress =='f'||keypress =='g'||keypress =='l'||keypress =='r'||keypress =='t'){   
        activecommand = currentkeypress;   
    }

// 1/17/25 removed this bit of code since it is seemed reducntant
//  kepresses are processed in the speed, function routines   
//  else if (keypress >= '0' && keypress <='9'){ 
   // function_value = value;
//        activekeypress = currentkeypress; //  TRACKING THE KEYPRESSES
                                          //  speed and functions have their own variables
//   }

}   //    bottom of keypress function
//*****************************bottom keypress functon****************************************

//************************  FUNCTION - PRINT KEYPRESS RESULTS*******************************
void keypress_results() {

//  only process commands if there is a key press  
if (keypress != NO_KEY){    //  use this to slow down the dispAY

Serial.print("active keypress: "); 
Serial.println(activekeypress); 
Serial.print("old keypress:    "); 
Serial.println(oldkeypress); 

Serial.print("current keypress:"); 
Serial.print(currentkeypress);
Serial.print("\t\t\t\t\t\t\tDirection:");
if(loco_direction == '1'){Serial.println("\tForward");}else{Serial.println("\tReverse");}
Serial.print("Loco number:     ");
Serial.print(loco_number[k]);
      Serial.print("\t\t\tFunction Group 1");
      Serial.println("\t\tName:\t\tVoltage: ");      
Serial.print("Direction =      ");
Serial.print(loco_direction);
      Serial.print("\t\t 1   0   0  F0  F4  F3  F2  F1 ");
          Serial.print("\t\t");Serial.print(Blunami_name[0]);
          Serial.print("\t ");Serial.println(decoder_voltage[0]);
Serial.print("Speed Value =    ");
Serial.print(speed_value);
      Serial.print("\t\tb7  b6  b5  b4  b3  b2  b1  b0  HEX");
          if(actual_blunami_connected>=2){
            Serial.print("\t");Serial.print(Blunami_name[1]);
            Serial.print("\t ");Serial.println(decoder_voltage[1]);}
          else{Serial.println();}
Serial.print("Function Value = ");
Serial.print(function_value);
      //    print function byte 
      Serial.print("\t\t");
      Serial.print(" 1   0   0");      
      Serial.print("   ");
      Serial.print(f0);
      Serial.print("   ");
      Serial.print(f4);
      Serial.print("   ");
      Serial.print(f3);
      Serial.print("   ");
      Serial.print(f2);
      Serial.print("   ");
      Serial.print(f1);
      Serial.print("   ");      
      Serial.print(fg1_current_command,HEX); // print hex value
          if(actual_blunami_connected>=3){
            Serial.print("\t");Serial.print(Blunami_name[2]);
            Serial.print("\t ");Serial.println(decoder_voltage[2]);}
          else{Serial.println();}
Serial.print("F0-Light:        ");
Serial.println(f0);      
Serial.print("F1-Bell:         ");
Serial.print(f1);
      Serial.println("\t\t\tFunction Group 2A");
Serial.print("F2-Long horn:    ");
Serial.print(f2);
      Serial.println("\t\t 1   0   1   1  F8  F7  F6  F5 ");
Serial.print("F3-short horn:   ");
Serial.print(f3);
      Serial.println("\t\tb7  b6  b5  b4  b3  b2  b1  b0  HEX");
Serial.print("F4-Beacon Light: ");    // not setup in blunami test stand
Serial.print(f4);
       //    print function byte 
      Serial.print("\t\t");
      Serial.print(" 1   0   1   1");      
      Serial.print("   ");
      Serial.print(f8);
      Serial.print("   ");
      Serial.print(f7);
      Serial.print("   ");
      Serial.print(f6);
      Serial.print("   ");
      Serial.print(f5);      
      Serial.print("   ");
      Serial.println(fg2a_current_command,HEX); // print hex value
Serial.print("F5-Battery off:  ");     // s-cab command to shut everything down. 
Serial.println(f5);  
Serial.print("F6-FX3:          ");             //  not setup
Serial.print(f6);
      Serial.println("\t\t\tFunction Group 2B");
Serial.print("F7-Dim Light:    ");
Serial.print(f7);
      Serial.println("\t\t 1   0   1   0 F12 F11 F10  F9 ");
Serial.print("F8-Mute:         ");
Serial.print(f8);
      Serial.println("\t\tb7  b6  b5  b4  b3  b2  b1  b0  HEX");
Serial.print("F9-Grade Cross:  ");
Serial.print(f9);
      //    print function byte 
      Serial.print("\t\t");
      Serial.print(" 1   0   1   0");      
      Serial.print("   ");
      Serial.print(f12);
      Serial.print("   ");
      Serial.print(f11);
      Serial.print("   ");
      Serial.print(f10);
      Serial.print("   ");
      Serial.print(f9);      
      Serial.print("   ");
      Serial.println(fg2b_current_command,HEX); // print hex value
Serial.println();

}   //  bottom NO_KEY
return;
}   
//***********************FUNCTION - PRINT KEYPRESS RESULTS********************************
//*******************************SPEED FUNCTION***************************************
void speed_command() {
//  this function is called if the active command is 't' for throttle
// placing these commands to the top seems to help the speed delay.
  speed[2] = loco_number[0]; //  PRIMARY LOCO NUMBER (# <= 127)         1/17/25 moved to top

  loco_extended_address();    //  calculate extended number hex pairs   1/17/25 moved to top
  speed_e[2] = loco_left_hex_pair;    //  update extended loco #        1/17/25 moved to top
  speed_e[3] = loco_right_hex_pair;  //  update extended loco #         1/17/25 moved to top      

  if (keypress >= '0' && keypress <='9'){ 
  //  speed_value = keypress; 
    speed_value = activekeypress;   //  change this to make consistent with function byte
  }
/*    for testing
Serial.print("\t\t\tkeypress(@690):  ");
Serial.println(keypress);
Serial.print("\t\t\tspeed value(@690):  ");
Serial.println(speed_value);
Serial.print("\t\t\told speed value(@690):  ");
Serial.println(old_speed_value);
*/
//    loco_extended_address();    //  calculate extended number hex pairs   1/17/25 moved to top
//    speed_e[2] = loco_left_hex_pair;    //  update extended loco #        1/17/25 moved to top
//    speed_e[3] = loco_right_hex_pair;  //  update extended loco #         1/17/25 moved to top 

// primary loco numbers are 1-127 and extended are 128-9999
// because extended uses 2 hex pairs  for the loco number, different commands will be required
//   the speed values are calculated and summarized in the DCC-EX BINARY COMMAND SPREADSHEET

switch (speed_value)  {   //  from keypress
    case '0':               //  stop -  keypress 0
      speed_step = 0;
      if(loco_direction == '0'){    //  reverse
        speed[4] = 0x00;        // turns on reverse light primary loco #
        speed_e[5] = 0x00;}     //  extended loco #    
      else{
        speed[4] = 0x80;    // turns on forward light
        speed_e[5] = 0x80;}                     
      break;  
    case '1':         //  SPEED STEP 1  - keypress 2
      speed_step = 1;  
      if (loco_direction == '1'){  //inner if statment - what's the direction?
        speed[4] = 0x82;    //  forward
        speed_e[5] = 0x82;}        
      else { 
        speed[4] = 0x02;    //  reverse 
        speed_e[5] = 0x02;}   
      break;  
    case '2':         //  SPEED STEP 2 - keypress 2
      speed_step = 2;
      if (loco_direction == '1') {    //inner if statment
        speed[4] = 0x83;    //  forward
        speed_e[5] = 0x83;}
      else {
        speed[4] = 0x03;   //  reverse 
        speed_e[5] = 0x03;}
      break;    //  case 2
    case '3':         //  SPEED STEP 4  - keypress 3
      speed_step = 4;
      if (loco_direction == '1') {    //inner if statement
        speed[4] = 0x85;    //  forward
        speed_e[5] = 0x85;}
      else {
        speed[4] = 0x05;    //  reverse
        speed_e[5] = 0x05; }
      break;    //  case 3
    case '4':         //  SPEED STEP 7 - keypress 4
      speed_step = 7;
      if (loco_direction == '1') {    //inner if statment
        speed[4] = 0x88;    //  forward 
        speed_e[5] = 0x88;}       
      else {
        speed[4] = 0x08;    //  reverse
        speed_e[5] = 0x08; }
      break;    //  case 4
    case '5':         //  SPEED STEP 10 - keypress 5
      speed_step = 7;
      if (loco_direction == '1') {    //inner if statment
        speed[4] = 0x8B;    //  forward 
        speed_e[5] = 0x8B;}       
      else {
        speed[4] = 0x0B;    //  reverse
        speed_e[5] = 0x0B; }
      break;    //  case 5



//******************* MUST ADD ADDITIONAL SPEED STEPS ******************************
//  8/3/24 add additional steps
    }   //  bottom switch speed
//---------------------------------------------------------------------------------
//  since this is a simple consist, all loco numbers are the same. So we can use the loco 0 index value
//  update loco number
// these 3 assignments were moved to the top to remove the slight delay the 1st time the
// speed command is made
//  loco_extended_address();    //  calculate extended number hex pairs   
//  speed_e[2] = loco_left_hex_pair;    //  update extended loco #
//  speed_e[3] = loco_right_hex_pair;  //  update extended loco #       


  if(loco_number[0] <= 127){
//    speed[2] = loco_number[0]; //  PRIMARY LOCO NUMBER (# <= 127)
    for(int s = 1;s <= 3; s++){   //    send the command 3 times. no reason for using variable "s"
          for(int l=0;l<=actual_blunami_connected-1;l++){     //  top of for "l" loop  
            BlunamiCommand[l].writeValue(speed,5);  
          }   //  bottom for speed
      }   //  bottom for 2 commands
    } //  bottom if < 127

  else {       //  EXTENDED LOCO NUMBER (# > 127 )
//    loco_extended_address();    //  calculate extended number hex pairs   1/17/25 moved to top
//    speed_e[2] = loco_left_hex_pair;    //  update extended loco #        1/17/25 moved to top
//    speed_e[3] = loco_right_hex_pair;  //  update extended loco #         1/17/25 moved to top      
    
      for(int s = 1;s <= 3; s++){   //    send the command 3 times. no reason for using variable "ss"
          for(int l=0;l<=actual_blunami_connected-1;l++){  //  top of for "l" loop 
            BlunamiCommand[l].writeValue(speed_e,6); 
               
      //      if(motor_start ==0 && l==0){
      //        // delay the 1st motor only at start up
      //        startMillis = millis(); while (millis() <= (startMillis + motor_start_delay)) {} //  delay command
      //      }   //  bottom if start
          }   //  bottom for speed command
      }   //  bottom for 3 commands
  } //  bottom else > 127        
  motor_start = 1;  //  motor has started
return;
}     //    bottom of speed_command function
//**************************bottom of speed_command function*************************
//*************************-HEX CALCULATION FUNCTION--GROUP 1*************************
String bin2hex_calculator (){
//  this function is not used for processing the command. only used for display
//  this function converts a binary byte into the 2 bit hex pair equivalent
//  used by the blunami for function group 1. 
//  5/27/24 this function uses the global variables for f0-f9. Trying to pass the
//  variable did not work. May revisit in the future.
//  6-13-24 use print mask to display hex. a lot easier!!

  String left_hex_symbol;   //  left character of the hex pair
  String right_hex_symbol;  //  right character of the hex pair
  String hex_command; //  final 2 bit hex pair

//  bit b7-b5 designate that this is function group 1 abd does not change
  int b7= 1;
  int b6= 0;
  int b5= 0;

//  convert command byte from string to int necessary for hex calcs.
  int f0int = f0.toInt();   //  function 0 - headlight
  int f4int = f4.toInt();   //  function 4 - loco beacon  
  int f3int = f3.toInt();   //  function 3 - airhorn  - short horn
  int f2int = f2.toInt();   //  function 2 - airhorn -long  
  int f1int = f1.toInt();   //  function 1 - bell
  
/*    declared in main body as reference only
String f0 = "0";                //  function 0 - headlight - turn on at startup
String f1 = "0";                //  function 1 - bell
String f2 = "0";                //  function 2 - airhorn -long      
String f3 = "0";                //  function 3 - airhorn  - short horn
String f4 = "0";                //  function 4 - loco beacon  
String f5 = "0";                //  function 5 - battery off
String f6 = "0";                //  function 6 - FX3 - not connected
String f7 = "0";                //  function 7 - dimlight
String f8 = "0";                //  function 8 - mute
String f9 = "0";                //  function 9 - grade crossing
*/
 //THIS SECTION USED TO TEST PASSING FUNCTION BITS 
 /*
    Serial.println("\t    function command byte");
    Serial.println("\tb7, b6, b5, b4, b3, b2, b1, b0");
   // Serial.println("\t 1,  0,  0, f0, f4, f3, f2, f1");    
    Serial.print("\t ");
    Serial.print(b7);
    Serial.print("   ");
    Serial.print(b6);
    Serial.print("   ");
    Serial.print(b5);
    Serial.print("   ");
    Serial.print(f0int);
    Serial.print("   ");
    Serial.print(f4int);
    Serial.print("   ");    
    Serial.print(f3int);
    Serial.print("   ");
    Serial.print(f2int);
    Serial.print("   ");
    Serial.println(f1int);
*/
//  calculate LEFT hex character  
  int left_hex_num = b7*8+b6*4+b5*2+f0int*1; //  sum of the LEFT 4 BITS  
  switch (left_hex_num) {
   
    case 0:
      left_hex_symbol = "0";
      break;
    case 1:
      left_hex_symbol = "1";
      break;
    case 2:
      left_hex_symbol = "2";
      break;
    case 3:
      left_hex_symbol = "3";
      break;
    case 4:
      left_hex_symbol = "4";
      break;
    case 5:
      left_hex_symbol = "5";
      break;
    case 6:
      left_hex_symbol = "6";
      break;
    case 7:
      left_hex_symbol = "7";
      break;
    case 8:
      left_hex_symbol = "8";
      break;
    case 9:
      left_hex_symbol = "9";
      break;
    case 10:
      left_hex_symbol = "A";
      break;
    case 11:
      left_hex_symbol = "B";
      break;
    case 12:
      left_hex_symbol = "C";
      break;
    case 13:
      left_hex_symbol = "D";
      break;
    case 14:
      left_hex_symbol = "E";
      break;
    case 15:
      left_hex_symbol = "F";
      break;    
}

  //  calculate RIGHT hex character
  int right_hex_num = f4int*8+f3int*4+f2int*2+f1int*1;  //  sum of the RIGHT 4 BITS  
  
  switch (right_hex_num) {
    case 0:
      right_hex_symbol = "0";
      break;
    case 1:
      right_hex_symbol = "1";
      break;
    case 2:
      right_hex_symbol = "2";
      break;
    case 3:
      right_hex_symbol = "3";
      break;
    case 4:
      right_hex_symbol = "4";
      break;
    case 5:
      right_hex_symbol = "5";
      break;
    case 6:
      right_hex_symbol = "6";
      break;
    case 7:
      right_hex_symbol = "7";
      break;
    case 8:
      right_hex_symbol = "8";
      break;
    case 9:
      right_hex_symbol = "9";
      break;
    case 10:
      right_hex_symbol = "A";
      break;
    case 11:
      right_hex_symbol = "B";
      break;
    case 12:
      right_hex_symbol = "C";
      break;
    case 13:
      right_hex_symbol = "D";
      break;
    case 14:
      right_hex_symbol = "E";
      break;
    case 15:
      right_hex_symbol = "F";
      break;    
}

// concatinate left & right hex symbols. used for display only.
//hex_command = ("0x"+left_hex_symbol+right_hex_symbol);
hex_command = (left_hex_symbol+right_hex_symbol); // removed 0x


return hex_command;   //  return the hex pair to calling location
}
// *******************bottom of bin2hex_calculator function***********************************

//***********************function groups 1, 2a, 2b command to blunami**********************************
void function_command(){
// this function sends all the function commands to mr blunami
//   there are different commands depending upon if loco numbers are primary (<=127) or extendedv (<127)

//  depending upon the loco direction, the functions will only be activated in the lead unit. the trailing
//  units will be silent
//  in forward the lead unit index is "0" and 
//  in reverse the trailing unit becomes the lead unit and the index is "actual_blunami_connected-1"

//  assign the index depending upon the loco direction
  if(loco_direction=='1'){         //  forward
      loco_index = 0;
      previous_loco_index = actual_blunami_connected-1;}   
  else{                           //  reverse
      loco_index = actual_blunami_connected-1;
      previous_loco_index = 0;}   

//  1/17/25 move all assignment statements to the top to simplify and to see if computation
//  speed increased, which it appears, makes no difference. go figure

//    loco_number <= 127
//    reference the loco #1 by using index = 0
      function_group_1[2] = loco_number[0];            // function group 1
      function_group_1[3] = fg1_current_command;    
      function_group_2a[2] = loco_number[0];           // function group 2a
      function_group_2a[3] = fg2a_current_command;  
      function_group_2b[2] = loco_number[0];           // function group 2b   
      function_group_2b[3] = fg2b_current_command;

//    loco_number >127
      function_group_1_e[2] = loco_left_hex_pair;            // function group 1
      function_group_1_e[3] = loco_right_hex_pair;
      function_group_1_e[4] = fg1_current_command;

      function_group_2a_e[2] = loco_left_hex_pair;           // function group 2a
      function_group_2a_e[3] = loco_right_hex_pair;
      function_group_2a_e[4] = fg2a_current_command;

      function_group_2b_e[2] = loco_left_hex_pair;           // function group 2b 
      function_group_2b_e[3] = loco_right_hex_pair;
      function_group_2b_e[4] = fg2b_current_command;
   
  //  1/13/25 function group 3 is not setup    
  //    function_group_3_e[2] = loco_left_hex_pair;            //  function group 3
  //    function_group_3_e[3] = loco_right_hex_pair;  

    if (loco_number[0] <= 127){   //  top if loco number <= 127  

//------------------------primary function group 1 command------------------------------
  //  only send 3 commands when function group 1 (F1) keypress is active  
      if (keypress >= '0' && keypress <='4') { // top if keypress
        function_group_1[2] = loco_number[l];            // function group 1
        function_group_1[3] = fg1_current_command;    
        for(int ff=1;ff<=3;ff++){   //    send the command 3 times. no reason to use variable "ff"
//          for(int l=0;l<=actual_blunami_connected-1;l++){     //  top of for "l" loop
            BlunamiCommand[loco_index].writeValue(function_group_1,4);   //  send command to blunami
//          }   //  bottom for blunami_connected
        }   //  bottom for send command 3 times
      }  //   bottom if keypress
 
//---------------------- primary function group 2a---------------------------------
      if (keypress >= '5' && keypress <='8') { // top if keypress
        function_group_2a[2] = loco_number[l];           // function group 2a
        function_group_2a[3] = fg2a_current_command;
        for(int ff=1;ff<=3;ff++){   //    send the command 3 times. no reason to use variable "ff"
//          for(int l=0;l<=actual_blunami_connected-1;l++){     //  top of for "l" loop  
            BlunamiCommand[loco_index].writeValue(function_group_2a,4);   //  send command to blunami
//          }   //  bottom for blunami_connected
        }   //  bottom for send command 3 times
      }    //   bottom if keypress >= 5 && <= 8
//-------------------------primary function group 2b---------------------------------
      if (keypress == '9' ) { // top if keypress
        function_group_2b[2] = loco_number[l];           // function group 2b   
        function_group_2b[3] = fg2b_current_command;
        for(int ff=1;ff<=3;ff++){   //    send the command 3 times. no reason to use variable "ff"
//          for(int l=0;l<=actual_blunami_connected-1;l++){     //  top of for "l" loop  
             BlunamiCommand[loco_index].writeValue(function_group_2b,4);   //  send command to blunami
//          }   //  bottom for blunami_connected
        }   //  bottom for send command 3 times

      }   //   bottom if keypress = 9

    }    //--bottom function command for loco # <= 127
//------------------------------bottom loco # <= 127 primary --------------------------------------
//---------------------------- top extended loco # >= 127  ---------------------------------
    else {   //  loco # >127
      
      function_group_1_e[2] = loco_left_hex_pair;            // function group 1
      function_group_1_e[3] = loco_right_hex_pair;
      function_group_1_e[4] = fg1_current_command;

      function_group_2a_e[2] = loco_left_hex_pair;           // function group 2a
      function_group_2a_e[3] = loco_right_hex_pair;
      function_group_2a_e[4] = fg2a_current_command;

      function_group_2b_e[2] = loco_left_hex_pair;           // function group 2b 
      function_group_2b_e[3] = loco_right_hex_pair;
      function_group_2b_e[4] = fg2b_current_command;

      if (keypress >= '0' && keypress <='2') {          // top if keypress 0 - 2

        for(int ff=1;ff<=3;ff++){   //    send the command 3 times. no reason for using variable "ff"
//          for(int l=0;l<=actual_blunami_connected-1;l++){     //  top of for "l" loop
            BlunamiCommand[loco_index].writeValue(function_group_1_e,5);   //  send command to each loco
//          }   //  bottom for blunami_connected
        }   //  bottom for send command 3 times
      }  //   bottom if keypress 1-2
//------------------------ keypress 3 extended function group 1------------------------------
      if (keypress == '3') {          // top if keypress 3
//    this command sounds a short horn twice. key is pushed a single time then
       
        for(int ff=1;ff<=2;ff++){   //    send the command 3 times. no reason for using variable "ff"
//          for(int l=0;l<=actual_blunami_connected-1;l++){     //  top of for "l" loop
            BlunamiCommand[loco_index].writeValue(function_group_1_e,5);   //  send 1st command to target loco
//          }   //  bottom "for" blunami_connected
        }   //  bottom for send command 3 times

      //  2/2/25 this approach  works. Not exactly what blue rail does but it is better than nothing 
      //  unlike other commands, this command is activated whenever its state changes. 
      //  with other commands, when the key is pressed once, it turns it on and then pressed again to turn off.
      //  for this command we only press the key a single time. the state goes from 0 gto 1 and sounds the horn.
      //  the next time the function is called the state goes from 1 to 0 and sounds the horn.
      //  so to set the state to 0, send another command using its previous value.
      //  to do this we will use  fg1_old_command 
      //  after it sounds we reset  f3 = 0 
      //  sound the horn a 2nd time horn command 
      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!111
      //  lets try muting the 2nd command to see if this works
      //  2/2/25 COULD NOT GET MUTING TO WORK. DOES NOT APPEAR TO REACT INSTANTANEOUSLY EVEN WITH MILLIS
      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!1111

        function_group_1_e[4] = fg1_old_command;  //  use the previous command which has horn off.         
        BlunamiCommand[loco_index].writeValue(function_group_1_e,5);   //  send 2nd horn command to loco

        f3="0";   //  here we set variable OFF  2/2/25 this REQUIRED

        function_group_1_e[4] = fg1_current_command;    //  put this back to the current value

      }  //   bottom if keypress 3

//------------------------ keypress 4 extended function group 1-------------------------------
      if (keypress =='4') {          // top if keypress 4

        for(int ff=1;ff<=3;ff++){   //    send the command 3 times. no reason for using variable "ff"
//          for(int l=0;l<=actual_blunami_connected-1;l++){     //  top of for "l" loop
            BlunamiCommand[loco_index].writeValue(function_group_1_e,5);   //  send command to each loco
//          }   //  bottom for blunami_connected
        }   //  bottom for send command 3 times
      }  //   bottom if keypress 4
//------------------------------ extended  function group 2a---------------------------------
      if (keypress >= '5' && keypress <='7') {   // top if keypress

        for(int ff=1;ff<=3;ff++){   //    send the command 3 times. no reason for using variable "ff"
//          for(int l=0;l<=actual_blunami_connected-1;l++){     //  top of for "l" loop
            BlunamiCommand[loco_index].writeValue(function_group_2a_e,5);   //  send command to blunami
//         }   //  bottom for blunami_connected
        }   //  bottom for send command 3 times
      }    //   bottom if keypress 5-7

// special case for F8 - muted - we have to send commmands to all locos
      if (keypress =='8') {   // top if keypress
        for(int ff=1;ff<=3;ff++){   //    send the command 3 times. no reason for using variable "ff"
          for(int l=0;l<=actual_blunami_connected-1;l++){     //  top of for "l" loop
            BlunamiCommand[l].writeValue(function_group_2a_e,5);   //  send command to blunami
         }   //  bottom for blunami_connected
        }   //  bottom for send command 3 times
      }    //   bottom if keypress = 8

//-------------------------extended function group 2b---------------------------------
      if (keypress == '9') {   // top if keypress
//    this command is not on/off. a single push turns it on and then we need to reset
          for(int ff=1;ff<=3;ff++){   //    send the command 3 times. no reason to use variable "ff"
//          for(int l=0;l<=actual_blunami_connected-1;l++){     //  top of for "l" loop
            BlunamiCommand[loco_index].writeValue(function_group_2b_e,5);   //  send command to blunami
  //        }   //  bottom for blunami_connected
          }   //  bottom for send command 3 times

        //  after command is sent we need to set f9 = 0 by sending another command with the bit 0 off
        //  to do this we will use  fg2b_old_command which in declared in the variable section
        //  update bit 4 which is the command
          function_group_2b_e[4] = fg2b_old_command;
//        for(int ll=0;ll<=actual_blunami_connected-1;ll++){  //  no reason to use ll
          BlunamiCommand[loco_index].writeValue(function_group_2b_e,5);   //  send command to blunami
//        }   //  bottom for
          f9="0";   //  here we set variable off
      }     //   bottom if keypress = 9
    //--------------------bottom function command for loco # > 127

    } //  end else loco # >127
  
//  2/1/25  see if this helps the F3 issue. iT DOES!!
    fg1_old_command = fg1_current_command;    //  save the previous command 


return;   // return no values.

}   //  bottom of function call
//************************bottom function command to blunami********************************

//************************ direction function ******************************************
void direction_command(){
//This funcion changes the direction of the loco.
//this function only works when speed = 0
// function group 3 controls the F/R light. remember this is not a bluerail command
//  but one that was created by me for this feature
//  we are using a simple consist so all loco numbers are the same. 
//  Therefore  the lead unit will be identified as index 0 
//  
function_group_3[2] = loco_number[0];        //  update lead loco number which is index 0
//loco_extended_address();    //  calculate extended number hex pairs  removed 1/17/25
function_group_3_e[2] = loco_left_hex_pair;    // extended left loco #     1/17/25 this needs to be used
function_group_3_e[3] = loco_right_hex_pair;   // xtended right loco #     1/17/25 this needs to be used

if (speed_value == '0'){    // only change direction when speed = 0

//  set direction and the bit corresponding command
  switch (activecommand) {
    case 'e':    //  forward light
        loco_direction = '1';                
        function_group_3[3] = 0x60;     //  primary loco #  light command
        function_group_3_e[4] = 0x60;   //  extended loco #  
     break; 

    case 'r':    //  reverse light
        loco_direction = '0';        
        function_group_3[3] = 0x40;     //  primary loco #  light command
        function_group_3_e[4] = 0x40;   //  extended loco # 
        break;
  }   //  bottom switch active command

//  1/24/25 put this delay in to see if it improves the time to change direction. not worth it
//  startMillis = millis(); while (millis() <= (startMillis + 20)) {} //  delay command

  int command_times = 3;    //  number of times we send commands to blunami
  if(loco_number[0] <=127){
       for(int mm=1;mm<=command_times;mm++){   //    send the command 3 times. no reason for using variable "mm"

          for(int l=0;l<=actual_blunami_connected-1;l++){     //  top of for "l" loop
        //  send direction light command to Mr. blue  3 times only if it is already lit    
            BlunamiCommand[l].writeValue(function_group_3,4);  // direction light command 
          }   //  bottom for blunami_connected

        }   //  bottom for send command 3 times
  }   //  bottom if loco # < 127
  else  {   //  loco # > 128
        for(int mm=1;mm<=command_times;mm++){   //    send the command 3 times. no reason to use variable "mm"
          for(int l=0;l<=actual_blunami_connected-1;l++){     //  top of for "l" loop
        //  send direction light command to Mr. blue  3 times only if it is already lit    
            BlunamiCommand[l].writeValue(function_group_3_e,5);  // direction light command         
          }   //  bottom if commands to loco 
      } //  send commands 3 times 
  }   //  bottom else


  //  with a direction change, we need to change the active function sounds emminating 
  //  from lead unit to sound from the trailing unit
  //  we do this by calling this function only when there are multiple locos
  if(actual_blunami_connected >= 2){   //  more than 1 loco
    //  trailing_unit_functions();    //  this does not work here
      lead_unit_function_change();
  }

}   //  bottom if speed = 0

  return; 
}   //  bottom function direction command

// ***************************** bottom of direction function*************************

//****************************** top of function command byte*********************************

 void function_command_byte (){ 
// this function builds the function command binary byte for functions group 1, 2a and 2b
// this function is called if ACTIVE COMMAND = 'f'
//------------------------------- FUNCTION VALUES ------------------------------
//  only process commands if there is a key press 
//  if this is not used functions oscillate

if (keypress != NO_KEY){    // 8/5/24 not sure if this is needed 

  if (keypress >= '0' && keypress <='9'){   
  //  function_value = currentkeypress;
    function_value = activekeypress;  //  8/4/24 not sure if this creates a problem
  }

if (keypress !=' '&& keypress != 'f'){  //  this is needed to ignore a no keypress or multiple "F"
                                        //  this will most likely be removed with the keypad
    switch (function_value){    //switch functions 0-9
      case '0':     //  turn light on or off
        if (f0 == "1" ){   //  light on, turn it off                   
          f0 = "0";               
        }
        else { f0 = "1";}  //  light off, turn it on              
      break;
//  -----------------case 1--------------------------------         
      case '1': //  bell on/off   function 1 
        if (f1 == "1" ){  //  on, turn off                  
          f1 = "0";       
        }
        else{f1 = "1";} //  off, turn on
      break;
//        -----------------case 2-----------------------------
      case '2':     //  function 2 long horn on/off  
        if (f2 == "1" ){                 
            f2 = "0";       
        }
        else{f2 = "1";}       
      break;
//     -----------------case 3-------------------------------
      case '3': //  function 3 short horn on/off - none latching
      //  if (f3 == "1"){
      //      f3 = "0"; }
      //  else{f3 = "1";}   //  we want to sound horn and then pause and set f3 = 0 
                      //  do this at the bottom  
          if (f3 == "0"){f3 = "1";}
          else{f3 = "0";}         
           
      break;
//  -------------------case 4-------------------------------
      case '4': //  function 4 FX3 beacon light on/off  (not on dyno stand)
        if (f4 == "1"){          
            f4 = "0";        
        }
        else { f4 = "1";}      
      break;
//  ------------------------ case 5 --------------------------------
      case '5': //  function 5  battery off no toggle, just off
        if (decoder_voltage[k] > 12){  //  voltge > 12, normal latching
          if (f5 == "1"){          
            f5 = "0";}         
          else {f5 = "1";}         
        } //  bottom if volt > 12
        else{ 
          f5 = "1";
          Serial.println("\t\t\tturn battery off (@1963)");
          }      // voltage <= 12, no latching turn battery off          
      break;
// ------------------------case 6------------------------------
      case '6': //  function 6  FX3 on/off  (not setup)
          if (f6 == "1" ){          
              f6 = "0";
          }
          else {f6 = "1"; } 
      break;
//        ------------------case 7------------------------------
      case '7':     //  function 7 dim light on/off 
          if (f7 == "1" ){
              f7 = "0";           
            }
          else {f7 = "1"; } 
      break;
//        -----------------case 8-------------------------------         
      case '8':   //  function 8  mute on/off  
            if (f8 == "1" ){          
              f8 = "0";
            }
            else{f8 = "1";}
      break;
//        -----------------case 9-------------------------------         
      case '9':   //  function 9  grade crossing 1 press only, then turn off  
        //  1/15/25 can't get this is work properly. have to press key a 2nd time to reset
        //   the command so it will work again.
          if (f9 == "0"){f9 = "1";}
          else{f9 = "0";}   
            
      break;      

    }   //  bottom of switch case "functions"   

}   //    bottom if (keypress !=' '&& keypress!='f') this makes it work
      //Serial.print("\t\t(@1907)f9:"  );Serial.println(f9);  //  used for testing
}   //  bottom NO_KEY
//------------------------------- bottom keypress----------------------------
//  if here we have the complete function binary command
//  build the command for each function group

//-----------------  FUNCTION GROUP 1 - build binary byte command----------------------
//  fg1_current_command is a global variable
//  bitWrite(x, n, b) arduino function that creates a binary byte
//  bitwrite requires an integer for "b"

//  we need to convert the keystroke Strings to integers
int f00 = f0.toInt();   //  f0 convert to integer
int f40 = f4.toInt();   //  f4 convert to integer
int f30 = f3.toInt();   //  f3 convert to integer
int f20 = f2.toInt();   //  f2 convert to integer
int f10 = f1.toInt();   //  f1 convert to integer

bitWrite(fg1_current_command, 7, 1);     //  function group 1, fixed value 
bitWrite(fg1_current_command, 6, 0);     //  function group 1, fixed value  
bitWrite(fg1_current_command, 5, 0);     //  function group 1, fixed value 
bitWrite(fg1_current_command, 4, f00);   //  f0
bitWrite(fg1_current_command, 3, f40);   //  f4
bitWrite(fg1_current_command, 2, f30);   //  f3
bitWrite(fg1_current_command, 1, f20);   //  f2
bitWrite(fg1_current_command, 0, f10);   //  f1

//-----------------  FUNCTION GROUP 2A - build binary byte command----------------------
//  we need to convert the keystroke Strings to integer
int f80 = f8.toInt();   //  f8 convert to integer
int f70 = f7.toInt();   //  f7 convert to integer
int f60 = f6.toInt();   //  f6 convert to integer
int f50 = f5.toInt();   //  f5 convert to integer

bitWrite(fg2a_current_command, 7, 1);     //  function group 2a, fixed value 
bitWrite(fg2a_current_command, 6, 0);     //  function group 2a, fixed value 
bitWrite(fg2a_current_command, 5, 1);     //  function group 2a, fixed value 
bitWrite(fg2a_current_command, 4, 1);     //  function group 2a, fixed value   
bitWrite(fg2a_current_command, 3, f80);   //  f8
bitWrite(fg2a_current_command, 2, f70);   //  f7
bitWrite(fg2a_current_command, 1, f60);   //  f6
bitWrite(fg2a_current_command, 0, f50);   //  f5    battery off

//-----------------  FUNCTION GROUP 2B - build binary byte command----------------------
//  we need to convert the keystroke Strings to integer
//int f120 = f12.toInt();   //  f12 converted to integer. not used in phase 1. default to 0
//int f110 = f11.toInt();   //  f11 converted to integer. not used in phase 1. default to 0
//int f100 = f10.toInt();   //  f10 converted to integer. not used in phase 1. default to 0
int f90 = f9.toInt();       //  f9 converted to integer

bitWrite(fg2b_current_command, 7, 1);     //  function group 2b, fixed value 
bitWrite(fg2b_current_command, 6, 0);     //  function group 2b, fixed value 
bitWrite(fg2b_current_command, 5, 1);     //  function group 2b, fixed value 
bitWrite(fg2b_current_command, 4, 0);     //  function group 2b, fixed value   
bitWrite(fg2b_current_command, 3, 0);     //  f12  not used in phase 1. default to 0
bitWrite(fg2b_current_command, 2, 0);     //  f11  not used in phase 1. default to 0
bitWrite(fg2b_current_command, 1, 0);     //  f10  not used in phase 1. default to 0
bitWrite(fg2b_current_command, 0, f90);   //  f9 - only function that is used


oldkeypress = currentkeypress;
//currentkeypress = ' ';    //  when button is released this is the value
// turn off for keypad
//keypress = ' ';    //  when button is released this is the null value

//  1/15/25 none of these commands solved the f9 problem
//if (f9 == "1"){f9 = "0";}   // set f9 = 0, see above line 1890
//f9 = "0";

//  f3 is a problem since it is not push on and then push again to turn off. 
//  after the byte is created, set f3 = 0
//if (f3=="1"){f3 = "0";}

  return;   //  function_command_byte
 }          //  function_command_byte

//****************************bottom of FUNCTION BYTE COMMAND function************************************

//***************************TOP KEYPAD KEYPRESS ******************************************
void keypad_keypress(){

// this function processes the keypad keypresses

keypress = membrane_keypad.getKey();// Read the key pressed
//  1/24/25 try delay to see if it improves response. not really
startMillis = millis(); while (millis() <= (startMillis + 20)) {} //  delay command

activekeypress = keypress;
currentkeypress = keypress;   //  1/19/25 currentkeypress is defined during keyboard entry.
//                                To be safe I initalized it here.
//  only process commands if there is a key press 
if (membrane_keypad.getState()==PRESSED && keypress != NO_KEY){ 
//if (keypress != NO_KEY){

  //  if any of the top level commands, throttle, F1, F2, loco #, forward, reverse, are issued
  //  save the keypress as active command. 
  //  1/19/25 sort commands to see if processing speeds up
  if (keypress =='e'||keypress=='f'||keypress=='g'||keypress=='l'||keypress=='r'||keypress=='t'){   
        activecommand = keypress;   
    }
// 1/19/25 not sure if this is needed because keypress is tested inside of functions and speed.
// decided to keep it in however 
  else if (keypress > '0' && keypress <='9'){    
      activekeypress = keypress;}  

//  oldkeypress = currentkeypress;    //  save this for diagnostics and viewing
  //if (currentkeypress != oldkeypress){
  //  oldkeypress = currentkeypress;}    //  save this for diagnostics and viewing

}   //  bottom of NO_KEY

oldkeypress = currentkeypress;    //  this does not work

return;  
}
//******************************-bottom keypad_keypress*********************************

//************************** top  LOCO_EXTENDED_ADDRESS*********************************
void loco_extended_address(){
//  this function calculates the integer values of the left and right HEX pairs
//  that are used to update the loco number in the function and speed HEX byte arrays
//  in the speed command it is the 3rd and 4th elements
//  in the function command it is the
//  echo loco number

//Serial.print("loco #: ");     //  turn off, for testing only
//Serial.println(loco_number);  //  turn off, for testing only
/*
//  loco number must be greater than 127. if not exit

if(loco_number[k] <128 || loco_number[k] > 9999){
  Serial.println("extended loco number required");
  Serial.println("must be >=128 and <=9999");
  return;
}
*/
//  convert the loco number to HEX
loco_hex_pair = String(loco_number[k], HEX);
//Serial.print("hex pair: ");     //  turn off, for testing only
//Serial.println(loco_hex_pair);  //  turn off, for testing only

// if loco hex number length is <4, then pad with zeros.
loco_hex_pair_length = loco_hex_pair.length();
//Serial.print("hex pair length: ");      //  turn off, for testing only
//Serial.println(loco_hex_pair_length);   //  turn off, for testing only

switch (loco_hex_pair_length){
  case 2:     //  hex pair is 2 characters, pad with 2 zeros
    loco_hex_pair_4digits = "00" + loco_hex_pair;
    break;
  case 3:     //  hex pair is 3 characters, pad with 1 zero
    loco_hex_pair_4digits= '0' + loco_hex_pair;
    break;
  case 4:
    loco_hex_pair_4digits= loco_hex_pair;
}
//Serial.print("hex command (edited): ");   //  turn off, for testing only
//Serial.println(loco_hex_pair_4digits);    //  turn off, for testing only

// parse the command into individual bits for b2 b1 b0
//  b3 is a blue rail character that changes depending upon the loco number 
b0 = loco_hex_pair_4digits.substring(3, 4);
b1 = loco_hex_pair_4digits.substring(2, 3);
b2 = loco_hex_pair_4digits.substring(1, 2);

//  replace b3 string value with  the bluerail character
if (loco_number[k] >=128 && loco_number[k] <=4095){b3 = "c";}
else if (loco_number[k] >=4096 && loco_number[k] <=8191){b3 = "d";}  
else if(loco_number[k] >=8192 && loco_number[k] <=9999){b3 = "e";}

//  print the bluerail bits, for testing only
/*
Serial.println();
Serial.println("     blue rail values");
Serial.println("b3:\tb2\tb1\tb0 ");
Serial.print(b3);
Serial.print("\t");
Serial.print(b2);
Serial.print("\t");
Serial.print(b1);
Serial.print("\t");
Serial.println(b0);
*/
//  call the hexstring_to_integer function to convert 
//  the string bits (b0, b1, b2, be) to integers (bit0, bit1, bit2, bit3)
bit0 = hexstring_to_integer(b0);
bit1 = hexstring_to_integer(b1);
bit2 = hexstring_to_integer(b2);
bit3 = hexstring_to_integer(b3);
//********************************************************************
//  this is all we care about.
//  calculate the hex pair for loco number
//  these are what are used to update the extended loco number for speed and function commands
loco_left_hex_pair = (bit2 + (bit3*16));
loco_right_hex_pair = (bit0 + (bit1*16));
//*********************************************************************

/*    for testing only
Serial.print ("bit 0 = ");
Serial.println(bit0);
Serial.print ("bit 1 = ");
Serial.println(bit1);
Serial.print ("bit 2 = ");
Serial.println(bit2);
Serial.print ("bit 3 = ");
Serial.println(bit3);

Serial.print("left hex pair:  ");
Serial.println(loco_left_hex_pair);
Serial.print("right hex pair:  ");
Serial.println(loco_right_hex_pair);
*/
return;
}   //  bottom of loco_extended_address function
//*********************************bottom of loco_extended_address function****************************

//********************************  hex to integer function************************************
int hexstring_to_integer(String hexstring)
//  this function receives the hex string bit and returns the integer value of it
//  ALL VARIABLES ARE LOCAL. DO NOT DECLARE AS GLOBAL
{
int bit_int;

if (hexstring == "a"){bit_int = 10;}
else if (hexstring=="b"){bit_int=11;}
else if (hexstring=="c"){bit_int=12;}
else if (hexstring=="d"){bit_int=13;}
else if (hexstring=="e"){bit_int=14;}
else if (hexstring=="f"){bit_int=15;}
else{bit_int=hexstring.toInt();}

 return (bit_int); // return the the integer value of hex character
}   //  bottom of hex to integer function
//**************************-bottom of hex to integer function*****************************************

//***********************************rail_voltage function-(top)******************************
//  this function reads the blunami RailVlts characteristic to get the
//  rail voltage. The value is divided by 10 for the actual value.
float rail_voltage(int k){

//  this byte has been declared globally
//byte rail_mtr_volts[] = {0x0A,0x01,0x01,0x00,0x00,0x00,0x00 };   // from light blue format
//  sending this command first seems to allow the correct voltages to be read.
/*
BlunamiCommand.readValue(speed,5);
int dcc1 = speed[0];
int dcc2 = speed[1];
int dcc3 = speed[2];
int dcc4 = speed[3];
int dcc5= speed[4];
*/
motorvoltage[k].readValue(rail_mtr_volts,7);
float battvolts1 = rail_mtr_volts[0];    //  this is the correct one
float battvolts2 = rail_mtr_volts[1];
float battvolts3 = rail_mtr_volts[2];
float battvolts4 = rail_mtr_volts[3];
float battvolts5 = rail_mtr_volts[4];
float battvolts6 = rail_mtr_volts[5];
float battvolts7 = rail_mtr_volts[6];

motorvoltage[k].readValue(rail_mtr_volts,7);
//startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command

start_rail_volts();   //  resend volts initialization command. not sure it makes any difference

motorvoltage[k].readValue(rail_mtr_volts,7);
//startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command

//Serial.print("\tvoltage (@1540):  ");
//Serial.println(battvolts1/10);

return (battvolts1/10);  //  return the rail volts to main sketch
}
//**************************rail voltage function (bottom)**********************************

//*****************************-LCD TITLE PAGE-************************************
void lcd_title_page(){
//  this function displays a title page at startup
  int title_col = 10;
  int title_row = 25;

  u8g2.clearBuffer(); //  must clear 1st

  u8g2.setFont(u8g2_font_lubBI14_te);   
  u8g2.setCursor(title_col, title_row);
  u8g2.print("BLUE CAB");

  u8g2.setCursor(title_col+13, title_row+15);
  u8g2.setFont(u8g2_font_Terminal_te);
  u8g2.print("Blue Tooth");

  u8g2.setCursor(title_col-8, title_row+27);
  u8g2.setFont(u8g2_font_Terminal_te);
  u8g2.print("Locomotive Control");

  u8g2.sendBuffer();    //  this sends the commands to the LCD

return;  
}
//**************************** BOTTOM LCD TITLE PAGE-**********************************

//**********************************LCD DISPLAY **********************************
void lcd_display(){
    //  this function displays the keypresses. 
    //  the variables initialized are local and not updated in the main sketch
    //  a global variable "startup_screen" will be used to display the screen at startup 
    //  before any keypresses are made
    int row_activecommand;  //  row of the active command (F,T,etc)
    int col_activecommand;  //  col of the active command (F,T,etc)
    int row_activekeypress; //  row of active command value (1, 2, etc)
    int col_activekeypress; //  col of active command value 
    String activecommand_cap; //  this stores the commands as upper case
    String activecommand_description;

    row_activecommand = 25;   //  active command
    col_activecommand = 3;
    row_activekeypress = row_activecommand; //  set the rows equal
    col_activekeypress = 55;  //  command value 
    int row_loconumber = 5;
    int col_loconumber =  80;
    String lcd_direction;   //  stores the loco integer direction as a string.
 
//-----------convert command to upper case----------------
    if (activecommand == 't'){        
      activecommand_cap = "T:";         //  throttle
      activecommand_description = "Throttle";
    }
    else if (activecommand == 'f'){    //  function group 1 
      activecommand_cap = "F1:";       
      activecommand_description = "Function Group 1";      
    }
    else if (activecommand == 'g'){   //  function group 2 
      activecommand_cap = "F2:";       
      activecommand_description = "Function Group 2";
    }
    else if (activecommand == 'l'){   //  loco number   3/6/25 disabled
//      activecommand_cap = "L:";        
//      activecommand_description = "Locomotive Number"; 
        lcd_loco_summary_page();
        
    }
    else if (activecommand == 'r'){   //  reverse
      activecommand_cap = "R";
      activecommand_description = "Reverse";       
    }       
    else if (activecommand == 'e'){  //  forward 
      activecommand_cap = "F";
      activecommand_description = "Forward";
    }
//---------------bottom convert to upper case-----------------------------------------
//---------------convert the integer direction command in main sketch to a string-------------
    if (loco_direction=='1'){lcd_direction = "For";}
    else {lcd_direction = "Rev";}
//---------------end convert integer direction to a string------------------

//  if (membrane_keypad.getState()==PRESSED && keypress != NO_KEY){     //  only update screen after a keypress
if (activecommand != 'l'){    //  do not run this code is keypress is "L" for loco
//-----------send command to LCD--to upper left-----------
    u8g2.clearBuffer(); //  must clear 1st

  // display active command
    u8g2.setFont(u8g2_font_helvB24_te);  // 24 point  
    if(activecommand=='t'){u8g2.setCursor(col_activecommand, row_activecommand);} // this lines up the commands
    else{u8g2.setCursor(col_activecommand-2, row_activecommand);}
    u8g2.print(activecommand_cap);  //  display active command

    //  display active command value (0-9)
    if (activecommand == 't'){u8g2.setCursor(col_activekeypress-16, row_activekeypress);}
    else{u8g2.setCursor(col_activekeypress, row_activekeypress);}
    //  only print activekeypress if it is >0 and <=9
    if(activekeypress >='0' && activekeypress <= '9'){
        u8g2.print(activekeypress);}  //  display active command value
    //u8g2.print(currentkeypress);  //  display active command value - keypad uses this value  

    //  display the active command summary
   // u8g2.setFont(u8g2_font_smallsimple_te); //
    u8g2.setFont(u8g2_font_micro_tr);   
    u8g2.setCursor(col_activecommand, row_activecommand+9);
    u8g2.print("Command:");
    u8g2.setCursor(col_activecommand+35, row_activecommand+9);
    u8g2.print(activecommand_description);

    //  display loco number upper right corner     3/7/25 DISABLED - INFO IN LOCO SCREEN
    //u8g2.setFont(u8g2_font_micro_tr);
    //u8g2.setCursor(col_loconumber, row_loconumber);
    //u8g2.print("Loco #:");
    //u8g2.setCursor(col_loconumber+30, row_loconumber);
    //u8g2.print(loco_number[k]);

    //  display voltage upper right corner  3/7/25 DISABLED - INFO IN LOCO SCREEN
  //  u8g2.setFont(u8g2_font_micro_tr);
  //  u8g2.setCursor(col_loconumber+4, row_loconumber+8);
  //  u8g2.print("Volts:");
  //  u8g2.setCursor(col_loconumber+28, row_loconumber+8);  
  //  u8g2.print(decoder_voltage[k], 2); //  requires float variable 

    //  draw line under command summary
    u8g2.drawLine(0, row_activecommand+11, 128, row_activecommand+11);

    //  display speed step and direction    
    u8g2.setFont(u8g2_font_micro_tr);
    u8g2.setCursor(col_activecommand, row_activecommand+18);
    u8g2.print("Speed Step:");
    u8g2.setCursor(col_activecommand+46, row_activecommand+18);
    u8g2.print(speed_step);

    u8g2.setCursor(col_activecommand+65, row_activecommand+18);
    u8g2.print("Direction:");
    u8g2.setCursor(col_activecommand+107, row_activecommand+18);
    u8g2.print(lcd_direction);

    //  draw line above function heading
    u8g2.drawLine(0, row_activecommand+20, 128, row_activecommand+20); 

    //  draw the title "function status" above circles
    int function_heading_row = row_activecommand+27;  //  this controls the heading and circle row locations
    u8g2.setFont(u8g2_font_micro_tr); 
    u8g2.setCursor(32, function_heading_row);
    u8g2.print("Active Functions");

    //  draw circles for function online summary    
    int circle_radius = 2;
    int circle_start_col = col_activecommand+2;
    int circle_spacing = 13;
    int circle_row = function_heading_row+3;    //  this is tied to function heading.


   // draw circles, either empty or filled depending upon function value 
    //                               (col,            row,      radius)
    if (f0 == "0"){u8g2.drawCircle(circle_start_col,circle_row,circle_radius);}       // f0, 1st circle
    //                               (col,            row,      radius 1,       radius 2)                               
      else {u8g2.drawFilledEllipse(circle_start_col,circle_row,circle_radius,circle_radius);}
    if (f1=="0"){u8g2.drawCircle(circle_start_col+(1*circle_spacing),circle_row,circle_radius);} // f1, 2nd circle
      else {u8g2.drawFilledEllipse(circle_start_col+(1*circle_spacing),circle_row,circle_radius,circle_radius);}
    if (f2=="0"){u8g2.drawCircle(circle_start_col+(2*circle_spacing),circle_row,circle_radius);} // f2, 3rd circle  
      else {u8g2.drawFilledEllipse(circle_start_col+(2*circle_spacing),circle_row,circle_radius,circle_radius);}      
    if(f3=="0"){u8g2.drawCircle(circle_start_col+(3*circle_spacing),circle_row,circle_radius);}// f3, 4th circle 
      else{u8g2.drawFilledEllipse(circle_start_col+(3*circle_spacing),circle_row,circle_radius,circle_radius);}
    if(f4=="0"){u8g2.drawCircle(circle_start_col+(4*circle_spacing),circle_row,circle_radius);}// f4, 5th circle
      else{u8g2.drawFilledEllipse(circle_start_col+(4*circle_spacing),circle_row,circle_radius,circle_radius);}
    if(f5=="0"){  u8g2.drawCircle(circle_start_col+(5*circle_spacing),circle_row,circle_radius);}// f5, 6th circle 
      else{u8g2.drawFilledEllipse(circle_start_col+(5*circle_spacing),circle_row,circle_radius,circle_radius);}
    if(f6=="0"){u8g2.drawCircle(circle_start_col+(6*circle_spacing),circle_row,circle_radius);}// f6, 7th circle 
      else{u8g2.drawFilledEllipse(circle_start_col+(6*circle_spacing),circle_row,circle_radius,circle_radius);}
    if(f7=="0"){u8g2.drawCircle(circle_start_col+(7*circle_spacing),circle_row,circle_radius);}// f7, 8th circle
      else{u8g2.drawFilledEllipse(circle_start_col+(7*circle_spacing),circle_row,circle_radius,circle_radius);}
    if(f8=="0"){u8g2.drawCircle(circle_start_col+(8*circle_spacing),circle_row,circle_radius);}// f8, 9th circle
      else{u8g2.drawFilledEllipse(circle_start_col+(8*circle_spacing),circle_row,circle_radius,circle_radius);}
    if(f9=="0"){u8g2.drawCircle(circle_start_col+(9*circle_spacing),circle_row,circle_radius);}// f9, 10th circle  
      else{u8g2.drawFilledEllipse(circle_start_col+(9*circle_spacing),circle_row,circle_radius,circle_radius);}

//  display function definitions below circles
    int function_desc_row = circle_row+9;   //  this row spaciing is linked to circles. do not change
    u8g2.setFont(u8g2_font_micro_tr); 
    u8g2.setCursor(circle_start_col-3, function_desc_row); //  f0
    u8g2.print("F0");
    u8g2.setCursor(circle_start_col-3+(1*circle_spacing), function_desc_row); //  f1
    u8g2.print("F1");
    u8g2.setCursor(circle_start_col-3+(2*circle_spacing), function_desc_row); //  f2
    u8g2.print("F2");
    u8g2.setCursor(circle_start_col-3+(3*circle_spacing), function_desc_row); //  f3
    u8g2.print("F3");
    u8g2.setCursor(circle_start_col-3+(4*circle_spacing), function_desc_row); //  f4
    u8g2.print("F4");
    u8g2.setCursor(circle_start_col-3+(5*circle_spacing), function_desc_row); //  f5
    u8g2.print("F5");
    u8g2.setCursor(circle_start_col-3+(6*circle_spacing), function_desc_row); //  f6
    u8g2.print("F6");
    u8g2.setCursor(circle_start_col-3+(7*circle_spacing), function_desc_row); //  f7
    u8g2.print("F7");
    u8g2.setCursor(circle_start_col-3+(8*circle_spacing), function_desc_row); //  f8
    u8g2.print("F8");
    u8g2.setCursor(circle_start_col-3+(9*circle_spacing), function_desc_row); //  f9
    u8g2.print("F9");

    u8g2.sendBuffer();    //  this sends the commands to the LCD

}   //  bottom of if command || l  
  //}   //  bottom of NO_KEY

return;
}
//********************************bottom LCD DISPLAY************************************

//************************* CV_LOCO_NUMBER FUNCTION*********************************
int cv_loco_number(int k){
//  this function reads CV29, CV1, CV17 & CV18 and then determines if the LOCO number is primary or extended
// if primary, then the loco number is CV 1.
//  if extended #, then CV 17 is the left hex pair and CV18 is the right hex pair which forms the
//  entire loco number
//  the cv bytes are defined globally  
//  byte cv_29[] = {0x03,0x04,0x00,0x74,0x1D,0x01 };  //  CV 29 - configuration data 1
//  byte cv_1[] =  {0x03,0x04,0x00,0x74,0x01,0x01 };  //  CV 1 - primary loco #
//  byte cv_17[] =  {0x03,0x04,0x00,0x74,0x11,0x01 };  // CV 17 - extended loco # left hex pair
//  byte cv_18[] =  {0x03,0x04,0x00,0x74,0x12,0x01 };  // CV 18 - extended loco # right hex pair
//  all varibles are local.
//  all print statements are for diagnostics and will be commented out.
//  the function returns the loco #  <-------------------------IMPORTANT

//  11/30/24 CREATING AN LOCONUMBER IS PROBABLY NOT NECESSARY
int loconumber[6] ={0,0,0,0,0,0};             //  local variable to store  the calculated loco number
int loco_number_type = 0;  //defines whether loco # is primary or extended. 0 for primary, 1 for extended

//--------------------------------READ CV 29-------------------------------
//  send CV 29 command to blunami
    BlunamiCommand[k].writeValue(cv_29,6);
    startMillis = millis(); while (millis() <= (startMillis + cv_delay)) {} //  delay command

//  read cv29 response
    BlunamiCommand[k].readValue(cv_29,6);    
    int cv29_5 = cv_29[5];    //  bit 5 which contains the loco # type (primary or extended)
    String cv29_5_hex = String(cv29_5, HEX);    //  convert cv29 to a HEX string
    int cv29_hex_length = cv29_5_hex.length();          //  calculate length of hex pair
    if (cv29_hex_length == 1){cv29_5_hex = "0" + cv29_5_hex;}   //  add leading zero if length = 0

    startMillis = millis(); while (millis() <= (startMillis + cv_delay)) {} //  delay command
    //Serial.print("\t\t\t\tcv29(5) (@1786):  ");
    //Serial.print(cv29_5);
    //Serial.print("\t");
    //Serial.println(cv29_5_hex);
//  the left bit of cv29 hex value determines if the loco # is primary or extended.
//  if the value is 0 or 1, then primary. If it is 2 or 3 then extended.
//  strip out the left character and test it.
    String cv29_bit1 = cv29_5_hex.substring(0, 1);
    //Serial.print("\t\t\t\tcv29 left bit(@1797) =  ");
    //Serial.println(cv29_bit1);

//  determine if loco # is primary or extended by looking at the left bit of cv29
    if (cv29_bit1 <= "1"){    //  primary number
        loco_number_type = 0;
    //  Serial.println("\t\t\t\tprimary loco #");
      }
      else{   //  extended number
        loco_number_type = 1;
    //    Serial.println("\t\t\t\textended loco #");
      }
 //-----------------------------READ--CV 1---------------------------------------------   
    BlunamiCommand[k].writeValue(cv_1,6);
    startMillis = millis(); while (millis() <= (startMillis + cv_delay)) {} //  delay command
    BlunamiCommand[k].readValue(cv_1,6);
    startMillis = millis(); while (millis() <= (startMillis + cv_delay)) {} //  delay command

    int cv1_5 = cv_1[5];  
    //Serial.print("\t\t\t\tcv1(5) (@1803):  ");
    //Serial.println(cv1_5);
//-------------------------------READ-CV 17---------------------------------------------------
    BlunamiCommand[k].writeValue(cv_17,6);
    startMillis = millis(); while (millis() <= (startMillis + cv_delay)) {} //  delay command
    BlunamiCommand[k].readValue(cv_17,6);
    startMillis = millis(); while (millis() <= (startMillis + cv_delay)) {} //  delay command

    int cv17_5 = cv_17[5];                              //  save read value as integer
    String cv17_5_hex = String(cv17_5, HEX)  ;          //  convert to string
    int cv17_hex_length = cv17_5_hex.length();          //  calculate length of hex pair
    if (cv17_hex_length == 1){cv17_5_hex = "0" + cv17_5_hex;}   //  add leading zero if length = 0
    //Serial.print("\t\t\t\tcv17(5) (@1812):  ");
    //Serial.print(cv17_5);
    //Serial.print("\t");
    //Serial.println(cv17_5_hex);

//-------------------------------READ-CV 18---------------------------------------------------------
    BlunamiCommand[k].writeValue(cv_18,6);
    startMillis = millis(); while (millis() <= (startMillis + cv_delay)) {} //  delay command
    BlunamiCommand[k].readValue(cv_18,6);
    startMillis = millis(); while (millis() <= (startMillis + cv_delay)) {} //  delay command

    int cv18_5 = cv_18[5];                          //  save read value as integer
    String cv18_5_hex = String(cv18_5, HEX)  ;      //  convert to string
    int cv18_hex_length = cv18_5_hex.length();      //  calculate length of hex pair
    if (cv18_hex_length == 1){cv18_5_hex = "0" + cv18_5_hex;}   //  add leading zero if length = 0
    //Serial.print("\t\t\t\tcv18(5) (@1828):  ");
    //Serial.print(cv18_5);
    //Serial.print("\t");
    //Serial.println(cv18_5_hex);

//---------------------------DEPENDING ON CV29 VALUES, CALCULATE THE LOCO NUMBER-----------------
    if(loco_number_type==0){loconumber[k] = cv1_5;}    //  primary number        
      
    else{         //  if here we calculate the extended number

//-------------------------calculate extended loco # from cv17 & cv 18---------------------
//      print original pairs    
//        Serial.print("\t\t\t\t original  hex pair: ");
//        Serial.print(cv17_5_hex);
//        Serial.println(cv18_5_hex);

//  bit 0 of cv17 is unique to bluerail. they substitue a letter for the correct number.
//  replace the letter with a number.
//  this arduino string function searches the string and replaces the character with a number.
//  spreadsheet "dcc-ex binary commands" shows how the hex pairs are created and what they should be
        switch (cv17_5_hex.charAt(0)) {
          case 'c':
              cv17_5_hex.setCharAt(0,'0');
            break;
          case 'd':
              cv17_5_hex.setCharAt(0,'1');         
            break;
          case 'e':
              cv17_5_hex.setCharAt(0,'2');         
            break; 
        } //  bottomof switch
//        Serial.print("\t\t\t\t updated hex pair (@1860): ");
//        Serial.print(cv17_5_hex);
//        Serial.println(cv18_5_hex);

//  calculate the loco number
//  first we need to extract the individual characters
//  the bits are cv17_b3, cv17_b2, cv18_b1, cv18_b0
//  loco # = (cv18_b0*1)+(cv18_b1*16)+(cv17_b2*256)+(cv17_b3*4096)
        String cv17_b3 = cv17_5_hex.substring(0, 1);
        String cv17_b2 = cv17_5_hex.substring(1, 2);
        String cv18_b1 = cv18_5_hex.substring(0, 1);
        String cv18_b0 = cv18_5_hex.substring(1, 2);
/*
    Serial.print("\t\t\t\tindividual bits (@1873):  ");
    Serial.println(cv17_b3);
    Serial.print("\t\t\t\t\t\t\t  ");
    Serial.println(cv17_b2);
    Serial.print("\t\t\t\t\t\t\t  ");
    Serial.println(cv18_b1);
    Serial.print("\t\t\t\t\t\t\t  ");
    Serial.println(cv18_b0);
*/
//  call the hexstring_to_integer function to convert 
//  the string bits (b0, b1, b2, be) to integers (bit0, bit1, bit2, bit3)
        int cv18bit0 = hexstring_to_integer(cv18_b0);
        int cv18bit1 = hexstring_to_integer(cv18_b1);
        int cv17bit2 = hexstring_to_integer(cv17_b2);
        int cv17bit3 = hexstring_to_integer(cv17_b3);

        loconumber[k] = (cv18bit0)+(cv18bit1*16)+(cv17bit2*256)+(cv17bit3*4096);  

    }   //  bottom else  extended number calculation 

  //    Serial.println();
  //    Serial.print("\t(@2300) Name:  ");Serial.print(Blunami_name[k]);
  //    Serial.print("\tIndex:  ");Serial.print(k);
  //    Serial.print("\tLoco number:  ");
  //    Serial.println(loconumber[k]);

// 11/30/24 there are times when the loco number calculated is 1, which is incorrect
//  if this happens then rerun the function
  if(loconumber[k]==1){
    loconumber[k]=0;
    cv_loco_number(k);    // recall the function
  }

return (loconumber[k]);   //  11/29/24 created array to see if that helps. it does not
}

//*************************BOTTOM CV_LOCO_NUMBER FUNCTION*********************************

//****************************RAIL VOLTS START COMMAND*************************************
void start_rail_volts(){
//  this function sends the hex command that seems to tell blunami to display the correct rail volts.
//  at start up the rail volts are read, the value is A0 or 16 volts. After sending the command, the rails volts are
//  6C or 10.8 volts which is what the blue rail app dislays.
//  the hex command sent is "0A 01 01"
//  this command does not require loco #
//  will define byte here and not globally    (seems to work 8/23/24)

  byte start_rail_volts[] = {0x0A,0x01,0x01,0x00 };

  BlunamiCommand[k].writeValue(start_rail_volts,4);  //  send command to blunami
  startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command

  return;
}

//*****************************BOTTOM VOLT START COMMAND*********************************

//*****************************CONFIRM CONNECTION****************************************
void confirm_connect(int k){
//  this function sounds the horn and turns the light on to confirm that we are connected. 
//  it will do this for  each blunami connected.
//  this mimicks what the bluerail app does.
//  these are the function group 1 commmands that are declared globally

//  byte function_group_1[] = {0x02,0x02,0x03,0x80 };
//  byte function_group_1_e[] = {0x02,0x03,0xC0,0x80,0x80 };
    fg1_current_command = 0x94; //  this turns on light and sounds horn
//  fg1_current_command = 149;    //  integer equivalent (not used)
  
  if (loco_number[k] <= 127){    //  <----------------PRIMARY
    // update the primary loco number and command for all functions
    function_group_1[2] = loco_number[k];            // function group 1
    fg1_current_command = 0x94;   //  light and short horn
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command 
    function_group_1[3] = fg1_current_command;

    //  turn on light on and sound horn
    BlunamiCommand[k].writeValue(function_group_1,4);   //  send command to blunami
//    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command 
    BlunamiCommand[k].writeValue(function_group_1,4);   //  send command to blunami
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command 
   // BlunamiCommand.writeValue(function_group_1,4);   //  send command to blunami
   // startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command 
    //  turn  light on only
    fg1_current_command = 0x90;   //  light only
      startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command
    function_group_1[3] = fg1_current_command;
    BlunamiCommand[k].writeValue(function_group_1,4);   //  send command to blunami
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command 
    BlunamiCommand[k].writeValue(function_group_1,4);   //  send command to blunami
  //  startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command 
  //  BlunamiCommand.writeValue(function_group_1,4);   //  send command to blunami
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command 
    }
  else {   //  loco # >127    <----------------EXTENDED
    // update the extended loco number and command for all functions
    //  THESE ARE GLOBAL VARIABLES
    loco_extended_address();    //  calculate extended number hex pairs  
    function_group_1_e[2] = loco_left_hex_pair;            // function group 1
    function_group_1_e[3] = loco_right_hex_pair;
    fg1_current_command = 0x94;   //  horn and light on
    function_group_1_e[4] = fg1_current_command;

     //  turn on light on and sound horn
    BlunamiCommand[k].writeValue(function_group_1_e,5);   //  send command to blunami
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command
  //  BlunamiCommand[k].writeValue(function_group_1_e,5);   //  send command to blunami
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command
    BlunamiCommand[k].writeValue(function_group_1_e,5);   //  send command to blunami
  //  startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command
    //  turn light on only
  //  startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command

    fg1_current_command = 0x90;  //  light only
    function_group_1_e[4] = fg1_current_command;
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command
    BlunamiCommand[k].writeValue(function_group_1_e,5);   //  send command to blunami
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command
    BlunamiCommand[k].writeValue(function_group_1_e,5);   //  send command to blunami
  //  startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command
    }
 
  return;
}
//********************************BOTTOM CONFIRM CONNECTION*********************************

//*******************************CONFIRM DISCONNECT****************************************
void confirm_disconnect(int k){
//  this function sounds the horn and turns the light off to confirm that we are disconnected. 
//  it also gives an E-stop command to prevent a run-away
//  it will do this for  each blunami connected.
//  these are the function group 1 commmands that are declared globally

//  byte function_group_1[] = {0x02,0x02,0x03,0x80 };
//  byte function_group_1_e[] = {0x02,0x03,0xC0,0x80,0x80 };
    fg1_current_command = 0x94; //  turns on light and horn
    //fg1_current_command = 0x80; //  turns all functions off
 
  if (loco_number[k] <= 127){    //  <------------------PRIMARY #
    // update the primary loco number and command for all functions
    function_group_1[2] = loco_number[k];            // function group 1
    fg1_current_command = 0x94; //  turns on light and horn
    function_group_1[3] = fg1_current_command;

    //  sound short horn
    fg1_current_command = 0x94; //  turns on light and horn
    function_group_1[3] = fg1_current_command;
    BlunamiCommand[k].writeValue(function_group_1,4);   //  send command to blunami
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command  
    BlunamiCommand[k].writeValue(function_group_1,4);   //  send command to blunami 
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command

    //  turn off all functions 
    fg1_current_command = 0x80;
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command 
    function_group_1[3] = fg1_current_command;
    BlunamiCommand[k].writeValue(function_group_1,4);   //  send command to blunami
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command  
    BlunamiCommand[k].writeValue(function_group_1,4);   //  send command to blunami 
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command
    }
  else {   //  loco # >127    <----------------------EXTENDED #
    // update the extended loco number and command for all functions
    loco_extended_address();    //  calculate extended number hex pairs  
    function_group_1_e[2] = loco_left_hex_pair;            // function group 1
    function_group_1_e[3] = loco_right_hex_pair;
    function_group_1_e[4] = fg1_current_command;

    //  sound short horn
    fg1_current_command = 0x94; //  turns on light and horn
    function_group_1_e[4] = fg1_current_command;
    BlunamiCommand[k].writeValue(function_group_1_e,5);   //  send command to blunami
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command    
    BlunamiCommand[k].writeValue(function_group_1_e,5);   //  send command to blunami
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command

    //  turn off all functions 
    fg1_current_command = 0x80;
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command  
    function_group_1_e[4] = fg1_current_command;
    BlunamiCommand[k].writeValue(function_group_1_e,5);   //  send command to blunami
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command  
    BlunamiCommand[k].writeValue(function_group_1_e,5);   //  send command to blunami 
    startMillis = millis(); while (millis() <= (startMillis + command_delay)) {}  //  delay command
    }   //  end else
  
  return;
}

//*******************************BOTTOM CONFIRM DISCONNECT*******************************

//********************************** top tickle ****************************
void tickle_all(int num_blunamis_connect){
// this function tickles the blunamis by sending a "if connect" command that does not PRINT RESULTS
//  10/27/24 not sure this will be of any value but blunami seems to drop connection when inactive
//  update the "int blunami_connected" array 
//  we want to recalculate the actual number of connected blunamis that are used as a test
//  in the IF CONNECTED AND WHILE CONNECTED
  
//  this checks the connect and connected status
//  this updates the "int blunami_connected" array

  actual_blunami_connected = 0;   //  set this to 0 and then recalculate
  for (int b=0; b<= num_blunamis_connect-1; b++){
    if(Blucab_Blunami[b].connect())
    if(Blucab_Blunami[b].connected()){blunami_connected[b]=1;
        actual_blunami_connected = actual_blunami_connected + 1;}   // this updates the actual connected
    else{blunami_connected[b]=0; } 

  } //  bottom for tickle

}   //  bottom tickle

//******************************************top connection status**********************************
void connected_status(int line)
//  this function shows all blunamis connected
//  and it updates the int array blunami_connected[k] which is used in the LOOP for and while loops

{     // open void connected_status

for (int x = 0; x <= (num_blunamis_connect-1); x++)
  {    //TOP FOR
  Serial.print("\t\t(@");
  Serial.print(line);Serial.print(") ");  
  Serial.print("\tName: ");Serial.print(Blunami_name[x]);  
  Serial.print("\tConnect:  ");if(Blucab_Blunami[x].connect()){Serial.print(1);}else{Serial.print(0);} 
  Serial.print("\tConnected:  "); if(Blucab_Blunami[x].connected())
  {Serial.print(1);blunami_connected[k]=1;}   //  update connected status integer
  else{Serial.print(0);}
  Serial.print("\tK: ");Serial.println(x);

  } // close for   

} // close void connected_status
//***************************************Bottom connection status**********************************
void connected_status1(int line, int x)
//  this function shows 1 blunamis connected at a time
//  you pass the line number and global index K
// x becomes the index
{     // open void connected_status(String label)
 
  startmillis = millis(); while (millis() <= (startmillis + 20)) {} //  delay command (20)
  Serial.print("\t\t(@");
  Serial.print(line);Serial.print(") ");  
  Serial.print("\tName: ");Serial.print(Blunami_name[x]);  
  Serial.print("\tConnect:  ");if(Blucab_Blunami[x].connect()){Serial.print(1);}else{Serial.print(0);} 
   startmillis = millis(); while (millis() <= (startmillis + 20)) {} //  delay command (20)
  Serial.print("\tConnected:  "); if(Blucab_Blunami[x].connected())
      {Serial.print(1);blunami_connected[x]=1;}   //  update connected status integer
  else{Serial.print(0);}
  startmillis = millis(); while (millis() <= (startmillis + 20)) {} //  delay command (20)
  Serial.print("\tK: ");Serial.println(x);

} // close void connected_status(label)
//***************************************Bottom connection status**********************************

//---------------------------DISCONNECT HANDLER---------------------------------------

void disconnecthandler(BLEDevice Blucab_Blunami) { 
  // central disconnected event handler
 
  Serial.println();
  Serial.print("\t\tDisconnect event: "); 
  Serial.print(Blunami_name[k]);
  Serial.print("  ");Serial.print(Blunami_Address[k]);
  Serial.print("  K: ");Serial.println(k);
  blunami_connected[k] = 0;   //  set the flag to disconnected
  
//  Serial.println("\tReconnecting in handler ");
//  clearscreen();
//  blunami_connected[k]=0;   //  update int array that stores connection status
//  k=0;    //   11/16/24 if there is a disconnect then start over
//  BLE.scanForUuid(Blunami_UUID);
//  Blunami = BLE.available();
//  find_peripheral();
//  Blunami_connect(k);   //  call connect function
 //setup();
}
//-------------------------- BOTTOM DISCONNECT HANDLER--------------------------------
//---------------------------CONNECT HANDLER---------------------------------------
void connecthandler(BLEDevice Blucab_Blunami) {
  // central disconnected event handler
  Serial.println();
  Serial.print("\t\tConnected event: ");
  Serial.print(Blunami_name[k]);
  Serial.print("  ");Serial.print(Blunami_Address[k]);
  Serial.print("  K: ");Serial.println(k);
  blunami_connected[k] = 1;   //  set flag to connected
}
//-------------------------- BOTTOM CONNECT HANDLER--------------------------------
//----------------------------------- top clear screen ------------------------------------
void clearscreen()
{
// this function prints 30 blank rows to simulate clearing the screen.
for(int row=0;row<=30;row++){Serial.println();}

return;
}   //  bottom function
//----------------------------------- bottom clear screen--------------------------------------------

//------------------------------ connection_status_all-------------------------------------------
void connection_status_all(int line_num,int k){
//  this function updates the connection status for all blunamis
//  remember "k" is the index of the connect array
//  max 6 connected
 switch (k) {    //  top switch
      case 0:   //  1 connected
        connected_status1(line_num,k);    //  check connections for current blunami
      //  Serial.println();
        break;
      case 1:   //  2 connected
        connected_status1(line_num,k-1);    //  check connections for previous blunami
        connected_status1(line_num,k);    //  check connections for current blunami        
        break;
      case 2:   //  3 connected
        connected_status1(line_num,k-2);    //  check connections for previous blunami
        connected_status1(line_num,k-1);    //  check connections for previous blunami        
        connected_status1(line_num,k);    //  check connections for current blunami        
        break;
      case 3:   //  4 connected
        connected_status1(line_num,k-3);    //  check connections for previous blunami
        connected_status1(line_num,k-2);    //  check connections for previous blunami
        connected_status1(line_num,k-1);    //  check connections for previous blunami     
        connected_status1(line_num,k);    //  check connections for current blunami        
        break;
      case 4:   //  5 connected
        connected_status1(line_num,k-4);    //  check connections for previous blunami
        connected_status1(line_num,k-3);    //  check connections for previous blunami
        connected_status1(line_num,k-2);    //  check connections for previous blunami
        connected_status1(line_num,k-1);    //  check connections for previous blunami     
        connected_status1(line_num,k);    //  check connections for current blunami      
        break;
      case 5:   //  6 connected
        connected_status1(line_num,k-5);    //  check connections for previous blunami
        connected_status1(line_num,k-4);    //  check connections for previous blunami
        connected_status1(line_num,k-3);    //  check connections for previous blunami
        connected_status1(line_num,k-2);    //  check connections for previous blunami
        connected_status1(line_num,k-1);    //  check connections for previous blunami     
        connected_status1(line_num,k);    //  check connections for current blunami      
        break;            
    }  // bottom switch

return;
}

//---------------------------- bottom connection_status_all----------------------------------------------
//--------------------------------------- PRINT_LINE FUNCTION ----------------------------------
void print_line(){
// this prints a line in the sketch
Serial.println("----------------------------------------------------------------------------------------");

return;  
}
//--------------------------------- BOTTOM  PRINT_LINE FUNCTION -------------------------
//--------------------------------- KEEP_CONNECTED -----------------------------------------------
void keep_connected(int num_blunamis_connect){
// this function keeps the blunamis connected and does not PRINT RESULTS
//  10/27/24 not sure this will be of any value but blunami seems to drop connection when inactive
//  update the "int blunami_connected" array 
//  we want to recalculate the actual number of connected blunamis that are used as a test
//  in the IF CONNECTED AND WHILE CONNECTED
  
//  this checks the connect and connected status
//  this updates the "int blunami_connected" array

  actual_blunami_connected = 0;   //  set this to 0 and then recalculate
  for (int b=0; b<= num_blunamis_connect-1; b++){
    //  if not connected, reconnect
    if(!Blucab_Blunami[b].connected()){
        Blucab_Blunami[b].connect();
        blunami_connected[b]=1;
    }
    else{
        blunami_connected[b]=0;
        actual_blunami_connected = actual_blunami_connected + 1;   // this updates the actual connected
    } 

  } //  bottom for 

}   //  keep_connected

//-------------------------------------- BOTTOM KEEP CONNECTED -------------------------------
//---------------------------------- LEAD UNIT FUNCTION CHANGE -----------------------
void lead_unit_function_change()
{
//  this function shifts the sound functions from the lead unit to the trailing unit
//  when there is a direction change

//  depending upon the loco direction, the functions will only be activated in the lead unit. the trailing
//  units will be silent
//  in forward the lead unit index is 0 and 
//  in reverse the trailing unit becomes the lead unit and the index is "actual_blunami_connected-1"

//  assign the index depending upon the loco direction
  if(loco_direction=='1'){         //  forward
      loco_index = 0;
      previous_loco_index = actual_blunami_connected-1;}   
  else{                           //  reverse
      loco_index = actual_blunami_connected-1;
      previous_loco_index = 0;}   

//  1/23/25 THIS NEEDS TO BE MODIFIED FOR LOCO # <= 127

//  for LOCO NUMBER > 127
//-------------------------------- extended function group 1 -----------------------------
//  1/22/25 we only want the functions to operate in the lead unit. when we change directions we have to first
//  turn off all functions in the lead unit (which will become the trailing unit) and then
//  turn on the the functions in the trailing unit which becomes the new lead unit.

//  turn off all function groups for the lead loco that becomes the trailing unit

//  1/24/25 we are assuming that the Group 1 functions are all we need to worry about.
//  for group 2A we will turn off all functions except for perhaps  F8 mute.
//  if F8 (mute) is active it might make sense to keep this on. If not it will be turned off and
//  then back on.  Does this matter?


    function_group_1_e[2] = loco_left_hex_pair;            // function group 1
    function_group_1_e[3] = loco_right_hex_pair;
    function_group_2a_e[2] = loco_left_hex_pair;           // function group 2a
    function_group_2a_e[3] = loco_right_hex_pair;
    function_group_2b_e[2] = loco_left_hex_pair;           // function group 2b 
    function_group_2b_e[3] = loco_right_hex_pair;

// ----------------- turn off function Group 1 previous function commands --------------------
//  check which function Group 1 functions are online then update command hex.
      if (f0=="0" && f3 == "0"){function_group_1_e[4] = 0x80;}    //  function group 1 all off
      if (f0=="1" && f3 == "0"){function_group_1_e[4] = 0x90;}     //  function group 1 all off (except light)
      if (f0=="0" && f3 == "1"){function_group_1_e[4] = 0x84;} //  function group 1 all off (except horn)
      if (f0=="1" && f3 == "1"){function_group_1_e[4] = 0x94;} //  function group 1 all off (except light and horn)
                             
      for(int ff=1;ff<=3;ff++){   //    send the command 3 times. no reason for using variable "ff"             
          BlunamiCommand[previous_loco_index].writeValue(function_group_1_e,5);   //  send command to each loco
      }

//------------------------------ top turning trailing unit(s) lights off -----------------
//  this is being done so only the lead unit front light is illuminated 
//  in the forward direction, we are turning off functions of the trailing units
//  from index = 1 to actual_blunami_connected-1
  
      trailing_unit_functions_2();    //  function group 1 turn off trailing unit headlight
//------------------------------- bottom turning trailing unit(s) functions off -----------------     

// ----------------- turn off function Group 1A previous functon commands --------------------
//  1/24/25 check if F8 (mute) is on and keep it on
      if(f8=="1"){function_group_2a_e[4] = 0xb8;}   //  all off except mute
        else{function_group_2a_e[4] = 0xb0;}    //  function group 2a all off

      for(int ff=1;ff<=3;ff++){   //    send the command 3 times. no reason for using variable "ff"  
          BlunamiCommand[previous_loco_index].writeValue(function_group_2a_e,5);   //  send command to blunami
      }
// ----------------- turn off function Group 2B previous function commands --------------------      
      function_group_2b_e[4] = 0xa0;    //  function group 2b all off
      for(int ff=1;ff<=3;ff++){   //    send the command 3 times. no reason for using variable "ff"  
          BlunamiCommand[previous_loco_index].writeValue(function_group_2b_e,5);   //  send command to blunami
      }
//-------------------------bottom turn off previous function commands--------------------------

// once the direction has changed resend the current function commands to the new lead unit
//    function group 1
//---------------------------------top send current commands --------------------------  
      function_group_1_e[4] = fg1_current_command;
      for(int ff=1;ff<=3;ff++){   //    send the command 3 times. no reason for using variable "ff"
            BlunamiCommand[loco_index].writeValue(function_group_1_e,5);   //  send command to each loco
      }   //  bottom for send command 3 times

//      1/26/25 THIS DOES NOT WORK
//  un mute once command has been sent
  //  if (f3=="1"){
       function_group_2a_e[4] = 0xb0;    //  function group 2b - unmute hex
       BlunamiCommand[loco_index].writeValue(function_group_2a_e,5);   //  send command to mute  
  //  }

//    function group 2a
      function_group_2a_e[4] = fg2a_current_command;
      for(int ff=1;ff<=3;ff++){   //    send the command 3 times. no reason for using variable "ff"
          BlunamiCommand[loco_index].writeValue(function_group_2a_e,5);   //  send command to blunami
      }

//    function group 2b
      function_group_2b_e[4] = fg2b_current_command;
      for(int ff=1;ff<=3;ff++){   //    send the command 3 times. no reason for using variable "ff"
          BlunamiCommand[loco_index].writeValue(function_group_2b_e,5);   //  send command to blunami
      }
//------------------------------ bottom sending current commands ---------------------------

//------------------------------ return ---------------------------------------
  return;
}
//----------------------------------- BOTTOM LEAD UNIT FUNCTION CHANGE -----------------------

//------------------------------ top TRAILING UNIT FUNCTIONS ---(NOT USED UPDATED VERSION)-------------
void trailing_unit_functions(){
//  this function turns off the group 1 functions in the trailing units in a
//  multiple unit consist. it does not mute the sounds.
//  the affect is that it turns out the headlight and taillight depending upon the direction


//  this is being done to have the front light illuminating on the lead unit only 
//  in the forward direction, we are turning off functions from index = 1 to actual_blunami_connected-1

//  we need to check the status of f3 because we don't want to change it state
      if (f3=="1"){function_group_1_e[4] = 0x84;} //  group 1 all of except short horn
      else {function_group_1_e[4] = 0x80;}   //  groupl 1 all off

      if(loco_direction=='1'){         //  forward
        for(int nn = 1;nn<=actual_blunami_connected-1;nn++){    //  no reason for choosing nn
            BlunamiCommand[nn].writeValue(function_group_1_e,5);   //  send command to each loco
        }
      }  
      else{                           //  reverse
        for(int pp = actual_blunami_connected-2;pp>=0;pp--){    //  no reason for choosing pp
           BlunamiCommand[pp].writeValue(function_group_1_e,5);   //  send command to each loco
        }   
      }
//------------------------------------------------------------------------
  return;
}

//----------------------------------- BOTTOM TRAILING UNIT FUNCTIONS ---------------------------------
void trailing_unit_functions_2(){

//  this function turns off the group 1 functions and lights in the trailing units in a
//  multiple unit consist. it does not mute the sounds.
//  the affect is that it turns out the headlight and taillight depending upon the direction

  //  we need to check the status of f3 because we don't want to change it state
  if (f3=="1"){function_group_1_e[4] = 0x84;} //  group 1 all of except short horn
      else {function_group_1_e[4] = 0x80;}   //  groupl 1 all off

  if(loco_direction=='1'){     //  forward

    switch (actual_blunami_connected)						
	    {					
	    case 2:		// 2 connected			
		    BlunamiCommand[1].writeValue(function_group_1_e,5);				
		      break;		
	    case 3:		// 3 connected			
		    BlunamiCommand[1].writeValue(function_group_1_e,5);				
		    BlunamiCommand[2].writeValue(function_group_1_e,5);				
		      break;				
	    case 4:		// 4 connected
		    BlunamiCommand[1].writeValue(function_group_1_e,5);	
		    BlunamiCommand[2].writeValue(function_group_1_e,5);	
		    BlunamiCommand[3].writeValue(function_group_1_e,5);	
		      break;	
	    case 5:		// 5 connected
		    BlunamiCommand[1].writeValue(function_group_1_e,5);	
		    BlunamiCommand[2].writeValue(function_group_1_e,5);	
		    BlunamiCommand[3].writeValue(function_group_1_e,5);	
		    BlunamiCommand[4].writeValue(function_group_1_e,5);
		      break;	        	
	    case 6:		// 6 connected
		    BlunamiCommand[1].writeValue(function_group_1_e,5);	
		    BlunamiCommand[2].writeValue(function_group_1_e,5);	
		    BlunamiCommand[3].writeValue(function_group_1_e,5);	
		    BlunamiCommand[4].writeValue(function_group_1_e,5);				
		    BlunamiCommand[5].writeValue(function_group_1_e,5);				
		      break;				
	    }		//  bottom switch case			
  }   //  bottom IF forward

  else{   //  reverse

    switch (actual_blunami_connected)						
	    {					
	    case 2:			// 2 connected					
		    BlunamiCommand[0].writeValue(function_group_1_e,5);				
		    break;				
	    case 3:					
		    BlunamiCommand[1].writeValue(function_group_1_e,5);				
		    BlunamiCommand[0].writeValue(function_group_1_e,5);				
		      break;				
	    case 4:	
		    BlunamiCommand[2].writeValue(function_group_1_e,5);
		    BlunamiCommand[1].writeValue(function_group_1_e,5);
		    BlunamiCommand[0].writeValue(function_group_1_e,5);
		      break;
	    case 5:	
		    BlunamiCommand[3].writeValue(function_group_1_e,5);
		    BlunamiCommand[2].writeValue(function_group_1_e,5);
		    BlunamiCommand[1].writeValue(function_group_1_e,5);
		    BlunamiCommand[0].writeValue(function_group_1_e,5);
		      break;
	    case 6:	
		    BlunamiCommand[4].writeValue(function_group_1_e,5);
		    BlunamiCommand[3].writeValue(function_group_1_e,5);
		    BlunamiCommand[2].writeValue(function_group_1_e,5);
		    BlunamiCommand[1].writeValue(function_group_1_e,5);
		    BlunamiCommand[0].writeValue(function_group_1_e,5);
		      break;
      }   //  bottom switch case
  }   //  bottom else  reverse

  return;
}
//--------------------------- BOTTOM TRAILING UNIT FUNCTIONS 2--------------------------------- 

//---------------------------------- LCD SCANNING -------------------------------------------
void lcd_loco_scanning(){
  // this function displays that the scans are beginning

 //  begin displaying setup steps on LCD
      u8g2.clearBuffer(); //  must clear 1st 
      u8g2.setFont(u8g2_font_micro_tr);
  //               (col,row)
      u8g2.setCursor(15,6);
      u8g2.print("Scanning for Locomotives"); 
      u8g2.sendBuffer();    //  this sends the commands to the LCD
      startMillis = millis(); while (millis() <= (startMillis + 1000)) {} //  delay 1 sec 

  return;
}
//---------------------------------- BOTTOM LCD SCANNING ----------------------------------------------------
//------------------------------------------LCD LOCO LIST------------------------------------------
void lcd_loco_list(){
  // this function lists the available locos on the lcd
  //  INCREASE ROW BY 6 FOR NEXT ROW
  //  begin displaying setup steps on LCD
    int tab1 = 5;   //  left list
    int tab2 = 20;  //  left list
    int tab3 = 70;  // right list
    int tab4 = 85;  // right list

    u8g2.clearBuffer(); //  must clear 1st 
    u8g2.setFont(u8g2_font_micro_tr);
  //             (col,row)
    u8g2.setCursor(tab1,6);
    u8g2.print("Available"); 
    u8g2.sendBuffer();    //  this sends the commands to the LCD
//  display "selected" column
  //             (col,row)
    u8g2.setCursor(tab3,6);
    u8g2.print("Selected"); 
    u8g2.sendBuffer();    //  this sends the commands to the LCD

// display available header 
  //  u8g2.clearBuffer(); //  must clear 1st 
    u8g2.setFont(u8g2_font_micro_tr);
  //             (col,row)
    u8g2.setCursor(tab1,12);
    u8g2.print("No.");
    u8g2.setCursor(tab2,12);
    u8g2.print("Name"); 
    u8g2.sendBuffer();    //  this sends the commands to the LCD

// display selected header 
  //  u8g2.clearBuffer(); //  must clear 1st 
    u8g2.setFont(u8g2_font_micro_tr);
  //             (col,row)
    u8g2.setCursor(tab3,12);
    u8g2.print("No.");
  //             (col,row)
    u8g2.setCursor(tab4,12);
    u8g2.print("Name");   
    u8g2.sendBuffer();    //  this sends the commands to the LCD

//  use case branch to display the available loco names
  switch (num_blunamis) {   //  number of available blunamis
  case 1:   //  1 available
    // statements
//    u8g2.clearBuffer(); //  must clear 1st 
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,18);
    u8g2.print("1");    //  display list number
    u8g2.setCursor(tab2,18);
    u8g2.print(Blunamis[0].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD
    break;    //  bottom case 1

  case 2:   //  2 available
    //  display 1st loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,18);
    u8g2.print("1");    //  display list number
    u8g2.setCursor(tab2,18);
    u8g2.print(Blunamis[0].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD
//  display 2nd loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,24);
    u8g2.print("2");    //  display list number
    u8g2.setCursor(tab2,24);
    u8g2.print(Blunamis[1].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD
    break;    //  bottom case 2
        
  case 3:   //  3 available
    //  display 1st loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,18);
    u8g2.print("1");    //  display list number
    u8g2.setCursor(tab2,18);
    u8g2.print(Blunamis[0].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

//  display 2nd loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,24);
    u8g2.print("2");    //  display list number
    u8g2.setCursor(tab2,24);
    u8g2.print(Blunamis[1].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

    //  display 3rd loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,30);
    u8g2.print("3");    //  display list number
    u8g2.setCursor(tab2,30);
    u8g2.print(Blunamis[2].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD
    break;    //  bottom case 3

  case 4:   //  4 locos available
    //  display 1st loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,18);
    u8g2.print("1");    //  display list number
    u8g2.setCursor(tab2,18);
    u8g2.print(Blunamis[0].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

//  display 2nd loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,24);
    u8g2.print("2");    //  display list number
    u8g2.setCursor(tab2,24);
    u8g2.print(Blunamis[1].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

    //  display 3rd loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,30);
    u8g2.print("3");    //  display list number
    u8g2.setCursor(tab2,30);
    u8g2.print(Blunamis[2].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

    //  display 4th loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,36);
    u8g2.print("4");    //  display list number
    u8g2.setCursor(tab2,36);
    u8g2.print(Blunamis[3].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD
    break;    //  bottom case 4

  case 5:   //  5 locos available
    //  display 1st loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,18);
    u8g2.print("1");    //  display list number
    u8g2.setCursor(tab2,18);
    u8g2.print(Blunamis[0].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

//  display 2nd loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,24);
    u8g2.print("2");    //  display list number
    u8g2.setCursor(tab2,24);
    u8g2.print(Blunamis[1].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

    //  display 3rd loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,30);
    u8g2.print("3");    //  display list number
    u8g2.setCursor(tab2,30);
    u8g2.print(Blunamis[2].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

    //  display 4th loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,36);
    u8g2.print("4");    //  display list number
    u8g2.setCursor(tab2,36);
    u8g2.print(Blunamis[3].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

    //  display 5th loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,42);
    u8g2.print("5");    //  display list number
    u8g2.setCursor(tab2,42);
    u8g2.print(Blunamis[4].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD
    break;    //  bottom case 5

  case 6:   //  6 locos available
    //  display 1st loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,18);
    u8g2.print("1");    //  display list number
    u8g2.setCursor(tab2,18);
    u8g2.print(Blunamis[0].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

//  display 2nd loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,24);
    u8g2.print("2");    //  display list number
    u8g2.setCursor(tab2,24);
    u8g2.print(Blunamis[1].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

    //  display 3rd loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,30);
    u8g2.print("3");    //  display list number
    u8g2.setCursor(tab2,30);
    u8g2.print(Blunamis[2].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

    //  display 4th loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,36);
    u8g2.print("4");    //  display list number
    u8g2.setCursor(tab2,36);
    u8g2.print(Blunamis[3].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

    //  display 5th loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,42);
    u8g2.print("5");    //  display list number
    u8g2.setCursor(tab2,42);
    u8g2.print(Blunamis[4].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

    //  display 6th loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,48);
    u8g2.print("6");    //  display list number
    u8g2.setCursor(tab2,48);
    u8g2.print(Blunamis[5].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD
    break;   //  bottom case 6

}  // bottom of switch
  
  return;
}
//----------------------------------------BOTTOM LOCO LIST ----------------------------------------
//----------------------------------LCD RESCAN ------------------------------------------
void lcd_rescan_loco(){
//  this function clears the screen and then displays "rescanning" on LCD
    u8g2.clearBuffer(); //  must clear 1st 
    u8g2.setFont(u8g2_font_micro_tr);
  //             (col,row)
    u8g2.setCursor(15,6);
    u8g2.print("Rescanning for Locomotives"); 
    u8g2.sendBuffer();    //  this sends the commands to the LCD

  return;
}

//-------------------------------BOTTOM LCD RESCAN ---------------------------------------

//-------------------------------------LCD LOCO SELECTED ------------------------------------------
void lcd_loco_selected(int keynumber_pressed,int keypress_order){
// this function lists the selected locos on the lcd
// the header title is displayed in the lcd available loco function
// the variables passed are key_id which is the numbered key pressed (1-9) and
// num_keypress which is the order the number keys were pressed. So 1 is the first time a key was pressed
// and 2 is the second time a key was pressed. This is used to determine the LCD row to list the data.
// keynumber_pressed is key_id
// keypress_order is num_keypress
// the row number is increased by 6 to display the next line so we will calculate what the row line is
// based upon what is passed.
int start_row = 18;   //  as the name implies. actual row is 6 less but this makes the cals work!!
int right_col = 70;   //  location of right table
int next_row = 6;     //  this is the row spacing. think of carriage return
int lcd_row = ((keypress_order-1)*next_row) + start_row;

//                       (col,row)
    u8g2.setCursor(right_col,lcd_row);
    u8g2.print(keypress_order);    //  display list number
    u8g2.setCursor(right_col+10,lcd_row);
    u8g2.print(Blunamis[keynumber_pressed-1].Blunami_name);   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

  return;
}

//---------------------------------------BOTTOM LCD LOCO SELECTED-------------------------------
//------------------------------- HEADER FOR LCD LOCO CONNECTED -------------------------------
void lcd_header(){
// this will be displayed before we display the loco connected
  int tab1 = 5;   //  left list
  int tab2 = 20;  //  left list
  int tab3 = 70;  // right list
  int tab4 = 85;  // right list
  int start_row = 6;
  int next_row = 6;     //  this is the row spacing. think of carriage return

  u8g2.clearBuffer(); //  must clear 1st 
  u8g2.setFont(u8g2_font_micro_tr);
  //             (col,row)    
  u8g2.setCursor(tab1,6);
  u8g2.print("Connecting to Selected locos");

  //             (col,row)
  u8g2.setCursor(tab1,12);
  u8g2.print("No.");
  u8g2.setCursor(tab2,12);
  u8g2.print("Name"); 
  u8g2.setCursor(tab3,12);
  u8g2.print("Status"); 
  u8g2.sendBuffer();    //  this sends the commands to the LCD  

  return;
}

//-------------------------------- BOTTOM HEADER FOR LCD LOCO CONNECTED -------------------------------

//-------------------------------------LCD LOCO CONNECTED ------------------------------------
void lcd_loco_connected(int lcd_sorted){
//  this functions lists the LOCOS on the LCD that will be connected 
//  we will pass the sorted index number from the SELECT function
  int tab1 = 5;   //  left list
  int tab2 = 20;  //  left list
  int tab3 = 70;  // right list
  int tab4 = 85;  // right list
  int start_row = 18;
  int next_row = 6;     //  this is the row spacing. think of carriage return
  int lcd_row = ((lcd_sorted)*next_row) + start_row;

//                (col,row)
  u8g2.setCursor(tab1,lcd_row);
  u8g2.print(Blunamis_sorted[lcd_sorted].Blunami_index2+1);    //  display list number
  u8g2.setCursor(tab2,lcd_row);
  u8g2.print(Blunamis_sorted[lcd_sorted].Blunami_name2);   //  display name
  u8g2.sendBuffer();    //  this sends the commands to the LCD

  return;
}

//----------------------------------- BOTTOM LCD LOCO CONNECTED ------------------------------------
//---------------------------------------LCD FOOTER ------------------------------------------
void lcd_footer(){
//  this function displays the LCD footer which gives instructions regarding connecting and rescanning
//  it is displayed after the available locos are displayed
// Serial.println("\t\tSelect Locomotive(s) - Press Number, then");
//    Serial.println("\t\t\t\t\       Press [F] to connect, [R] to rescan");
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(2,57);
    u8g2.print("Select Locomotives in order");    //  display list number
    u8g2.setCursor(2,63);
    u8g2.print("[R] to rescan, [0] to confirm");   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

  return;
}

//---------------------------------------BOTTOM LCD FOOTER--------------------------------------
//--------------------------------------LCD CONNECT FOOTER-----------------------------------------
void lcd_connect_footer(){
// after the locos are selected for connection this footer is displayed to
//  remind the user that the "F" key has to be pressed
  //            (col,row)
    u8g2.setCursor(5,63);
    u8g2.print("Press [F] to connect");   //  display name
    u8g2.sendBuffer();    //  this sends the commands to the LCD

return;
}
//----------------------------------BOTTOM LCD CONNECT FOOTER-----------------------------------------
//--------------------------------------TOP LCD LOCO DISPLAY --------------------------------------
void lcd_loco_summary_page(){
//  this function will display the connected loco status, voltage and whatever I can fit on the screen
    int tab1 = 1;   //  left list
    int tab2 = 16;  //  left list
    int tab3 = 73;  // right list
    int tab4 = tab3 + 27;  // right list
    int first_row = 6;
    int start_row = 12;
    int next_row = 6;     //  this is the row spacing. think of carriage return
    int local_index;      //  as the name implies
    int lcd_row = (local_index*next_row) + start_row; //  uses loco index to calculate row

    u8g2.clearBuffer(); //  must clear screen 1st 
    u8g2.setFont(u8g2_font_micro_tr);
    //             (col,row)    
    u8g2.setCursor(25,first_row);
    u8g2.print("Connected locomotives");
    //  display titles
    //             (col,row)
    u8g2.setCursor(tab1,start_row);
    u8g2.print("No.");
    u8g2.setCursor(tab2,start_row);
    u8g2.print("Name"); 
    u8g2.setCursor(tab3,start_row);
    u8g2.print("loco #");
    u8g2.setCursor(tab4,start_row);
    u8g2.print("Voltage");
 //   u8g2.setCursor(tab4,12);
 //   u8g2.print("Status"); 
    u8g2.sendBuffer();    //  sends commands to the LCD  

  //start_rail_volts();
  //  use case branch to display the connected locos 
  switch (num_blunamis_connect) {   //  number of connected blunamis
  case 1:   //  1 connect
    start_rail_volts();
  //  startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command         
    decoder_voltage[0] = rail_voltage(0);      
    // statements
//    u8g2.clearBuffer(); //  must clear 1st 
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,18);
    u8g2.print("1");    //  display list number
    u8g2.setCursor(tab2,18);
    u8g2.print(Blunamis[0].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,18);
    u8g2.print(loco_number[0]);
    u8g2.setCursor(tab4+2,18);
    u8g2.print(decoder_voltage[0]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD
    break;    //  bottom case 1

  case 2:   //  2 connected    
    //  display 1st loco
    start_rail_volts();
    startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command    
    decoder_voltage[0] = rail_voltage(0); 
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,18);
    u8g2.print("1");    //  display list number
    u8g2.setCursor(tab2,18);
    u8g2.print(Blunamis[0].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,18);
    u8g2.print(loco_number[0]);
    u8g2.setCursor(tab4+2,18);
    u8g2.print(decoder_voltage[0]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD

//  display 2nd loco
    start_rail_volts();
    startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command
    decoder_voltage[1] = rail_voltage(1); 
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,24);
    u8g2.print("2");    //  display list number
    u8g2.setCursor(tab2,24);
    u8g2.print(Blunamis[1].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,24);
    u8g2.print(loco_number[1]);
    u8g2.setCursor(tab4+2,24);
    u8g2.print(decoder_voltage[1]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD    
    break;    //  bottom case 2
        
  case 3:   //  3 connected
    //  display 1st loco
    start_rail_volts();
    startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command    
    decoder_voltage[0] = rail_voltage(0); 
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,18);
    u8g2.print("1");    //  display list number
    u8g2.setCursor(tab2,18);
    u8g2.print(Blunamis[0].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,18);
    u8g2.print(loco_number[0]);
    u8g2.setCursor(tab4+2,18);
    u8g2.print(decoder_voltage[0]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD

//  display 2nd loco
    start_rail_volts();
    startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command
    decoder_voltage[1] = rail_voltage(1); 
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,24);
    u8g2.print("2");    //  display list number
    u8g2.setCursor(tab2,24);
    u8g2.print(Blunamis[1].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,24);
    u8g2.print(loco_number[1]);
    u8g2.setCursor(tab4+2,24);
    u8g2.print(decoder_voltage[1]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD    

    //  display 3rd loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,30);
    u8g2.print("3");    //  display list number
    u8g2.setCursor(tab2,30);
    u8g2.print(Blunamis[2].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,30);
    u8g2.print(loco_number[2]);
    u8g2.setCursor(tab4+2,30);
    u8g2.print(decoder_voltage[2]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD   
    break;    //  bottom case 3

  case 4:   //  4 locos available
    //  display 1st loco
    start_rail_volts();
    startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command    
    decoder_voltage[0] = rail_voltage(0); 
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,18);
    u8g2.print("1");    //  display list number
    u8g2.setCursor(tab2,18);
    u8g2.print(Blunamis[0].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,18);
    u8g2.print(loco_number[0]);
    u8g2.setCursor(tab4+2,18);
    u8g2.print(decoder_voltage[0]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD

//  display 2nd loco
    start_rail_volts();
    startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command
    decoder_voltage[1] = rail_voltage(1); 
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,24);
    u8g2.print("2");    //  display list number
    u8g2.setCursor(tab2,24);
    u8g2.print(Blunamis[1].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,24);
    u8g2.print(loco_number[1]);
    u8g2.setCursor(tab4+2,24);
    u8g2.print(decoder_voltage[1]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD    

    //  display 3rd loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,30);
    u8g2.print("3");    //  display list number
    u8g2.setCursor(tab2,30);
    u8g2.print(Blunamis[2].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,30);
    u8g2.print(loco_number[2]);
    u8g2.setCursor(tab4+2,30);
    u8g2.print(decoder_voltage[2]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD   

    //  display 4th loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,36);
    u8g2.print("4");    //  display list number
    u8g2.setCursor(tab2,36);
    u8g2.print(Blunamis[3].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,36);
    u8g2.print(loco_number[3]);
    u8g2.setCursor(tab4+2,36);
    u8g2.print(decoder_voltage[3]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD       
    break;    //  bottom case 4

  case 5:   //  5 locos available
    //  display 1st loco
    start_rail_volts();
    startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command    
    decoder_voltage[0] = rail_voltage(0); 
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,18);
    u8g2.print("1");    //  display list number
    u8g2.setCursor(tab2,18);
    u8g2.print(Blunamis[0].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,18);
    u8g2.print(loco_number[0]);
    u8g2.setCursor(tab4+2,18);
    u8g2.print(decoder_voltage[0]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD

//  display 2nd loco
    start_rail_volts();
    startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command
    decoder_voltage[1] = rail_voltage(1); 
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,24);
    u8g2.print("2");    //  display list number
    u8g2.setCursor(tab2,24);
    u8g2.print(Blunamis[1].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,24);
    u8g2.print(loco_number[1]);
    u8g2.setCursor(tab4+2,24);
    u8g2.print(decoder_voltage[1]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD    

    //  display 3rd loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,30);
    u8g2.print("3");    //  display list number
    u8g2.setCursor(tab2,30);
    u8g2.print(Blunamis[2].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,30);
    u8g2.print(loco_number[2]);
    u8g2.setCursor(tab4+2,30);
    u8g2.print(decoder_voltage[2]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD   

    //  display 4th loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,36);
    u8g2.print("4");    //  display list number
    u8g2.setCursor(tab2,36);
    u8g2.print(Blunamis[3].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,36);
    u8g2.print(loco_number[3]);
    u8g2.setCursor(tab4+2,36);
    u8g2.print(decoder_voltage[3]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD     

    //  display 5th loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,42);
    u8g2.print("5");    //  display list number
    u8g2.setCursor(tab2,42);
    u8g2.print(Blunamis[4].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,42);
    u8g2.print(loco_number[4]);
    u8g2.setCursor(tab4+2,42);
    u8g2.print(decoder_voltage[4]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD     
    break;    //  bottom case 5

  case 6:   //  6 locos available
    //  display 1st loco
    start_rail_volts();
    startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command    
    decoder_voltage[0] = rail_voltage(0); 
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,18);
    u8g2.print("1");    //  display list number
    u8g2.setCursor(tab2,18);
    u8g2.print(Blunamis[0].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,18);
    u8g2.print(loco_number[0]);
    u8g2.setCursor(tab4+2,18);
    u8g2.print(decoder_voltage[0]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD

//  display 2nd loco
    start_rail_volts();
    startMillis = millis(); while (millis() <= (startMillis + setup_delay)) {} //  delay command
    decoder_voltage[1] = rail_voltage(1); 
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,24);
    u8g2.print("2");    //  display list number
    u8g2.setCursor(tab2,24);
    u8g2.print(Blunamis[1].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,24);
    u8g2.print(loco_number[1]);
    u8g2.setCursor(tab4+2,24);
    u8g2.print(decoder_voltage[1]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD    

    //  display 3rd loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,30);
    u8g2.print("3");    //  display list number
    u8g2.setCursor(tab2,30);
    u8g2.print(Blunamis[2].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,30);
    u8g2.print(loco_number[2]);
    u8g2.setCursor(tab4+2,30);
    u8g2.print(decoder_voltage[2]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD   

    //  display 4th loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,36);
    u8g2.print("4");    //  display list number
    u8g2.setCursor(tab2,36);
    u8g2.print(Blunamis[3].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,36);
    u8g2.print(loco_number[3]);
    u8g2.setCursor(tab4+2,36);
    u8g2.print(decoder_voltage[3]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD     

    //  display 5th loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,42);
    u8g2.print("5");    //  display list number
    u8g2.setCursor(tab2,42);
    u8g2.print(Blunamis[4].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,42);
    u8g2.print(loco_number[4]);
    u8g2.setCursor(tab4+2,42);
    u8g2.print(decoder_voltage[4]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD    

    //  display 6th loco
    u8g2.setFont(u8g2_font_micro_tr);
  //            (col,row)
    u8g2.setCursor(tab1,48);
    u8g2.print("6");    //  display list number
    u8g2.setCursor(tab2,48);
    u8g2.print(Blunamis[5].Blunami_name);   //  display name
    u8g2.setCursor(tab3+2,48);
    u8g2.print(loco_number[5]);
    u8g2.setCursor(tab4+2,48);
    u8g2.print(decoder_voltage[5]);         //  display voltage
    u8g2.sendBuffer();    //  this sends the commands to the LCD    
    break;   //  bottom case 6

}  // bottom of switch

  return;
}

//----------------------------------- BOTTOM LCD LOCO DISPLAY --------------------------------------

//  ----------------------------TOP LCD CONNECTING---------------------------------- 
void lcd_connecting() { 
  // this function displays "connecting" on the LCD
  //  it displays consecutive periods to simulate the process timing
    int start_row = 18;
    int next_row = 6;     //  this is the row spacing. think of carriage return
    int lcd_row = (k*next_row) + start_row; //  uses loco index to calculate row
    int tab1 = 5;   //  left list
    int tab2 = 20;  //  left list
    int tab3 = 70;  // right list
    int tab4 = 85;  // right list
    int tab5 = tab4 + 25;

    u8g2.setFont(u8g2_font_micro_tr);
//  clear bottom row
      //         (col,row)
    u8g2.setCursor(5,63);
    u8g2.setDrawColor(0); // set font color to background (blue)
    //       col  row  width height
    u8g2.drawBox(1,57,320,12);  //  draw box to clear text footer
//  update status
    u8g2.setDrawColor(1); // set font color to fore ground (white)
//  update right column
      //            (col,row)
    u8g2.setCursor(tab3,lcd_row);
    u8g2.print("Connecting");
//    u8g2.sendBuffer();    //  this sends the commands to the LCD 
//  print "periods" during connection
    u8g2.setCursor(tab5,lcd_row);
    u8g2.print(".");    
    u8g2.sendBuffer();    //  this sends the commands to the LCD 
    startMillis = millis(); while (millis() <= (startMillis + 600)) {} //  connection delay
    u8g2.setCursor(tab5+3,lcd_row);
    u8g2.print(".");    
    u8g2.sendBuffer();    //  this sends the commands to the LCD
    startMillis = millis(); while (millis() <= (startMillis + 600)) {} //  connection delay
    u8g2.setCursor(tab5+6,lcd_row);
    u8g2.print(".");    
    u8g2.sendBuffer();    //  this sends the commands to the LCD
    startMillis = millis(); while (millis() <= (startMillis + 600)) {} //  connection delay 
    u8g2.setCursor(tab5+9,lcd_row);
    u8g2.print(".");    
    u8g2.sendBuffer();    //  this sends the commands to the LCD 
    startMillis = millis(); while (millis() <= (startMillis + 600)) {} //  connection delay 
    u8g2.setCursor(tab5+12,lcd_row);
    u8g2.print(".");    
    u8g2.sendBuffer();    //  this sends the commands to the LCD 

    return;
}

//----------------------------- BOTTOM LCD CONNECTING --------------------------------

//------------------------------ TOP LCD CONNECTED MESSAGE-----------------------------------
void lcd_connected() {

  // this function confirms the blunami connection on the LCD 
    int start_row = 18;
    int next_row = 6;     //  this is the row spacing. think of carriage return
    int lcd_row = (k*next_row) + start_row; //  uses loco index to calculate row
    int tab3 = 70;  // right list
    u8g2.setFont(u8g2_font_micro_tr);
      //            (col,row)
    u8g2.setCursor(tab3,lcd_row);
    u8g2.setDrawColor(0); // set font color to background (blue)
//           (col,row,width,height)    
    u8g2.drawBox(tab3,lcd_row-6,55,lcd_row);    //  draw box to erase print
//    u8g2.print("Connecting"); //  display letters in background color
    u8g2.setCursor(tab3,lcd_row);
    u8g2.setDrawColor(1);   // set font color to foreground (white)  
    u8g2.print("Connected");
    u8g2.sendBuffer();    //  this sends the commands to the LCD

  return;
} 
//----------------------------- BOTTOM LCD CONNECTED MESSAGE --------------------------------
