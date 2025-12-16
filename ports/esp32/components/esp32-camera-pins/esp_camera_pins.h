/*
Sets PIN config for next boards:
WROVER_KIT 		"ESP32-Wrover CAM Board"
ESP32CAM_AITHINKER 	"ESP32CAM AI-Thinker board"
ESP32S3_WROOM		"ESP32-S3 WROOM Board"
ESP32S3_GOOUUU		"GOOUUU ESP32 S3 CAM Board"
ESP32CAM_TTGO		"TTGO T-Journal Board"
ESP32CAM_TTGO_PLUS	"TTGO T-Camera Plus Board"
ESP32CAM_TTGO_PIR	"TTGO T-Camera with PIR Sensor Board"
M5_MODEL_A		"M5-Camera Model A"
M5_MODEL_B		"M5-Camera Model B"
M5_STACK		"M5-Stack ESP32-Camera"
ESP_EYE			"ESP-EYE"

Info from esp32-camera/examples and  https://randomnerdtutorials.com/esp32-cam-camera-pin-gpios/
*/


#if !(defined(CONFIG_BOARD_WROVER_KIT) || defined(CONFIG_BOARD_ESP32CAM_AITHINKER) || defined(CONFIG_BOARD_ESP32S3_WROOM) || defined(CONFIG_BOARD_ESP32S3_GOOUUU) || defined(CONFIG_BOARD_ESP32CAM_TTGO) || defined(CONFIG_BOARD_ESP32CAM_TTGO_PLUS) || defined(CONFIG_BOARD_ESP32CAM_TTGO_PIR) || defined(CONFIG_BOARD_M5_MODEL_A) || defined(CONFIG_BOARD_M5_MODEL_B) || defined(CONFIG_BOARD_M5_STACK) || defined(CONFIG_BOARD_ESP_EYE))
#error "Board type not defined. Use idf.py menuconfig"
#endif

#ifdef CONFIG_BOARD_WROVER_KIT

#define CAM_PIN_PWDN -1  //power down is not used
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 21
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 19
#define CAM_PIN_D2 18
#define CAM_PIN_D1 5
#define CAM_PIN_D0 4
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#endif

// ESP32Cam (AiThinker) PIN Map
#ifdef CONFIG_BOARD_ESP32CAM_AITHINKER

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#endif

// ESP32S3 (WROOM) PIN Map
#ifdef CONFIG_BOARD_ESP32S3_WROOM
#define CAM_PIN_PWDN 38
#define CAM_PIN_RESET -1   //software reset will be performed
#define CAM_PIN_VSYNC 6
#define CAM_PIN_HREF 7
#define CAM_PIN_PCLK 13
#define CAM_PIN_XCLK 15
#define CAM_PIN_SIOD 4
#define CAM_PIN_SIOC 5
#define CAM_PIN_D0 11
#define CAM_PIN_D1 9
#define CAM_PIN_D2 8
#define CAM_PIN_D3 10
#define CAM_PIN_D4 12
#define CAM_PIN_D5 18
#define CAM_PIN_D6 17
#define CAM_PIN_D7 16
#endif

// ESP32S3 (GOOUU TECH)
#ifdef CONFIG_BOARD_ESP32S3_GOOUUU
#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET -1   //software reset will be performed
#define CAM_PIN_VSYNC 6
#define CAM_PIN_HREF 7
#define CAM_PIN_PCLK 13
#define CAM_PIN_XCLK 15
#define CAM_PIN_SIOD 4
#define CAM_PIN_SIOC 5
#define CAM_PIN_D0 11
#define CAM_PIN_D1 9
#define CAM_PIN_D2 8
#define CAM_PIN_D3 10
#define CAM_PIN_D4 12
#define CAM_PIN_D5 18
#define CAM_PIN_D6 17
#define CAM_PIN_D7 16
#endif

// TTGO T-Journal Board
#ifdef CONFIG_BOARD_ESP32CAM_TTGO
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 27
#define CAM_PIN_SIOD 25
#define CAM_PIN_SIOC 23
#define CAM_PIN_D7 19
#define CAM_PIN_D6 36
#define CAM_PIN_D5 18
#define CAM_PIN_D4 39
#define CAM_PIN_D3 5
#define CAM_PIN_D2 34
#define CAM_PIN_D1 35
#define CAM_PIN_D0 17
#define CAM_PIN_VSYNC 22
#define CAM_PIN_HREF 26
#define CAM_PIN_PCLK 21
#endif

