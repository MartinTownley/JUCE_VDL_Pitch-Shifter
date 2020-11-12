/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VdlpitchAudioProcessor::VdlpitchAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

VdlpitchAudioProcessor::~VdlpitchAudioProcessor()
{
}

//==============================================================================
const juce::String VdlpitchAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VdlpitchAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VdlpitchAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VdlpitchAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VdlpitchAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VdlpitchAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int VdlpitchAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VdlpitchAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String VdlpitchAudioProcessor::getProgramName (int index)
{
    return {};
}

void VdlpitchAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void VdlpitchAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    
    const int numInputChannels = getTotalNumInputChannels();
    
    
    // Access to two seconds of the audio buffer:
    const int delayBufferSize = 2 * (sampleRate + samplesPerBlock); //add the buffer length to the sample rate for safety, in case you get too close to the original position
    
    mSampleRate = sampleRate;
    
    //DBG(delayBufferSize);
    
    // [2] Set size of the delay buffer:
    mDelayBuffer.setSize(numInputChannels, delayBufferSize);
}

void VdlpitchAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VdlpitchAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
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

void VdlpitchAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    const int bufferLength = buffer.getNumSamples();
    const int delayBufferLength = mDelayBuffer.getNumSamples();
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        
        
        
        // [3] Create read pointers:
        
        const float* bufferData = buffer.getReadPointer(channel);
        const float* delayBufferData = mDelayBuffer.getReadPointer(channel);
        
        // [4] Copy data from main buffer to delay buffer:
        fillDelayBuffer(channel, bufferLength, delayBufferLength, bufferData, delayBufferData);
        getFromDelayBuffer(buffer, channel, bufferLength, delayBufferLength, bufferData, delayBufferData);
    }
    
    //[5] Advance write position – the first time it copies 0-511 values, and the next time we want to start at 512.
    mWritePosition += bufferLength;
    // [6] Wrap around when you reach the end of the delay buffer:
    mWritePosition %= delayBufferLength;
    
    //
    
    
}

void VdlpitchAudioProcessor::fillDelayBuffer (int channel, const int bufferLength, const int delayBufferLength, const float* bufferData, const float* delayBufferData)
{
    // Check that delay buffer length is greater than the buffer length + write position.
    
    
    if (delayBufferLength > bufferLength + mWritePosition)
    {
        mDelayBuffer.copyFromWithRamp(channel, mWritePosition, bufferData, bufferLength, 0.8, 0.8);
        
    } else {
        const int bufferRemaining = delayBufferLength - mWritePosition; //covers the remaining values in the delayBuffer.
        
        // Copy remaining samples
        mDelayBuffer.copyFromWithRamp(channel, mWritePosition, bufferData, bufferRemaining, 0.8, 0.8);
        
        // Set delayBuffer back to copy from the beginning of the next chunk of bufferData:
        
        // Delay buffer is now filled, so go back to the beginning of the delayBuffer and start overwriting values.
        mDelayBuffer.copyFromWithRamp(channel, 0, bufferData, bufferLength - bufferRemaining, 0.8, 0.8);
    }
    
}

void VdlpitchAudioProcessor::getFromDelayBuffer (juce::AudioBuffer<float>& buffer, int channel, const int bufferLength, const int delayBufferLength, const float* bufferData, const float* delayBufferData)
{
    int delayTime = 500; //ms
    
    // Create a read position
    const int readPosition = static_cast<int> (delayBufferLength + mWritePosition - (mSampleRate * delayTime / 1000)) % delayBufferLength; // Would be half a second, 22050 samples.
    
    // Is there enough space in the delayBuffer for us not to go off the edge?
    if (delayBufferLength > bufferLength + readPosition)
    {
        // Add samples to our buffer FROM our delayed buffer:
        buffer.addFrom(channel, 0, delayBufferData + readPosition, bufferLength);
    } else {
        const int bufferRemaining = delayBufferLength - readPosition;
        
        buffer.addFrom(channel, 0, delayBufferData + readPosition, bufferRemaining);
        buffer.addFrom(channel, bufferRemaining, delayBufferData, bufferLength - bufferRemaining);
        
    }
    
}

//==============================================================================
bool VdlpitchAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* VdlpitchAudioProcessor::createEditor()
{
    return new VdlpitchAudioProcessorEditor (*this);
}

//==============================================================================
void VdlpitchAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void VdlpitchAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VdlpitchAudioProcessor();
}
