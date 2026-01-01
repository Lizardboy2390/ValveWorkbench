#include <math.h>
#include <avr/pgmspace.h>
#include <Wire.h>   //Include the Wire library to talk I2C

#include "AnalyserValve.h"
#include "CommandParser.h"

/************************************************************
   GLOBAL VARIABLES
 ************************************************************/
int targetValues[ARRAY_LENGTH];   //Array to hold target values (array lenth must be less than 32 ints or 16 unsigned ints owing to I2C limit)
int measuredValues[ARRAY_LENGTH]; //Array to hold measured values (array lenth must be less than 32 ints or 16 unsigned ints owing to I2C limit)

int slaveCounter = 0;

#define AVG_FACTOR 0.99

bool averageHT = false;
int iaSamples = 3;

CommandParser parser(infoCommand, modeCommand, getCommand, setCommand, commandError);

enum {
  ERR_INVALID_MODE,
  ERR_INVALID_SET,
  ERR_INVALID_GET,
  ERR_GRID_RANGE,
  ERR_HT_RANGE,
  ERR_HT_TIMEOUT,
  ERR_UNSAFE
};

const char *errorMessages[] = {
  "Invalid mode command",
  "Invalid set command",
  "Invalid get command",
  "Grid voltage out of range",
  "HT voltage out of range",
  "Timeout setting HT voltage",
  "Unsafe to test"
};

//#define T1_COUNTER 65411;   // preload timer 65536-16MHz/256/500Hz
#define T1_COUNTER 64911;   // preload timer 65536-16MHz/256/100Hz
//#define T1_COUNTER 34286;   // preload timer 65536-16MHz/256/2Hz

/************************************************************
   SETUP
 ************************************************************/
void setup() {
  Serial.begin(115200); //Setup serial interface

  pinMode(DO_NOT_USE_PIN, INPUT);
  //I2C SDA is on Arduino Nano pin A4 as standard
  //I2C SCL is on Arduino Nano pin A5 as standard. These pins need no further setup.
  //By default, analog input pins also need no setup

  analogReference(EXTERNAL);                //Use external voltage reference for ADC
  //TCCR3B = (TCCR3B & 0b11111000) | 0x01;    //Configure Timers 3 & 4 for internal clock, no prescaling (bottom 3 bits of TCCRxB) for higher PWM frequency
 // TCCR4B = (TCCR4B & 0b11111000) | 0x01;

/* Set up timer 1 if we want to use interrupts for heater control
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = T1_COUNTER;       // preload timer
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
  interrupts();             // enable all interrupts
*/

  pinMode(LED_BUILTIN, OUTPUT);             //Arduino built-in LED for debugging

  for (int i = 0; i < ARRAY_LENGTH; i++) {
    targetValues[i] = 0;
  }

  digitalWrite(LED_BUILTIN, HIGH);

  pinMode(CHARGE1_PIN, OUTPUT);
  pinMode(DISCHARGE1_PIN, OUTPUT);
  pinMode(FIRE1_PIN, OUTPUT);
 
  pinMode(CHARGE2_PIN, OUTPUT);
  pinMode(DISCHARGE2_PIN, OUTPUT);
  pinMode(FIRE2_PIN, OUTPUT);

  digitalWrite(FIRE1_PIN, LOW);
  digitalWrite(FIRE2_PIN, LOW);
  digitalWrite(CHARGE1_PIN, LOW);
  digitalWrite(CHARGE2_PIN, LOW);

  //dischargeHighVoltages(1);
  //dischargeHighVoltages(2);

  Wire.begin(MASTER_ADDR); 
    
  setGridVolts();


  digitalWrite(LED_BUILTIN, LOW);
}

/************************************************************
   MAIN LOOP
 ************************************************************/
void loop() {
  while (Serial.available() > 0) {
    parser.parseInput(Serial.read());
  }

  //heaterVolts(); // Only if we're not using timer interrupts 

} //End of main program loop

//ISR(TIMER1_OVF_vect)        // interrupt service routine 
//{
//  TCNT1 = T1_COUNTER;       // preload timer
  //setHeaterVolts();
//}

// USB command interface functions

/************************************************************
   Callback for Info commands
 ************************************************************/
void infoCommand(int index) {
  int success = 1;

  switch (index) {
    case 0: // H/W Version info
      Serial.println("OK: Info(0) = Rev 3 (Nano)");
      break;
    case 1: // S/W Version info
      Serial.print("OK: Info(");
      Serial.print(index);
      Serial.println(") = 1.0");
      break;
    default:
      success = -ERR_INVALID_MODE;
      break;
  }

  if (success < 0) {
    Serial.print("ERR: ");
    Serial.print(errorMessages[-success]);
    Serial.print(" - ");
    printValues();
  }
}

