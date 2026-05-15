#include <Adafruit_NeoPixel.h>

#define PIN      6
#define NUM_LEDS 29

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);
String input = "";

bool pasaBuque = false;
unsigned long lastUpdate = 0;

void setup()
{
    Serial.begin(115200);
    strip.begin();
    strip.show();
    strip.setBrightness(25);
    Serial.println("Arduino listo");
}

void setLed(int pos, int r, int g, int b)
{
    if (pos >= 0 && pos < NUM_LEDS) {
        strip.setPixelColor(pos, strip.Color(r, g, b));
        strip.show();
    }
}

void setAll(int r, int g, int b)
{
    for (int i = 0; i < NUM_LEDS; i++)
        strip.setPixelColor(i, strip.Color(r, g, b));
    strip.show();
}

void clearAll()
{
    strip.clear();
    strip.show();
}

void loop()
{
    if (pasaBuque && (millis() - lastUpdate > 100)) {
        for (int i = 4; i <= 23; i++) {
            strip.setPixelColor(i, strip.Color(random(255), random(255), random(255)));
        }
        strip.show();
        lastUpdate = millis();
    }

    if (!Serial.available()) return;

    input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("BUQUE ")) {
        pasaBuque = (input.substring(6).toInt() == 1);
        if (!pasaBuque) strip.clear(); 
        Serial.println("BUQUE OK");
    } 
    else if (input.startsWith("FRAME ")) {
        String data = input.substring(6);
        int idx = 0;
        int start = 0;

        while (idx < NUM_LEDS) {
            int sep = data.indexOf(';', start);
            String pixel = (sep == -1) ? data.substring(start) : data.substring(start, sep);

            // SI pasa el buque Y el LED está en el rango del canal (4-23), 
            // ignoramos el color del FRAME para ese LED específico.
            if (!(pasaBuque && idx >= 4 && idx <= 23)) {
                int pr, pg, pb;
                if (sscanf(pixel.c_str(), "%d,%d,%d", &pr, &pg, &pb) == 3) {
                    strip.setPixelColor(idx, strip.Color(pr, pg, pb));
                }
            }

            idx++;
            if (sep == -1) break;
            start = sep + 1;
        }
        strip.show();
    }

    int pos, r, g, b;

    // ── FRAME: actualizar todos los LEDs de una vez ───────
    if (input.startsWith("FRAME ")) {
        String data  = input.substring(6);
        int    idx   = 0;
        int    start = 0;

        while (idx < NUM_LEDS) {
            int    sep   = data.indexOf(';', start);
            String pixel = (sep == -1)
                           ? data.substring(start)
                           : data.substring(start, sep);

            int pr, pg, pb;
            if (sscanf(pixel.c_str(), "%d,%d,%d", &pr, &pg, &pb) == 3)
                strip.setPixelColor(idx, strip.Color(pr, pg, pb));

            idx++;
            if (sep == -1) break;
            start = sep + 1;
        }

        strip.show();
        Serial.println("FRAME OK");
    }

    // ── LED individual ────────────────────────────────────
    else if (sscanf(input.c_str(), "LED %d %d %d %d", &pos, &r, &g, &b) == 4) {
        setLed(pos, r, g, b);
        Serial.println("LED OK");
    }

    // ── ALL ───────────────────────────────────────────────
    else if (sscanf(input.c_str(), "ALL %d %d %d", &r, &g, &b) == 3) {
        setAll(r, g, b);
        Serial.println("ALL OK");
    }

    // ── CLR ───────────────────────────────────────────────
    else if (input == "CLR") {
        clearAll();
        Serial.println("CLEAR OK");
    }
}