/*
    Example code compatible with the Knock Shield for Arduino.
    
    Copyright (C) 2018 - 2019 Bylund Automotive AB
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    Contact information of author:
    http://www.bylund-automotive.com/
    
    info@bylund-automotive.com

    Version history:
    2018-08-12        v1.0.0        First release to Github.
    2019-08-31        v1.0.1        Correction in variable descriptions.
    2019-12-18        v1.1.0        Implemented an analog output feature.
    2020-05-24        v1.2.0        Implemented both channels.
    2020-07-05        v1.3.0        Implemented support for data logging.
*/

//Define included headers.
#include <SPI.h>
#include <SD.h>

//Define registers and parameters used.
#define           SPU_SET_PRESCALAR_6MHz         0b01000100    /* 6MHz prescalar with SDO active */
#define           SPU_SET_CHANNEL_1              0b11100000    /* Setting active channel to 1 */
#define           SPU_SET_CHANNEL_2              0b11100001    /* Setting active channel to 2 */
#define           SPU_SET_BAND_PASS_FREQUENCY    0b00101010    /* Setting band pass frequency to 7.27kHz */
#define           SPU_SET_PROGRAMMABLE_GAIN      0b10100010    /* Setting programmable gain to 0.381 */
#define           SPU_SET_INTEGRATOR_TIME        0b11001010    /* Setting programmable integrator time constant to 100µs */
#define           KNOCK_THRESHOLD_LEVEL          204           /* Setting knock LED level, max value of 255. */
#define           MEASUREMENT_WINDOW_TIME        3000          /* Defining the time window of measurement to 3ms. */

//Define pin assignments.
#define           SPU_NSS_PIN                    10            /* Pin used for chip select in SPI communication. */
#define           SDCARD_CS_PIN                  9             /* Pin used for chip select of SD-card in SPI communication. */
#define           SPU_TEST_PIN                   5             /* Pin used for SPU communication. */
#define           SPU_HOLD_PIN                   4             /* Pin used for defining the knock window. */
#define           LED_STATUS                     7             /* Pin used for power the status LED, indicating we have power. */
#define           LED_LIMIT                      6             /* Pin used for the limit LED. */
#define           ANALOG_INPUT_PIN               0             /* Analog input for knock. */
#define           ANALOG_OUTPUT_PIN              3             /* Pin used for the PWM to the 0-1V analog output. */

//Global variables.
bool logEnabled = false;                                            /* Variable used for setting data logging enable or disabled. */

//Function for transfering SPI data to the SPU.
byte COM_SPI(byte TX_data) {

  //Configure SPI for knock controller.
  SPI.setDataMode(SPI_MODE1);
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  
  //Set chip select pin low, chip in use. 
  digitalWrite(SPU_NSS_PIN, LOW);

  //Transmit request.
  uint16_t Response =  SPI.transfer(TX_data);

  //Set chip select pin high, chip not in use.
  digitalWrite(SPU_NSS_PIN, HIGH);

  return Response;
}

//Data logging function.
void logData(String logString) {

  //Connect to SD-Card.
  if ( SD.begin(SDCARD_CS_PIN) ) {

    //Open file.
    File logFile = SD.open("log.txt", FILE_WRITE);

    //Store data.
    logFile.println(logString);

    //Close file.
    logFile.close();

  } else {
    
    //Error handling.
    Serial.println("Error accessing SD-card.");  
    
  }
  
}

