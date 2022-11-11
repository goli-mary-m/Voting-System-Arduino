#include <LiquidCrystal.h>
#include <Keypad.h>
#include <Wire.h>
#include <SPI.h>

#define SS1_PIN 49
#define SS2_PIN 48
#define SS3_PIN 47
#define SS4_PIN 46

#define led_user4 8
#define led_user3 9
#define led_user2 10
#define led_user1 11

#define RS 7
#define E  6
#define D7 5
#define D6 4
#define D5 3
#define D4 2

#define ADDRESS 0b1010000
/*
 * Content of EEPROM
 * 
 * 0x00 (0)   -> mode
 * 
 * 0x0A (10)  -> token for user-1
 * 0x0B (11)  -> token for user-2
 * 0x0C (12)  -> token for user-3
 * 0x0D (13)  -> token for user-4
 * 
 * 0x32 (50)  -> vote for user-1
 * 0x4B (75)  -> vote for user-2
 * 0x64 (100) -> vote for user-3
 * 0x7D (125) -> vote for user-4
 * 
 */

LiquidCrystal lcd (RS, E, D4, D5, D6, D7);

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
    {'1','2','3'},
    {'4','5','6'},
    {'7','8','9'},
    {'*','0','#'}
};

byte rowPins[ROWS] = {25, 26, 27, 28}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {24, 23, 22}; //connect to the column pinouts of the keypad

Keypad keyPad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

char key;
int mode;
/*
 * mode = 1 -> no/yes                      -> input: 0 or 1
 * mode = 2 -> 0/1/2/3/4/5 star            -> input: 0 or 1 or 2 or 3 or 4 or 5
 * mode = 3 -> select one person           -> input: 0 or 1 or 2 or 3
 * mode = 4 -> select multiple people      -> input: 0 or 1 or 2 or 3 or more 
 */ 

int userToken[4];

int userVote_1[7];
int userVote_2[7];
int userVote_3[7];
int userVote_4[7];
/*
 * mode = 1 -> userVote[0]
 * mode = 2 -> userVote[1]
 * mode = 3 -> userVote[2]
 * mode = 4 -> userVote[3] - userVote[4] - userVote[5] - userVote[6]
 *             (person-0)    (person-1)    (person-2)    (person-3) 
 *             
 *             0 -> not selected -- 1 -> selected
 */

String receiveData_fromSlave1 = "";
String receiveData_fromSlave2 = "";
String receiveData_fromSlave3 = "";
String receiveData_fromSlave4 = "";

int restart_bit = 0;
int pause_bit = 0;

int cnt;

void setup(){

  cnt = 1;

  Wire.begin();
  Serial.begin(9600);  
  Serial.println("-> Master");
  Serial.println();
  
  mode = modeRead();
  restart_bit = 0;
  pause_bit = 0;
  
  lcd.begin(16, 4);
  lcd.clear();
  lcd.print("Mode: ");
  if(mode == 255){
    lcd.print("-");
  }else{
    lcd.print(mode);
  }

  pinMode(led_user1, OUTPUT);
  pinMode(led_user2, OUTPUT);
  pinMode(led_user3, OUTPUT);
  pinMode(led_user4, OUTPUT);
  
  digitalWrite(led_user1, LOW);
  digitalWrite(led_user2, LOW);
  digitalWrite(led_user3, LOW);
  digitalWrite(led_user4, LOW);

  userTokenRead();
  for(int i = 0; i < 4; i++){
    if(userToken[i] == 255){
      userToken[i] = 0;
    }
  }
  Serial.println("userToken");
  Serial.println(userToken[0]);
  Serial.println(userToken[1]);
  Serial.println(userToken[2]);
  Serial.println(userToken[3]);

  setUserLED();

  user1VoteRead();
  user2VoteRead();
  user3VoteRead();
  user4VoteRead();
  
  // set the slaveSelectPins as an output:
  pinMode(SS1_PIN, OUTPUT);
  pinMode(SS2_PIN, OUTPUT);
  pinMode(SS3_PIN, OUTPUT);
  pinMode(SS4_PIN, OUTPUT);
  digitalWrite(SS1_PIN, HIGH);
  digitalWrite(SS2_PIN, HIGH);
  digitalWrite(SS3_PIN, HIGH);
  digitalWrite(SS4_PIN, HIGH);

  // initialize SPI:
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV4);

}

