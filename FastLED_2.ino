/* button_demoReel100
 *
 * Originally By: Mark Kriegsman
 * 
 * Modified for EEPROM and button usage by: Andrew Tuline
 *
 * Date: January, 2017
 *
 * This takes Mark's DemoReel100 routine and adds the ability to change modes via button press.
 * It also allows the user to save the current mode into EEPROM and use that as the starting mode.
 *
 * Instructions:
 * 
 * Program reads display mode from EEPROM and displays it.
 * Click to change to the next mode.
 * Hold button for > 1 second to write the current mode to EEPROM.
 * Double click to reset to mode 0.
 * 
 * There's also debug output to the serial monitor . . just to be sure.
 * 
 * Requirements:
 * 
 * In addition, to FastLED, you need to download and install JChristensen's button library, available at:
 * 
 * https://github.com/JChristensen/Button
 * 
 */

#define buttonPin 0   // input pin to use as a digital input
#include "jcbutton.h" // Nice button routine by Jeff Saltzman

#include "EEPROM.h"
#include "FastLED.h"

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

// BLE 
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Fixed definitions cannot change on the fly.
#define LED_DT 15        // Data pin to connect to the strip.
#define LED_CK 11        // Clock pin for the strip.
#define COLOR_ORDER BGR  // Are they RGB, GRB or what??
#define LED_TYPE WS2812B // Don't forget to change LEDS.addLeds
#define NUM_LEDS 60      // Number of LED's.

// Definition for the array of routines to display.
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

//Mode and EEPROM variables
uint8_t maxMode = 9; // Maximum number of display modes. Would prefer to get this another way, but whatever.
int eepaddress = 0;

// Global variables can be changed on the fly.
uint8_t max_bright = 128; // Overall brightness definition. It can be changed on the fly.

struct CRGB leds[NUM_LEDS]; // Initialize our LED array.

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0;                  // rotating "base color" used by many of the patterns

typedef void (*SimplePatternList[])(); // List of patterns to cycle through.  Each is defined as a separate function below.



/// config modes
int led_mode = 0;
// blendwawe
CRGB clr1;
CRGB clr2;
uint8_t speed;
uint8_t loc1;
uint8_t loc2;
uint8_t ran1;
uint8_t ran2;
// dot_beat// Define variables used by the sequences.
int thisdelay = 10;    // A delay value for the sequence(s)
uint8_t count = 0;     // Count up to 255 and then reverts to 0
uint8_t fadeval = 224; // Trail behind the LED's. Lower => faster fade.
uint8_t bpm = 30;
// animation_a
CRGBPalette16 currentPalette;
CRGBPalette16 targetPalette;
TBlendType currentBlending; // NOBLEND or LINEARBLEND



// BLE Callback
class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string value = pCharacteristic->getValue();

        gCurrentPatternNumber = (gCurrentPatternNumber + 1) % maxMode;
        Serial.println(gCurrentPatternNumber);

        // if (value.length() > 0)
        // {
        //     Serial.println("*********");
        //     Serial.print("New value: ");
        //     //for (int i = 0; i < value.length(); i++)
        //     Serial.print(value[0]);
        //     Serial.println();
        //     Serial.println("*********");

        //     led_mode = (int)value[0];
        // }
    }
};

void setup()
{

    Serial.begin(115200); // Initialize serial port for debugging.
    delay(1000);         // Soft startup to ease the flow of electrons.

    pinMode(buttonPin, INPUT); // Set button input pin
    digitalWrite(buttonPin, HIGH);

    LEDS.addLeds<LED_TYPE, LED_DT, COLOR_ORDER>(leds, NUM_LEDS); // Use this for WS2812B

    FastLED.setBrightness(max_bright / 2);
    set_max_power_in_volts_and_milliamps(5, 1000); // FastLED power management set at 5V, 500mA.

    gCurrentPatternNumber = EEPROM.read(eepaddress);

    if (gCurrentPatternNumber > maxMode)
        gCurrentPatternNumber = 0; // A safety in case the EEPROM has an illegal value.

    Serial.print("Starting ledMode: ");
    Serial.println(gCurrentPatternNumber);

    currentBlending = LINEARBLEND;



    BLEDevice::init("ESP32-stick");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE);

    pCharacteristic->setCallbacks(new MyCallbacks());

    pCharacteristic->setValue("Hello World");
    pService->start();

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->start();

} // setup()

SimplePatternList gPatterns = {rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpmx, fill_grad, animation_a, blend_wave, dot_beat}; // Don't know why this has to be here. . .

void loop()
{

    readbutton();
    //CheckMode(led_mode);

    EVERY_N_MILLISECONDS(50)
    {
        gPatterns[gCurrentPatternNumber](); // Call the current pattern function once, updating the 'leds' array
    }

    EVERY_N_MILLISECONDS(20)
    { // slowly cycle the "base color" through the rainbow
        gHue++;
    }

    FastLED.show();

} // loop()