//Function to set up device for operation.
void setup() {
  
  //Set up serial communication.
  Serial.begin(9600);

  //Set up SPI.
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  
  //Set up digital output pins.
  pinMode(SPU_NSS_PIN, OUTPUT);  
  pinMode(SDCARD_CS_PIN, OUTPUT); 
  pinMode(SPU_TEST_PIN, OUTPUT);  
  pinMode(SPU_HOLD_PIN, OUTPUT);  
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_LIMIT, OUTPUT);

  //Set initial values.
  digitalWrite(SPU_NSS_PIN, HIGH);
  digitalWrite(SDCARD_CS_PIN, HIGH);
  digitalWrite(SPU_TEST_PIN, HIGH);
  digitalWrite(SPU_HOLD_PIN, LOW);

  //Start of operation. (Flash LED's).
  Serial.print("Device reset.\n\r");
  digitalWrite(LED_STATUS, HIGH);
  digitalWrite(LED_LIMIT, HIGH);
  delay(200);
  digitalWrite(LED_STATUS, LOW);
  digitalWrite(LED_LIMIT, LOW);
  delay(200);

  //Configure data logging.
  if ( SD.begin(SDCARD_CS_PIN) ) {
    
    //Enable data logging.
    Serial.println("Data logging enabled.");
    logEnabled = true;

  }

  //Configure SPU.
  Serial.print("Configuring SPU: 0x");
  Serial.print( COM_SPI(SPU_SET_PRESCALAR_6MHz), HEX );
  Serial.print(", 0x");
  Serial.print( COM_SPI(SPU_SET_BAND_PASS_FREQUENCY), HEX );
  Serial.print(", 0x");
  Serial.print( COM_SPI(SPU_SET_PROGRAMMABLE_GAIN), HEX );
  Serial.print(", 0x");
  Serial.print( COM_SPI(SPU_SET_INTEGRATOR_TIME), HEX );
  Serial.print("\n\r");

}

//Main operation function.
void loop() {

  //Infinite loop.
  while (1) {
    
    //Set status LED on, operation starts.
    digitalWrite(LED_STATUS, HIGH);

    //Set channel 1.
    COM_SPI(SPU_SET_CHANNEL_1);
      
    //The measurement window starts by driving digital pin 4 high.
    digitalWrite(SPU_HOLD_PIN, HIGH);

    //The SPU performs the integration process and increases the output voltage based on the signal processing result.
    delayMicroseconds(MEASUREMENT_WINDOW_TIME);
    
    //The measurement window ends by driving digital pin 4 low. The SPU stops the integration process and the output voltage is frozen until the window starts again.
    digitalWrite(SPU_HOLD_PIN, LOW);

    //The SPU output voltage is read by the Arduino ADC on analogue input pin 0.
    uint16_t adcChannel1 = analogRead(ANALOG_INPUT_PIN);

    //Set channel 2.
    COM_SPI(SPU_SET_CHANNEL_2);
      
    //The measurement window starts by driving digital pin 4 high.
    digitalWrite(SPU_HOLD_PIN, HIGH);

    //The SPU performs the integration process and increases the output voltage based on the signal processing result.
    delayMicroseconds(MEASUREMENT_WINDOW_TIME);
    
    //The measurement window ends by driving digital pin 4 low. The SPU stops the integration process and the output voltage is frozen until the window starts again.
    digitalWrite(SPU_HOLD_PIN, LOW);

    //The SPU output voltage is read by the Arduino ADC on analogue input pin 0.
    uint16_t adcChannel2 = analogRead(ANALOG_INPUT_PIN); 

    //Convert ADC-values to percentages.
    float knockChannel1 = ((float)adcChannel1 / 1023) * 100;
    float knockChannel2 = ((float)adcChannel2 / 1023) * 100;

    //Set analog output to highest value from either channels.
    uint16_t adcValue = 0;
    if (adcChannel1 < adcChannel2) adcValue = adcChannel2; else adcValue = adcChannel1;

    //Set analog 0-5V output.
    uint8_t analogOutput = map(adcValue, 0, 1023, 0, 255);
    analogWrite(ANALOG_OUTPUT_PIN, analogOutput);
      
    //Set Limit LED if knock percentage reaches 80%.
    if (analogOutput >= KNOCK_THRESHOLD_LEVEL) digitalWrite(LED_LIMIT, HIGH); else digitalWrite(LED_LIMIT, LOW);
    
    //Assemble data.
    String txString = "Channel 1: ";
    txString += String(knockChannel1, 0);
    txString += "% - Channel 2: ";
    txString += String(knockChannel2, 0);
    txString += "%";
    
    //Output string
    Serial.println(txString);

    //Log string.
    if (logEnabled == true) logData(txString);
    
    //Set status LED off, operation stops. Inlcuding additional delay time for visibility.
    delay(100);
    digitalWrite(LED_STATUS, LOW);
    
    //Delay to update serial output every ~500ms. 
    delay(400);

  }
  
}
