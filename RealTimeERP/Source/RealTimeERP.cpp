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
    , ttlTimestampBuffer({})
    , currentlyFilling(false)
    , ERPLenSec(1.0)
    , alpha(0)
    , avgERP(0,vector<RWA>(getNumOutputs()))
    , curSum(0,vector<float>(getNumOutputs()))
    //, avgLFP            (getNumOutputs(), vector<AudioSampleBuffer>()
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
            Array<int> activeChannels = getActiveInputs();
            int numChannels = activeChannels.size();
            if (numChannels <= 0)
            {
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
                curBufStartSamp = stimTime - bufTimestamp;
            }
            else if (stimTime + ERPLenSamps < bufTimestamp + nBufSamps) // last buffer, starts at 0, ends in middle
            {
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
                    float val = rpIn[samp];
                    avgLFP[t][chan][samp].addValue(val); // Add to avg waveform
                    curSum[t][chan] += abs(val); // Add to area under curve sum
                }
                if (done)
                {
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
                ttlTimestampBuffer[n].push_back(Event::getTimestamp(event)); // add timestamp of TTL to buffer
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


bool ProcessorPlugin::hasEditor() const
{
    return true;
}



void ProcessorPlugin::setParameter(int parameterIndex, float newValue)
{

}

void ProcessorPlugin::saveCustomParametersToXml(XmlElement* parentElement)
{

}

void ProcessorPlugin::loadCustomParametersFromXml()
{

}