/************************************************************
   Callback for Mode commands
 ************************************************************/
void modeCommand(int index) {
  int success = 1;

  switch (index) {
    case 0: // Safe mode
      dischargeHighVoltages(1);
      dischargeHighVoltages(2);
      for (int i = 0; i < ARRAY_LENGTH; i++) {
        targetValues[i] = 0;
      }
      setGridVolts();
      Serial.print("OK: Mode(");
      Serial.print(index);
      Serial.println(')');
      break;
    case 1: // Discharge high voltages
      dischargeHighVoltages(1);
      dischargeHighVoltages(2);
      Serial.print("OK: Mode(");
      Serial.print(index);
      Serial.print(") ");
      measureValues();
      printValues();
      break;
    case 2: // Run test
      success = runTest2();
      if (success > 0) {
        Serial.print("OK: Mode(");
        Serial.print(index);
        Serial.print(") ");
        printValues();
      }
      break;
    case 3: // Prepare HV (for debugging)
      success = chargeHighVoltages();
      if (success > 0) {
        measureValues();
        Serial.print("OK: Mode(");
        Serial.print(index);
        Serial.print(") ");
        printValues();
      }
      break;
    case 4: // Prepare HV and apply (for debugging)
      success = chargeHighVoltages();
      if (success > 0) {
        digitalWrite(FIRE1_PIN, HIGH);    //Apply high voltage to the DUT
        digitalWrite(FIRE2_PIN, HIGH);
        measureValues();
        Serial.print("OK: Mode(");
        Serial.print(index);
        Serial.print(") ");
        printValues();
      }
      break;
    case 5: // Return all measured values
      measureValues();
      Serial.print("OK: Mode(");
      Serial.print(index);
      Serial.print(") ");
      printValues();
      break;

    case 6: // Charge HV banks to current targets (no measurement)
      success = chargeHighVoltages();
      if (success > 0) {
        measureValues();              // capture final voltages/currents
        Serial.print("OK: Mode(");
        Serial.print(index);
        Serial.print(") ");
        printValues();
      }
      break;

    default:
      success = -ERR_INVALID_MODE;
      break;

       }
  if (success < 0) {
    Serial.print("ERR: ");
    Serial.print(errorMessages[-success]);
    Serial.print(" - ");
    printValues();
  }

}

void printValues() {
  for (int i = 0; i < ARRAY_LENGTH; i++) {
    Serial.print(measuredValues[i]);
    if (i < ARRAY_LENGTH - 1) {
      Serial.print(", ");
    } else {
      Serial.println("");
    }
  }  
}

/************************************************************
   Callback for Set commands
 ************************************************************/
void setCommand(int index, int intParam) {
  int success = 1;

  switch (index) {
    
    case VH: // Repurposed: current averaging samples (1..IA_SAMPLES)
      if (intParam < 1) {
        iaSamples = 1;
      } else if (intParam > IA_SAMPLES) {
        iaSamples = IA_SAMPLES;
      } else {
        iaSamples = intParam;
      }
      break;
    case VG1: // Grid 1 voltage
    case VG2: // Grid 2 voltage
      if (intParam < 0 || intParam > 4095) {
        success = -ERR_GRID_RANGE;
      }
      break;
    case HV1: // Anode 1 voltage
    case HV2: // Anode 2 voltage
      if (intParam < 0 || intParam > 1023) {
        success = -ERR_HT_RANGE;
      }
      break;
    case SET_AVERAGE_MODE:
      averageHT = intParam > 0;
      break;
    default:
      success = -ERR_INVALID_SET;
      break;
  }

  if (success > 0) {
    targetValues[index] = intParam;

    if (index < 2) {
      //sendToSlave();
    } else if (index == VG1 || index == VG2) {
      setGridVolts();
    }

    Serial.print("OK: Set(");
    Serial.print(index);
    Serial.print(") = ");
    Serial.println(intParam);
  } else {
    Serial.print("ERR: Set(");
    Serial.print(index);
    Serial.print(") = ");
    Serial.println(intParam);
    Serial.print(" ");
    Serial.println(errorMessages[-success]);
  }
}

/************************************************************
   Callback for Get commands
 ************************************************************/
void getCommand(int index) {
  measureValues();
  
  if (index >= 0 && index <= 9) {
    Serial.print("OK: Get(");
    Serial.print(index);
    Serial.print(") = ");
    Serial.println(measuredValues[index]);
  } else {
    Serial.print("ERR: ");
    Serial.println(errorMessages[ERR_INVALID_GET]);
  }
}

