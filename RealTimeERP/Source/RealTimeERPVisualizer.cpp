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
using namespace RealTimeERP;
ERPVisualizer::ERPVisualizer(ProcessorPlugin* n)
	: viewport(new Viewport())
	, canvas(new Component("canvas"))
	, processor(n)
	, canvasBounds(0, 0, 1, 1)
{
	refreshRate = 2;
	juce::Rectangle<int> bounds;

	const int TEXT_HT = 20;
	int yPos = 60;
	int titlePos = 5;
	title = new Label("Title", "Real Time Evoked Potentials");
	title->setBounds(bounds = { titlePos, 0, 250, 50 });
	title->setFont(Font(20, Font::bold));
	canvas->addAndMakeVisible(title);
	canvasBounds = canvasBounds.getUnion(bounds);

	viewport->setViewedComponent(canvas, false);
	viewport->setScrollBarsShown(true, true);
	addAndMakeVisible(viewport);

	startCallbacks();
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
void ERPVisualizer::update()
{
	int numInputs = processor->getActiveInputs().size();
}


void ERPVisualizer::paint(Graphics& g)
{
	// To make background not black.
	ColourGradient editorBg = processor->getEditor()->getBackgroundGradient();
	g.fillAll(editorBg.getColourAtPosition(0.5)); // roughly matches editor background (without gradient)
}

void ERPVisualizer::refresh()
{
}

void ERPVisualizer::buttonClicked(Button* buttonClicked)
{
	if (buttonClicked == reset)
	{
		processor->reset();
	}
}


void ERPVisualizer::beginAnimation()
{
}
void ERPVisualizer::endAnimation()
{}

void ERPVisualizer::setParameter(int, float) {}
void ERPVisualizer::setParameter(int, int, int, float) {}
