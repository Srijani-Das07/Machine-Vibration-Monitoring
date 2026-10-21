/*
 * Vibration monitor firmware -- Arduino Uno, IR speed-fluctuation mode.
 *
 * Drives the DC motor through the L298N and times the interval between
 * consecutive IR speed-sensor pulses using a hardware interrupt. Each
 * interval is converted to an instantaneous speed (rpm). After
 * WINDOW_PULSES intervals, the mean-removed speed sequence is sent as
 * one CSV line over Serial:
 *     <rpm>,<pulse_rate_hz>,<dev_1>,<dev_2>,...,<dev_N>
 *
 * The sequence is sampled once per pulse, so pulse_rate_hz is the
 * effective sample rate of the signal. Imbalance and other faults
 * modulate shaft speed, which shows up as variation in this sequence.
 *
 * Wiring:
 *   IR sensor: VCC -> 5V rail, GND -> GND rail, OUT -> D2
 *   L298N: ENA -> D10 (PWM), IN1 -> D8, IN2 -> D9
 *   Motor on OUT1/OUT2; separate motor supply on the 12V terminal,
 *   with a common ground shared with the Uno.
 *
 * Matches the packet format expected by serving/serial_reader.py
 * (WINDOW_PULSES there must match the value below).
 */

const byte IR_SENSOR_PIN = 2;  // must support external interrupts
const byte MOTOR_ENA_PIN = 10;  // PWM capable
const byte MOTOR_IN1_PIN = 8;
const byte MOTOR_IN2_PIN = 9;
const byte MOTOR_PWM_DUTY = 180;  // 0-255, motor speed

const byte WINDOW_PULSES = 64;
const float PULSES_PER_REV = 1.0;  // slots on the speed disc
const unsigned long MIN_PERIOD_US = 2000;  // ignores pulses closer than 2 ms (noise)

volatile unsigned long periods[WINDOW_PULSES];  // microseconds between pulses
volatile byte periodCount = 0;
volatile unsigned long lastPulseUs = 0;
volatile bool haveReference = false;

void onPulse() {
  unsigned long now = micros();
  unsigned long p = now - lastPulseUs;

  if (haveReference && p < MIN_PERIOD_US) {
    return;
  }
  lastPulseUs = now;

  if (!haveReference) {
    haveReference = true;  // first pulse only sets the time reference
    return;
  }
  if (periodCount < WINDOW_PULSES) {
    periods[periodCount++] = p;
  }
}

void startMotor() {
  pinMode(MOTOR_ENA_PIN, OUTPUT);
  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);
  digitalWrite(MOTOR_IN1_PIN, HIGH);
  digitalWrite(MOTOR_IN2_PIN, LOW);
  analogWrite(MOTOR_ENA_PIN, MOTOR_PWM_DUTY);
}

void sendWindow() {
  unsigned long totalUs = 0;
  float speedSum = 0;

  for (byte i = 0; i < WINDOW_PULSES; i++) {
    totalUs += periods[i];
    speedSum += 60000000.0 / ((float)periods[i] * PULSES_PER_REV);
  }

  float meanPeriodUs = (float)totalUs / WINDOW_PULSES;
  float pulseRateHz = 1000000.0 / meanPeriodUs;
  float rpm = pulseRateHz * 60.0 / PULSES_PER_REV;
  float meanSpeed = speedSum / WINDOW_PULSES;

  Serial.print(rpm, 2);
  Serial.print(',');
  Serial.print(pulseRateHz, 3);
  for (byte i = 0; i < WINDOW_PULSES; i++) {
    float speed = 60000000.0 / ((float)periods[i] * PULSES_PER_REV);
    Serial.print(',');
    Serial.print(speed - meanSpeed, 2);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(IR_SENSOR_PIN), onPulse, FALLING);

  startMotor();
}

void loop() {
  if (periodCount >= WINDOW_PULSES) {
    sendWindow();

    noInterrupts();
    periodCount = 0;
    haveReference = false;  // the gap spanning the transmission is discarded
    interrupts();
  }
}