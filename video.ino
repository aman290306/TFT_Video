#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <TJpg_Decoder.h>

// ---------- Wi-Fi ----------
const char* WIFI_NAME = "Rngesus";
const char* WIFI_PASSWORD = "amanemad";

// ---------- Cloud video ----------
const char* VIDEO_URL =
    "https://raw.githubusercontent.com/aman290306/TFT_Video/main/video_160x128_30fps.mjpeg";

// ---------- TFT pins ----------
#define TFT_CS    5
#define TFT_DC    27
#define TFT_RST   4

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

// Allow JPEG frames up to 32 KB
constexpr size_t JPEG_BUFFER_SIZE = 32 * 1024;
uint8_t jpegBuffer[JPEG_BUFFER_SIZE];

// 30 FPS
constexpr uint32_t FRAME_INTERVAL_US = 33333;

// TJpg_Decoder sends small image blocks to this function
bool drawJpegBlock(
    int16_t x,
    int16_t y,
    uint16_t width,
    uint16_t height,
    uint16_t* bitmap
) {
    if (y >= tft.height()) {
        return false;
    }

    tft.drawRGBBitmap(x, y, bitmap, width, height);
    return true;
}

void showMessage(const char* message, uint16_t color = ST77XX_WHITE) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(4, 4);
    tft.setTextColor(color);
    tft.setTextSize(1);
    tft.setTextWrap(true);
    tft.println(message);
}

bool connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    showMessage("Connecting to Wi-Fi...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_NAME, WIFI_PASSWORD);

    uint32_t startTime = millis();

    while (
        WiFi.status() != WL_CONNECTED &&
        millis() - startTime < 20000
    ) {
        delay(250);
    }

    if (WiFi.status() != WL_CONNECTED) {
        showMessage("Wi-Fi connection failed", ST77XX_RED);
        return false;
    }

    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());

    return true;
}

uint32_t playMjpeg(
    Stream& stream,
    HTTPClient& http,
    int32_t bytesRemaining
) {
    bool insideJpeg = false;
    uint8_t previousByte = 0;

    size_t jpegLength = 0;
    uint32_t frameNumber = 0;
    uint32_t playbackStart = micros();
    uint32_t lastDataTime = millis();

    while (
        bytesRemaining != 0 &&
        (http.connected() || stream.available())
    ) {
        if (!stream.available()) {
            if (millis() - lastDataTime > 15000) {
                Serial.println("Stream timeout");
                break;
            }

            delay(1);
            continue;
        }

        int incomingValue = stream.read();

        if (incomingValue < 0) {
            continue;
        }

        lastDataTime = millis();

        if (bytesRemaining > 0) {
            bytesRemaining--;
        }

        uint8_t currentByte = (uint8_t)incomingValue;

        // Search for JPEG start marker: FF D8
        if (!insideJpeg) {
            if (
                previousByte == 0xFF &&
                currentByte == 0xD8
            ) {
                jpegBuffer[0] = 0xFF;
                jpegBuffer[1] = 0xD8;

                jpegLength = 2;
                insideJpeg = true;
            }

            previousByte = currentByte;
            continue;
        }

        // Prevent buffer overflow
        if (jpegLength >= JPEG_BUFFER_SIZE) {
            Serial.println("JPEG frame too large — skipped");

            jpegLength = 0;
            insideJpeg = false;
            previousByte = currentByte;
            continue;
        }

        jpegBuffer[jpegLength++] = currentByte;

        // JPEG end marker: FF D9
        if (
            previousByte == 0xFF &&
            currentByte == 0xD9
        ) {
            uint32_t targetTime =
                playbackStart +
                frameNumber * FRAME_INTERVAL_US;

            int32_t waitTime =
                (int32_t)(targetTime - micros());

            if (waitTime > 0) {
                delayMicroseconds(waitTime);
            }

            TJpgDec.drawJpg(
                0,
                0,
                jpegBuffer,
                jpegLength
            );

            frameNumber++;

            Serial.print("Frame: ");
            Serial.println(frameNumber);

            jpegLength = 0;
            insideJpeg = false;
        }

        previousByte = currentByte;
    }

    return frameNumber;
}

bool playCloudVideo() {
    if (!connectWiFi()) {
        return false;
    }

    showMessage("Downloading video...");

    WiFiClientSecure secureClient;

    // Suitable for testing; certificate is not validated.
    secureClient.setInsecure();

    HTTPClient http;

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(15000);

    if (!http.begin(secureClient, VIDEO_URL)) {
        showMessage("Could not open URL", ST77XX_RED);
        return false;
    }

    // Request a normal Content-Length response
    http.useHTTP10(true);

    int responseCode = http.GET();

    Serial.print("HTTP response: ");
    Serial.println(responseCode);

    if (responseCode != HTTP_CODE_OK) {
        showMessage("Cloud download failed", ST77XX_RED);
        http.end();
        return false;
    }

    Stream& videoStream = http.getStream();
    int32_t videoSize = http.getSize();

    Serial.print("Video bytes: ");
    Serial.println(videoSize);

    tft.fillScreen(ST77XX_BLACK);

    uint32_t displayedFrames =
        playMjpeg(videoStream, http, videoSize);

    http.end();

    Serial.print("Total displayed frames: ");
    Serial.println(displayedFrames);

    return displayedFrames > 0;
}

void setup() {
    Serial.begin(115200);

    SPI.begin(
        18,       // SCK
        -1,       // MISO not used
        23,       // MOSI/SDA
        TFT_CS
    );

    tft.initR(INITR_BLACKTAB);
    tft.setRotation(1);

    // Increase SPI speed for video playback
    tft.setSPISpeed(40000000);

    tft.fillScreen(ST77XX_BLACK);

    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(drawJpegBlock);

    Serial.print("Display: ");
    Serial.print(tft.width());
    Serial.print(" x ");
    Serial.println(tft.height());
}

void loop() {
    bool success = playCloudVideo();

    if (!success) {
        delay(5000);
        return;
    }

    showMessage("Video finished");
    delay(2000); // Replay after two seconds
}