void loop(){
  if(cnt == 1){
    userTokenRead();
    for(int i = 0; i < 4; i++){
      if(userToken[i] == 255){
        userToken[i] = 0;
      }
    }
    setUserLED();

    user1VoteRead();
    user2VoteRead();
    user3VoteRead();
    user4VoteRead();
    
    for(int i = 0; i < 7; i++){
      if(userVote_1[i] == 255){
        userVote_1[i] = 0;
      }
      if(userVote_2[i] == 255){
        userVote_2[i] = 0;
      }
      if(userVote_3[i] == 255){
        userVote_3[i] = 0;
      }
      if(userVote_4[i] == 255){
        userVote_4[i] = 0;
      }
    }
  
  
    Serial.println("userVote_1");
    for(int i = 0; i < 7; i++){
      Serial.print(userVote_1[i]);
    }
    Serial.println();
  
    Serial.println("userVote_2");
    for(int i = 0; i < 7; i++){
      Serial.print(userVote_2[i]);
    }
    Serial.println();

    Serial.println("userVote_3");
    for(int i = 0; i < 7; i++){
      Serial.print(userVote_3[i]);
    }
    Serial.println();
  
    Serial.println("userVote_4");
    for(int i = 0; i < 7; i++){
      Serial.print(userVote_4[i]);
    }
    Serial.println();   

    showResult();
  }
  cnt++;

  Serial.println("userToken");
  Serial.println(userToken[0]);
  Serial.println(userToken[1]);
  Serial.println(userToken[2]);
  Serial.println(userToken[3]);

  mode = modeRead();

  delay(50);
  
  key = keyPad.getKey();
  if(key){ 

    Serial.println();
    Serial.print("key:");
    Serial.print(key);
     
    if(key == '#'){
       lcd.setCursor(0, 3);
       lcd.print(key);
       char keyForPauseOrStop = keyPad.waitForKey();  
       lcd.print(keyForPauseOrStop);    
       if(keyForPauseOrStop == '1'){        //pause & resume
          pause_bit = 1;
       }else if(keyForPauseOrStop == '2'){  //stop
          endVotingAndRestart();
       }
       Serial.print("keyForPauseOrStop:");
       Serial.print(keyForPauseOrStop);
    }
          
    if(key == '0'){
      changeMode();
    } 
         
    if(key == '*'){
      showResult();
    }
    
  }

  lcd.setCursor(0, 3);
  lcd.print("                ");
  
  //setSendDataToSlaves();
  digitalWrite(SS1_PIN, LOW);
  digitalWrite(SS2_PIN, LOW);
  digitalWrite(SS3_PIN, LOW);
  digitalWrite(SS4_PIN, LOW);
  delay(10);

  char c_mode = mode + '0';
  SPI.transfer(c_mode);
  Serial.println(c_mode);
  char c_restart = restart_bit + '0';
  SPI.transfer(c_restart);
  Serial.println(c_restart);
  char c_pause = pause_bit + '0';
  SPI.transfer(c_pause);
  Serial.println(c_pause);

  digitalWrite(SS1_PIN, HIGH);
  digitalWrite(SS2_PIN, HIGH);
  digitalWrite(SS3_PIN, HIGH);
  digitalWrite(SS4_PIN, HIGH);
  delay(10);

  if(pause_bit == 1){
    pauseAndResume(); 
  }
  
  
  // Slave-1
  // take the SS1 pin low to select the chip:
  digitalWrite(SS1_PIN, LOW);
  delay(10);
  // send and reseive data via SPI -> slave-1 : 
  receiveFromSlave1(); 
  // take the SS1 pin high to de-select the chip:
  digitalWrite(SS1_PIN, HIGH);
  delay(10);
//  checkKey();

  // Slave-2
  // take the SS2 pin low to select the chip:
  digitalWrite(SS2_PIN, LOW);
  delay(10);
  // send and reseive data via SPI -> slave-2 :
  receiveFromSlave2(); 
  // take the SS2 pin high to de-select the chip:
  digitalWrite(SS2_PIN, HIGH);
  delay(10);
//  checkKey();

  // Slave-3
  // take the SS3 pin low to select the chip:
  digitalWrite(SS3_PIN, LOW);
  delay(10);
  // send and reseive data via SPI -> slave-3 :
  receiveFromSlave3(); 
  // take the SS3 pin high to de-select the chip:
  digitalWrite(SS3_PIN, HIGH);
  delay(10);
//  checkKey();

  // Slave-4
  // take the SS4 pin low to select the chip:
  digitalWrite(SS4_PIN, LOW);
  delay(10);
  // send and reseive data via SPI -> slave-4 :
  receiveFromSlave4(); 
  // take the SS4 pin high to de-select the chip:
  digitalWrite(SS4_PIN, HIGH);
  delay(10);
//  checkKey();
  

  if(restart_bit == 1){
    restart_bit = 0;
  }
}

