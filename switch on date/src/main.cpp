#include <Arduino.h>
#include <GyverOLED.h>

#define pin_SW_SDA 6 // микросхема времени
#define pin_SW_SCL 5 // микросхема времени
#include <iarduino_RTC.h>
#include <Wire.h>
#include <EEPROM.h>
#define SDA_display A4
#define SCK_display A5
#define S1_encoder 2
#define S2_encoder 3
#define KEY_encoder 4
#define enable_disable_pin 8

iarduino_RTC time(RTC_DS1307); 

// encoder
int right;
int cur1s, cur2s;
int prev1s;
int val = 0;
bool flag_encode;
// encoder

char* now_time;
char* day_week;
bool first_start = true;
bool start_m = true;
unsigned long beftime;
unsigned long every_time_pause;
unsigned long last_active;
bool left_encode = false;
bool right_encode = false;
bool click_encode = false;
bool select_menu = false;
bool status_move = false;
bool first_menu = false;
int8_t move_menu = 0;
int8_t move_menu_before;
bool select_settime = false;
bool select_setonetime = false;

uint16_t attime; // in minutes
uint16_t totime; // in minutes

uint8_t attime_hours;
uint8_t attime_minutes;
uint8_t totime_hours;
uint8_t totime_minutes;

bool selected_ontime;
bool selected_on_at_hours;
bool selected_on_at_minutes;
bool selected_on_to_hours;
bool selected_on_to_minutes;

uint8_t time_now_hours;
uint8_t time_now_minutes;
uint8_t time_now_day;

void getcurrect_on_time(){
  // beftime = 14500;
  EEPROM.get(3, attime);
  EEPROM.get(9, totime);
  attime_hours = attime/60;
  attime_minutes = attime-attime_hours*60;
  totime_hours = totime/60;
  totime_minutes = totime-totime_hours*60;
}

GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;
void setup() {
  every_time_pause = 57000;
  time.begin();
  oled.init();
  getcurrect_on_time();
  pinMode(enable_disable_pin, OUTPUT);
  pinMode(S2_encoder, INPUT);
  pinMode(S1_encoder, INPUT);
  pinMode(KEY_encoder, INPUT);
}

void print_zero(uint8_t check_ten){
  if (check_ten < 10){
    oled.print(0);
  }
}


void check_encoder(int cur1s){
  if (cur1s != prev1s){
    cur2s = digitalRead(S2_encoder);
    if (flag_encode){
      if (cur1s == cur2s){
        val++;
        right = 2;
      }
      else {
        val--;
        right = 1; 
      }
      flag_encode = false;
    }
    else {
      flag_encode = true;
      right = 0;
    }
  }
  prev1s = cur1s;
}



char* get_weekday(uint8_t day){
  switch (day) {
    case 0: return (char *) "ВС";
    case 1: return (char *) "ПН";
    case 2: return (char *) "ВТ";
    case 3: return (char *) "СР";
    case 4: return (char *) "ЧТ";
    case 5: return (char *) "ПТ";
    case 6: return (char *) "СБ";
  }
  return (char *) "ERROR";
}


void start_menu(){
  if (first_start){
    oled.setContrast(70);
    oled.clear();       // очистка
    oled.home();        // курсор в 0,0
    oled.setScale(2);   // масштаб текста (1..4)
    oled.print("Время:");
    oled.setCursor(0, 5);
    oled.print("День:");
    first_start = false;
  }
  if (millis() - every_time_pause > 60000){
    time.gettime();
    every_time_pause = millis();
    // if (time_now_hours > time.Hours && time_now_hours != 23) {
    //   time.settime(0, time_now_minutes, time_now_hours);
    // }
    time_now_hours = time.Hours;
    time_now_minutes = time.minutes;
    time_now_day = time.weekday;
  }
  oled.setCursor(0, 2);
  day_week = get_weekday(time_now_day);
  print_zero(time_now_hours);
  oled.print(time_now_hours);
  oled.print(":");
  print_zero(time_now_minutes);
  oled.print(time_now_minutes);
  oled.setCursor(60, 5);
  oled.print(day_week);
}

void get_right_encode(){
  if (millis() - beftime > 330 && digitalRead(KEY_encoder) == LOW){
    beftime = millis();
    click_encode = true;
  }
  cur1s = digitalRead(S1_encoder);
  check_encoder(cur1s);
  switch (right) {
    case 1:
      right_encode = true;
      right = 0;
    break;
    case 2:
      left_encode = true;
      right = 0;
    break;
  }
}

