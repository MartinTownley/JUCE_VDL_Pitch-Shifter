/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VdlpitchAudioProcessorEditor::VdlpitchAudioProcessorEditor (VdlpitchAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    dTimeSlider.setSliderStyle (juce::Slider::SliderStyle::RotaryVerticalDrag);
    //setTextBoxStyle
    //dTimeSlider.setRange (0, 2000, 1);
    addAndMakeVisible (dTimeSlider);
    
    dTimeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.getAPVTS(), "TIME", dTimeSlider);
    
    setSize (400, 300);
    
    
    
    
}

VdlpitchAudioProcessorEditor::~VdlpitchAudioProcessorEditor()
{
}

//==============================================================================
void VdlpitchAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void VdlpitchAudioProcessorEditor::resized()
{
    dTimeSlider.setBoundsRelative (0.2, 0.2, 0.6, 0.6);
}
