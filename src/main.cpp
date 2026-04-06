#include <Arduino.h>
#include <usbh_midi.h>
#include <usbhub.h>

#include "voice.hpp"
#include "pitchbend.hpp"

// 8-voice square mixer with per-voice variable duty, R-2R output (4-bit on PORTD2..4)
// Sample rate: 31250 Hz (Timer1 ISR)
// R-2R bits: PORTD2 = LSB, PORTD3, PORTD4 = MSB (3-bit -> 0..7 levels)
// Adjust DAC_BITS to expand to more pins if you build larger ladder.

// R-2R output bits (using PORTD) uses D2..D7 for 4 bits
#define DAC_PIN_BASE        2
#define DAC_BITS            6
uint8_t dac_mask;

int8_t octave_pitch;
bool sustain_on;

#define AERODYN_HOLD       400
bool aerodyn_on;
Voice * aerodyn_voice;
uint16_t triggi;

USB         Usb;
USBHub      Hub(&Usb);
USBH_MIDI   Midi(&Usb);


const int8_t aerodyn_mods[64] = {
    0, -8, 5, -5,       8, -8, 5, -5,       8, -8, 5, -5,       8, -8, 5, -5,
    8, -6, 3, -3,       6, -6, 3, -3,       6, -6, 3, -3,       6, -6, 3, -3,
   11, -8, 5, -5,       8, -8, 5, -5,       8, -8, 5, -5,       8, -8, 5, -5,
    5, -7, 4, -4,       7, -7, 4, -4,       7, -7, 4, -4,       7, -7, 4, -4
};

void onInit()
{
  char buf[20];
  uint16_t vid = Midi.idVendor();
  uint16_t pid = Midi.idProduct();
  sprintf(buf, "VID:%04X, PID:%04X", vid, pid);
  Serial.println(buf);
  digitalWrite(A5, HIGH);
  delay(300);
  digitalWrite(A5, LOW);
}


void MIDI_poll ()
{
    uint8_t bufMidi[MIDI_EVENT_PACKET_SIZE];
    uint16_t rcvd;
    //uint8_t channel;
    uint8_t status;
    uint8_t note;
    uint8_t velocity;
    Voice *v;

    if (Midi.RecvData(&rcvd,  bufMidi) == 0 ) {
        if (rcvd == 4) {
            //channel = bufMidi[0];
            status = bufMidi[1];
            note = bufMidi[2];
            velocity = bufMidi[3];

            /* NOTE ON */
            if (status == 0x90 && velocity > 0)
            {
                v = voice_get();
                v->freqX100 = voice_get_freq(note + octave_pitch);
                v->incr = pitchbend_incr(v->freqX100);
                v->note = note;
                v->phase = 0;
                v->hold = 2*SAMPLE_RATE;
                v->volume = ((uint32_t)velocity)<<10;
                v->release = VOICE_SUSTAIN_RELEASE;

                noInterrupts();
                v->state = VOICE_ON;
                voice_active_value++;
                interrupts();
            }
            
            /* NOTE OFF */
            else if (status == 0x80 || (status == 0x90 && velocity == 0)) {
                v = voice_find_by_note(note);
                if (v != nullptr) // else: voice may be reused, because not enough
                {
                    noInterrupts();
                    v->hold = 0;
                    v->release = sustain_on? VOICE_SUSTAIN_RELEASE : VOICE_FAST_RELEASE;
                    interrupts();
                    // start aerodyn with this voice?
                    if (aerodyn_on && aerodyn_voice == nullptr) {
                        aerodyn_voice = v;
                        triggi = 0;
                    }
                }
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
                for (uint8_t i = 0; i < VOICE_MAX; ++i) {
                    v = &voices[i];
                    if (v->state != VOICE_OFF) {
                        v->incr = pitchbend_incr(v->freqX100);
                    }
                }
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

void trigger_aerodyn ()
{
    static uint8_t id = 0;
    if (aerodyn_on == true)
    {
        if (voice_active_value == 0)
        {
            digitalWrite(A2, triggi%16==0 ? HIGH : LOW);
            digitalWrite(A3, LOW);
            digitalWrite(A4, LOW);
            digitalWrite(A5, LOW);
            id = 0;
        } else if  (aerodyn_voice != nullptr) {
            if (triggi%600 == 0)
            {
                int16_t note = aerodyn_voice->note;
                note += aerodyn_mods[id];
                aerodyn_voice = voice_get();
                aerodyn_voice->note = note;
                aerodyn_voice->freqX100 = voice_get_freq(aerodyn_voice->note);
                aerodyn_voice->incr = pitchbend_incr(aerodyn_voice->freqX100);
                aerodyn_voice->phase = 0;
                aerodyn_voice->volume = 65536;
                aerodyn_voice->hold = AERODYN_HOLD;
                aerodyn_voice->release = VOICE_FAST_RELEASE;

                noInterrupts();
                aerodyn_voice->state = VOICE_ON;
                voice_active_value++;
                interrupts();

                if (id == 1) {
                    digitalWrite(A2, HIGH);
                } else if (id == 16) {
                    digitalWrite(A2, LOW);
                    digitalWrite(A3, HIGH);
                } else if (id == 32) {
                    digitalWrite(A3, LOW);
                    digitalWrite(A4, HIGH);
                } else if (id == 48) {
                    digitalWrite(A4, LOW);
                    digitalWrite(A5, HIGH);
                } else if (id == 63) {
                    aerodyn_voice = nullptr;
                    digitalWrite(A5, LOW);
                }
                id = (id+1)%64;
            }
        }
    } else {
        digitalWrite(A2, LOW);
        digitalWrite(A3, LOW);
        digitalWrite(A4, LOW);
        digitalWrite(A5, LOW);
    }
}

void setup ()
{
    Serial.begin(115200);
    
    dac_mask = ((1 << DAC_BITS) - 1) << DAC_PIN_BASE;
    voice_active_value = 0;
    pitchbend = 0;
    triggi = 0;
    aerodyn_on = false;
    aerodyn_voice = nullptr;
    sustain_on = false;

    pinMode(A0, INPUT_PULLUP);
    octave_pitch = -12;

    // aerodyn
    pinMode(A1, INPUT_PULLUP);
    pinMode(A2, OUTPUT);
    pinMode(A3, OUTPUT);
    pinMode(A4, OUTPUT);
    pinMode(A5, OUTPUT);

    cli();
    for (uint8_t i = 0; i < VOICE_MAX; ++i) {
        voice_init(&(voices[i]));
    }
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
    triggi++;
    Usb.Task();
    if ( Midi ) MIDI_poll();

    if (digitalRead(A1) == LOW) {
        delay(200);
        aerodyn_on = aerodyn_on? false : true;
    }

    if (digitalRead(A0) == LOW)
    {
        octave_pitch = 0;
    } else {
        octave_pitch = -12;
    }

    trigger_aerodyn();
}


ISR(TIMER1_COMPA_vect) {
    uint8_t i,vol,sum =0;
    uint8_t port = PORTD;
    Voice *v;

    for (i = 0; i < VOICE_MAX; ++i) {
        v = &voices[i];
        if (v->state != VOICE_OFF) {
            v->phase += v->incr;
            if (v->phase < 0x40000000UL)
            {
                vol = (v->volume>>11);
                sum += (vol == 0)? 1 : vol;
            }
            if (v->state == VOICE_RELEASE) {
                
                if (v->volume < v->release)
                {
                    v->state = VOICE_OFF;
                    voice_active_value--;
                } else {
                    v->volume -= v->release;
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