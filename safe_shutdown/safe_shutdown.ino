#include <DigiKeyboard.h>
//#include <TinyWireS.h> commented this out, as there is an issue compiling for the ATTiny167 right now
#define I2C_SLAVE_ADDRESS 0x4
#define MAX_TASKS 8
#define CHECK_STATE_INTERVAL 500
#define CHECK_STATE_DELAY 5000
#define BATTERY_READ_INTERVAL 1000
#define BATTERY_READ_DELAY 0

typedef void(*TaskFunction)(); // Function pointer

int ADCPin = A5;
int SwitchPin = PB3;
int AlivePin = PB4;

uint8_t vIndex = 1;
int voltages[5] = { 0 };

// ----- STATE -----
typedef enum state {BOOTUP, RUNNING, SHUTDOWN} State;

typedef struct {
  State current_state;
  int battery_voltage;
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

void ExecuteTasks() {
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
  // Read the voltage
  voltages[vIndex] = analogRead(ADCPin);
  float av = voltages[vIndex] * (5.00 / 1023.00);
  char buffer[16];
  sprintf(buffer, "Battery: %d", av); // this doesn't work yet
  DigiKeyboard.println(buffer);

  vIndex++;
  if (vIndex > 4) {
    vIndex = 0;
  }
  // Total the values
  int sum = 0;
  for (int i = 1; i < 5; i++) {
    sum += voltages[i];
  }
  // Re-calculate the average
  system_state.battery_voltage = sum / 5;
}

/* Checks the state of the power switch
 */
void checkState() {
  DigiKeyboard.println("Checking switch...");

  int switch_state = digitalRead(SwitchPin);
  if (switch_state == 1) { // subject to change (inverse)
    system_state.current_state = SHUTDOWN;
  }
  else {
    system_state.current_state = RUNNING;
  }
}

// ----- I2C -----
/* Writes the SystemState struct to the I2C bus */
/*
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
*/
/* Used to take instructions from the I2C master python script
 * eg. change polling frequency of battery Reads
 * eg. enable/disable power switch
 */
 /*
void tws_receiveEvent(uint8_t howMany) {
  while(TinyWireS.available()) {
    int data = TinyWireS.receive();
  }
}
*/
// ----- START -----
void setup() {
  DigiKeyboard.println("Running...");
  system_state.current_state = BOOTUP;

  // Initialise the pins
  pinMode(ADCPin, INPUT);
  pinMode(SwitchPin, INPUT);
  analogReference(DEFAULT);
  //pinMode(AlivePin, OUTPUT);
  //digitalWrite(AlivePin, HIGH);
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
  /*
  TinyWireS.begin(I2C_SLAVE_ADDRESS);
  TinyWireS.onReceive(tws_receiveEvent);
  TinyWireS.onRequest(tws_requestEvent);
  DigiKeyboard.println("I2C OK");
*/
}

void loop() {
  ExecuteTasks();
  //TinyWireS_stop_check();
}
