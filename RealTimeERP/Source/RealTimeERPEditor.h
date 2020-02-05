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

/*
Alright all of the options need to fit on editor, vis will be onyl for vis and clear.
 - So we want an electrode button array for choosing your ttl channel events (1-8 work).    
 --   Steal from coherence vis 
 - Update length of ERP calculation and plot - label
 - Alpha for decay value (0 is linear) - label (steal from coherence vis as well)
*/

#ifndef ERP_EDITOR_H_INCLUDED
#define ERP_EDITOR_H_INCLUDED

#include "RealTimeERP.h"
#include "RealTimeERPVisualizer.h"
#include <VisualizerEditorHeaders.h>

namespace RealTimeERP
{
    class ERPEditor
        : public VisualizerEditor
        , public Label::Listener
    {
        friend class ERPVisualizer;
    public:
        ERPEditor(Node* n);
        ~ERPEditor();

        void labelTextChanged(Label* labelThatHasChanged) override;
        void buttonEvent(Button* buttonClick) override;
        void channelChanged(int chan, bool newState) override;

        void startAcquisition() override;
        void stopAcquisition() override;

        Visualizer* createNewCanvas() override;


       void updateSettings() override;

    private:
        Node* processor;
        
        // Length of ERP calculation
        ScopedPointer<Label> ERPLenLabel;
        ScopedPointer<Label> ERPLenEditable;

        // Decay
        ScopedPointer<ToggleButton> linearButton;
        ScopedPointer<ToggleButton> expButton;
        ScopedPointer<Label> alpha;
        ScopedPointer<Label> alphaE;
        
        Label* ERPEditor::createLabel(const String& name, const String& text,
            juce::Rectangle<int> bounds);
        Label* ERPEditor::createEditable(const String& name, const String& initialValue,
            const String& tooltip, juce::Rectangle<int> bounds);

        bool updateIntLabel(Label* label, int min, int max, int defaultValue, int* out);
        bool updateFloatLabel(Label* label, float min, float max,
            float defaultValue, float* out);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ERPEditor);
    };

}


#endif // ERP_EDITOR_H_INCLUDED