/************************************************************
   Callback for syntax erros
 ************************************************************/
void commandError(const char *command) {
  Serial.print("ERR: Unrecognised command - ");
  Serial.println(command);
}

// Testing functions

/************************************************************
   Runs a test
 ************************************************************/
int runTest() {
  int status;
  setGridVolts();
  status = chargeHighVoltages();
  if (status > 0) {
    doMeasurement();
  }

  return status;
}



/****************************************************************************
  Updates the bias DACs with target values
****************************************************************************/
void setGridVolts() {
  byte buf[3];

  Wire.beginTransmission(DAC1_ADDR);          // This DAC programmed in Fast mode
  buf[0] = targetValues[VG1] >> 8;
  buf[1] = targetValues[VG1] & 255;
  Wire.write(buf, 2);
  if (Wire.endTransmission() == 0) {          //If I2C tramission was a success
    measuredValues[VG1] = targetValues[VG1];  //Store the new grid voltage
  }

  Wire.beginTransmission(DAC2_ADDR);
  buf[0] = targetValues[VG2] >> 8;
  buf[1] = targetValues[VG2] & 255;
  Wire.write(buf, 2);
  if (Wire.endTransmission() == 0) {          //If I2C tramission was a success
    measuredValues[VG2] = targetValues[VG2];  //Store the new grid voltage
  }
}

/****************************************************************************
  Takes a measurement and puts the results in the measuredValues[] array
****************************************************************************/
void doMeasurement(void) {

  noInterrupts();                   //We don't want the measurement to be affected by servicing the heater

  digitalWrite(FIRE1_PIN, HIGH);    //Apply high voltage to the DUT
  digitalWrite(FIRE2_PIN, HIGH);

  measureValues();
  
  digitalWrite(FIRE1_PIN, LOW);      //Remove high voltage from the DUT
  digitalWrite(FIRE2_PIN, LOW);

  interrupts();
}

void measureValues() {
  measuredValues[HV1] = analogRead(VA1_PIN); // Extra read for delay on switch closure - probably superfluous here
  measuredValues[HV1] = analogRead(VA1_PIN);
  measuredValues[IA_HI_1] = analogRead(IA1_HI_PIN);
  measuredValues[IA_HI_1] = analogRead(IA1_HI_PIN);
  measuredValues[IA_LO_1] = analogRead(IA1_LO_PIN);
  measuredValues[IA_LO_1] = analogRead(IA1_LO_PIN);
//  measuredValues[IA_XHI_1] = analogRead(IA1_XHI_PIN);
 // measuredValues[IA_XHI_1] = analogRead(IA1_XHI_PIN);
  
  measuredValues[HV2] = analogRead(VA2_PIN);
  measuredValues[HV2] = analogRead(VA2_PIN);
  measuredValues[IA_HI_2] = analogRead(IA2_HI_PIN);
  measuredValues[IA_HI_2] = analogRead(IA2_HI_PIN);
  measuredValues[IA_LO_2] = analogRead(IA2_LO_PIN);
  measuredValues[IA_LO_2] = analogRead(IA2_LO_PIN);
 // measuredValues[IA_XHI_2] = analogRead(IA2_XHI_PIN);
 // measuredValues[IA_XHI_2] = analogRead(IA2_XHI_PIN);
}