void readbutton()
{ // Read the button and increase the mode

    uint8_t b = checkButton();

    if (b == 1)
    { // Just a click event to advance to next pattern
        gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
        Serial.println(gCurrentPatternNumber);
    }

    if (b == 2)
    { // A double-click event to reset to 0 pattern
        gCurrentPatternNumber = 0;
        Serial.println(gCurrentPatternNumber);
    }

    if (b == 3)
    { // A hold event to write current pattern to EEPROM
        EEPROM.write(eepaddress, gCurrentPatternNumber);
        Serial.print("Writing: ");
        Serial.println(gCurrentPatternNumber);
    }

} // readbutton()

void CheckMode(int cur_mode)
{
  //TODO: implement read from BLE Write
}

    //--------------------[ Effects are below here ]------------------------------------------------------------------------------

    void rainbow()
    {

        fill_rainbow(leds, NUM_LEDS, gHue, 7); // FastLED's built-in rainbow generator.

    } // rainbow()

    void rainbowWithGlitter()
    {

        rainbow(); // Built-in FastLED rainbow, plus some random sparkly glitter.
        addGlitter(180);

    } // rainbowWithGlitter()

    void addGlitter(fract8 chanceOfGlitter)
    {

        if (random8() < chanceOfGlitter)
        {
            leds[random16(NUM_LEDS)] += CRGB::White;
        }

    } // addGlitter()

    void confetti()
    { // Random colored speckles that blink in and fade smoothly.

        fadeToBlackBy(leds, NUM_LEDS, 10);
        int pos = random16(NUM_LEDS);
        leds[pos] += CHSV(gHue + random8(64), 200, 255);

    } // confetti()

    void sinelon()
    { // A colored dot sweeping back and forth, with fading trails.

        fadeToBlackBy(leds, NUM_LEDS, 20);
        int pos = beatsin16(13, 0, NUM_LEDS - 1);
        leds[pos] += CHSV(gHue, 255, 192);

    } // sinelon()

    void bpmx()
    { // Colored stripes pulsing at a defined Beats-Per-Minute.

        uint8_t BeatsPerMinute = 62;
        CRGBPalette16 palette = PartyColors_p;
        uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);

        for (int i = 0; i < NUM_LEDS; i++)
        { //9948
            leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
        }

    } // bpmx()

    void juggle()
    { // Eight colored dots, weaving in and out of sync with each other.

        fadeToBlackBy(leds, NUM_LEDS, 20);
        byte dothue = 0;

        for (int i = 0; i < 8; i++)
        {
            leds[beatsin16(i + 7, 0, NUM_LEDS - 1)] |= CHSV(dothue, 200, 255);
            dothue += 32;
        }

    } // juggle()

    void fill_grad()
    {

        uint8_t starthue = beatsin8(5, 0, 255);
        uint8_t endhue = beatsin8(7, 0, 255);

        if (starthue < endhue)
        {
            fill_gradient(leds, NUM_LEDS, CHSV(starthue, 255, 255), CHSV(endhue, 255, 255), FORWARD_HUES); // If we don't have this, the colour fill will flip around.
        }
        else
        {
            fill_gradient(leds, NUM_LEDS, CHSV(starthue, 255, 255), CHSV(endhue, 255, 255), BACKWARD_HUES);
        }

    } // fill_grad()

    void animation_a()
    { // running red stripe.
        for (int i = 0; i < NUM_LEDS; i++)
        {
            uint8_t red = (millis() / 5) + (i * 12); // speed, length
            if (red > 128)
            {
                red = 0;
                leds[i] = ColorFromPalette(currentPalette, red, red, currentBlending);
            }
        }
    } // animation_a()

    void blend_wave()
    {

        speed = beatsin8(6, 0, 255);

        clr1 = blend(CHSV(beatsin8(3, 0, 255), 255, 255), CHSV(beatsin8(4, 0, 255), 255, 255), speed);
        clr2 = blend(CHSV(beatsin8(4, 0, 255), 255, 255), CHSV(beatsin8(3, 0, 255), 255, 255), speed);

        loc1 = beatsin8(10, 0, NUM_LEDS - 1);

        fill_gradient_RGB(leds, 0, clr2, loc1, clr1);
        fill_gradient_RGB(leds, loc1, clr2, NUM_LEDS - 1, clr1);

    } // blend_wave()

    void dot_beat()
    {

        uint8_t inner = beatsin8(bpm, NUM_LEDS / 4, NUM_LEDS / 4 * 3);  // Move 1/4 to 3/4
        uint8_t outer = beatsin8(bpm, 0, NUM_LEDS - 1);                 // Move entire length
        uint8_t middle = beatsin8(bpm, NUM_LEDS / 3, NUM_LEDS / 3 * 2); // Move 1/3 to 2/3

        leds[middle] = CRGB::Purple;
        leds[inner] = CRGB::Blue;
        leds[outer] = CRGB::Aqua;

        nscale8(leds, NUM_LEDS, fadeval); // Fade the entire array. Or for just a few LED's, use  nscale8(&leds[2], 5, fadeval);

    } // dot_beat()

    