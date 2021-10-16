/*
 * 10.02.2021
 * ************************************
 * Сергей Новиков. nsv47nsv36@yandex.ru
 * ************************************
 * acc = 200
 * sp = 9-148(162) мм/сек
 * 1600 импульсов на оборот
 * //----------------------------------------------------------------------------------------------------------
 * Скетч выдает импульсы, количество импульсов определяется расстоянием, которое можно задать.
 * Чтобы задать расстояние необходимо нажать клавишу SET, при этом на OLED дисплее выделится
 * первая строка, при еще одном нажатии выделится следующая строка, при следующем нажатии - выделение исчезнет.
 * Когда строка выделена можно крутить ручку энкодера и на дисплее будет отображаться изменение параметра.
 * При нажатии на кнопку "Движение" произойдет перемещение.
 * //----------------------------------------------------------------------------------------------------------
 * скорость нельзя ставить больше, чем значение acc
 * //----------------------------------------------
 * ускорение в попугаях
 * //------------------------------------------------------------------
 * установка отрицательного значения в скорости приводит к зависанию - 
 * сделать переменную беззнаковой
 * //------------------------------------------------------------------
 * 15.02.21
 * при сбрасывании контроллера переменные сбрасываются до значений
 * по умолчанию - хранить в eeprom
 * //------------------------------------------------------------------
 * delay в функции rotation
 * //------------------------------------------------------------------
 * очень много места занимает библиотека для дисплея - использовать 
 * только те функции, которые необходимы
 * //----------------------------------------------------------------
 * 17.02.21
 * нет сигнала direction (подключил к D7, но не запрограммировал)
 * нет концевиков
 * подсветка кнопки вперед\вверх - D9
 * подсветка кнопки назад\вниз - D10
 * подсветка кнопки SET - D11
 * //----------------------------------------------------------------
 * 18.02.21
 * нет возможности поменять шаг винта
 * баг, запрограммировал включение и отключение кнопок подсветки,
 * но при последующем переходе в меню подсветки значение обнуляется
 * //----------------------------------------------------------------
 * 19.03.21
 * не все платы NANO работают с softSerial
 * не работало и не мог понять почему, но
 * когда заменил плату - заработало
 * //-----------------------------------------------------------------
 * проверить RX TX - работает, но RX-RX TX-TX или крест-накрест
 * //-----------------------------------------------------------------
 * !Баг. При отключении питания питание лазера фактически сбрасывается, 
 * но на дисплее кнопка остается включенной
 * //-----------------------------------------------------------------
 * 18.09.21
 * избавился от delay при формировании импульсов для двигателя.
 * Пытаюсь прикрутить VL53LOX, но иногда он вешает всю систему
 * //-----------------------------------------------------------------
 * 18.09.21
 * добавить установку состояний новых кнопок на дисплее, например,
 * 0.1, 0.5, 1 и 5
 * //-----------------------------------------------------------------
 * 18.09.21
 * рассмотреть функцию controlFromTheDisplay() в блоке pow_on. Там есть комментарий.
 * 372 строка.
 * //-----------------------------------------------------------------
 */

#include <SoftwareSerial.h>

#include <Wire.h>
//----polulu-------------------
#include <VL53L0X.h>

VL53L0X sensor;
//-----------------------------

/*
//----adafruit-----------------------------
#include "Adafruit_VL53L0X.h"

Adafruit_VL53L0X sensor = Adafruit_VL53L0X();
//-----------------------------------------
*/
bool state_LED_BUILTIN = LOW;
const byte port_stepOut = 6;
const byte port_direction = 7;
const uint8_t pinRX = 5;
const uint8_t pinTX = 4;
const uint8_t port_power = 8;
const uint8_t port_las = 9;
const uint8_t port_light = 10;
const uint8_t port_EN_ROT_DEV = 11;
const uint8_t port_limit_up = 12;

bool old_state_limit_up = LOW;
unsigned long timer_limit_up = 0;
unsigned long timer_impulse = 0;

bool state_power = false; //на 96 стр есть похожая переменная, может можно объединить???

bool state_port_stepOut = LOW;

//bool flag_departure_to_limit_up = false; // для определения выезда на концевик и установки 
// маркирующего узла в точку фокуса. После этого буду инициализировать VL53L0X
//bool flag_sensorInit = false; // для того, чтобы выполнить sensorInit один раз.

SoftwareSerial softSerial(pinRX,pinTX); 