//----------------------------------------------------------


// =========================================================

// Vote -> SPI
//----------------------------------------------------------

void receiveFromSlave1(){

  receiveData_fromSlave1 = ""; 
  char c1_s, c1_r;
  c1_s = ' ';
  for (int i = 0; i < 12; i++){
    c1_r = SPI.transfer(c1_s);
    receiveData_fromSlave1 += String(c1_r);  
  } 
  delay(100);

  // update vote and token -> user1
  userToken[0] = receiveData_fromSlave1.charAt(1) - '0';
  for(int i = 0; i < 7; i++){
    userVote_1[i] = receiveData_fromSlave1.charAt(i+2) - '0';
  }

  setUserLED();
  user1TokenWrite(userToken[0]);
  user1VoteWrite();
  
  Serial.print("Receive data from Slave-1: ");
  Serial.print(receiveData_fromSlave1);

  Serial.println();
  Serial.print("userToken[0]: ");
  Serial.print(userToken[0]);
  Serial.println();
  Serial.print("userVote_1: ");
  for(int i = 0; i < 7; i++){
    Serial.print(userVote_1[i]); 
  }

  Serial.println();
  Serial.println("----------------------------------------");
  Serial.println();
}


void receiveFromSlave2(){

  receiveData_fromSlave2 = ""; 
  char c2_s, c2_r;
  c2_s = ' ';
  for (int i = 0; i < 12; i++){
    c2_r = SPI.transfer(c2_s);
    receiveData_fromSlave2 += String(c2_r);  
  } 
  delay(10);

  // update vote and token -> user2
  userToken[1] = receiveData_fromSlave2.charAt(1) - '0';
  for(int i = 0; i < 7; i++){
    userVote_2[i] = receiveData_fromSlave2.charAt(i+2) - '0';
  }

  setUserLED();
  user2TokenWrite(userToken[1]);
  user2VoteWrite();
  
  Serial.print("Receive data from Slave-2: ");
  Serial.print(receiveData_fromSlave2);

  Serial.println();
  Serial.print("userToken[1]: ");
  Serial.print(userToken[1]);
  Serial.println();
  Serial.print("userVote_2: ");
  for(int i = 0; i < 7; i++){
    Serial.print(userVote_2[i]); 
  }

  Serial.println();
  Serial.println("----------------------------------------");
  Serial.println();
}


void receiveFromSlave3(){

  receiveData_fromSlave3 = ""; 
  char c3_s, c3_r;
  c3_s = ' ';
  for (int i = 0; i < 12; i++){
    c3_r = SPI.transfer(c3_s);
    receiveData_fromSlave3 += String(c3_r);  
  } 
  delay(10);

  // update vote and token -> user3
  userToken[2] = receiveData_fromSlave3.charAt(1) - '0';
  for(int i = 0; i < 7; i++){
    userVote_3[i] = receiveData_fromSlave3.charAt(i+2) - '0';
  }

  setUserLED();
  user3TokenWrite(userToken[2]);
  user3VoteWrite();
  
  Serial.print("Receive data from Slave-3: ");
  Serial.print(receiveData_fromSlave3);

  Serial.println();
  Serial.print("userToken[2]: ");
  Serial.print(userToken[2]);
  Serial.println();
  Serial.print("userVote_3: ");
  for(int i = 0; i < 7; i++){
    Serial.print(userVote_3[i]); 
  }

  Serial.println();
  Serial.println("----------------------------------------");
  Serial.println();
}


