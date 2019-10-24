#include "esphome.h"
using namespace esphome;

#include <FastLED.h>
#include "FFT.h"
#include "VisualEffect.h"

enum PLAYMODE {MODE_SCROLL, MODE_ENERGY, MODE_SPECTRUM};

class MusicLeds{
    private:
        //N_PIXELS: The number of the LEDS on the led strip, must be even.
        static const uint16_t N_PIXELS = 256;
        //MIN_VOLUME_THRESHOLD: If the audio's volume is less than this number, the signal will not be processed.
        static constexpr float MIN_VOLUME_THRESHOLD = 0.0003;

        static const uint16_t BUFFER_SIZE = 512; 
        static const uint8_t N_ROLLING_HISTORY = 2;
        static const uint16_t SAMPLE_RATE = 16000;
        static const uint16_t N_MEL_BIN = 18;
        static constexpr float MIN_FREQUENCY = 200;
        static constexpr float MAX_FREQUENCY = 8000;

        float y_data[BUFFER_SIZE * N_ROLLING_HISTORY];
        class FFT *fft;
        class VisualEffect * effect;

        CRGB physic_leds[N_PIXELS];

        PLAYMODE CurrentMode = MODE_SCROLL;

    public:
        MusicLeds();
        ~MusicLeds();
        void ShowFrame( PLAYMODE CurrentMode, light::AddressableLight *p_it);
};

MusicLeds::MusicLeds(){
    fft = new FFT(BUFFER_SIZE*N_ROLLING_HISTORY, N_MEL_BIN, MIN_FREQUENCY, MAX_FREQUENCY, SAMPLE_RATE, MIN_VOLUME_THRESHOLD);
    effect = new VisualEffect(N_MEL_BIN,N_PIXELS);
}

MusicLeds::~MusicLeds(){
    delete fft;
    delete effect;
}


void MusicLeds::ShowFrame( PLAYMODE CurrentMode, light::AddressableLight *p_it){
            static float mel_data[N_MEL_BIN];

            for (int i = 0; i < N_ROLLING_HISTORY - 1; i++)
                memcpy(y_data + i * BUFFER_SIZE, y_data + (i + 1)*BUFFER_SIZE, sizeof(float)*BUFFER_SIZE);

            int16_t l[BUFFER_SIZE];

            unsigned int read_num;
          
            for(int i = 0; i < BUFFER_SIZE; i++) {
	    // read the input on analog ADC1_0:
		read_num = analogRead(A0);
                y_data[BUFFER_SIZE * (N_ROLLING_HISTORY - 1) + i] = analogRead(A0) / 32768.0;
                static int ii=0;
                ii++;
                if(ii%SAMPLE_RATE==0)
                  ESP_LOGD("custom","%lu\t%d\n",millis(),ii);
            }
            fft->t2mel( y_data, mel_data );

            switch(CurrentMode){
              case MODE_SCROLL:
                effect->visualize_scroll(mel_data, physic_leds);
                break;
              case MODE_ENERGY:
                effect->visualize_energy(mel_data, physic_leds);
                break;
              case MODE_SPECTRUM:
                effect->visualize_spectrum(mel_data, physic_leds);
                break;
            }

            for (int i = 0; i < p_it->size(); i++) {
                light::ESPColor c;
                c.r = physic_leds[i].r;
                c.g = physic_leds[i].g;
                c.b = physic_leds[i].b;
                c.w = 0;
                (*p_it)[i] = c;
            }
}

class MusicLeds music_leds;