int T = 625; // чем меньше, тем выше частота вращения
int mySpeed = 20; //скорость в мм/сек, 1 об/сек = 1600 имп/сек = 0.000625 сек

int acceleration = 600; // чем больше, с тем меньшей скорости начинаем движение
byte screwPitch = 5;
float distance = 1;

double theDifferenceIsActual = 0; //переменная для количества миллиметров до фокуса на столе фактическая

//--------------------------------------------------------------------------------
bool access_to_limit_up = false; //чтобы отследить первую из двух команд от дисплея. 
//Глобальная, потому что заходим в функцию два раза из-за особенностей softSerial
//на 81 стр есть похожая переменная, может можно объединить???
//скорее всего нет, потому что эта переменная для того чтобы нельзя было включить 
//лазер без общего питания???
//--------------------------------------------------------------------------------

//int min_value_VX53L0X = 32760;

void impulse(int& T, long& pulses){
  pulses*=2;
  while(pulses){
    bool state_port_limit_up = digitalRead(port_limit_up);
    if(micros() - timer_impulse >= T){
      state_port_stepOut = !state_port_stepOut;
      digitalWrite(port_stepOut, state_port_stepOut);
      timer_impulse = micros();
      pulses--;
    }
    if(state_port_limit_up){
      pulses = 0;
    }
    
  }
}

void rotation(long pulses, int T){
    bool state_port_limit_up = digitalRead(port_limit_up);
    bool state_port_direction = digitalRead(port_direction);
    if(state_port_limit_up){
      if(!state_port_direction){
        if(millis()%500<=5){delay(5); 
          Serial.println("limit UP!");
        }
      }else{
        impulse(T, pulses);
        state_port_limit_up = digitalRead(port_limit_up);
      }
    }else{
      impulse(T, pulses);
      state_port_limit_up = digitalRead(port_limit_up);
    }
}

void acceleration_function(int initialFreqiency, int finalFrequency){
  for(int i = initialFreqiency; i > finalFrequency; i--){ 
    rotation(8, i);
  }
}

void bracking_function(int initialFreqiency, int finalFrequency){
  for(int i = initialFreqiency; i < finalFrequency; i++){
    rotation(8, i);
  }
}

void action(float distance, int mySpeed, int acceleration){
  int T = (float(1.0f/(1600L/screwPitch))*1000000L)/mySpeed/2;
  long numberOfPulses = distance*1600L/screwPitch;
  long pulsesOnTheAcceleration = (acceleration - T)*8L; //разбил оборот на 200 частей
  if(pulsesOnTheAcceleration*2L>=numberOfPulses){  
  acceleration_function(acceleration, acceleration-numberOfPulses/2/8);
  bracking_function(acceleration-numberOfPulses/2/8, acceleration);
  }else{
    acceleration_function(acceleration, T);
    rotation(numberOfPulses-pulsesOnTheAcceleration*2L, T);
    bracking_function(T, acceleration);
  }
}

int readdata(){                              //Эта функция возвращает значение переменной в int, введенной в Serialmonitor  
  byte getByte;
  int outByte=0;
  do{
    while(Serial.available()!=0){ 
      getByte=Serial.read()-48;     // Вычитаем из принятого символа 48 для преобразования из ASCII в int
      outByte=(outByte*10)+getByte; //Сдвигаем outByte на 1 разряд влево, и прибавляем getByte 
      delay(500);                   // Чуть ждем для получения следующего байта из буфера (Чем больше скорость COM, тем меньше ставим задержку. 500 для скорости 9600, и приема 5-значных чисел)
    }
    }while (outByte==0);            //Зацикливаем функцию для получения всего числа
  Serial.println("Ok");
  Serial.flush();                   //Вычищаем буфер (не обязательно)
  return outByte;
}