void receiveFromSlave4(){

  receiveData_fromSlave4 = ""; 
  char c4_s, c4_r;
  c4_s = ' ';
  for (int i = 0; i < 12; i++){
    c4_r = SPI.transfer(c4_s);
    receiveData_fromSlave4 += String(c4_r);  
  } 
  delay(10);

  // update vote and token -> user1
  userToken[3] = receiveData_fromSlave4.charAt(1) - '0';
  for(int i = 0; i < 7; i++){
    userVote_4[i] = receiveData_fromSlave4.charAt(i+2) - '0';
  }

  setUserLED();
  user4TokenWrite(userToken[3]);
  user4VoteWrite();
  
  Serial.print("Receive data from Slave-4: ");
  Serial.print(receiveData_fromSlave4);

  Serial.println();
  Serial.print("userToken[3]: ");
  Serial.print(userToken[3]);
  Serial.println();
  Serial.print("userVote_4: ");
  for(int i = 0; i < 7; i++){
    Serial.print(userVote_4[i]); 
  }

  Serial.println();
  Serial.println("----------------------------------------");
  Serial.println();
}


//----------------------------------------------------------


// =========================================================

// show result 
//----------------------------------------------------------

void showResult(){

  lcd.setCursor(0, 3);
  lcd.print("[Show-Results]");
  delay(200);

  printUserVotes();
    
  key = keyPad.waitForKey();
  if(key == '*'){
    lcd.clear();
    lcd.setCursor(0, 3);
    lcd.print("[End-Showing]");
  }else{
    while(key != '*'){
      key = keyPad.waitForKey();
    }
    lcd.clear();
    lcd.setCursor(0, 3);
    lcd.print("[End-Showing]");
  }
  delay(200);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mode: ");
  if(mode == 255){
    lcd.print("-");
  }else{
    lcd.print(mode);
  }

}

