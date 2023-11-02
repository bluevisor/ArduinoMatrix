#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S_kbv.h>

// Constants for the display and raindrop properties
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
const int gravity = 1;
const uint16_t fontColor = 0x07E0;
const uint16_t backgroundColor = ST7796S_BLACK;
const int maxDrops = 20;
const int maxStringLength = 10;
const int fontSize = 3;
const int charWidth = fontSize * 6;
const int charHeight = fontSize * 8;
const int screenWidth = 480;
const int screenHeight = 320;
const int numColumns = screenWidth / charWidth;

// Initialize the TFT display
Adafruit_ST7796S_kbv tft = Adafruit_ST7796S_kbv(TFT_CS, TFT_DC, TFT_RST);

// Class for handling color fading calculations
class ColorFader {
public:
  // Linear interpolation between two colors
  static uint16_t lerpColor(uint16_t color1, uint16_t color2, float t) {
    uint8_t r1 = (color1 & 0xF800) >> 8;
    uint8_t g1 = (color1 & 0x07E0) >> 3;
    uint8_t b1 = (color1 & 0x001F) << 3;
    uint8_t r2 = (color2 & 0xF800) >> 8;
    uint8_t g2 = (color2 & 0x07E0) >> 3;
    uint8_t b2 = (color2 & 0x001F) << 3;
    uint8_t r = r1 + t * (r2 - r1);
    uint8_t g = g1 + t * (g2 - g1);
    uint8_t b = b1 + t * (b2 - b1);
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }

  // Get fading color based on a fade factor
  static uint16_t getFadingColor(uint16_t startColor, uint16_t endColor, float fadeFactor) {
    return lerpColor(startColor, endColor, fadeFactor);
  }

  // Fade a color based on a fade factor
  static uint16_t fadeColor(uint16_t color, float fadeFactor) {
    uint8_t r = (color & 0xF800) >> 8;
    uint8_t g = (color & 0x07E0) >> 3;
    uint8_t b = (color & 0x001F) << 3;
    r = r * fadeFactor;
    g = g * fadeFactor;
    b = b * fadeFactor;
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }
};

// Class representing a single raindrop
class Raindrop {
public:
  int x, y, speed;
  unsigned long startTime;
  String characters;
  bool active;
  struct {
    int x, y;
  } lastPositions[maxStringLength];
  String lastCharacters;

  // Constructor to initialize a raindrop
  Raindrop()
    : active(false), lastCharacters("") {}

  // Activate and initialize the raindrop
  void activate(int x) {
    this->x = x;
    this->y = 0;
    this->speed = random(1, 15);
    this->startTime = millis();
    this->active = true;
    this->characters = "";
    int length = random(1, maxStringLength + 1);
    for (int i = 0; i < length; i++) {
      this->characters += char(random(33, 126));
    }
  }

  // Update the position of the raindrop
  void update() {
    if (!active) return;
    y += (speed * gravity);
    int raindropHeight = characters.length() * (charHeight + 1);
    if ((gravity == 1 && (y - raindropHeight) > tft.height()) || (gravity == -1 && (y + raindropHeight) < 0)) {
      active = false;
    }
  }

  // Draw the raindrop on the display
  void draw() {
    if (!active) return;

    // for (int j = 0; j < lastCharacters.length(); j++) {
    //   tft.setCursor(lastPositions[j].x, lastPositions[j].y);
    //   tft.setTextColor(backgroundColor);
    //   tft.print(lastCharacters[j]);
    // }

    // Erase previous characters
    for (int j = 0; j < lastCharacters.length(); j++) {
      tft.fillRect(lastPositions[j].x, lastPositions[j].y, charWidth, charHeight, backgroundColor);
    }

    for (int k = 0; k < characters.length(); k++) {
      if (random(100) < 10) {
        characters[k] = char(random(33, 126));
      }
    }

    int yPos = y;
    for (int j = 0; j < characters.length(); j++) {
      char c = characters[j];
      uint16_t color;
      float t = (sin((millis() - startTime) * 0.002) + 1) / 2;
      if (j == 0) {
        color = ColorFader::lerpColor(fontColor, ST7796S_WHITE, t);
      } else if (characters.length() > 2) {
        float fadeFactor = 1.0 - ((float)j / characters.length());
        color = ColorFader::fadeColor(fontColor, fadeFactor);
      } else {
        color = fontColor;
      }
      tft.setCursor(x, yPos);
      tft.setTextColor(color);
      tft.print(c);
      lastPositions[j].x = x;
      lastPositions[j].y = yPos;
      if (gravity == 1) yPos -= (charHeight + 1);
      else if (gravity == -1) yPos += (charHeight + 1);
    }
    lastCharacters = characters;
  }
};

// Class for managing and updating all raindrops
class RainManager {
public:
  Raindrop raindrops[maxDrops];

  // Update the state of all raindrops
  void update() {
    int activeDrops = 0;
    for (int i = 0; i < maxDrops; i++) {
      if (raindrops[i].active) activeDrops++;
      raindrops[i].update();
    }
    if (activeDrops < maxDrops) {
      int newDropIndex = -1;
      for (int i = 0; i < maxDrops; i++) {
        if (!raindrops[i].active) {
          newDropIndex = i;
          break;
        }
      }
      if (newDropIndex != -1) {
        int x = random(0, numColumns) * charWidth;
        if (!willOverlap(x)) raindrops[newDropIndex].activate(x);
      }
    }
  }

  // Draw all active raindrops
  void draw() {
    for (int i = 0; i < maxDrops; i++) {
      raindrops[i].draw();
    }
  }

  // Check if a new raindrop will overlap with existing ones
  bool willOverlap(int x) {
    for (int i = 0; i < maxDrops; i++) {
      if (raindrops[i].active && raindrops[i].x == x) {
        return true;
      }
    }
    return false;
  }
};

RainManager rainManager;

// Setup the display and initialize settings
void setup() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(backgroundColor);
  tft.setTextSize(fontSize);
}

// Main loop to update and draw raindrops
void loop() {
  rainManager.update();
  rainManager.draw();
}
