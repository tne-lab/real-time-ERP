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

#include "RealTimeERP.h"
#include "RealTimeERPEditor.h"

using namespace RealTimeERP;

ProcessorPlugin::ProcessorPlugin()
    : GenericProcessor("Real Time ERP")
    , triggerChannels({})
    , ttlTimestampBuffer(0, std::deque<uint64>())
    , currentlyFilling(false)
    , ERPLenSec(1.0)
    , alpha(0)
    , avgERP(0,vector<RWA>(getNumOutputs()))
    , curSum(0,vector<float>(getNumOutputs()))
    //, avgLFP            (getNumOutputs(), vector<AudioSampleBuffer>() // make a new audio buffer?
    , avgLFP(0, vector<vector<RWA>>(getNumOutputs(), vector<RWA>(0)))
{
    // Can do init stuff here
    // Events and such maybe?
}

ProcessorPlugin::~ProcessorPlugin() {}

AudioProcessorEditor* ProcessorPlugin::createEditor()
{
    editor = new ERPEditor(this);
    return editor;
}

void ProcessorPlugin::updateSettings()
{
    fs = GenericProcessor::getSampleRate();
    ERPLenSamps = fs * ERPLenSec;

    int numChannels = getActiveInputs().size();
    for (int i = 0; i < triggerChannels.size(); i++)
    {
        ttlTimestampBuffer.push_back(std::deque<uint64>());
    }
   // vector<std::deque<uint64>> ttlTimestampBuffer(triggerChannels.size(), std::deque<uint64>());
    vector<vector<vector<RWA>>> avgLFP(triggerChannels.size(), vector<vector<RWA>>(numChannels, vector<RWA>(ERPLenSamps, RWA(alpha))));
    vector<vector<RWA>> avgERP(triggerChannels.size(), vector<RWA>(numChannels, RWA(alpha)));
    vector<vector<float>> curSum(triggerChannels.size(), vector<float>(numChannels));
}

void ProcessorPlugin::process(AudioSampleBuffer& buffer)
{
	checkForEvents(false); // Check for ttl events

    // If new TTL timestamp to handle (stim happened), timestamp is removed once calculation is complete
    for (int t = 0; t < triggerChannels.size(); t++)
    {
        if (!ttlTimestampBuffer[t].empty())
        {
            std::cout << "GOT AN EVENT!!" << std::endl;
            Array<int> activeChannels = getActiveInputs();
            int numChannels = activeChannels.size();
            if (numChannels <= 0)
            {
                std::cout << "yikes" << std::endl;
                ttlTimestampBuffer[t].pop_front();
                jassertfalse;
                return;
            }
            uint64 stimTime = ttlTimestampBuffer[t].front();
            uint32 nBufSamps = getNumSamples(0);
            uint64 bufTimestamp = getTimestamp(0);
            float curBufStartSamp = 0; // Middle buffers use all ...
            float curBufEndSamp = nBufSamps; // samples in buffer
            bool done = false;
            if (stimTime < bufTimestamp + nBufSamps) // first buffer, starts at stim, ends at end
            {
                std::cout << "first buffer" << std::endl;
                curBufStartSamp = stimTime - bufTimestamp;
            }
            else if (stimTime + ERPLenSamps < bufTimestamp + nBufSamps) // last buffer, starts at 0, ends in middle
            {
                std::cout << "Popping buffer" << std::endl;
                curBufEndSamp = stimTime + ERPLenSamps - bufTimestamp;
                ttlTimestampBuffer[t].pop_front();
                done = true;
            }

            // Loop through each channel and add up LFP area under curve and average out the ERP data.
            for (int n = 0; n < numChannels; n++)
            {
                int chan = activeChannels[n];
                const float* rpIn = buffer.getReadPointer(chan);
                for (int samp = curBufStartSamp; samp < curBufEndSamp; samp++)
                {
                    std::cout << "Adding up samples" << std::endl;
                    float val = rpIn[samp];
                    avgLFP[t][chan][samp].addValue(val); // Add to avg waveform
                    curSum[t][chan] += abs(val); // Add to area under curve sum
                }
                if (done)
                {
                    std::cout << "Current Sum: " << curSum[t][chan] << " for trigger " << t << " from channel " << chan << std::endl;
                    avgERP[t][chan].addValue(curSum[t][chan]);
                }
            }
        }
    }  
}

void ProcessorPlugin::handleEvent(const EventChannel* eventInfo, const MidiMessage& event, int sampleNum)
{
    // Check if TTL event
    if (eventInfo->getChannelType() == EventChannel::TTL)
    {   // check if TTL from right channel
        TTLEventPtr ttl = TTLEvent::deserializeFromMessage(event, eventInfo);
        for (int n = 0; n < triggerChannels.size(); n++)
        { // Check which ttl event is triggered
            if (ttl->getChannel() == triggerChannels[n] && ttl->getState())
            {
                std::cout << "Putting this into buffer " << Event::getTimestamp(event) << std::endl;
                ttlTimestampBuffer[n].push_back(Event::getTimestamp(event)); // add timestamp of TTL to buffer
            }
        }
    }
}


Array<int> ProcessorPlugin::getActiveInputs()
{
    int numInputs = getNumInputs();
    auto ed = static_cast<ERPEditor*>(getEditor());

    if (numInputs == 0 || !ed)
    {
        return Array<int>();
    }

    Array<int> activeChannels = ed->getActiveChannels();
    return activeChannels;
}



void ProcessorPlugin::setParameter(int parameterIndex, float newValue)
{
    if (parameterIndex == ALPHA_E)
    {
        alpha = newValue;
        updateSettings();
    }
    else if (parameterIndex == ERP_LEN)
    {
        ERPLenSec = newValue;
        updateSettings();
    }
}

void ProcessorPlugin::saveCustomParametersToXml(XmlElement* parentElement)
{
    XmlElement* mainNode = parentElement->createNewChildElement("REALTIMEERP");

    // ------ Save Trigger Channels ------ //
    XmlElement* ttlNode = mainNode->createNewChildElement("ttl");

    for (int i = 0; i < triggerChannels.size(); i++)
    {
        ttlNode->setAttribute("ttl" + String(i), triggerChannels[i]);
    }

    // ------ Save Other Params ------ //
    mainNode->setAttribute("alpha", alpha);
    mainNode->setAttribute("ERPLen", ERPLenSec);
}

void ProcessorPlugin::loadCustomParametersFromXml()
{
 
    if (parametersAsXml)
    {
        forEachXmlChildElementWithTagName(*parametersAsXml, mainNode, "REALTIMEERP")
        {
            // Load trigger channels
            forEachXmlChildElementWithTagName(*mainNode, node, "ttl")
            {
                triggerChannels.clear();
                for (int i = 0; i < 8; i++)
                {
                    int channel = node->getIntAttribute("ttl" + String(i), -1);
                    if (channel != -1)
                    {
                        triggerChannels.addIfNotAlreadyThere(channel);
                    }
                    else
                    {
                        break;
                    }
                }       
            }
            // Load other params
            alpha = mainNode->getDoubleAttribute("alpha");
            ERPLenSec = mainNode->getDoubleAttribute("ERPLen");
        }
    }
    editor->update();
}