void printUserVotes(){
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("User-1: ");
  if(mode == 1){
    if(userToken[0] == 1){
      if(userVote_1[0] == 0){
        lcd.print("No");
      }else if(userVote_1[0] == 1){
        lcd.print("Yes");
      }   
    }else{
      lcd.print("-");
    }  
  }else if(mode == 2){
    if(userToken[0] == 1){
      int n_star = userVote_1[1];
      for(int i = 0; i < n_star; i++ ){
        lcd.print("*");
      } 
    }else{
      lcd.print("-");  
    }   
  }else if(mode == 3){
    if(userToken[0] == 1){
      lcd.print(String(userVote_1[2])); 
    }else{
      lcd.print("-");   
    }    
  }else if(mode == 4){
    if(userToken[0] == 1){
      int selected[4];
      int cnt_selected = 0;
      for(int i = 3; i < 7; i++){
        if(userVote_1[i] == 1){
          selected[cnt_selected] = i-3; 
          cnt_selected++; 
        }
      }
      for(int i = 0; i < cnt_selected; i++){
        lcd.print(String(selected[i]) + " ");
      } 
    }else{
      lcd.print("-");
    }
  }


  lcd.setCursor(0, 1);
  lcd.print("User-2: ");
  if(mode == 1){
    if(userToken[1] == 1){
      if(userVote_2[0] == 0){
        lcd.print("No");
      }else if(userVote_2[0] == 1){
        lcd.print("Yes");
      } 
    }else{
      lcd.print("-");   
    }
  }else if(mode == 2){
    if(userToken[1] == 1){
      int n_star = userVote_2[1];
      for(int i = 0; i < n_star; i++ ){
        lcd.print("*");
      }   
    }else{
      lcd.print("-");
    }
  }else if(mode == 3){
    if(userToken[1] == 1){
      lcd.print(String(userVote_2[2])); 
    }else{
      lcd.print("-");   
    }
  }else if(mode == 4){
    if(userToken[1] == 1){
      int selected[4];
      int cnt_selected = 0;
      for(int i = 3; i < 7; i++){
        if(userVote_2[i] == 1){
          selected[cnt_selected] = i-3; 
          cnt_selected++; 
        }
      }
      for(int i = 0; i < cnt_selected; i++){
        lcd.print(String(selected[i]) + " ");
      } 
    }else{
      lcd.print("-");
    }
  }


  lcd.setCursor(0, 2);
  lcd.print("User-3: ");
  if(mode == 1){
    if(userToken[2] == 1){
      if(userVote_3[0] == 0){
        lcd.print("No");
      }else if(userVote_3[0] == 1){
        lcd.print("Yes");
      } 
    }else{
      lcd.print("-");   
    }
  }else if(mode == 2){
    if(userToken[2] == 1){
      int n_star = userVote_3[1];
      for(int i = 0; i < n_star; i++ ){
        lcd.print("*");
      }  
    }else{
      lcd.print("-"); 
    }
  }else if(mode == 3){
    if(userToken[2] == 1){
      lcd.print(String(userVote_3[2]));   
    }else{
      lcd.print("-"); 
    }
  }else if(mode == 4){
    if(userToken[2] == 1){
      int selected[4];
      int cnt_selected = 0;
      for(int i = 3; i < 7; i++){
        if(userVote_3[i] == 1){
          selected[cnt_selected] = i-3; 
          cnt_selected++; 
        }
      }
      for(int i = 0; i < cnt_selected; i++){
        lcd.print(String(selected[i]) + " ");
      }
    }else{
      lcd.print("-"); 
    }
  }


  lcd.setCursor(0, 3);
  lcd.print("User-4: ");
  if(mode == 1){
    if(userToken[3] == 1){
      if(userVote_4[0] == 0){
        lcd.print("No");
      }else if(userVote_4[0] == 1){
        lcd.print("Yes");
      } 
    }else{
      lcd.print("-");    
    }
  }else if(mode == 2){
    if(userToken[3] == 1){
      int n_star = userVote_4
      [1];
      for(int i = 0; i < n_star; i++ ){
        lcd.print("*");
      } 
    }else{
      lcd.print("-");   
    }
  }else if(mode == 3){
    if(userToken[3] == 1){
      lcd.print(String(userVote_4[2])); 
    }else{
      lcd.print("-");    
    }
  }else if(mode == 4){
    if(userToken[3] == 1){
      int selected[4];
      int cnt_selected = 0;
      for(int i = 3; i < 7; i++){
        if(userVote_4[i] == 1){
          selected[cnt_selected] = i-3; 
          cnt_selected++; 
        }
      }
      for(int i = 0; i < cnt_selected; i++){
        lcd.print(String(selected[i]) + " ");
      } 
    }else{
      lcd.print("-"); 
    }
  }
}

//----------------------------------------------------------


// =========================================================

// Pause and Resume + Stop
//----------------------------------------------------------
void pauseAndResume(){

  lcd.setCursor(0, 3);
  lcd.print("[Pause]");
  
  key = keyPad.waitForKey();
  if(key == '#'){
    pause_bit = 0;
    lcd.setCursor(0, 3);
    lcd.print("[Resume]");
  }else{
    while(key != '#'){
      key = keyPad.waitForKey();
      if(key != '#'){
        lcd.setCursor(0, 3);
        lcd.print("[Pause]");
      }
    }
    pause_bit = 0;
    lcd.setCursor(0, 3);
    lcd.print("[Resume]");
  }
  delay(200);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mode: ");
  if(mode == 255){
    lcd.print("-");
  }else{
    lcd.print(mode);
  }
}