void controlUart(){                          // Эта функция позволяет управлять системой из монитора порта
  if (Serial.available()) {         // есть что на вход
    String cmd;
    cmd = Serial.readString();
    if (cmd.equals("d")) {           
      Serial.println("введите количество миллиметров");      
      distance = readdata();
      Serial.print("distance: ");
      Serial.println(distance);
    }else if (cmd.equals("sp")) {           
      Serial.println("введите скорость");      
      mySpeed = readdata();
      Serial.print("скорость: ");
      Serial.println(mySpeed);
      Serial.print("T: ");
      Serial.println((float(1.0f/(1600L/screwPitch))*1000000L)/mySpeed);
    }else if (cmd.equals("acc")) {           
      Serial.println("введите ускорение");       
      acceleration = readdata();
      Serial.print("acceleration: ");
      Serial.println(acceleration);
    }else if (cmd.equals("start")) {          
      Serial.println("Поехали!");
      action(distance, mySpeed, acceleration);
    }else if(cmd.equals("up")){
      Serial.println("Поехали!");
      digitalWrite(port_direction, LOW);
      action(distance, mySpeed, acceleration);
    }else if(cmd.equals("down")){
      Serial.println("Поехали!");
      digitalWrite(port_direction, HIGH);
      action(distance, mySpeed, acceleration);
    }else if(cmd.equals("focus")){
      Serial.println("focus!");
      digitalWrite(port_direction, LOW);
      action(500, mySpeed, acceleration);
      digitalWrite(port_direction, HIGH);
      action(100, mySpeed, acceleration);
    }else if(cmd.equals("sensorInit")){
      Serial.println("debugging information");
      sensorInit();
    }else if(cmd.equals("sensorRead")){     
      Serial.println("debugging information");
      
      Serial.print(sensor.readRangeSingleMillimeters());
      if (sensor.timeoutOccurred()) { Serial.print(" TIMEOUT"); }
      Serial.println();
      
    /* 
	* 16.10.21
	* в данной функции есть необходимость считать на сколько миллиметров маркировочный узел уехал от точки фокуса, потому что датчик пока не дает
	* точных показаний. Функция сделана по аналогии с движением вверх/вниз, но система не уходит в положения фокуса после попыток автофокусировки.
	* Есть придположение, что это из-за ситуации, когда "показания_датчика - 225" отрицательное. Нет возможности проверить, программирую из дома.
	* Удалить если после проверки все работает.
	*
	* При отладке "вручную" я пришел к тому, что при ситуации, когда показания_датчика-225 меньше нуля (например -25), при этом функция автофокусировки
	* увела маркировочный узел на 25 мм вверх и вызове функции выхода в положение фокуса на 
	* рабочий стол переменная theDifferenceIsActual принимает значение "не в ту сторону" (становится равной 25). То есть место того чтобы вычитать 
	* из значения theDifferenceIsActual произойдет прибавление (минус на минус дает плюс) и функция movingToZero уведет систему вверх еще на 25 мм.
	*
	* При отладке "вручную" в случае положения над фокусом ошибки не произошло. НО необходимо все проверить! Нужно попробовать автофокусировать
	* систему из положения надо фокусом и, на всякий случай смотреть что выдает датчик, чтобы не проскочило отричательное значение. Версия этой функции с 
	* ошибкой есть в коммите от 16.10.21 на GitHub (самом первом после переноса с локального репозитория).
	*/
    }else if(cmd.equals("autoFocus")){     
      Serial.println("this feature is in development");
      if ((int)sensor.readRangeSingleMillimeters()-225>0){
        digitalWrite(port_direction, HIGH);
      }else{
        digitalWrite(port_direction, LOW);
      }
	  theDifferenceIsActual += (int)sensor.readRangeSingleMillimeters()-225; // Проверить! вычитание не требуется, потому что при отрицательном значении
	  // (int)sensor.readRangeSingleMillimeters()-225 плюс на минус даст минус
      action(abs((int)sensor.readRangeSingleMillimeters()-225), mySpeed, acceleration);
      //Serial.println(abs((int)sensor.readRangeSingleMillimeters()-225));
    }else{
      Serial.println("error");    // ошибка
    }
  }
}

void movingToZero(double count){             // Эта функция перемещает систему в положение нуля
  if (count>0){
    digitalWrite(port_direction, HIGH);
  }else{
    digitalWrite(port_direction, LOW);
  }
  action(abs(count), mySpeed, acceleration);
}

void focusOnTheTable(){                      // Эта функция при первом включении уводит систему на верхний концевик, а потом перемещает систему в положение нуля и обнуляет положение
   Serial.println("focus on the table!");   
   if(!access_to_limit_up){
     access_to_limit_up = true;
     digitalWrite(port_direction, LOW);
     action(500, mySpeed, acceleration);
     digitalWrite(port_direction, HIGH);
     action(142, mySpeed, acceleration);
     theDifferenceIsActual = 0;
     Serial.println("Система в точке фокуса");
     //flag_departure_to_limit_up = true;
     access_to_limit_up = true;
   }else{
      movingToZero(theDifferenceIsActual);
      theDifferenceIsActual = 0;
   }
}

