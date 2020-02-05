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

#include "RealTimeERPVisualizer.h"
#include <algorithm>

using namespace RealTimeERP;


/************** Visualizer *************/
ERPVisualizer::ERPVisualizer(Node* n)
	: viewport(new Viewport())
	, canvas(new Component("canvas"))
	, processor(n)
	, canvasBounds(0, 0, 1, 1)
	, chanList ({})
	, chanLabels({})
	, channelYStart(140)
	, channelYJump(50)
{
	refreshRate = 2;
	juce::Rectangle<int> bounds;
	colorList = { Colours::lightblue, Colours::lightcoral, Colours::lightcyan, Colours::lightgoldenrodyellow, Colours::lightgreen
	,Colours::lightgrey, Colours::lightpink };

	const int TEXT_HT = 20;
	int yPos = 60;
	int titlePos = 5;

	for (double i = 0; i < 30000; i++)
	{
		dummy.push_back(i / 3000.0);
		dummy2.push_back(i * 2 / 3000.0);
	}
	// -- Title -- //
	title = createLabel("Title", "Real Time Evoked Potentials", { titlePos, 0, 250, 50 });

	// -- Reset Button -- //
	resetButton = new TextButton("Reset");
	resetButton->setBounds(bounds = { 270, 10, 50, 30 });
	resetButton->addListener(this);
	resetButton->setColour(Label::backgroundColourId, Colours::red);
	resetButton->setColour(Label::textColourId, Colours::white);
	canvas->addAndMakeVisible(resetButton);
	canvasBounds = canvasBounds.getUnion(bounds);

	// -- Calculation Select -- //
	calcSelect = new ComboBox("CalcSelect");
	calcSelect->setTooltip("Select between displaying AUC, Peak Height or Time to Peak");
	calcSelect->setBounds(bounds = { 170, channelYStart - 25 , 150,20 });
	calcSelect->addListener(this);
	canvas->addAndMakeVisible(calcSelect);
	canvasBounds = canvasBounds.getUnion(bounds);

	calcSelect->addItem("Area Under Curve", 1);
	calcSelect->addItem("Peak Height", 2);
	calcSelect->addItem("Time to Peak", 3);
	calcSelect->setSelectedId(1);

	// -- Trigger Select -- //
	trigSelect = new ComboBox("trigSelect");
	trigSelect->setTooltip("Select between trigger channels to display the calcuated values for that trigger");
	trigSelect->setBounds(bounds = { 5, channelYStart - 25, 150, 20 });
	trigSelect->addListener(this);
	canvas->addAndMakeVisible(trigSelect);
	canvasBounds = canvasBounds.getUnion(bounds);
	int numTriggers = processor->triggerChannels.size();
	for (int t = 0; t < numTriggers; t++)
	{
		trigSelect->addItem(String(processor->triggerChannels[t].name), t + 1);
	}
	if (numTriggers > 0)
	{
		trigSelect->setSelectedId(1);
	}
	

	// -- Create Channel Row Labels -- //
	chanList = processor->getActiveInputs();
	createChannelRowLabels();

	// -- Pad Edges and Show -- //
	flipCanvas();

	startCallbacks();
}

void ERPVisualizer::flipCanvas()
{
	canvasBounds.setBottom(canvasBounds.getBottom() + 10);
	canvasBounds.setRight(1000);
	canvas->setBounds(canvasBounds);

	viewport->setViewedComponent(canvas, false);
	viewport->setScrollBarsShown(false, true);
	addAndMakeVisible(viewport);
}

void ERPVisualizer::drawERP(int chan)
{
	int yPos = channelYStart + chan * channelYJump;

	for (int i = 0; i < 30000; i++)
	{
		dummy.push_back(i / 3000);
	}
}


ERPVisualizer::~ERPVisualizer()
{
	stopCallbacks();
}

void ERPVisualizer::resized()
{
	viewport->setSize(getWidth(), getHeight());
}

void ERPVisualizer::refreshState() {}


void ERPVisualizer::resetTriggerChannels()
{
	trigSelect->clear(dontSendNotification);
	// Create items in combo box
	for (int t = 0; t < processor->triggerChannels.size(); t++)
	{
		trigSelect->addItem(String(processor->triggerChannels[t].name), t + 1);
	}

	// If number of trig exists
	if (trigSelect->getNumItems() > 0 && trigSelect->getSelectedId() == 0)
	{
		trigSelect->setSelectedId(1);
	}
}