/****************************************************************************
  Discharges the capacitor banks
****************************************************************************/
void dischargeHighVoltages(int bank) {
  const int dischargeThreshold = 3;          // ADC counts considered "zero"
  const unsigned long dischargeTimeoutMs = 2000; // fail-safe timeout

  if (bank == 1) {
    digitalWrite(FIRE1_PIN, LOW);
    //analogWrite(CHARGE1_PIN, 0); // If we're in PWM mode then set the duty cycle for the charge pins to 0
    digitalWrite(CHARGE1_PIN,LOW);
    digitalWrite(DISCHARGE1_PIN, HIGH);
    //analogWrite(DISCHARGE1_PIN, 128); //to be kind to the discharge resistor!
    measuredValues[IA_HI_1] = analogRead(IA1_HI_PIN); // Needs some delay for the switch to close
    measuredValues[IA_HI_1] = analogRead(IA1_HI_PIN);
    measuredValues[IA_LO_1] = analogRead(IA1_LO_PIN);
    measuredValues[IA_LO_1] = analogRead(IA1_LO_PIN);
    unsigned long startMs = millis();
    while (analogRead(IA1_HI_PIN) > dischargeThreshold) { // wait until current effectively zero
      if (millis() - startMs > dischargeTimeoutMs) {
        Serial.println(F("WARN: dischargeHighVoltages(1) timeout"));
        break;
      }
    }
    digitalWrite(DISCHARGE1_PIN,LOW);
    //analogWrite(DISCHARGE1_PIN, 0);
  } else if (bank == 2) {
    digitalWrite(FIRE2_PIN, LOW);
    //analogWrite(CHARGE2_PIN, 0);
    digitalWrite(CHARGE2_PIN,LOW); 
    digitalWrite(DISCHARGE2_PIN, HIGH);
    //analogWrite(DISCHARGE2_PIN, 128); //to be kind to the discharge resistor!
    measuredValues[IA_HI_2] = analogRead(IA2_HI_PIN);
    measuredValues[IA_HI_2] = analogRead(IA2_HI_PIN);
    measuredValues[IA_LO_1] = analogRead(IA1_LO_PIN);
    measuredValues[IA_LO_1] = analogRead(IA1_LO_PIN);
    unsigned long startMs = millis();
    while (analogRead(IA2_HI_PIN) > dischargeThreshold) { // wait until current effectively zero
      if (millis() - startMs > dischargeTimeoutMs) {
        Serial.println(F("WARN: dischargeHighVoltages(2) timeout"));
        break;
      }
    }
    digitalWrite(DISCHARGE2_PIN,LOW);
    //analogWrite(DISCHARGE2_PIN, 0);
  }
}

int sgn(int value) {
  if (value == 0) {
    return 0;
  }

  if (value > 0) {
    return 1;
  }

  if (value < 0) {
    return -1;
  }
}

/****************************************************************************
  Charges up the high-voltage capacitor banks to the target values
****************************************************************************/
int chargeHighVoltages() {
  digitalWrite(FIRE1_PIN, LOW);
  digitalWrite(FIRE2_PIN, LOW);
  digitalWrite(CHARGE1_PIN, LOW);
  digitalWrite(CHARGE2_PIN, LOW);
  digitalWrite(DISCHARGE1_PIN, LOW);
  digitalWrite(DISCHARGE2_PIN, LOW);

  // Measure initial voltages
  measuredValues[HV1] = analogRead(VA1_PIN);
  measuredValues[HV1] = analogRead(VA1_PIN);
  measuredValues[HV2] = analogRead(VA2_PIN);
  measuredValues[HV2] = analogRead(VA2_PIN);

  // Keep charging until both are at target (with rechecking)
  do {
    // Charge HV1 if needed
    if (measuredValues[HV1] < targetValues[HV1]) {
      while (measuredValues[HV1] < targetValues[HV1]) {
        digitalWrite(CHARGE1_PIN, HIGH);
        measuredValues[HV1] = analogRead(VA1_PIN);
      }
      digitalWrite(CHARGE1_PIN, LOW);
    }

    // Charge HV2 if needed
    if (measuredValues[HV2] < targetValues[HV2]) {
      while (measuredValues[HV2] < targetValues[HV2]) {
        digitalWrite(CHARGE2_PIN, HIGH);
        measuredValues[HV2] = analogRead(VA2_PIN);
      }
      digitalWrite(CHARGE2_PIN, LOW);
    }

    // Re-measure both voltages (they may have discharged)
    measuredValues[HV1] = analogRead(VA1_PIN);
    measuredValues[HV2] = analogRead(VA2_PIN);
  } while (measuredValues[HV1] < targetValues[HV1] || measuredValues[HV2] < targetValues[HV2]);

  return 1;  // Success
}



/************************************************************
   Runs a test (inside a single routine to minimise delays)
 ************************************************************/