void terminal(){                             // Эта функция выводит в монитор порта информацию о настройках системы
  Serial.println("Готов");
  Serial.println("---------------------------");
  Serial.print("+ ");
  Serial.print("Количество миллиметров: ");
  Serial.println(distance);
  Serial.print("+ ");
  Serial.print("Скорость: ");
  Serial.println(mySpeed);
  Serial.print("+ ");
  Serial.print("Ускорение: ");
  Serial.println(acceleration);
  Serial.print("+ ");
  Serial.print("Скорость мкс: ");
  Serial.println((float(1.0f/(1600L/screwPitch))*1000000L)/mySpeed);
  Serial.println("---------------------------");
}

void settingTheDisplayButtonStates(){        // Эта функция устанавливает состояние выходов в соответствие с состоянием кнопок на дисплее
  //  Устанавливаем состояние кнопки bt0:  
  softSerial.print((String) "print bt0.val"+char(255)+char(255)+char(255)); // Отправляем команду дисплею: «print bt0.val» заканчивая её тремя байтами 0xFF
  while(!softSerial.available()){}                                          // Ждём ответа. Дисплей должен вернуть состояние кнопки bt0, отправив 4 байта данных, 
                                                                            // где первый байт равен 0x01 или 0x00, а остальные 3 равны 0x00
  digitalWrite(port_power, softSerial.read());                              // Устанавливаем на выходе port_power состояние в соответствии с первым принятым байтом ответа дисплея
  delay(10);
  while(softSerial.available()){
    softSerial.read();
    delay(10);
  }
  //-----------------------------------------------------------------------
  softSerial.print((String) "print bt1.val"+char(255)+char(255)+char(255));
  while(!softSerial.available()){}
  digitalWrite(port_las, softSerial.read());         
  delay(10);
  while(softSerial.available()){
    softSerial.read();
    delay(10);
  }
  //-----------------------------------------------------------------------
  softSerial.print((String) "print bt2.val"+char(255)+char(255)+char(255));
  while(!softSerial.available()){}
  digitalWrite(port_light, softSerial.read());          
  delay(10);
  while(softSerial.available()){
    softSerial.read();
    delay(10);
  }
  Serial.println("settingTheDisplayButtonStates has been completed");
}

