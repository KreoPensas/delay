#include "daisy_petal.h"
#include "daisysp.h"
#include "terrarium.h"

#define MAX_DELAY static_cast<size_t>(48000 * 4)

using namespace daisy;
using namespace daisysp;
using namespace terrarium;  

DaisyPetal hw;
Parameter  time, feedback, drymix;
bool      delayHold;

Led led2;

struct delay
{
    DelayLine<float, MAX_DELAY> *del;
    float                        currentDelay;
    float                        delayTarget;
    float                        feedback;

    float Process(float in)
    {
        fonepole(currentDelay, delayTarget, .0001f);
        del->SetDelay(currentDelay);
        float read = del->Read();
        if(delayHold==false){
        del->Write((feedback * read) + in);
        }
        if(delayHold==true){
        del->Write(read);
        }     
        return read;
    }
};

delay     delays;

DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delMems;

void InitDelays(float samplerate)
{
        delMems.Init();
        delays.del = &delMems;
        time.Init(hw.knob[Terrarium::KNOB_1], samplerate * .25, MAX_DELAY, Parameter::LINEAR);        
}

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
              AudioHandle::InterleavingOutputBuffer out,
              size_t                                size)
{
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();
    led2.Update();
    float delayfeedback = feedback.Process(); 
    float drywet = drymix.Process();	
    delays.delayTarget = time.Process();   
    delays.feedback= delayfeedback;            
    if(hw.switches[Terrarium::FOOTSWITCH_2].FallingEdge())
    {  
         delayHold = !delayHold;
         led2.Set(delayHold ? 1.0f : 0.0f);
    }
      
    for(size_t i = 0; i < size; i++)
    {
        float mix     = 0;
        float dry = in[i];
        float sig = delays.Process(in[i]);
        mix += sig;	
        mix       = drywet * mix * .9f + (1.0f - drywet) * dry;	
        out[i] = out[i+1] = mix;
    }
}

int main(void)
{
    float samplerate;
    hw.Init();
    samplerate = hw.AudioSampleRate();
    hw.SetAudioBlockSize(2);
    drymix.Init(hw.knob[Terrarium::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);    
    feedback.Init(hw.knob[Terrarium::KNOB_2], 0.0f, 1.00f, Parameter::LINEAR);
    InitDelays(samplerate);
    led2.Init(hw.seed.GetPin(Terrarium::LED_2),false);
    led2.Update();
    delayHold = false;
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
    }
}