int runTest2() {
  setGridVolts();

  digitalWrite(FIRE1_PIN, LOW);
  digitalWrite(FIRE2_PIN, LOW);
  digitalWrite(CHARGE1_PIN, LOW);
  digitalWrite(CHARGE2_PIN, LOW);
  digitalWrite(DISCHARGE1_PIN, LOW);
  digitalWrite(DISCHARGE2_PIN, LOW);

  int hv1_temp;
  int hv2_temp;
  int hv1a_temp;
  int hv2a_temp;

  hv1_temp = analogRead(VA1_PIN);
  hv1_temp = analogRead(VA1_PIN);
  hv2_temp = analogRead(VA2_PIN);
  hv2_temp = analogRead(VA2_PIN);

  do {
    if (hv1_temp < targetValues[HV1]) {
      while (hv1_temp < targetValues[HV1]) {
        digitalWrite(CHARGE1_PIN, HIGH);
        hv1_temp = analogRead(VA1_PIN);
      }
      digitalWrite(CHARGE1_PIN, LOW);
    }

    if (hv2_temp < targetValues[HV2]) {
      while (hv2_temp < targetValues[HV2]) {
        digitalWrite(CHARGE2_PIN, HIGH);
        hv2_temp = analogRead(VA2_PIN);
      }
      digitalWrite(CHARGE2_PIN, LOW);
    }

    hv1_temp = analogRead(VA1_PIN);
    hv2_temp = analogRead(VA2_PIN);
  } while (hv1_temp < targetValues[HV1] || hv2_temp < targetValues[HV2]);

  hv1_temp = analogRead(VA1_PIN);
  hv1_temp = analogRead(VA1_PIN);
  hv2_temp = analogRead(VA2_PIN);
  hv2_temp = analogRead(VA2_PIN);

  digitalWrite(CHARGE1_PIN, LOW);
  digitalWrite(DISCHARGE1_PIN, LOW);
  digitalWrite(CHARGE2_PIN, LOW);
  digitalWrite(DISCHARGE2_PIN, LOW);

  digitalWrite(FIRE1_PIN, HIGH);
  digitalWrite(FIRE2_PIN, HIGH);

  auto sampleCurrents = [&](int hiPin, int loPin, int *hiOut, int *loOut, int *retriesOut) -> bool {
    int hiSamples[IA_SAMPLES];
    int loSamples[IA_SAMPLES];

    for (int attempt = 0; attempt < IA_RETRY_LIMIT; ++attempt) {
      for (int i = 0; i < iaSamples && i < IA_SAMPLES; ++i) {
        hiSamples[i] = analogRead(hiPin);
        hiSamples[i] = analogRead(hiPin);
        
        loSamples[i] = analogRead(loPin);
        loSamples[i] = analogRead(loPin);
      }

      bool ok = true;
      for (int i = 0; i < iaSamples && i < IA_SAMPLES && ok; ++i) {
        for (int j = i + 1; j < iaSamples && j < IA_SAMPLES; ++j) {
          if (abs(hiSamples[i] - hiSamples[j]) > IA_ACCURACY ||
              abs(loSamples[i] - loSamples[j]) > IA_ACCURACY) {
            ok = false;
            break;
          }
        }
      }

      if (ok) {
        long hiSum = 0;
        long loSum = 0;
        for (int i = 0; i < iaSamples && i < IA_SAMPLES; ++i) {
          hiSum += hiSamples[i];
          loSum += loSamples[i];
        }
        *hiOut = (int)(hiSum / iaSamples);
        *loOut = (int)(loSum / iaSamples);
        if (retriesOut) {
          *retriesOut = attempt;
        }
        return true;
      }
    }
    long hiSum = 0;
    long loSum = 0;
    for (int i = 0; i < iaSamples && i < IA_SAMPLES; ++i) {
      hiSum += hiSamples[i];
      loSum += loSamples[i];
    }
    *hiOut = (int)(hiSum / iaSamples);
    *loOut = (int)(loSum / iaSamples);
    if (retriesOut) {
      *retriesOut = IA_RETRY_LIMIT;
    }
    return false;
  };

  int retries1 = 0;
  int retries2 = 0;

  sampleCurrents(IA1_HI_PIN, IA1_LO_PIN, &measuredValues[IA_HI_1], &measuredValues[IA_LO_1], &retries1);
  sampleCurrents(IA2_HI_PIN, IA2_LO_PIN, &measuredValues[IA_HI_2], &measuredValues[IA_LO_2], &retries2);

  int worstRetries = retries1;
  if (retries2 > worstRetries) {
    worstRetries = retries2;
  }

  measuredValues[VH] = iaSamples;
  measuredValues[IH] = worstRetries;

  hv1a_temp = analogRead(VA1_PIN);
  hv1a_temp = analogRead(VA1_PIN);
  hv2a_temp = analogRead(VA2_PIN);
  hv2a_temp = analogRead(VA2_PIN);

  digitalWrite(FIRE1_PIN, LOW);
  digitalWrite(FIRE2_PIN, LOW);

  if (averageHT) {
    measuredValues[HV1] = (hv1_temp + hv1a_temp) / 2;
    measuredValues[HV2] = (hv2_temp + hv2a_temp) / 2;
  } else {
    measuredValues[HV1] = hv1_temp;
    measuredValues[HV2] = hv2_temp;
  }

  return 1;
}