void endVotingAndRestart(){

  lcd.setCursor(0, 3);
  lcd.print("[End-Voting]");
  
  key = keyPad.waitForKey();
  if(key == '#'){
    lcd.setCursor(0, 3);
    lcd.print("[Restart-Voting]");
    restartVoting();
  }else{
    while(key != '#'){
      key = keyPad.waitForKey();
      if(key != '#'){
        lcd.setCursor(0, 3);
        lcd.print("[End-Voting]");
      }
    }
    lcd.setCursor(0, 3);
    lcd.print("Restart-Voting]");
    restartVoting();
  }
  delay(200);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mode: ");
  if(mode == 255){
    lcd.print("-");
  }else{
    lcd.print(mode);
  }
}

void restartVoting(){
  restart_bit = 1;
}

//----------------------------------------------------------


// =========================================================

// Mode -> I2C
//----------------------------------------------------------
void modeWrite(int data){
    Wire.beginTransmission(ADDRESS);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(data);
    Wire.endTransmission();
}

int modeRead(){
  Wire.beginTransmission(ADDRESS);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom(ADDRESS, 1);
  byte temp = Wire.read();
  return temp;
}

void changeMode(){
  
  lcd.setCursor(0, 3);
  lcd.print("Enter mode: ");
  
  char keyForMode = keyPad.waitForKey();
  lcd.print(keyForMode); 
  if(keyForMode == '1' or keyForMode == '2' or keyForMode == '3' or keyForMode == '4'){
    mode = keyForMode - '0';
    modeWrite(mode);
  } 
  delay(10);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mode: ");
  if(mode == 255){
    lcd.print("-");
  }else{
    lcd.print(mode);
  }

}
//----------------------------------------------------------


// =========================================================

// Token for user -> I2C
//----------------------------------------------------------

void user1TokenWrite(int token){
  Wire.beginTransmission(ADDRESS);
  Wire.write(0x00);
  Wire.write(0x0A);
  Wire.write(token);
  Wire.endTransmission();
}

void user2TokenWrite(int token){
  Wire.beginTransmission(ADDRESS);
  Wire.write(0x00);
  Wire.write(0x0B);
  Wire.write(token);
  Wire.endTransmission();
}

void user3TokenWrite(int token){
  Wire.beginTransmission(ADDRESS);
  Wire.write(0x00);
  Wire.write(0x0C);
  Wire.write(token);
  Wire.endTransmission();
}

void user4TokenWrite(int token){
  Wire.beginTransmission(ADDRESS);
  Wire.write(0x00);
  Wire.write(0x0D);
  Wire.write(token);
  Wire.endTransmission();
}

void userTokenRead(){
  int temp;
  
  Wire.beginTransmission(ADDRESS);
  Wire.write(0x00);
  Wire.write(0x0A);
  Wire.endTransmission();
  Wire.requestFrom(ADDRESS, 1);
  temp = Wire.read();
  userToken[0] = temp;

  Wire.beginTransmission(ADDRESS);
  Wire.write(0x00);
  Wire.write(0x0B);
  Wire.endTransmission();
  Wire.requestFrom(ADDRESS, 1);
  temp = Wire.read();
  userToken[1] = temp;

  Wire.beginTransmission(ADDRESS);
  Wire.write(0x00);
  Wire.write(0x0C);
  Wire.endTransmission();
  Wire.requestFrom(ADDRESS, 1);
  temp = Wire.read();
  userToken[2] = temp;

  Wire.beginTransmission(ADDRESS);
  Wire.write(0x00);
  Wire.write(0x0D);
  Wire.endTransmission();
  Wire.requestFrom(ADDRESS, 1);
  temp = Wire.read();
  userToken[3] = temp;
}

void setUserLED(){
  if(userToken[0] == 1){
    digitalWrite(led_user1, HIGH);
  }else{
    digitalWrite(led_user1, LOW);
  }

  if(userToken[1] == 1){
    digitalWrite(led_user2, HIGH);
  }else{
    digitalWrite(led_user2, LOW);
  }

  if(userToken[2] == 1){
    digitalWrite(led_user3, HIGH);
  }else{
    digitalWrite(led_user3, LOW);
  }

  if(userToken[3] == 1){
    digitalWrite(led_user4, HIGH);
  }else{
    digitalWrite(led_user4, LOW);
  }
}
//----------------------------------------------------------


