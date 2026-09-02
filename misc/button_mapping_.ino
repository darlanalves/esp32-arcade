// Controller Mapping & Testing Firmware
// Reads all inputs from connected controllers and prints them over serial
// Use this to map buttons and joysticks before integrating into games

#include <Bluepad32.h>

// Bluepad32 Global Controller Pointer
ControllerPtr P1;

// Handle freshly paired or connected gamepads
void onConnectedController(ControllerPtr ctl) {
    P1 = ctl;
    Serial.printf("\n\n=== CONTROLLER CONNECTED ===\n");
    Serial.println("Testing all buttons and axes. Press buttons and move sticks...\n");

    // Ready to print raw inputs
}

// Handle disconnected gamepads
void onDisconnectedController(ControllerPtr ctl) {
  P1 = nullptr;
  Serial.printf("\n=== CONTROLLER DISCONNECTED ===\n\n");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n\n========================================");
  Serial.println("    ESP32 ARCADE - CONTROLLER MAPPER    ");
  Serial.println("========================================");
  Serial.println("\nWaiting for controller connections...");
  Serial.println("Press buttons and move joysticks to test.\n");

  // Setup Bluepad32
  BP32.setup(&onConnectedController, &onDisconnectedController);
  // BP32.forgetBluetoothKeys();
}

void loop() {
  BP32.update();

  if (P1 && P1->isConnected()) {
    processController();
    delay(200);  // Slow down output for readability
  }
}

void processController() {
  uint32_t buttons = P1->buttons();
  uint16_t misc = P1->miscButtons();
  uint8_t dpad = P1->dpad();
  uint8_t battery = P1->battery();

  if (buttons || misc || dpad) {
    Serial.printf("[P1] BUTTONS=0x%08X MISC=0x%04X DPAD=0x%04X BAT=0x%d\n", buttons, misc, dpad, battery);
    printButtonNames(dpad);
    Serial.println();
    P1->setRumble(255, 100);  // Rumble for 100ms
  }

  // Get analog sticks and print when outside dead zone
  int16_t axisLX = P1->axisX();   // Left stick X (-512 to 511)
  int16_t axisLY = P1->axisY();   // Left stick Y (-512 to 511)
  int16_t axisRX = P1->axisRX();  // Right stick X (-512 to 511)
  int16_t axisRY = P1->axisRY();  // Right stick Y (-512 to 511)
  const int DEAD_ZONE = 80;

  if (abs(axisLX) > DEAD_ZONE || abs(axisLY) > DEAD_ZONE) {
    Serial.printf("[CTL] LEFT STICK:  X=%6d  Y=%6d\n", axisLX, axisLY);
  }

  if (abs(axisRX) > DEAD_ZONE || abs(axisRY) > DEAD_ZONE) {
    Serial.printf("[CTL] RIGHT STICK: X=%6d  Y=%6d\n", axisRX, axisRY);
  }
}

void printButtonNames(uint32_t dpad) {
  if (P1->a()) {
    Serial.print("A");
  }

  if (P1->b()) {
    Serial.print("B");
  }

  if (P1->x()) {
    Serial.print("X");
  }

  if (P1->y()) {
    Serial.print("Y");
  }

  if (P1->l1()) {
    Serial.print("L1");
  }

  if (P1->l2()) {
    Serial.print("L2");

  }
  if (P1->r1()) {
    Serial.print("R1");

  }
  if (P1->r2()) {
    Serial.print("R2");
  }

  if (P1->thumbL()) {
    Serial.print("L3");
  }

  if (P1->thumbR()) {
    Serial.print("R3");
  }

  if (P1->miscSystem()) {
    Serial.print("HOME/SYSTEM");
  }
  if (P1->miscBack()) {
    Serial.print("SELECT/SHARE");
  }

  if (P1->miscHome()) {
    Serial.print("START/OPTIONS");
  }
  if (dpad & DPAD_UP) {
    Serial.print("D-PAD UP");
  }

  if (dpad & DPAD_DOWN) {
    Serial.print("D-PAD DOWN");
  }

  if (dpad & DPAD_LEFT) {
    Serial.print("D-PAD LEFT");
  }

  if (dpad & DPAD_RIGHT) {
    Serial.print("D-PAD RIGHT");
  }
}

