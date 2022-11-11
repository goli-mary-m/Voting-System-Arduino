#include <LiquidCrystal.h>
#include <Keypad.h>
#include <Wire.h>
#include <SPI.h>


#define SCK_PIN   52
#define MISO_PIN  50
#define MOSI_PIN  51
#define SS_PIN    53

#define led_voted    8
#define led_notVoted 9

#define buzzPin 34

#define RS 7
#define E  6
#define D7 5
#define D6 4
#define D5 3
#define D4 2

LiquidCrystal lcd (RS, E, D4, D5, D6, D7);

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte rowPins[ROWS] = {25, 26, 27, 28}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {24, 23, 22}; //connect to the column pinouts of the keypad

Keypad keyPad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

int token;
/*
   token = 0 -> Not-Voted
   token = 1 -> Voted
*/
int mode;
/*
   mode = 1 -> no/yes                      -> input: 0 or 1
   mode = 2 -> 0/1/2/3/4/5 star            -> input: 0 or 1 or 2 or 3 or 4 or 5
   mode = 3 -> select one person           -> input: 0 or 1 or 2 or 3
   mode = 4 -> select multiple people      -> input: 0 or 1 or 2 or 3 or more
*/
int userVote[7];
/*
   mode = 1 -> userVote[0]
   mode = 2 -> userVote[1]
   mode = 3 -> userVote[2]
   mode = 4 -> userVote[3] - userVote[4] - userVote[5] - userVote[6]
               (person-0)    (person-1)    (person-2)    (person-3)

               0 -> not selected -- 1 -> selected
*/

int restart_bit;
int pause_bit;

String input = "";
int input_vote;
String registeredVote = "";
bool flag_invalidInput = false;

// SPI
// receive from Master
volatile int i_receive = 0;
volatile bool flag_receive = false;
char data_receive[3];
/*
   data_receive[0] -> mode
   data_receive[1] -> restart_bit
   data_receive[2] -> pause_bit
*/

// send to Master
volatile int i_send = 0;
volatile bool flag_send = false;
char data_send[12];
/*
    data_send[0] ... data_send[2]  -> -
    data_send[3]                   -> token
    data_send[4] ... data_send[10] -> userVote (userVote[0] ... userVote[6])
    data_send[11]                  -> '\r'
*/


int cnt;

void setup() {
  
  Serial.begin(9600);
  Serial.println("-> Slave-1");
  Serial.println();

  pinMode(buzzPin, OUTPUT);

  cnt = 1;
  
  // SPI
  pinMode(SS_PIN, INPUT_PULLUP);
  pinMode(MOSI_PIN, INPUT);
  pinMode(MISO_PIN, OUTPUT);
  pinMode(SCK_PIN, INPUT);
  SPCR |= _BV(SPE);
  SPI.attachInterrupt();  //allows SPI interrupt

  lcd.begin(16, 4);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mode: ");
  if (mode == 255) {
    lcd.print("-");
  } else {
    lcd.print(mode);
  }

  restart_bit = 0;
  pause_bit = 0;
  
  pinMode(led_voted,    OUTPUT);
  pinMode(led_notVoted, OUTPUT);
  setUserLED();
}