void controlFromTheDisplay(){
  if(softSerial.available()>0){         // Если есть данные принятые от дисплея, то ...
    String str;                         // Объявляем строку для получения этих данных
    while(softSerial.available()){
      str+=char(softSerial.read());     // Читаем принятые от дисплея данные побайтно в строку str
      delay(10);
    }
    Serial.println(str);                
    for(int i=0; i<str.length(); i++){ // Проходимся по каждому символу строки str
      //----------------------------------------------
      if(memcmp(&str[i],"movingUp" , 8)==0){ // Если в строке str начиная с символа i находится текст "movingUp",  значит кнопка дисплея была включена
        i+=7; 
        digitalWrite(port_direction, LOW);
        action(distance, mySpeed, acceleration);
        theDifferenceIsActual += distance;
      }else
      //----------------------------------------------
      if(memcmp(&str[i],"movingDown", 10)==0){
        i+=9; 
        digitalWrite(port_direction, HIGH);
        state_LED_BUILTIN = !state_LED_BUILTIN;
        digitalWrite(LED_BUILTIN, state_LED_BUILTIN);
        action(distance, mySpeed, acceleration);
        theDifferenceIsActual -= distance;
      }else              
      //----------------------------------------------
      if(memcmp(&str[i],"light_ON", 8)==0){
        i+=7; 
        digitalWrite(port_light, HIGH);
      }else 
      //----------------------------------------------
      if(memcmp(&str[i],"light_OFF", 9)==0){
        i+=8; 
        digitalWrite(port_light, LOW);
      }else 
      //----------------------------------------------
      if(memcmp(&str[i],"pow_OFF", 7)==0){
        i+=6; 
        digitalWrite(port_power, LOW);
        state_power = false;
        digitalWrite(port_las, LOW);
        state_LED_BUILTIN = false;
        digitalWrite(LED_BUILTIN, state_LED_BUILTIN);
      }else 
      //----------------------------------------------
      if(memcmp(&str[i],"pow_ON", 6)==0){
        i+=5; 
        digitalWrite(port_power, HIGH);
        state_power = true;
        //------------------------------------------------------------------------------------------------------------------
        // скорее всего этот блок кода для случая, когда кнопка питания лазера уже активирована, а основного питания еще нет. При этом выход контроллера включит
        // питание лазера. Рассмотреть правильность, может быть не позволять активировать кнопку включения питания лазера без основного питания???
        // Устанавливаем состояние кнопки включения питания лазера:                  
        softSerial.print((String) "print bt1.val"+char(255)+char(255)+char(255)); // Отправляем команду дисплею: «print bt1.val» заканчивая её тремя байтами 0xFF
        while(!softSerial.available()){}                                          // Ждём ответа. Дисплей должен вернуть состояние кнопки bt1, отправив 4 байта данных, где 1 байт равен 0x01 или 0x00, а остальные 3 равны 0x00
        digitalWrite(port_las, softSerial.read());       delay(10);               // Устанавливаем на выходе port_las состояние в соответствии с первым принятым байтом ответа дисплея
        while(softSerial.available()){softSerial.read(); delay(10);}
        //------------------------------------------------------------------------------------------------------------------
        if(!access_to_limit_up){     // при первом включении отправить систему на верхний концевик
          focusOnTheTable();
          access_to_limit_up = true;
        }
      }else 
      //----------------------------------------------
      if(memcmp(&str[i],"las_OFF", 7)==0){
        i+=6; 
        digitalWrite(port_las, LOW);
      }else
      //----------------------------------------------
      if(memcmp(&str[i],"las_ON", 6)==0){
        i+=5;
        if(state_power){
          digitalWrite(port_las, HIGH);
        }
      }else
      //----------------------------------------------
      if(memcmp(&str[i],"EN_ROT_DEV_ON", 13)==0){
        i+=12;
        digitalWrite(port_EN_ROT_DEV, HIGH);
      }else
      //----------------------------------------------
      if(memcmp(&str[i],"EN_ROT_DEV_OFF", 14)==0){
        i+=13;
        digitalWrite(port_EN_ROT_DEV, LOW);
      }else
      //----------------------------------------------
      if(memcmp(&str[i],"set_dist_0.1", 12)==0){
        i+=11;
        distance = 0.1;
      }else
      //----------------------------------------------
      if(memcmp(&str[i],"set_dist_0.5", 12)==0){
        i+=11;
        distance = 0.5;
      }else
      //----------------------------------------------
      if(memcmp(&str[i],"set_dist_1", 10)==0){
        i+=9;
        distance = 1;
      }else
      //----------------------------------------------
      if(memcmp(&str[i],"set_dist_5", 10)==0){
        i+=9;
        distance = 5;
      }else
      //----------------------------------------------
      if(memcmp(&str[i],"focus", 5)==0){
        i+=4;
        Serial.println("focus on the table!");
        focusOnTheTable();
      }
    }
  }
}

void sensorInit(){
  
  //----polulu--------------------------------------------------
  sensor.setTimeout(500);
  if (!sensor.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1) {}
  }

  sensor.setMeasurementTimingBudget(200000);
  Serial.println("sensorInit has been completed");
  //-----------------------------------------------------------
  
  /*
  //----adafruit-----------------------------------------------
  Serial.println("Adafruit VL53L0X test");
  if (!sensor.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }
  // power 
  Serial.println(F("VL53L0X API Simple Ranging example\n\n")); 
  //-----------------------------------------------------------
  */
}

void setup() {
 
  Serial.begin(9600);
  
  softSerial.begin(9600);

  Wire.begin();
  
  pinMode(port_stepOut, OUTPUT);  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(port_direction, OUTPUT);
  pinMode(port_power, OUTPUT);
  pinMode(port_las, OUTPUT);
  pinMode(port_light, OUTPUT);
  pinMode(port_EN_ROT_DEV, OUTPUT);
  pinMode(port_limit_up, INPUT_PULLUP);

  digitalWrite(port_light, HIGH);

  terminal();
  //-----------------------------------------------------------------------
  settingTheDisplayButtonStates();
  //-----------------------------------------------------------------------
  Serial.println("Ready!");
  sensorInit();
}

void loop() {
    if(millis()%500<=5){
      delay(5);
      
      //Serial.println(min_value_VX53L0X);
    } 
      
    controlFromTheDisplay();
  
    controlUart();
  
}
