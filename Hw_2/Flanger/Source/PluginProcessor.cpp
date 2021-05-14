/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#ifndef M_PI
#define M_PI 3.14159265
#endif

//==============================================================================
FlangerAudioProcessor::FlangerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "Parameters", createParameters()) 
                       // Adding the instatiation of the Value Tree State to the Initialization list
#endif
{
}

FlangerAudioProcessor::~FlangerAudioProcessor()
{
}

//==============================================================================
const juce::String FlangerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FlangerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FlangerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FlangerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FlangerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FlangerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FlangerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FlangerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String FlangerAudioProcessor::getProgramName (int index)
{
    return {};
}

void FlangerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void FlangerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    //Delay buffer init as max sum of a constant and an alternating component
    float maxDelay = apvts.getParameterRange("DELAY").end;
    float maxWidth = apvts.getParameterRange("WIDTH").end;
    int totalFlagerDelaySamples = (int)((maxDelay + maxWidth)*(float)sampleRate) + 1; 
    
    dBuf.setSize(2, totalFlagerDelaySamples);
    dBuf.clear();
    dBufLength = dBuf.getNumSamples();


    //Private members init
    dw = 0;
    dr = 0;
    phase = 0;
}

void FlangerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FlangerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FlangerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }

    //Retrieving all the current parameters from the Value Tree State for the current batch of samples in buffer
    int numSamples = buffer.getNumSamples();
    float feedforward_now = *apvts.getRawParameterValue("GFF");
    float baseDelay_now = *apvts.getRawParameterValue("DELAY") * 0.001f; //ms to s
    float sweepWidth_now = *apvts.getRawParameterValue("WIDTH") * 0.001f; //ms to s
    float lfoFrequency_now = *apvts.getRawParameterValue("FREQ");
    float feedback_now = *apvts.getRawParameterValue("GFB");
    int lfoWaveformIndex = (int)*apvts.getRawParameterValue("WVFORM");

    //Getting buffer output channel pointers
    float* channelOutDataL = buffer.getWritePointer(0);
    float* channelOutDataR = buffer.getWritePointer(1);

    //Getting buffer input channel pointer
    const float* channelInData = buffer.getReadPointer(0);

    //For each sample of the buffer
    for (int i = 0; i < numSamples; ++i) {
        //Current input value	
        const float in = channelInData[i];
        float interpolatedSample = 0.0f;
        float waveformFunction = 0.0f;

        //Definitions of the LFO's wave functions, by convention:
        // lfoWaveformIndex == 0 --> Sine
        // lfoWaveformIndex == 1 --> Triangle
        // lfoWaveformIndex == 2 --> Sawtooth
        switch (lfoWaveformIndex)
        {
        case 0:
            //Sine
            waveformFunction = 0.5f + 0.5f * sinf(2.0f * M_PI * phase);
            break;
        case 1:
            //Triangle
            if (phase < 0.25f)
                waveformFunction = 0.5f + 2.0f * phase;
            else if (phase < 0.75f)
                waveformFunction = 1.0f - 2.0f * (phase - 0.25f);
            else
                waveformFunction = 2.0f * (phase - 0.75f);
            break;
        case 2:
            //Sawtooth
            if (phase < 0.5f)
                waveformFunction = 0.5f + phase;
            else
                waveformFunction = phase - 0.5f;
            break;
        default:
            break;
        }

         //Definition of the total current delay [in seconds] as a sum of a constant and an alternating component
        float currentDelay = baseDelay_now + sweepWidth_now * waveformFunction;
        //Computation of the delay read pointer as distance with the delay write pointer
        dr = fmodf((float)dw - (float)(currentDelay * getSampleRate()) + (float)dBufLength, (float)dBufLength);
        int dr_sample = floorf(dr);



        //CUBIC INTERPOLATION
        float fraction = dr - (float)dr_sample; //Decimal residue
        float fractionSquare = fraction * fraction; //Square coefficient
        float fractionCube = fractionSquare * fraction; //Cube coefficient

        //For special cases as dr=0 or ds=dBufLength, we exploit the circularity of the delay buffer
        float sample0 = dBuf.getSample(0, (dr - 1 + dBufLength) % dBufLength); //Previous sample 
        float sample1 = dBuf.getSample(0, dr); //Current sample
        float sample2 = dBuf.getSample(0, (dr + 1) % dBufLength); //Next sample
        float sample3 = dBuf.getSample(0, (dr + 2) % dBufLength); //2 step next sample

        //Computation of the coefficients and the interpolated sample
        float a0 = -0.5f * sample0 + 1.5f * sample1 - 1.5f * sample2 + 0.5f * sample3;
        float a1 = sample0 - 2.5f * sample1 + 2.0f * sample2 - 0.5f * sample3;
        float a2 = -0.5f * sample0 + 0.5f * sample2;
        float a3 = sample1;
        interpolatedSample = a0 * fractionCube + a1 * fractionSquare + a2 * fraction + a3;


        // LINEAR INTERPOLATION
        //float fraction = dr - floorf(dr);
        //int previousSample = (int)floorf(dr);
        //int nextSample = (previousSample + 1) % dBufLength;
        //interpolatedSample = fraction * dBuf.getSample(0, nextSample) + (1.0f - fraction) * dBuf.getSample(0, previousSample);
        
        //We feedback the signal
        dBuf.setSample(0, dw, in + interpolatedSample * feedback_now);
        dBuf.setSample(1, dw, in + interpolatedSample * feedback_now);

        //Increment the delay write index
        if (++dw >= dBufLength)
            dw = 0;

        //Store the result in the output channel
        channelOutDataL[i] = in + feedforward_now * interpolatedSample;
        channelOutDataR[i] = channelOutDataL[i];

        // LFO phase update 
        phase += lfoFrequency_now * (1 / getSampleRate());
        if (phase >= 1.0)
            phase -= 1.0;

    }


}

//==============================================================================
bool FlangerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FlangerAudioProcessor::createEditor()
{
    return new FlangerAudioProcessorEditor (*this);
}

//==============================================================================
void FlangerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void FlangerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FlangerAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout FlangerAudioProcessor::createParameters()
{
    //In this vector are contained all the parameters as unique pointers to a RangedAudioParameter, helper
    //class for various types of parameters in JUCE (e.g. AudioParameterFloat and AudioParameterChoice)
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    //We push the parameters in the vector instantiating the memory for the various parameters' pointers,
    //the generic AudioParameter has constructor: (StringID, name, rangeStart, rangeEnd, defaultValue)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("GFF", "Amount", 0.0f, 1.0f, 1.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("GFB", "Feedback", 0.0f, 0.5f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("DELAY", "Delay", 1.0f, 5.0f, 1.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("WIDTH", "LFO Width", 1.0f, 20.0f, 1.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("FREQ", "LFO Frequency", 0.05f, 2.0f, 0.1f));
    parameters.push_back(std::make_unique<juce::AudioParameterChoice>("WVFORM", "LFO Waveform", juce::StringArray("Sine","Triangle","Sawtooth"), 0));

    return { parameters.begin(), parameters.end() };
}