#include <Wire.h>
#include <SparkFun_IS31FL3737.h>

// --- PIN DEFINITIONS ---
const int SDB_PIN = 8;      // Hardware Shutdown Pin (IO8)
const int I2C_SCL_PIN = 9;  // I2C Clock Pin (IO9)
const int I2C_SDA_PIN = 10; // I2C Data Pin (IO10)

// Potentiometer pins 
const int POT_PLAYER_1_PIN = 0; 
const int POT_PLAYER_2_PIN = 1; 

// --- GAME VARIABLES ---
const int MATRIX_SIZE = 12;
const int PADDLE_HEIGHT = 3;
const int MAX_BRIGHTNESS = 255;

// Paddle Y positions (0 to 11)
int paddle1Y = 0; 
int paddle2Y = 0; 

// Ball physics
float ballX = 6.0;
float ballY = 6.0;
float ballVelX = 0.6; // X Speed and direction
float ballVelY = 0.4; // Y Speed and direction

// Initialize your matrix object
IS31FL3737 matrix; 

void setup() {
  Serial.begin(115200);
  
  // 1. WAKE UP THE MATRIX CHIP
  pinMode(SDB_PIN, OUTPUT);
  digitalWrite(SDB_PIN, HIGH); 
  
  // Give the chip a tiny moment to boot up after receiving power
  delay(10); 
  
  // 2. Set up Potentiometer pins
  pinMode(POT_PLAYER_1_PIN, INPUT);
  pinMode(POT_PLAYER_2_PIN, INPUT);
  
  // 3. Initialize I2C for the ESP32-C3
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  
  // 4. Initialize the matrix
  if (!matrix.begin()) {
    Serial.println("IS31FL3737 not found! Check your I2C wiring.");
    while (1); 
  }
  
  matrix.init();
  matrix.clear();
}

void loop() {
  updatePaddles();
  updateBall();
  drawGame();
  
  delay(50); // Game speed control (adjust to make the game faster/slower)
}

void updatePaddles() {
  // ESP32-C3 ADC reads from 0 to 4095. 
  // Map this to the 12x12 matrix, offsetting by paddle height so it stays on screen.
  int rawPot1 = analogRead(POT_PLAYER_1_PIN);
  int rawPot2 = analogRead(POT_PLAYER_2_PIN);
  
  paddle1Y = map(rawPot1, 0, 4095, 0, MATRIX_SIZE - PADDLE_HEIGHT);
  paddle2Y = map(rawPot2, 0, 4095, 0, MATRIX_SIZE - PADDLE_HEIGHT);
}

void updateBall() {
  // Move ball
  ballX += ballVelX;
  ballY += ballVelY;
  
  // Top and Bottom Wall Collisions
  if (ballY <= 0 || ballY >= MATRIX_SIZE - 1) {
    ballVelY = -ballVelY; // Bounce vertically
    
    // Clamp to prevent the ball from getting stuck in the wall
    if (ballY <= 0) ballY = 0.1;
    if (ballY >= MATRIX_SIZE - 1) ballY = MATRIX_SIZE - 1.1; 
  }
  
  // Player 1 (Left) Paddle Collision
  if (ballX <= 1.0) {
    if (ballY >= paddle1Y && ballY <= paddle1Y + PADDLE_HEIGHT) {
      ballVelX = -ballVelX; // Bounce horizontally
      ballX = 1.1; // Prevent getting stuck inside paddle
    } else if (ballX < 0) {
      scoreGoal(); // Ball passed paddle
    }
  }
  
  // Player 2 (Right) Paddle Collision
  if (ballX >= MATRIX_SIZE - 2) {
    if (ballY >= paddle2Y && ballY <= paddle2Y + PADDLE_HEIGHT) {
      ballVelX = -ballVelX;
      ballX = MATRIX_SIZE - 2.1;
    } else if (ballX >= MATRIX_SIZE - 1) {
      scoreGoal();
    }
  }
}

void scoreGoal() {
  // Reset ball to center
  ballX = MATRIX_SIZE / 2;
  ballY = MATRIX_SIZE / 2;
  
  // Send it in the opposite direction for the next round
  ballVelX = -ballVelX; 
}

void drawGame() {
  // 1. Clear the previous frame
  matrix.clear();
  
  // 2. Draw Player 1 Paddle (Left column: x = 0)
  for (int i = 0; i < PADDLE_HEIGHT; i++) {
    // setLEDPWM takes (CS, SW, Brightness)
    setPixel(0, paddle1Y + i, MAX_BRIGHTNESS);
  }
  
  // 3. Draw Player 2 Paddle (Right column: x = 11)
  for (int i = 0; i < PADDLE_HEIGHT; i++) {
    setPixel(MATRIX_SIZE - 1, paddle2Y + i, MAX_BRIGHTNESS);
  }
  
  // 4. Draw Ball
  setPixel((int)ballX, (int)ballY, MAX_BRIGHTNESS);
}

// --- HELPER FUNCTION FOR MATRIX MAPPING ---
void setPixel(int x, int y, int brightness) {
  // Prevent drawing outside the 12x12 grid
  if (x < 0 || x >= MATRIX_SIZE || y < 0 || y >= MATRIX_SIZE) return;
  
  // The SparkFun library uses CS (Current Sink) and SW (Switch) to address LEDs.
  // Depending on how you routed the rows and columns on your PCB, 
  // you may need to swap 'x' and 'y' below to rotate the screen 90 degrees.
  matrix.setLEDPWM(x, y, brightness); 
}