void ERPVisualizer::update()
{
	//int numInputs = processor->getActiveInputs().size();
	int numChannels = processor->getActiveInputs().size();
	int numTriggers = processor->triggerChannels.size();
	avgLFP = vector<vector<vector<double>>>(numTriggers, vector<vector<double>>(numChannels, vector<double>(processor->ERPLenSamps, 0)));
	avgSum = vector<vector<String>>(numTriggers, vector<String>(numChannels, "0"));
	avgPeak = vector<vector<String>>(numTriggers, vector<String>(numChannels, "0"));
	avgTimeToPeak = vector<vector<String>>(numTriggers, vector<String>(numChannels, "0"));
	createElectrodeButtons();
	resetTriggerChannels();
}

//void ERPVisualizer::comboBoxChanged(ComboBox* comboBoxThatHasChanged){// Set which of our calculations to view (AUC, peak height or time to peak)}

void ERPVisualizer::paint(Graphics& g) 
{
	// Draw out our ERPS
	int trigIndex = trigSelect->getSelectedId() - 1;
	int trig = processor->triggerChannels[trigIndex].channel;
	double max = NULL;
	double min = NULL;
	for (int j = 0; j < chanList.size(); j++)
	{
		auto tempMax = std::max_element(avgLFP[trigIndex][j].begin(), avgLFP[trigIndex][j].end());
		auto tempMin = std::min_element(avgLFP[trigIndex][j].begin(), avgLFP[trigIndex][j].end());

		if (max == NULL || *tempMax > max)
		{
			max = *tempMax;
		}
		if (min == NULL || *tempMin < min)
		{
			min = *tempMin;
		}
	}
	double midPoint = max / 2 + min / 2;
	double totalY = max - min;
	// Make sure there is data
	if (totalY > 0)
	{
		// Loop through all channels
		for (int chan = 0; chan < chanList.size(); chan++)
		{
			// Find pixel positions for start/end and top/bottom based on voltage data and length
			double xPos = 250;
			int xEnd = 1000;
			int yPosMid = (channelYStart + (chan + 1) * (channelYJump)) - channelYJump / 2;

			double step = (xEnd - xPos) / double(avgLFP[trigIndex][chan].size());

			for (int samp = 0; samp < avgLFP[trigIndex][chan].size(); samp++)
			{
				// Turn this into looping through ttl channels
				g.setColour(colorList[chan]);
				double ratio = (avgLFP[trigIndex][chan][samp] - midPoint) / totalY;
				double yPos = yPosMid + channelYJump * ratio;
				g.fillRect(xPos, yPos, 1, 1); // Draw pixel rects of data.. Maybe not the best way!
				xPos += step;
			}

		}
	}

}

void ERPVisualizer::refresh() 
{
	int numChannels = processor->getActiveInputs().size();

	if (processor->avgSum.hasUpdate())
	{
		int numTriggers = processor->triggerChannels.size();
		AtomicScopedReadPtr<vector<vector<RWA>>> sumReader(processor->avgSum);
		AtomicScopedReadPtr<vector<vector<vector<RWA>>>> LFPReader(processor->avgLFP);
		AtomicScopedReadPtr<vector<vector<RWA>>> peakReader(processor->avgPeak);
		AtomicScopedReadPtr<vector<vector<RWA>>> ttPeakReader(processor->avgTimeToPeak);
		sumReader.pullUpdate();
		LFPReader.pullUpdate();
		peakReader.pullUpdate();
		ttPeakReader.pullUpdate();

		

		for (int t = 0; t < numTriggers; t++)
		{
			for (int chan = 0; chan < numChannels; chan++)
			{
				avgSum[t][chan] = String(sumReader->at(t)[chan].getAverage());
				avgPeak[t][chan] = String(peakReader->at(t)[chan].getAverage());
				avgTimeToPeak[t][chan] = String(ttPeakReader->at(t)[chan].getAverage() / processor->fs) + 's';
				for (int n = 0; n < processor->ERPLenSamps; n++)
				{
					avgLFP[t][chan][n] = LFPReader->at(t)[chan][n].getAverage();
				}
			}
		}

		canvasBounds.setBottom(canvasBounds.getBottom() - 10);
		flipCanvas();
		repaint();
	}

	int trigIndex = trigSelect->getSelectedId() - 1;
	int trig = processor->triggerChannels[trigIndex].channel;
	int calc = calcSelect->getSelectedId();
	calcLabels.clear();
	int xPos = 75;
	calcLabels.add(createLabel(calcSelect->getText() + "label", calcSelect->getText(), { xPos, channelYStart - 5, 125 , 40 }));
	for (int chan = 0; chan < numChannels; chan++)
	{
		switch (calc)
		{
		case 1:
			calcLabels.add(createLabel("Calc" + String(chan), avgSum[trigIndex][chan], { xPos, channelYStart + chan * channelYJump + channelYJump / 2, 125 , 40 }));
			break;
		case 2:
			calcLabels.add(createLabel("Calc" + String(chan), avgPeak[trigIndex][chan], { xPos, channelYStart + chan * channelYJump + channelYJump / 2, 125 , 40 }));
			break;
		case 3:
			calcLabels.add(createLabel("Calc" + String(chan), avgTimeToPeak[trigIndex][chan], { xPos, channelYStart + chan * channelYJump + channelYJump / 2, 125 , 40 }));
			break;
		}
	}
	repaint();
}


