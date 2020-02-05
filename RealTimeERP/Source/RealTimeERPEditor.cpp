/*
------------------------------------------------------------------

This file is part of a plugin for the Open Ephys GUI
Copyright (C) 2019 Translational NeuroEngineering Laboratory

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


#include "RealTimeERPEditor.h"
using namespace RealTimeERP;

/************** editor *************/
ERPEditor::ERPEditor(Node* p)
    : VisualizerEditor(p, 200, true)
{
    tabText = "Real-Time ERP";
    processor = p;
    // Segment length
    int col0 = 5;
    int col1 = 130;
    int col2 = 170;
    int row0 = 25;
    int row1 = 50;
    int row2 = 75;
    int row3 = 100;

    const int TEXT_HT = 20;
    
    // Window Length
    ERPLenLabel = createLabel("winLabel", "Window Length(s):", { col0, row0, col1-col0, TEXT_HT });
    addAndMakeVisible(ERPLenLabel);

    ERPLenEditable = createEditable("winEditable", "1", "Input length of window", { col1, row0, 35, TEXT_HT });
    addAndMakeVisible(ERPLenEditable);

    juce::Rectangle<int> bounds;
    
    // Step Length
    static const String linearTip = "Linear weighting of ERPs.";
    linearButton = new ToggleButton("Linear");
    linearButton->setBounds(bounds = { col0, row1, col1 - col0, TEXT_HT });
    linearButton->setToggleState(true, dontSendNotification);
    linearButton->addListener(this);
    linearButton->setTooltip(linearTip);
    addAndMakeVisible(linearButton);

    static const String expTip = "Exponential weighting of ERPs. Set alpha using -1/alpha weighting.";
    expButton = new ToggleButton("Exponential");
    expButton->setBounds(bounds = { col0, row2, col1 - col0, TEXT_HT });
    expButton->setToggleState(false, dontSendNotification);
    expButton->addListener(this);
    expButton->setTooltip(expTip);
    addAndMakeVisible(expButton);

    alpha = createLabel("alphaLabel", "Alpha:", { col0 + 25, row3, col1-col0-25, TEXT_HT });
    addAndMakeVisible(alpha);

    alphaE = createEditable("alphaEditable", "0", "Input Value of Alpha", { col1, row3, 35, 27 });
    addAndMakeVisible(alphaE);

    setEnabledState(false);
}

ERPEditor::~ERPEditor() {}


Label* ERPEditor::createEditable(const String& name, const String& initialValue,
    const String& tooltip, juce::Rectangle<int> bounds)
{
    Label* editable = new Label(name, initialValue);
    editable->setEditable(true);
    editable->addListener(this);
    editable->setBounds(bounds);
    editable->setColour(Label::backgroundColourId, Colours::grey);
    editable->setColour(Label::textColourId, Colours::white);
    if (tooltip.length() > 0)
    {
        editable->setTooltip(tooltip);
    }
    return editable;
}

Label* ERPEditor::createLabel(const String& name, const String& text,
    juce::Rectangle<int> bounds)
{
    Label* label = new Label(name, text);
    label->setBounds(bounds);
    label->setFont(Font("Small Text", 12, Font::plain));
    label->setColour(Label::textColourId, Colours::darkgrey);
    return label;
}

void ERPEditor::labelTextChanged(Label* labelThatHasChanged)
{
    auto processor = static_cast<Node*>(getProcessor());
    if (labelThatHasChanged == alphaE)
    {
        float newVal;
        if (updateFloatLabel(labelThatHasChanged, 0, FLT_MAX, 0.0, &newVal))
        {
            processor->setParameter(Node::ALPHA_E, static_cast<float>(newVal));
        }
    }
    if (labelThatHasChanged == ERPLenEditable)
    {
        float newVal;
        if (updateFloatLabel(labelThatHasChanged, 0, FLT_MAX, 1.0, &newVal))
        {
            processor->setParameter(Node::ERP_LEN, static_cast<float>(newVal));
        }
    }
}

void ERPEditor::buttonEvent(Button* buttonClicked)
{
    if (buttonClicked == linearButton)
    {
        processor->setParameter(Node::ALPHA_E, static_cast<float>(0));
        expButton->setToggleState(false, dontSendNotification);
    }

    if (buttonClicked == expButton)
    {
        linearButton->setToggleState(false, dontSendNotification);
        float newVal;
        if (updateFloatLabel(alphaE, 0, FLT_MAX, 0.0, &newVal))
        {
            processor->setParameter(Node::ALPHA_E, static_cast<float>(newVal));
        }
    }

    processor->updateSettings();
}



void ERPEditor::channelChanged(int chan, bool newState)
{
    // Let vis know to update number of channels shown
    ERPVisualizer* ERPcanvas = static_cast<ERPVisualizer*>(canvas.get());
    
    //ERPcanvas->channelChanged(chan, newState);

    if (!acquisitionIsActive)
    {
        processor->updateSettings();
        canvas->update();
    }
}

void ERPEditor::startAcquisition()
{
    alphaE->setEditable(false);
    ERPLenEditable->setEditable(false);
    expButton->setEnabled(false);
    linearButton->setEnabled(false);
    if (canvas != NULL)
    {
        canvas->beginAnimation();
    }
}

void ERPEditor::stopAcquisition()
{
    alphaE->setEditable(true);
    ERPLenEditable->setEditable(true);
    expButton->setEnabled(true);
    linearButton->setEnabled(true);
    if (canvas != NULL)
    {
        canvas->endAnimation();
    }
}
   
Visualizer* ERPEditor::createNewCanvas()
{
    canvas = new ERPVisualizer(processor);
    return canvas;
}

void ERPEditor::updateSettings()
{
    alphaE->setText(String(processor->alpha), dontSendNotification);
    ERPLenEditable->setText(String(processor->ERPLenSec), dontSendNotification);
}


bool ERPEditor::updateIntLabel(Label* label, int min, int max, int defaultValue, int* out)
{
    const String& in = label->getText();
    int parsedInt;
    try
    {
        parsedInt = std::stoi(in.toRawUTF8());
    }
    catch (const std::logic_error&)
    {
        label->setText(String(defaultValue), dontSendNotification);
        return false;
    }

    *out = jmax(min, jmin(max, parsedInt));

    label->setText(String(*out), dontSendNotification);
    return true;
}

// Like updateIntLabel, but for floats
bool ERPEditor::updateFloatLabel(Label* label, float min, float max,
    float defaultValue, float* out)
{
    const String& in = label->getText();
    float parsedFloat;
    try
    {
        parsedFloat = std::stof(in.toRawUTF8());
    }
    catch (const std::logic_error&)
    {
        label->setText(String(defaultValue), dontSendNotification);
        return false;
    }

    *out = jmax(min, jmin(max, parsedFloat));

    label->setText(String(*out), dontSendNotification);
}