// TTGO T-Camera Plus Board
#ifdef CONFIG_BOARD_ESP32CAM_TTGO_PLUS
#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 4
#define CAM_PIN_SIOD 18
#define CAM_PIN_SIOC 23
#define CAM_PIN_D7 36
#define CAM_PIN_D6 37
#define CAM_PIN_D5 38
#define CAM_PIN_D4 39
#define CAM_PIN_D3 35
#define CAM_PIN_D2 26
#define CAM_PIN_D1 13
#define CAM_PIN_D0 34
#define CAM_PIN_VSYNC 5
#define CAM_PIN_HREF 27
#define CAM_PIN_PCLK 25
#endif

// TTGO T-Camera with PIR Sensor Board
#ifdef CONFIG_BOARD_ESP32CAM_TTGO_PIR
#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 32
#define CAM_PIN_SIOD 13
#define CAM_PIN_SIOC 12
#define CAM_PIN_D7 39
#define CAM_PIN_D6 36
#define CAM_PIN_D5 23
#define CAM_PIN_D4 18
#define CAM_PIN_D3 5
#define CAM_PIN_D2 4
#define CAM_PIN_D1 14
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 27
#define CAM_PIN_HREF 25
#define CAM_PIN_PCLK 19
#endif

// M5-Camera Model A
#ifdef CONFIG_BOARD_M5_MODEL_A
#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET 15
#define CAM_PIN_XCLK 27
#define CAM_PIN_SIOD 25
#define CAM_PIN_SIOC 23
#define CAM_PIN_D7 19
#define CAM_PIN_D6 36
#define CAM_PIN_D5 18
#define CAM_PIN_D4 39
#define CAM_PIN_D3 5
#define CAM_PIN_D2 34
#define CAM_PIN_D1 35
#define CAM_PIN_D0 32
#define CAM_PIN_VSYNC 22
#define CAM_PIN_HREF 26
#define CAM_PIN_PCLK 21
#endif

// M5-Camera Model B
#ifdef CONFIG_BOARD_M5_MODEL_B
#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET 15
#define CAM_PIN_XCLK 27
#define CAM_PIN_SIOD 22
#define CAM_PIN_SIOC 23
#define CAM_PIN_D7 19
#define CAM_PIN_D6 36
#define CAM_PIN_D5 18
#define CAM_PIN_D4 39
#define CAM_PIN_D3 5
#define CAM_PIN_D2 34
#define CAM_PIN_D1 35
#define CAM_PIN_D0 32
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 26
#define CAM_PIN_PCLK 21
#endif

// M5-Stack ESP32-Camera
#ifdef CONFIG_BOARD_M5_STACK
#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET 15
#define CAM_PIN_XCLK 27
#define CAM_PIN_SIOD 25
#define CAM_PIN_SIOC 23
#define CAM_PIN_D7 19
#define CAM_PIN_D6 36
#define CAM_PIN_D5 18
#define CAM_PIN_D4 39
#define CAM_PIN_D3 5
#define CAM_PIN_D2 34
#define CAM_PIN_D1 35
#define CAM_PIN_D0 17
#define CAM_PIN_VSYNC 22
#define CAM_PIN_HREF 26
#define CAM_PIN_PCLK 21
#endif

// ESP_EYE
#ifdef CONFIG_BOARD_ESP_EYE
#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 4
#define CAM_PIN_SIOD 18
#define CAM_PIN_SIOC 23
#define CAM_PIN_D7 36
#define CAM_PIN_D6 37
#define CAM_PIN_D5 38
#define CAM_PIN_D4 39
#define CAM_PIN_D3 35
#define CAM_PIN_D2 14
#define CAM_PIN_D1 13
#define CAM_PIN_D0 34
#define CAM_PIN_VSYNC 5
#define CAM_PIN_HREF 27
#define CAM_PIN_PCLK 25
#endif
