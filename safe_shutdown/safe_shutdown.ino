#include <DigiKeyboard.h>
#include <TinyWireS.h>
#define I2C_SLAVE_ADDRESS 0x4
#define MAX_TASKS 2
#define MAX_BATTERY_READS 4
#define CHECK_STATE_INTERVAL 500
#define CHECK_STATE_DELAY 5000
#define BATTERY_READ_INTERVAL 1000
#define BATTERY_READ_DELAY 1000

typedef void(*TaskFunction)(); // Function pointer

/* PIN DESIGNATIONS
** INTERNAL **
* pinBattery:           PA1
* pinSwitch:
* pinKeepAlive:         PB3

** RASPBERRY PI GPIO **
* Keep-Alive from Pi:   PA3
* Shutdown to Pi:       PA7
* I²C SDA:              PB0
* I²C SCL:              PB2

** ISP **
* MOSI:                 PA4
* MISO:                 PA2
* SCK:                  PA5
*/

int pinBattery = PA1;
int pinSwitch = PB7;
int pinKeepAlive = PB3;
int gpioKeepAlive = PA3;
int gpioShutdown = PA7;
int pinSDA = PB0;
int pinSCL = PB3;

int pinMOSI = PA4;
int pinMISO = PA2;
int pinSCK = PA5;

// ----- BATTERY -----

uint8_t vIndex = 0;
int voltages[MAX_BATTERY_READS] = { 1 };
int vTotal = MAX_BATTERY_READS;

// ----- STATE -----
typedef enum state {BOOTUP, RUNNING, SHUTDOWN} State;

typedef struct {
  State current_state;
  int average_voltage;
} SystemState;

SystemState system_state;

// ----- TASKS -----
typedef struct {
  TaskFunction func;
  int count;
  int max_count;
  uint16_t interval_millis;
  uint64_t previous_millis;
} Task;

Task all_tasks[MAX_TASKS];
volatile uint8_t num_tasks = 0;

int createTask(TaskFunction function, int interval, int delay, int repeat) {
  if (num_tasks == MAX_TASKS) { // Too many tasks?
    // Find one which is complete & overwrite it
    for (int i = 0; i < num_tasks; i++) {
      if (all_tasks[i].count >= all_tasks[i].max_count) {
        all_tasks[i].func = function;
        all_tasks[i].max_count = repeat;
        all_tasks[i].count = 0;
        all_tasks[i].interval_millis = interval;
        all_tasks[i].previous_millis = millis() - interval + delay;
        return 1; // Success
      }
    }
    return 0; // Failure
  }
  else {
    // Or add a new task
    all_tasks[num_tasks].func = function;
    all_tasks[num_tasks].max_count = repeat;
    all_tasks[num_tasks].count = 0;
    all_tasks[num_tasks].interval_millis = interval;
    all_tasks[num_tasks].previous_millis = millis() - interval + delay;
  }
  num_tasks += 1;
  return 1; // Success
}

void executeTasks() {
  if (num_tasks == 0) { return; }
  for (int i = 0; i <= num_tasks; i++) {
    // Execute infinite tasks and those whose max_count has not been reached
    if ((all_tasks[i].max_count == -1) || (all_tasks[i].count < all_tasks[i].max_count)) {
      if (all_tasks[i].previous_millis + all_tasks[i].interval_millis <= millis()) {
        // Reset the elapsed time
        all_tasks[i].previous_millis = millis();
        // Don't count infinite tasks
        if (all_tasks[i].max_count > -1) { all_tasks[i].count += 1; }
        // Run the task
        all_tasks[i].func();
      }
    }
  }
}

// ----- FUNCTIONS -----

/* Reads the pin voltage and stores
 * the average of thelast 5 reads in
 * SystemState.battery_voltage
 */
void readBatteryVoltage() {
  //DigiKeyboard.println("Reading battery...");
  // Increment the voltages[] index
  vIndex++;
  if (vIndex >= MAX_BATTERY_READS) {
    vIndex = 0;
  }
  // Subtract the oldest value
  vTotal -= voltages[vIndex];

  // Store the latest value
  voltages[vIndex] = analogRead(pinBattery);
  vTotal += voltages[vIndex];

  // Some debugging output. This can be removed from the final sketch.
  /*
  float v = voltages[vIndex] * (5.00 / 1023.00);
  char str_v[8];
  char str_a[8];
  dtostrf(v, 4, 2, str_v);
  sprintf(str_a, "%d", voltages[vIndex]);
  char buffer[24] = "Battery: ";
  strcat(buffer, str_v);
  strcat(buffer, "v (");
  strcat(buffer, str_a);
  strcat(buffer, ")");
  DigiKeyboard.println(buffer);
  */
  DigiKeyboard.println(vTotal);
}

/* Checks the state of the power switch
 */
void checkState() {
  //DigiKeyboard.println("Checking switch...");

  int switch_state = digitalRead(pinSwitch);
  if (switch_state == 1) { // subject to change (inverse)
    system_state.current_state = SHUTDOWN;
  }
  else {
    system_state.current_state = RUNNING;
  }
}

// ----- I2C -----
/* Writes the SystemState struct to the I2C bus
 */

void tws_requestEvent() {
  // Copy the system_state struct into a byte array
  void* p = &system_state;
  uint8_t buffer[sizeof(SystemState)];
  memcpy(buffer, p, sizeof(SystemState));

  // Write buffer to I2C
  for (int i = 0; i < sizeof(buffer); i++) {
    TinyWireS.send(buffer[i]);
  }
}

/* Used to take instructions from the I2C master python script
 * eg. change polling frequency of battery Reads
 * eg. enable/disable power switch
 */
void tws_receiveEvent(uint8_t howMany) {
  while(TinyWireS.available()) {
    int data = TinyWireS.receive();
  }
}

// ----- START -----
void setup() {
  DigiKeyboard.println("Running...");
  system_state.current_state = BOOTUP;

  // Initialise the pins
  pinMode(pinBattery, INPUT);
  pinMode(pinSwitch, INPUT);
  analogReference(DEFAULT);
  //pinMode(pinKeepAlive, OUTPUT);
  //digitalWrite(pinKeepAlive, HIGH);
  DigiKeyboard.println("Pins OK");

  // Create some tasks
  int task_result = createTask(readBatteryVoltage, BATTERY_READ_INTERVAL, BATTERY_READ_DELAY, -1);
  int task_result2 = createTask(checkState, CHECK_STATE_INTERVAL, CHECK_STATE_DELAY, -1);

  if (task_result + task_result2 == 2) {
    DigiKeyboard.println("Tasks OK");
  }
  else {
    DigiKeyboard.println("Problem creating one or more tasks!");
  }

  // Setup the I2C bus
  TinyWireS.begin(I2C_SLAVE_ADDRESS);
  TinyWireS.onReceive(tws_receiveEvent);
  TinyWireS.onRequest(tws_requestEvent);
  DigiKeyboard.println("I2C OK");
}

void loop() {
  executeTasks();
  TinyWireS_stop_check();
}