void off_flags_encm(){
  start_m = false;
  left_encode  = false;
  right_encode = false;
  click_encode = false;
}

bool check_move(){
  status_move = (left_encode || right_encode || click_encode);
  return status_move;
}

void get_clearmenutext(){
  oled.clear();
  oled.setScale(2);
  oled.setCursor(33, 0);
  oled.println("меню");
}

int8_t get_move(){
  move_menu_before = 0;
  move_menu = 0;
  if (right_encode){
      move_menu += 1;
    }
  if (left_encode){
    move_menu -= 1;
  }
  return move_menu;
}

void get_first_menu(){
  get_clearmenutext();
  oled.setCursor(0, 2);
  oled.println("set time");
  oled.println("set ontime");
  first_menu = false;
  right_encode = false;
  left_encode = false;
}


void check_start_menu(){
  if (check_move() && !select_menu) {
    if (select_settime + select_setonetime == 0){
      last_active = millis();
      select_menu = true;
      first_menu = true;
      oled.setContrast(230);
    }
  }
}
void start_menu_selected(){
  if (check_move() && select_menu){
    last_active = millis();
    if (first_menu){
      get_first_menu();
    }
    move_menu = get_move();
    if (move_menu_before > move_menu){
      select_settime = true;
      select_setonetime = false;
      get_clearmenutext();
      oled.setCursor(0, 2);
      oled.println("set time");
    }
    if (move_menu_before < move_menu){
      select_setonetime = true;
      select_settime = false;
      get_clearmenutext();
      oled.setCursor(0, 4);
      oled.println("set ontime");
    }
    if (click_encode){
      if (select_settime || select_setonetime){
        select_menu = false;
      }
    }
    off_flags_encm();
  }
}

bool selected_minutes = false;
bool selected_hours = false;
bool selected_settime = false;
bool selected_day = false;
uint8_t day_move = 0;
uint32_t unix_time;

void get_clearsettimetext(){
  oled.clear();
  day_week = get_weekday(time.weekday);
  oled.setCursor(0, 5);
  oled.println(day_week);
  oled.home();
  oled.println("set time:");
}

void print_setdays(){
  oled.fastLineH(60, 0, 22);
}
void print_setminutes(){
  oled.fastLineH(33, 34, 50);
}
void print_sethours(){
  oled.fastLineH(33, 0, 22);
}
void print_start_menusettime(){
  get_clearsettimetext();
  oled.setCursor(0,2);
  oled.print(time.gettime("H:i"));
}

void get_daymove(uint8_t maxint){
  if (move_menu_before > move_menu && !selected_settime){ // - left
    if (day_move >= 1){
      day_move -= 1;
    }
  }
  if (move_menu_before < move_menu && !selected_settime){ // + right
    day_move += 1;
    if (day_move >= maxint){
      day_move = maxint;
    }
  }
}

void alloff_select_settime(){
  selected_hours = false;
  selected_minutes = false;
  selected_day = false;
}

void ifsetup_settime(){
  if (check_move() && select_settime && !select_menu){
    last_active = millis();
    if (selected_minutes + selected_hours + selected_day == 0 || !selected_settime){
      print_start_menusettime();
    }
    move_menu = get_move();
    if (!selected_settime){
      get_daymove(2);
      switch (day_move) {
      case 0:
        alloff_select_settime();
        selected_hours = true;
        print_sethours();
      break;
      case 1:
        alloff_select_settime();
        selected_minutes = true;
        print_setminutes();
      break;
      case 2:
        alloff_select_settime();
        selected_day = true;
        print_setdays();
      break;
      }
    }
    if (selected_minutes + selected_hours + selected_day == 1 && selected_settime){
      unix_time = time.gettimeUnix();
      if (selected_minutes){
        if (left_encode){
          unix_time -= 60;
        }
        if (right_encode){
          unix_time += 60;
        }
      }
      if (selected_hours){
        if (left_encode){
          unix_time -= 3600;
        }
        if (right_encode){
          unix_time += 3600;
        }
      }
      if (selected_day){
        if (left_encode){
          unix_time -= 86400;
        }
        if (right_encode){
          unix_time += 86400;
        }
      }
      time.settimeUnix(unix_time);
      print_start_menusettime();
    }
    if (click_encode){ // choise
      if (selected_settime){
        selected_settime = false;
        every_time_pause = 57000;
      }
      else if (selected_minutes || selected_hours || selected_day){
        selected_settime = true;
      }
    }
    off_flags_encm();
  }
}


