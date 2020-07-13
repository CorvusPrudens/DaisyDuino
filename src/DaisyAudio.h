#ifndef DSY_ARDUINO_AUDIO_H
#define DSY_ARDUINO_AUDIO_H

// ENABLE HAL SAI 
#define HAL_SAI_MODULE_ENABLED

#include <stdio.h>
#include "Arduino.h"
#include "DaisyDSP.h"
#include "daisy_pod.h"

enum DaisyAudioDevice {
    DAISY_SEED,
    DAISY_POD,
    DAISY_PETAL,
    DAISY_FIELD,
    DAISY_PATCH,
    DAISY_LAST,
};

enum DaisyAudioSampleRate {
    AUDIO_SR_48K,
    AUDIO_SR_96K,
    AUDIO_SR_LAST,
};

typedef void (*DaisyAudioCallback)(float**, float**, size_t);


//TODO
// updateanalogcontrols? (likely covered by analogRead)
// Parameter would be nice
// PWM for leds would be nice too

class Switch
{
    public:
        void Init(float update_rate, bool invert, uint8_t pin)
	{
	    flip_ = invert;
	    time_per_update_ = 1.f / update_rate;
	    state_ = 0;
	    time_held_ = 0.f;
	    pin_ = pin;
	    pinMode(pin, INPUT_PULLUP);
	}

	//debounces and processes input
	void Debounce()
	{
	    uint8_t in = digitalRead(pin_);
	    state_ = (state_ << 1) | (flip_ ? !in : in);
	    
	    if (state_ == 0x7f || state_ == 0x80)
		time_held_ = 0;
	    
	    if (state_ == 0xff)
		time_held_ += time_per_update_;
	}
    
	bool RisingEdge() { return state_ == 0x7f; }
	
	bool FallingEdge() { return state_ == 0x80; }
	
	bool Pressed() { return state_ == 0xff; }
	
	float TimeHeldMs() { return Pressed() ? time_held_ * 1000.f : 0; }
	
    private:
	bool flip_;
	float time_per_update_, time_held_;
	uint8_t state_, pin_;
};

class Encoder
{
    public:

    void Init(float update_rate, uint8_t pinA, uint8_t pinB, uint8_t pinClick)
    {
	inc_ = 0;
	a_ = b_ = 0xff;

	pinA_ = pinA;
	pinB_ = pinB;

	pinMode(pinA, INPUT_PULLUP);
	pinMode(pinB, INPUT_PULLUP);
	
	encSwitch.Init(update_rate, true, pinClick);
    }
    
    void Debounce()
    {
	uint8_t a_in = digitalRead(pinA_);
	uint8_t b_in = digitalRead(pinB_);

	encSwitch.Debounce();
	
	a_ = (a_ << 1) | (a_in);
	b_ = (b_ << 1) | (b_in);

	inc_ = 0;
	
	if((a_ & 0x0f) == 0x0e && (b_ & 0x07) == 0x00)
	{
	    inc_ = 1;
	}
	else if((b_ & 0x0f) == 0x0e && (a_ & 0x07) == 0x00)
	{
	    inc_ = -1;
	}
    }

    int32_t Increment() { return inc_; }
        
    bool RisingEdge()  { return encSwitch.RisingEdge(); }
	
    bool FallingEdge() { return encSwitch.FallingEdge(); }
    
    bool Pressed()     { return encSwitch.Pressed(); }
    
    float TimeHeldMs() { return encSwitch.TimeHeldMs(); }
	
    private:    
        Switch encSwitch;
        uint8_t a_, b_, pinA_, pinB_;
	int32_t inc_;
};

class DaisyPod
{
public:
    Switch button1;
    Switch button2;
    
    Encoder encoder;
    
    void Init(float control_update_rate)
    {
        button1.Init(control_update_rate, true, PIN_POD_SWITCH_1);
	button2.Init(control_update_rate, true, PIN_POD_SWITCH_2);

	encoder.Init(control_update_rate, PIN_POD_ENC_A, PIN_POD_ENC_B, PIN_POD_ENC_CLICK);
    }

    void Debounce()
    {
        button1.Debounce();
	button2.Debounce();
	encoder.Debounce();
    }
    
};

class AudioClass
{
    public:
        AudioClass();

        // Initializes the audio for the given platform, and returns the number of channels.
        size_t init(DaisyAudioDevice device, DaisyAudioSampleRate sr);
        void begin(DaisyAudioCallback cb);
        void end();
        float get_samplerate();
 	inline float get_callbackrate() { return get_samplerate() / _blocksize; }
	
    private:
        size_t _blocksize;
        DaisyAudioSampleRate _samplerate;
        DaisyAudioDevice _device;
	void InitPins(DaisyAudioDevice device);
};

extern AudioClass DAISY;

#endif