// =========================================================

// Vote for each user -> I2C
//----------------------------------------------------------

// write
void userVoteWrite(uint16_t memory_address, int data){
  delay(10);
  Wire.beginTransmission(ADDRESS);
  Wire.write((uint8_t)((memory_address & 0xFF00) >> 8));
  Wire.write((uint8_t)((memory_address & 0x00FF) >> 0));
  Wire.write(data);
  Wire.endTransmission();
}

void user1VoteWrite(){
  int data_vote;
  uint16_t address = 0x0032; //50
  for(int i = 0; i < 7; i++){
    data_vote = userVote_1[i];
    userVoteWrite(address, data_vote);
    address += 0x0001;
  }
}

void user2VoteWrite(){
  int data_vote;
  uint16_t address = 0x004B; //75
  for(int i = 0; i < 7; i++){
    data_vote = userVote_2[i];
    userVoteWrite(address, data_vote);
    address += 0x0001;
  }
}

void user3VoteWrite(){
  int data_vote;
  uint16_t address = 0x0064; //100
  for(int i = 0; i < 7; i++){
    data_vote = userVote_3[i];
    userVoteWrite(address, data_vote);
    address += 0x0001;
  }
}

void user4VoteWrite(){
  int data_vote;
  uint16_t address = 0x007D; //125
  for(int i = 0; i < 7; i++){
    data_vote = userVote_4[i];
    userVoteWrite(address, data_vote);
    address += 0x0001;
  }
}


// read
int userVoteRead(uint16_t memory_address){
  int temp;
  Wire.beginTransmission(ADDRESS);
  Wire.write((uint8_t)((memory_address & 0xFF00) >> 8));
  Wire.write((uint8_t)((memory_address & 0x00FF) >> 0));
  Wire.endTransmission();
  Wire.requestFrom(ADDRESS, 1);
  temp = Wire.read();
  return temp;
}

void user1VoteRead(){
  int data_vote;
  uint16_t address = 0x0032; //50
  for(int i = 0; i < 7; i++){
    data_vote = userVoteRead(address);
    userVote_1[i] = data_vote;
    address += 0x0001;
  }
}

void user2VoteRead(){
  int data_vote;
  uint16_t address = 0x004B; //75
  for(int i = 0; i < 7; i++){
    data_vote = userVoteRead(address);
    userVote_2[i] = data_vote;
    address += 0x0001;
  }
}

void user3VoteRead(){
  int data_vote;
  uint16_t address = 0x0064; //100
  for(int i = 0; i < 7; i++){
    data_vote = userVoteRead(address);
    userVote_3[i] = data_vote;
    address += 0x0001;
  }
}

void user4VoteRead(){
  int data_vote;
  uint16_t address = 0x007D; //125
  for(int i = 0; i < 7; i++){
    data_vote = userVoteRead(address);
    userVote_4[i] = data_vote;
    address += 0x0001;
  }
}

//----------------------------------------------------------


// =========================================================

// check key from keypad
//----------------------------------------------------------
void checkKey(){
  key = keyPad.getKey();
  if(key){ 

    Serial.println();
    Serial.print("key:");
    Serial.print(key);
     
    if(key == '#'){
       lcd.setCursor(0, 3);
       lcd.print(key);
       char keyForPauseOrStop = keyPad.waitForKey();  
       lcd.print(keyForPauseOrStop);    
       if(keyForPauseOrStop == '1'){        //pause & resume
          pause_bit = 1;
       }else if(keyForPauseOrStop == '2'){  //stop
          endVotingAndRestart();
       }
       Serial.print("keyForPauseOrStop:");
       Serial.print(keyForPauseOrStop);
    }
          
    if(key == '0'){
      changeMode();
    } 
         
    if(key == '*'){
      showResult();
    }
    
  }

  lcd.setCursor(0, 3);
  lcd.print("                ");
}
