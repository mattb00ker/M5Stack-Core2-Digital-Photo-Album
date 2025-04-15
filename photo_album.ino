/*
  Digital Photo Frame for M5Stack Core2 using M5Core2 Library

  This application reads JPEG and BMP image files from the SD card located in the "/images" folder
  and displays them on the Core2’s screen as a slideshow.
  
  Each image is shown for 5 seconds. There is an immediate (direct) transition between images
  (no fades or blank screen in between). When the slideshow is paused (via the middle button),
  a small pause symbol appears in the top-left corner.

  Controls:
    - Left Button (BtnA): Show the previous image.
    - Middle Button (BtnB): Toggle pause/resume.
    - Right Button (BtnC): Show the next image.

  Requirements:
    - M5Stack Core2 with SD card inserted.
    - M5Core2 library installed via Arduino IDE.
    - SD card formatted as FAT32 with a folder named "images" containing JPEG (.jpg/.jpeg)
      or BMP (.bmp) files.
    - Note: If an image is larger than the display resolution (320×240), only the upper-left portion is displayed.
*/

#include <M5Core2.h>  // M5Stack Core2 library (provides display, touch, and SD support)
#include <FS.h>       // Filesystem support
#include <SD.h>       // SD card access
#include <vector>     // For std::vector to store image file paths

// Configuration constants
#define IMAGE_DIRECTORY "/images"         // Folder on SD containing images
const uint32_t DISP_INTERVAL = 5000;        // Time (in ms) each image is displayed
const uint8_t FULL_BRIGHTNESS = 80;         // Display brightness (0–255)

// Global variables
std::vector<String> imageFiles;   // Stores full paths of valid image files
size_t currentIndex = 0;          // Current image index in the vector
bool paused = false;              // Indicates if the slideshow is paused
uint32_t lastChangeTime = 0;      // Timestamp when the current image was displayed

/**
 * @brief Helper function to check if a filename has a JPEG extension.
 *
 * @param path The file path or name.
 * @return true if the path ends with ".jpg" or ".jpeg", false otherwise.
 */
bool isJpeg(const String &path) {
  String lower = path;
  lower.toLowerCase();
  return lower.endsWith(".jpg") || lower.endsWith(".jpeg");
}

/**
 * @brief Draws the image at the given index onto the display.
 *
 * This function draws directly onto the screen without clearing it separately.
 * It uses the appropriate draw function based on the file extension.
 *
 * @param index Index in the imageFiles vector.
 */
void showImage(size_t index) {
  if (index >= imageFiles.size()) return;
  String path = imageFiles[index];

  // Draw the image without an intermediate fill to avoid blank flashes.
  if (isJpeg(path)) {
    M5.Lcd.drawJpgFile(SD, path.c_str(), 0, 0);
  }
  else if (path.endsWith(".bmp")) {
    M5.Lcd.drawBmpFile(SD, path.c_str(), 0, 0);
  }
}

/**
 * @brief Draws a small pause symbol in the upper-left corner.
 *
 * The pause symbol consists of two filled vertical rectangles.
 */
void drawPauseSymbol() {
  const int x = 5;
  const int y = 5;
  const int rectWidth = 5;
  const int rectHeight = 15;
  const int gap = 5;
  M5.Lcd.fillRect(x, y, rectWidth, rectHeight, WHITE);
  M5.Lcd.fillRect(x + rectWidth + gap, y, rectWidth, rectHeight, WHITE);
}

/**
 * @brief Loads valid image file paths from the specified SD card folder.
 */
void loadImageFiles() {
  File root = SD.open(IMAGE_DIRECTORY);
  if (!root) {
    M5.Lcd.printf("Folder \"%s\" not found!\n", IMAGE_DIRECTORY);
    while (true) {
      M5.update();
    }
  }

  // Iterate through all files in the directory.
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String filename = String(file.name());
      filename.toLowerCase();
      if (filename.endsWith(".jpg") || filename.endsWith(".jpeg") || filename.endsWith(".bmp")) {
        // Construct the full file path.
        String fullPath = String(IMAGE_DIRECTORY);
        if (!fullPath.endsWith("/")) fullPath += "/";
        // Extract only the file name from the SD card file path.
        fullPath += String(file.name()).substring(String(file.name()).lastIndexOf('/') + 1);
        imageFiles.push_back(fullPath);
        Serial.println("Found image: " + fullPath);
      }
    }
    file = root.openNextFile();
  }
  root.close();
}

void setup() {
  // Initialize M5Stack Core2 (this sets up display, touch input, SD card, etc.)
  M5.begin();
  M5.Lcd.setBrightness(FULL_BRIGHTNESS);
  // Clear the screen at startup.
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  
  Serial.begin(115200);
  Serial.println("Initializing photo frame...");

  // Initialize SD card.
  if (!SD.begin()) {
    M5.Lcd.println("SD Card Init Failed!");
    while (true) {
      M5.update();
    }
  }
  
  // Load image file paths.
  loadImageFiles();
  if (imageFiles.empty()) {
    M5.Lcd.println("No images found in " IMAGE_DIRECTORY);
    Serial.println("ERROR: Please add .jpg, .jpeg, or .bmp files to the SD card folder.");
    while (true) {
      M5.update();
    }
  }
  
  // Display the first image.
  currentIndex = 0;
  showImage(currentIndex);
  lastChangeTime = millis();
}

void loop() {
  M5.update();  // Update the M5Stack Core2 internal state (touch, buttons, etc.)

  // Handle left button (BtnA) for previous image.
  if (M5.BtnA.wasPressed()) {
    currentIndex = (currentIndex == 0) ? imageFiles.size() - 1 : currentIndex - 1;
    showImage(currentIndex);
    lastChangeTime = millis();
  }
  // Handle right button (BtnC) for next image.
  else if (M5.BtnC.wasPressed()) {
    currentIndex = (currentIndex + 1) % imageFiles.size();
    showImage(currentIndex);
    lastChangeTime = millis();
  }
  // Handle middle button (BtnB) for pause/resume toggle.
  if (M5.BtnB.wasPressed()) {
    paused = !paused;
    Serial.println(paused ? "Slideshow paused." : "Slideshow resumed.");
    // If resuming, immediately redraw the current image and reset the timer.
    if (!paused) {
      showImage(currentIndex);
      lastChangeTime = millis();
    }
  }
  
  // Automatically advance the slideshow if not paused and the display interval has passed.
  if (!paused && (millis() - lastChangeTime >= DISP_INTERVAL)) {
    currentIndex = (currentIndex + 1) % imageFiles.size();
    showImage(currentIndex);
    lastChangeTime = millis();
  }
  
  // If paused, overlay the pause symbol.
  if (paused) {
    drawPauseSymbol();
  }
  
  delay(10);  // Small delay to reduce CPU usage and help with button debouncing.
}
