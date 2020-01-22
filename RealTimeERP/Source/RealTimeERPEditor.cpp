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
namespace RealTimeERP
{
    /************** editor *************/
    ERPEditor::ERPEditor(ProcessorPlugin* p)
        : VisualizerEditor(p, 300, true)
    {
        tabText = "Real-Time ERP";
        processor = p;
        // Segment length
        int col0 = 5;
        int col1 = 80;
        int col2 = 110;
        int row0 = 25;
        int row1 = 60;
        int row2 = 95;

        const int TEXT_HT = 20;

        //Make ttl buttons
        ttlButtonLabel = createLabel("segLabel", "TTL Channels to Watch:", { col0, row0 + 25, 70, TEXT_HT });
        addAndMakeVisible(ttlButtonLabel);

        int nEvents = processor->getTotalEventChannels();
        createElectrodeButtons(nEvents);

        // Window Length
        ERPLenLabel = createLabel("winLabel", "Window Length(s):", { col1, row0 + 25, 70, TEXT_HT });
        addAndMakeVisible(ERPLenLabel);

        ERPLenEditable = createEditable("winEditable", "2", "Input length of window", { col1 + 75, row0 + 25, 35, TEXT_HT });
        addAndMakeVisible(ERPLenEditable);

        juce::Rectangle<int> bounds;
        // Step Length
        static const String linearTip = "Linear weighting of ERPs.";
        linearButton = new ToggleButton("Linear");
        linearButton->setBounds(bounds = { col1, row1, 70, TEXT_HT });
        linearButton->setToggleState(true, dontSendNotification);
        linearButton->addListener(this);
        linearButton->setTooltip(linearTip);
        addAndMakeVisible(linearButton);

        static const String expTip = "Exponential weighting of ERPs. Set alpha using -1/alpha weighting.";
        expButton = new ToggleButton("Exp");
        expButton->setBounds(bounds = { col1, row2, 30, TEXT_HT });
        expButton->setToggleState(false, dontSendNotification);
        expButton->addListener(this);
        expButton->setTooltip(expTip);
        addAndMakeVisible(expButton);

        alpha = createLabel("stepLabel", "Step Length(s):", { col1 + 30, row2, 75, TEXT_HT });
        addAndMakeVisible(alpha);

        alphaE = createEditable("winEditable", "2", "Input length of window", { col1 + 115, row2, 35, 27 });
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
        auto processor = static_cast<ProcessorPlugin*>(getProcessor());
        if (labelThatHasChanged == alphaE)
        {
            float newVal;
            if (updateFloatLabel(labelThatHasChanged, 0, INT_MAX, 8, &newVal))
            {
                processor->setParameter(ProcessorPlugin::ALPHA_E, static_cast<float>(newVal));
            }
        }
    }

    void ERPEditor::buttonClicked(Button* buttonClicked)
    {
        if (ttlButtons.contains((ElectrodeButton*)buttonClicked))
        {
            ElectrodeButton* eButton = static_cast<ElectrodeButton*>(buttonClicked);
            int buttonChan = eButton->getChannelNum() - 1;
            processor->triggerChannels.addIfNotAlreadyThere(buttonChan);
        }
    }

    void ERPEditor::createElectrodeButtons(int nEvents)
    {
        int xPos = 5;
        juce::Rectangle<int> bounds;
        // GROSS Fix
        int halfEvents = int(nEvents / 2);
        for (int chan = 0; chan < halfEvents; chan++)
        {
            // row 1 buttons
            ElectrodeButton* button = new ElectrodeButton(chan + 1);
            button->setBounds(bounds = { xPos + chan * 20, 60, 20, 15 });
            button->setRadioGroupId(0);
            button->setButtonText(String(chan + 1));
            button->addListener(this);
            addAndMakeVisible(button);
            ttlButtons.insert(chan, button);
        }
        for (int chan = halfEvents; chan < nEvents; chan++)
        {
            // row 2 buttons
            ElectrodeButton* button = new ElectrodeButton(chan + 1);
            button->setBounds(bounds = { xPos + (chan - halfEvents) * 20, 75, 20, 15 });
            button->setRadioGroupId(0);
            button->setButtonText(String(chan + 1));
            button->addListener(this);
            addAndMakeVisible(button);
            ttlButtons.insert(chan, button);
        }
    }
/*
    void ERPEditor::startAcquisition()
    {
        //canvas->beginAnimation();
    }

    void ERPEditor::stopAcquisition()
    {
        //canvas->endAnimation();
    }

    Visualizer* ERPEditor::createNewCanvas()
    {
        canvas = new ERPVisualizer(processor);
        return canvas;
    }
*/

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
}