void loop() {

  if (flag_receive == true) {
    mode = data_receive[0] - '0';
    restart_bit = data_receive[1] - '0';
    pause_bit = data_receive[2] - '0';
  }

  Serial.println(mode);
  Serial.println(restart_bit);
  Serial.println(pause_bit);

  setUserLED();

  lcd.clear();
  lcd.print("Mode: ");
  if (mode == 255) {
    lcd.print("-");
  } else {
    lcd.print(mode);
  }

  if(restart_bit == 1){
    token = 0;
    for(int i = 0; i < 7; i++){
      userVote[i] = 0;
    }
  }

  if (pause_bit == 1) {
    lcd.setCursor(0, 3);
    lcd.print("[Pause]");
    delay(10);
  } else {
    lcd.setCursor(0, 3);
    lcd.print("       ");
    char key = keyPad.getKey();
    if (key) {

      if (key == '#') {
        if (token == 1) {
          lcd.setCursor(0, 3);
          lcd.print("Voted before");
          
          tone(buzzPin, 1000); // Send 1KHz sound signal
          delay(200);
          noTone(buzzPin);
          delay(100);

        } else if (token == 0) {
          token = 1;

          if (mode == 1) {
            if (input.length() == 1) {
              input_vote = input.toInt();
              if (input_vote == 0 or input_vote == 1) {
                userVote[0] = input_vote;
              } else {
                flag_invalidInput = true;
              }
            } else {
              flag_invalidInput = true;
            }

          } else if (mode == 2) {
            if (input.length() == 1) {
              input_vote = input.toInt();
              if (input_vote == 0 or input_vote == 1 or input_vote == 2 or input_vote == 3 or input_vote == 4 or input_vote == 5) {
                userVote[1] = input_vote;
              } else {
                flag_invalidInput = true;
              }
            } else {
              flag_invalidInput = true;
            }

          } else if (mode == 3) {
            if (input.length() == 1) {
              input_vote = input.toInt();
              if (input_vote == 0 or input_vote == 1 or input_vote == 2 or input_vote == 3) {
                userVote[2] = input_vote;
              } else {
                flag_invalidInput = true;
              }
            } else {
              flag_invalidInput = true;
            }

          } else if (mode == 4) {
            char input_str [10];
            input.toCharArray(input_str, 10);
            char *ptr;
            ptr = strtok(input_str, "*");
            while (ptr != NULL) {
              input_vote = ptr[0] - '0';
              ptr = strtok(NULL, "*");
              if (input_vote == 0 or input_vote == 1 or input_vote == 2 or input_vote == 3) {
                int index = input_vote + 3;
                userVote[index] = 1;
              } else {
                flag_invalidInput = true;
              }
            }
            for (int i = 3; i < 7; i++) {
              if (userVote[i] != 1) {
                userVote[i] = 0;
              }
            }
          }
        }

        input = "";

      } else {
        if (key == '9') {
          input = "";
        } else {
          input += key;
        }
      }
    }

    lcd.setCursor(0, 2);
    lcd.print("Input: ");
    lcd.print(input);

    if (flag_invalidInput == true) {
      lcd.setCursor(0, 3);
      lcd.print("Invalid Input!");
      flag_invalidInput = false;
      
      tone(buzzPin, 100); // Send 1KHz sound signal
      delay(200);
      noTone(buzzPin);
      delay(100);    
           
      token = 0;
    }

    printUserVote();
    delay(10);
  }

  sendDataToMaster();
}

// SPI
//----------------------------------------------------------
void sendDataToMaster() {

  Serial.print("Send data to Master: ");
  data_send[0] = ' ';
  data_send[1] = ' ';
  data_send[2] = ' ';

  data_send[3] = token + '0';

  String str = "";
  for (int i = 0; i < 7; i++) {
    str += String(userVote[i]);
  }
  str.toCharArray(data_send + 4, 8);
  data_send[11] = '\r';

  Serial.println(data_send + 3);

  if (flag_send) {
    i_send = 0;
    flag_send = false;
    i_receive = 0;
  }

  Serial.println();
  Serial.println("----------------------------------------");
  Serial.println();
}

ISR (SPI_STC_vect)   //Inerrrput routine function
{
  // receive from Master
  byte b_r = SPDR;
  if (i_receive <= 3) {
    data_receive[i_receive] = b_r;
    if (i_receive == 3) {
      flag_receive = true;
    }
    i_receive++;
  }

  // send to Master
  byte b_s;
  if (i_send <= 12) {
    b_s = data_send[i_send];
    SPDR = b_s;
    if (b_s == '\r') {
      flag_send = true;
    }
    i_send++;
  }
}


//----------------------------------------------------------

void setUserLED() {
  if (token == 1) {
    digitalWrite(led_voted, HIGH);
    digitalWrite(led_notVoted, LOW);
  } else {
    digitalWrite(led_voted, LOW);
    digitalWrite(led_notVoted, HIGH);
  }
}

void printUserVote() {

  lcd.setCursor(0, 1);
  lcd.print("Vote: ");
  if (mode == 1) {
    if (token == 1) {
      if (userVote[0] == 0) {
        lcd.print("No");
      } else if (userVote[0] == 1) {
        lcd.print("Yes");
      }
    } else {
      lcd.print("-");
    }
  } else if (mode == 2) {
    if (token == 1) {
      int n_star = userVote[1];
      for (int i = 0; i < n_star; i++ ) {
        lcd.print("*");
      }
    } else {
      lcd.print("-");
    }

  } else if (mode == 3) {
    if (token == 1) {
      lcd.print(String(userVote[2]));
    } else {
      lcd.print("-");
    }

  } else if (mode == 4) {
    if (token == 1) {
      int selected[4];
      int cnt_selected = 0;
      for (int i = 3; i < 7; i++) {
        if (userVote[i] == 1) {
          selected[cnt_selected] = i - 3;
          cnt_selected++;
        }
      }
      for (int i = 0; i < cnt_selected; i++) {
        lcd.print(String(selected[i]) + " ");
      }
    } else {
      lcd.print("-");
    }
  }
}