void print_attime_hours(){
  oled.fastLineH(33, 29, 48);
}
void print_attime_min(){
  oled.fastLineH(33, 61, 80);
}
void print_totime_hours(){
  oled.fastLineH(60, 29, 48);
}
void print_totime_min(){
  oled.fastLineH(60, 61, 80);
}

void update_attime(){
  oled.setCursor(0, 2);
  oled.print("at ");
  print_zero(attime_hours);
  oled.print(attime_hours);
  oled.print(":");
  print_zero(attime_minutes);
  oled.print(attime_minutes);
}



void update_totime(){
  oled.setCursor(0, 5);
  oled.print("to ");
  print_zero(totime_hours);
  oled.print(totime_hours);
  oled.print(":");
  print_zero(totime_minutes);
  oled.print(totime_minutes);
}

void get_clearset_on_timetext(){
  oled.clear();
  oled.home();
  oled.println("time work:");
  update_attime();
  update_totime();
}


void alloff_select_one_time(){
  selected_on_at_hours = false;
  selected_on_at_minutes = false;
  selected_on_to_hours = false;
  selected_on_to_minutes = false;
}

uint8_t allsumm_bool_onetime(){
  return selected_on_at_hours + selected_on_at_minutes + selected_on_to_hours + selected_on_to_minutes;
}


void ifsetup_setonetime(){ // не работаит ещо
  if (check_move() && select_setonetime && !select_menu){
    last_active = millis();
    if (allsumm_bool_onetime() == 0 || !selected_ontime){
        get_clearset_on_timetext();
      }
    move_menu = get_move();
    if (!selected_ontime){
      get_daymove(3);
      switch (day_move) {
      case 0:
        alloff_select_one_time();
        selected_on_at_hours = true;
        print_attime_hours();
      break;
      case 1:
        alloff_select_one_time();
        selected_on_at_minutes = true;
        print_attime_min();
      break;
      case 2:
        alloff_select_one_time();
        selected_on_to_hours = true;
        print_totime_hours();
      break;
      case 3:
        alloff_select_one_time();
        selected_on_to_minutes = true;
        print_totime_min();
      break;
      }
    }
    if (allsumm_bool_onetime() == 1 && selected_ontime){
      if (selected_on_at_hours){
        if (left_encode){
          attime -= 60;
        }
        if (right_encode){
          attime += 60;
        }
      }
      if (selected_on_at_minutes){
        if (left_encode){
          attime -= 1;
        }
        if (right_encode){
          attime += 1;
        }
      }
      if (selected_on_to_hours){
        if (left_encode){
          totime -= 60;
        }
        if (right_encode){
          totime += 60;
        }
      }
      if (selected_on_to_minutes){
        if (left_encode){
          totime -= 1;
        }
        if (right_encode){
          totime += 1;
        }
      }
      if (attime > 62000){
        attime = 1439;
      }
      else if (attime >= 1440)
      {
        attime = 0;
      }
      if (totime > 62000){
        totime = 1439;
      }
      else if (totime >= 1440)
      {
        totime = 0;
      }
      attime_hours = attime/60;
      attime_minutes = attime-attime_hours*60;
      totime_hours = totime/60;
      totime_minutes = totime-totime_hours*60;
      update_attime();
      update_totime();
    }
    if (click_encode){ // choise
      if (selected_ontime){
        selected_ontime = false;
        EEPROM.put(3, attime);
        EEPROM.put(9, totime);
      }
      else if (allsumm_bool_onetime() == 1){
        selected_ontime = true;
      }
    }
    off_flags_encm();
  }
}


void check_active(){
  if (!start_m && millis() - last_active > 5000){
    off_flags_encm();
    start_m = true;
    first_start = true;
    select_menu = false;
    select_settime = false;
    select_setonetime = false;
    alloff_select_one_time();
    selected_ontime = false;
    selected_settime = false;
    alloff_select_settime();
  }
}

bool worked_time = false;
uint16_t current_time;

void check_enable_disable_pin(){
  if (millis() - every_time_pause > 60000){
    current_time = time_now_hours*60 + time_now_minutes;
    if (attime <= current_time+1 && current_time <= totime && worked_time == false){
      worked_time = true;
      digitalWrite(enable_disable_pin, 1);
    }
    if (current_time >= totime && worked_time == true){
      worked_time = false;
      digitalWrite(enable_disable_pin, 0);
    }
  }
}

void loop() {
  check_enable_disable_pin();
  if (start_m && millis() - beftime > 1000){
    beftime = millis();
    start_menu();
  }
  check_active();
  get_right_encode();
  check_start_menu();
  ifsetup_settime();
  ifsetup_setonetime();
  start_menu_selected();
}