void ERPVisualizer::createElectrodeButtons()
{
	// Set consts for buttons
	int xPos = 5;
	int yPos = 45;
	juce::Rectangle<int> bounds;

	bool buttonState;

	for (int i = 0; i < ttlButtons.size(); i++)
	{
		canvas->removeChildComponent(canvas->getIndexOfChildComponent(ttlButtons[i]));
	}
	ttlButtons.clear();

	if (!processor->eventSourceArray.isEmpty())
	{
		// Create Buttons
		// Calc Size of buttons
		int totalHeight = 60;

		int nTTLEvents = processor->eventSourceArray.size();
		int halfEvents = int(nTTLEvents / 2);

		int width =  100;

		//int width = totalWidth / halfEvents; // total Width / Num cols
		int height = totalHeight / 2; // total height / Num rums

		for (int e = 0; e < nTTLEvents; e++)
		{
			// Is this already selected (when loaded in through xml)
			if (processor->triggerChannels.contains(processor->eventSourceArray[e]))
			{
				buttonState = true;
			}
			else
			{
				buttonState = false;
			}

			// Create buttons
			ElectrodeButton* button = new ElectrodeButton(e + 1);

			if (e < halfEvents) // Row 1
			{
				button->setBounds(bounds = { xPos + e * width, yPos, width, height });
			}
			else // Row 2
			{
				button->setBounds(bounds = { xPos + (e - halfEvents) * width, yPos + height, width, height });
			}

			button->setRadioGroupId(0);
			button->setButtonText(String(processor->eventSourceArray[e].name));
			button->addListener(this);
			button->setToggleState(buttonState, dontSendNotification);
			canvas->addAndMakeVisible(button);
			ttlButtons.insert(e, button);
		}
	}
}

void ERPVisualizer::buttonClicked(Button* buttonClicked)
{
	if (buttonClicked == resetButton)
	{
		// Clears all vectors of data to start from scratch.
		processor->reset();
	}

	if (ttlButtons.contains((ElectrodeButton*)buttonClicked))
	{
		ElectrodeButton* eButton = static_cast<ElectrodeButton*>(buttonClicked);
		int n = eButton->getChannelNum() - 1; // button chan correspond with eventSourceArray
		EventSources es = processor->eventSourceArray[n];
		
		// Remove or add to trigger list
		if (eButton->getToggleState())
		{
			processor->triggerChannels.addIfNotAlreadyThere(es);
		}
		else
		{
			processor->triggerChannels.removeAllInstancesOf(es);
		}
	}
}

void ERPVisualizer::channelChanged(int chan, bool newState)
{
	// Edit list of channels we are looking at
	if (newState)
	{
		chanList.addIfNotAlreadyThere(chan);
		chanList.sort();
	}
	else
	{
		chanList.removeFirstMatchingValue(chan);
	}
	createChannelRowLabels();
}

void ERPVisualizer::createChannelRowLabels()
{
	// Remove edging (bounds grow infintely if you don't)
	canvasBounds.setBottom(canvasBounds.getBottom() - 10);
	
	// Redraw all for simplicity
	chanLabels.clear();
	for (int i = 0; i < chanList.size(); i++)
	{
		chanLabels.add(createLabel("ChanLabel" + String(chanList[i] + 1), "Chan" + String(chanList[i] + 1) + " - ", { 5, channelYStart + i * channelYJump + channelYJump / 2, 125 , 40 }));
	}

	// Redraw Canvas
	flipCanvas();
	repaint(); // Remove eventually (will have nothing to draw yet in actual setting)
}

Label* ERPVisualizer::createLabel(const String& name, const String& text,
	juce::Rectangle<int> bounds)
{
	Label* label = new Label(name, text);
	label->setBounds(bounds);
	label->setFont(Font("Chan Labels", 20, Font::bold));
	label->setColour(Label::textColourId, Colours::white);
	canvas->addAndMakeVisible(label);
	canvasBounds = canvasBounds.getUnion(bounds);
	return label;
}


void ERPVisualizer::beginAnimation() {}
void ERPVisualizer::endAnimation() {}

void ERPVisualizer::setParameter(int, float) {}
void ERPVisualizer::setParameter(int, int, int, float) {}
