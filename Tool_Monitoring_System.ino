// Monitor temperature from temperature sensor
// Monitor voltage from voltage sensor
// Monitor current from current sensor
// Handle the updating of the LCD screen

#define POT_PIN 36
#define ZERO_CROSS_PIN 23
#define SCR_GATE_PIN 26
#define PUSH_BUTTON 21
#define IR_SENSOR_PIN 27
#define VOLTAGE_SENSOR 37
#define PULSES_PER_REV 1

#define POT_HIGH 4095 // due 12 bit ADC resolution of esp32
#define AC_HALF_CYCLE_PERIOD 10000 // in microseconds

int firingDelay = 0; // in microseconds
bool zeroCrossDetected = false;
bool iotControl = false;
volatile int pulseCount = 0;
int rpm = 0;

portMUX_TYPE taskMux = portMUX_INITIALIZER_UNLOCKED; // critical section mutex

// Interrupt Service Routine for Zero Crossing Detection
void IRAM_ATTR handleZeroCross() {
  zeroCrossDetected = true;
}

// Handle toggling action for the pushbuttons
// Interrupt Service Routine for Pushbutton toggle
void IRAM_ATTR handleButton() {
  iotControl = !iotControl;
}

void IRAM_ATTR handlePulse() {
  pulseCount++;
}

// Take potentiometer readings if control is manual
void readPotTask(void *pvParameters) {
  int potValue;

  while (1) {
    if (!iotControl) {
      potValue = analogRead(POT_PIN);

      // Map the potentiometer readings to timing for the firing delay
      firingDelay = map(potValue, 0, POT_HIGH, 0, AC_HALF_CYCLE_PERIOD);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// Turn on and off the SCR based on the signal from the ZCDC and potentiometer readings
void triggerSCRTask(void *pvParameters) {
  while (1) {
    if (zeroCrossDetected) {
      zeroCrossDetected = false;
      delayMicroseconds(firingDelay);
      digitalWrite(SCR_GATE_PIN, HIGH);
      delayMicroseconds(100);
      digitalWrite(SCR_GATE_PIN, LOW);
    }
  }
}

// Use IR sensor as a tachometer
void rpmTask(void *pvParameters) {
  int tempCount = 0;

  while (1)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    portENTER_CRITICAL(&taskMux);
    tempCount = pulseCount;
    pulseCount = 0;
    portEXIT_CRITICAL(&taskMux);

    rpm = (tempCount / PULSES_PER_REV) * 60;
    Serial.print("RPM: ");
    Serial.println(rpm);
  }
}

void voltageTask(void *pvParameters) {

}

void setup() {

  pinMode(POT_PIN, INPUT);
  pinMode(ZERO_CROSS_PIN, INPUT_PULLUP);
  pinMode(SCR_GATE_PIN, OUTPUT);
  pinMode(PUSH_BUTTON, INPUT_PULLUP);
  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(VOLTAGE_SENSOR, INPUT);

  // Inteprete signals from the Zero-Crossing Detection Circuit (ZCDC)
  attachInterrupt(digitalPinToInterrupt(ZERO_CROSS_PIN), handleZeroCross, FALLING);
  attachInterrupt(digitalPinToInterrupt(PUSH_BUTTON), handleButton, LOW);
  attachInterrupt(digitalPinToInterrupt(IR_SENSOR_PIN), handlePulse, FALLING);

  xTaskCreatePinnedToCore(triggerSCRTask, "triggerSCRTask", 2048, NULL, 2, NULL, 1);
  xTaskCreate(readPotTask, "readPotTask", 2048, NULL, 1, NULL);
  xTaskCreatePinnedToCore(rpmTask, "rpmTask", 2048, NULL, 1, NULL, 1);
  xTaskCreate(voltageTask, "voltageTask", 2048, NULL, 1, NULL);
}

void loop() {
  // I'll just leave this nigga empty :)
}
