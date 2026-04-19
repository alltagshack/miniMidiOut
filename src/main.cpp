#include <Arduino.h>
#include <usbh_midi.h>
#include <usbhub.h>

#include "voice.hpp"
#include "pitchbend.hpp"
#include "sustain.hpp"

// multi voice square mixer with per-voice variable duty, R-2R output (6-bit on PORTD2..7)
// Adjust DAC_BITS to expand to more pins if you build larger ladder.

// R-2R output bits (using PORTD) uses D2..D7 for 6 bits
#define DAC_PIN_BASE        2
#define DAC_BITS            6
uint8_t dac_mask;

#define PITCH_POTI          A0
#define PITCH_LED           A1
#define SUSTAIN_BTN         A2
#define OCTAVE_SW           A3

int16_t poti_pitch;
int16_t poti_pitch_old;
int8_t octave_pitch;
int    sustain_last;


USB         Usb;
USBHub      Hub(&Usb);
USBH_MIDI   Midi(&Usb);

void onInit()
{
  char buf[20];
  uint16_t vid = Midi.idVendor();
  uint16_t pid = Midi.idProduct();
  sprintf(buf, "VID:%04X, PID:%04X", vid, pid);
  Serial.println(buf);
}


void MIDI_poll ()
{
    uint8_t bufMidi[MIDI_EVENT_PACKET_SIZE];
    uint16_t rcvd;
    uint8_t status;
    uint8_t note;
    uint8_t velocity;
    Voice *v;

    if (Midi.RecvData(&rcvd, bufMidi) == 0 ) {
        if (rcvd == 4) {
            status = bufMidi[1];
            note = bufMidi[2];
            velocity = bufMidi[3];

            /* NOTE ON */
            if (status == 0x90 && velocity > 0)
            {
                Serial.println(note);
                v = voice_get();

                v->freqX100 = voice_get_freq(note + octave_pitch);
                v->incr = pitchbend_incr(v->freqX100);
                v->note = note;
                v->phase = 0;
                v->hold = 4*SAMPLE_RATE;
                v->volume = ((uint32_t)velocity)<<10;
                v->release = VOICE_SUSTAIN_RELEASE;

                noInterrupts();
                v->state = VOICE_ON;
                voice_active_value++;
                interrupts();

            }
            
            /* NOTE OFF */
            else if (status == 0x80 || (status == 0x90 && velocity == 0)) {
                voice_off(note);
            }

            /* SUSTAIN PEDAL */
            else if (status == 0xB0 && note == 0x40) {
                uint8_t rel;
                if (velocity == 0) {
                    sustain_on = false;
                    rel = VOICE_FAST_RELEASE;
                } else {
                    sustain_on = true;
                    rel = VOICE_SUSTAIN_RELEASE;
                }
                for (int i = 0; i < VOICE_MAX; ++i) {
                    v = &voices[i];
                    v->release = rel;
                    v->hold = 0;
                }
            }
            /* BITCH BEND */
            else if (status == 0xE0) {
                pitchbend = ((velocity << 7) | note) - 8192;
                pitchbend_refresh();
            }
        }
    }
}


void setupTimers ()
{
    // Configure DAC pins as outputs (PORTD)
    for (uint8_t i = 0; i < DAC_BITS; ++i) {
        pinMode(DAC_PIN_BASE + i, OUTPUT);
        digitalWrite(DAC_PIN_BASE + i, LOW);
    }

    // Timer0: leave default (millis/delay) — we don't use OC0A PWM here
    // Timer1: CTC for SAMPLE_RATE using prescaler 8
    const uint32_t prescaler = 8UL;
    const uint16_t ocr = (uint16_t)((F_CPU / prescaler / SAMPLE_RATE) - 1UL);

    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11); // CTC + prescaler 8
    OCR1A = ocr;
    TIMSK1 = (1 << OCIE1A);
}

void setup ()
{
    Serial.begin(115200);
    
    dac_mask = ((1 << DAC_BITS) - 1) << DAC_PIN_BASE;
    voice_active_value = 0;
    pitchbend = 0;
    sustain_on = false;
    sustain_last = HIGH;
    octave_pitch = 0;
    poti_pitch_old = 0;

    pinMode(PITCH_POTI, INPUT);
    pinMode(PITCH_LED, OUTPUT);
    pinMode(SUSTAIN_BTN, INPUT_PULLUP);
    pinMode(OCTAVE_SW, INPUT_PULLUP);

    for (uint8_t i = 0; i < VOICE_MAX; ++i) {
        voice_init(&(voices[i]));
    }
    cli();
    setupTimers();
    sei();


    Serial.println("Try to init USB Host Shield.");
    while (Usb.Init() == -1) {
        delay(200);
        Serial.print(".");
    }
    Serial.println("success!");
    Midi.attachOnInit(onInit);
}

void loop ()
{
    Usb.Task();
    if ( Midi ) MIDI_poll();

    poti_pitch = analogRead(PITCH_POTI);
    if (poti_pitch > (poti_pitch_old+5) || poti_pitch < (poti_pitch_old-5)) {
        pitchbend = map(poti_pitch, 0, 1023, -8192, 8191);
        pitchbend_refresh();
        poti_pitch_old = poti_pitch;
    }

    if (pitchbend == 0 || (poti_pitch < 514 && poti_pitch > 508)) {
        digitalWrite(PITCH_LED, HIGH);
    } else {
        digitalWrite(PITCH_LED, LOW);
    }

    if (digitalRead(OCTAVE_SW) == LOW) {
        octave_pitch = -12;
    } else {
        octave_pitch = 0;
    }

    if (digitalRead(SUSTAIN_BTN) == LOW) {
        if (sustain_last == HIGH) {
            sustain_on = true;
            sustain_last = LOW;
            for (int i = 0; i < VOICE_MAX; ++i) {
                voices[i].release = VOICE_SUSTAIN_RELEASE;
                voices[i].hold = 0;
            }
        }
    } else {
        if (sustain_last == LOW) {
            sustain_on = false;
            sustain_last = HIGH;
            for (int i = 0; i < VOICE_MAX; ++i) {
                voices[i].release = VOICE_FAST_RELEASE;
                voices[i].hold = 0;
            }
        }
    }
}


ISR(TIMER1_COMPA_vect) {
    int8_t i,vol,sum = 0;
    uint8_t port = PORTD;
    Voice *v;

    for (i = 0; i < VOICE_MAX; ++i) {
        v = &voices[i];
        if (v->state != VOICE_OFF) {
            v->phase += v->incr;
            if (v->phase < 0x40000000UL)
            {
                vol = (v->volume >> 11);
                sum += (vol < 1)? 1 : vol;
            }
            if (v->state == VOICE_RELEASE) {
                
                if (v->volume > v->release)
                {
                    v->volume -= v->release;
                } else {
                    v->state = VOICE_OFF;
                    voice_active_value--;
                }
            } else {
                if (v->hold == 0) {
                    v->state = VOICE_RELEASE;
                } else if (v->hold > 0) {
                    v->hold--;
                }
            }

        }
    }
    port &= ~dac_mask;
    port |= ( (sum << DAC_PIN_BASE) & dac_mask );
    PORTD = port;
}