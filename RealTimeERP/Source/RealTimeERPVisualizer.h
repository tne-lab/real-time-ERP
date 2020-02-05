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

#ifndef ERP_VIS_H_INCLUDED
#define ERP_VIS_H_INCLUDED

#include "RealTimeERP.h"
#include <VisualizerWindowHeaders.h>

#include "../../Source/Processors/Visualization/MatlabLikePlot.h"
namespace RealTimeERP
{
    class ERPVisualizer : public Visualizer
        , public ComboBox::Listener
        , public Button::Listener
        , public Label::Listener
    {
        template<typename T>
        using vector = std::vector<T>;

        friend class ERPEditor;
        friend class Node;
    public:
        ERPVisualizer(Node* n);
        ~ERPVisualizer();

        void resized() override;

        void refreshState() override;
        void update() override;
        void refresh() override;
        void beginAnimation() override;
        void endAnimation() override;
        void setParameter(int, float) override;
        void setParameter(int, int, int, float) override;
        void comboBoxChanged(ComboBox* comboBoxThatHasChanged)  override {};
        void labelTextChanged(Label* labelThatHasChanged) override {};
        void buttonClicked(Button* buttonClick) override;
        void paint(Graphics& g) override;

        void channelChanged(int chan, bool newState);
        void createElectrodeButtons();

    private:
        Node* processor;

        // Creates labeled rows based on num active channels
        void createChannelRowLabels();
        // Code to show canvas. Save on copy/pasting
        void flipCanvas();
        void drawERP(int chan);
        void resetTriggerChannels();

        Label* createLabel(const String& name, const String& text, juce::Rectangle<int> bounds);

        ScopedPointer<Viewport>  viewport;
        ScopedPointer<Component> canvas;
        juce::Rectangle<int> canvasBounds;

        ScopedPointer<Label> title;
        ScopedPointer<TextButton> resetButton;
        ScopedPointer<ComboBox> calcSelect;
        ScopedPointer<ComboBox> trigSelect;

        Array<ScopedPointer<Label>> chanLabels;
        Array<ScopedPointer<Label>> calcLabels;


        Array<int> chanList;

        Array<Colour> colorList;

        vector<double> dummy;
        vector<double> dummy2;

        int channelYStart;
        int channelYJump;

        // Label explaining that these are for TTL Channels
        ScopedPointer<Label> ttlButtonLabel;
        Array<ElectrodeButton*> ttlButtons;

        // Store most recent update so we decide which to show a
        vector<vector<String>> avgSum; // Average area under curve (trigger(ttl 1-8) x channel)
        vector<vector<vector<double>>> avgLFP; // Save the average waveform (trigger(ttl 1-8) x channel x vector of waveform)
        vector<vector<String>> avgPeak; // Avg Peak height (trigger(ttl 1-8) x channel)
        vector<vector<String>> avgTimeToPeak; // Avg time to the peak height (trigger(ttl 1-8) x channel)
 

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ERPVisualizer);
    };
}



#endif // ERP_VIS_H_INCLUDED
