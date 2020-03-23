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

// TODO : ADD IN ATOMIC SYNC TO SEND DATA TO VIS TO GET PAINTED
// Update peak time to ms????

#include "RealTimeERP.h"
#include "RealTimeERPEditor.h"

using namespace RealTimeERP;

Node::Node()
    : GenericProcessor("Real Time ERP")
    , triggerChannels   ({})
    //, ttlTimestampBuffer({})
    , ERPLenSec         (1.0)
    , alpha             (0)
    , curLFP            ({})
    , curSamp           ({})
    , resetBuffer       (false)
    //, avgLFP            ({})//(0, vector<vector<RWA>>(0, vector<RWA>(0)))
    //, avgSum            ({})//(0,vector<RWA>(0))
    //, avgPeak           ({})//(0, vector<RWA>(0, RWA(0)))
    //, avgTimeToPeak     ({})//(0, vector<RWA>(0,RWA(0)))
{
    setProcessorType(PROCESSOR_TYPE_SINK);
}

Node::~Node() {}

AudioProcessorEditor* Node::createEditor()
{
    editor = new ERPEditor(this);
    return editor;
}

void Node::updateSettings()
{
    // Things got updated, reset vector sizes based on new data
    fs = GenericProcessor::getSampleRate();
    ERPLenSamps = fs * ERPLenSec;

    activeChannels = getActiveInputs();
    numChannels = activeChannels.size();

    int numTriggers = triggerChannels.size();

    // Init ttlTimestampBuffer to empty deques
    ttlTimestampBuffer = std::vector<std::vector<int64>>(numTriggers, std::vector<int64>(5));
    
    //std::fill(ttlTimestampBuffer.begin(), ttlTimestampBuffer.end(), std::vector<int64>(5));
    for (int i = 0; i < numTriggers; i++)
    {
        ttlTimestampBuffer[i].clear();
    }

    // Make a new audio sample buffer for each ttl trigger
    curLFP = std::vector<AudioSampleBuffer>(numTriggers, AudioSampleBuffer(numChannels, ERPLenSamps));
    curSamp = std::vector<uint64>(numTriggers, 0);
    
    localAvgLFP = vector<vector<vector<RWA>>>(numTriggers, vector<vector<RWA>>(numChannels, vector<RWA>(ERPLenSamps, RWA(alpha))));
    avgLFP.map([=](vector<vector<vector<RWA>>>& vec)
        {
            vec = vector<vector<vector<RWA>>>(numTriggers, vector<vector<RWA>>(numChannels, vector<RWA>(ERPLenSamps, RWA(alpha))));
        });

    localAvgSum = vector<vector<RWA>>(numTriggers, vector<RWA>(numChannels, RWA(alpha)));
    avgSum.map([=](vector<vector<RWA>>& vec)
        {
            vec = vector<vector<RWA>>(numTriggers, vector<RWA>(numChannels, RWA(alpha)));
        });

    localAvgPeak = vector<vector<RWA>>(numTriggers, vector<RWA>(numChannels, RWA(alpha)));
    avgPeak.map([=](vector<vector<RWA>>& vec)
        {
            vec = vector<vector<RWA>>(numTriggers, vector<RWA>(numChannels, RWA(alpha)));
        });

    localAvgTimeToPeak = vector<vector<RWA>>(numTriggers, vector<RWA>(numChannels, RWA(alpha)));
    avgTimeToPeak.map([=](vector<vector<RWA>>& vec)
        {
            vec = vector<vector<RWA>>(numTriggers, vector<RWA>(numChannels, RWA(alpha)));
        });

    // Populate Event sources
    EventSources s;
    String name;
    eventSourceArray.clear();
    int nEvents = getTotalEventChannels();
    for (int chan = 0; chan < nEvents; chan++)
    {
        const EventChannel* event = getEventChannel(chan);
        if (event->getChannelType() == EventChannel::TTL)
        {
            s.eventIndex = chan;
            int nChans = event->getNumChannels();
            for (int c = 0; c < nChans; c++)
            {
                s.channel = c;
                name = event->getSourceName() + " (TTL" + String(c + 1) + ")";
                s.name = name;
                eventSourceArray.addIfNotAlreadyThere(s);
            }
        }
    }
}

void Node::process(AudioSampleBuffer& buffer)
{
	checkForEvents(false); // Check for ttl events
    AtomicScopedWritePtr<vector<vector<RWA>>> sumWriter(avgSum); 
    AtomicScopedWritePtr<vector<vector<vector<RWA>>>> LFPWriter(avgLFP);
    AtomicScopedWritePtr<vector<vector<RWA>>> peakWriter(avgPeak);
    AtomicScopedWritePtr<vector<vector<RWA>>> ttPeakWriter(avgTimeToPeak);
    if (!sumWriter.isValid() && !LFPWriter.isValid() && !peakWriter.isValid() && !peakWriter.isValid())
    {
        std::cout << "Not valid writers" << std::endl;
        jassertfalse; // atomic sync data writer broken
    }
    // If new TTL timestamp to handle (stim happened), timestamp is removed once calculation is complete
    for (int t = 0; t < triggerChannels.size(); t++)
    {
        if (!ttlTimestampBuffer[t].empty())
        {
            // Make sure we have input
            if (numChannels <= 0)
            {
                ttlTimestampBuffer[t].erase(ttlTimestampBuffer[t].begin());
                jassertfalse;
                return;
            }

            // Get timestamp of trigger 
            uint64 stimTime = ttlTimestampBuffer[t].front();
            uint64 nBufSamps = getNumSamples(activeChannels[0]);
            uint64 bufTimestamp = getTimestamp(activeChannels[0]);
            // 0/110 for 
            uint64 networkTS = getSourceTimestamp(110, 0);
            uint64 curBufStartSamp = 0; // Middle buffers use all ...
            uint64 curBufEndSamp = nBufSamps; // samples in buffer
            bool done = false;
            // first buffer, starts at stim, ends at end of buffer
            if (stimTime > bufTimestamp || (stimTime < bufTimestamp && curSamp[t] == 0)) 
            {
                if (stimTime > bufTimestamp)
                {
                    curBufStartSamp = stimTime - bufTimestamp;
                }
                else
                {
                    ttlTimestampBuffer[t][0] = bufTimestamp;
                }
                curSamp[t] = 0; // Start at beginning for the saved buffer, new data
            }
            
            // last buffer, starts at 0, ends in middle of buffer
            else if (stimTime + ERPLenSamps < bufTimestamp + nBufSamps) 
            {
                curBufEndSamp = stimTime + ERPLenSamps - bufTimestamp;
                ttlTimestampBuffer[t].erase(ttlTimestampBuffer[t].begin());
                done = true;
            }
            uint64 curNSamps = curBufEndSamp - curBufStartSamp;
            // Loop through each channel and fill current audio buffer
            for (int n = 0; n < numChannels; n++)
            {  
                int chan = activeChannels[n];
                curLFP[t].copyFrom(n, curSamp[t], buffer, chan, curBufStartSamp, curNSamps);

                // Buffer filled, compute!
                // Thread this? Can't imagine it's too difficult to compute though?
                if (done)
                {
                    // Get read pointer for curLFP
                    const float* rpIn = curLFP[t].getReadPointer(n);
                    
                    // Get our peak and sum by looping through buffer
                    double curSum = 0;
                    double curPeak = 0;
                    uint64 curTimeToPeak = 0;
                    for (uint64 samp = 0; samp < ERPLenSamps; samp++)
                    {
                        curSum += abs(rpIn[samp]);
                        localAvgLFP[t][n][samp].addValue(rpIn[samp]);
                        // Probably don't want the entire ERPLen samps for peak hmmm
                        // Need to look at if this correct.
                        if (curPeak <= abs(rpIn[samp])) 
                        {
                            curPeak = abs(rpIn[samp]);
                            curTimeToPeak = samp;
                        }
                    }

                    // Update values
                    localAvgSum[t][n].addValue(curSum);
                    
                    localAvgPeak[t][n].addValue(curPeak);
                    localAvgTimeToPeak[t][n].addValue(curTimeToPeak);
                }
            }
            if (!done)
            {
                curSamp[t] += curNSamps; // Keep track of where we are in our saved buffer
            }
            else
            {
                // Send to Vis!

                sumWriter->assign(localAvgSum.begin(), localAvgSum.end());
                peakWriter->assign(localAvgPeak.begin(), localAvgPeak.end());
                ttPeakWriter->assign(localAvgTimeToPeak.begin(), localAvgTimeToPeak.end());
                LFPWriter->assign(localAvgLFP.begin(), localAvgLFP.end());

                LFPWriter.pushUpdate();
                sumWriter.pushUpdate();
                peakWriter.pushUpdate();
                ttPeakWriter.pushUpdate();
                std::cout << "Pushed update!" << std::endl;

                curSamp[t] = 0;
                if (resetBuffer)
                {
                    int numTriggers = triggerChannels.size();
                    for (int trig = 0; trig < numTriggers; trig++)
                    {
                        for (int chan = 0; chan < numChannels; chan++)
                        {
                            localAvgSum[trig][chan].reset();
                            localAvgPeak[trig][chan].reset();
                            localAvgTimeToPeak[trig][chan].reset();
                            for (int samp = 0; samp < ERPLenSamps; samp++)
                            {
                                localAvgLFP[trig][chan][samp].reset();
                            }
                        }
                    }  
                }
            }
        }
    }  
}

void Node::handleEvent(const EventChannel* eventInfo, const MidiMessage& event, int sampleNum)
{
    // Check if TTL event
    if (eventInfo->getChannelType() == EventChannel::TTL)
    {
       // Loop through watched events
        for (int n = 0; n < triggerChannels.size(); n++)
        {
            // Check if Event is from the correct processor
            if (eventInfo == eventChannelArray[triggerChannels[n].eventIndex])
            {
                
                // check if TTL from right channel
                TTLEventPtr ttl = TTLEvent::deserializeFromMessage(event, eventInfo);
                if (ttl->getChannel() == triggerChannels[n].channel && ttl->getState())
                {
                    std::cout << "Got an event from " << ttl->getChannel() << std::endl;
                    ttlTimestampBuffer[n].push_back(Event::getTimestamp(event)); // add timestamp of TTL to buffer
                }
            }
        }
    }
}

void Node::resetVectors()
{
    updateSettings();
}

void Node::setInstOrAvg(bool instOrAvg)
/*
    Sending a 0 has the plugin save the average of ERPs over time.
    Sending a 1 has the plugin only show data for the most recent Event.
*/
{
    resetBuffer = instOrAvg ? true : false;
    if (resetBuffer)
    {
        int numTriggers = triggerChannels.size();
        for (int trig = 0; trig < numTriggers; trig++)
        {
            for (int chan = 0; chan < numChannels; chan++)
            {
                localAvgSum[trig][chan].reset();
                localAvgPeak[trig][chan].reset();
                localAvgTimeToPeak[trig][chan].reset();
                for (int samp = 0; samp < ERPLenSamps; samp++)
                {
                    localAvgLFP[trig][chan][samp].reset();
                }
            }
        }
    }
}

Array<int> Node::getActiveInputs()
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



void Node::setParameter(int parameterIndex, float newValue)
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

void Node::saveCustomParametersToXml(XmlElement* parentElement)
{
    
    XmlElement* mainNode = parentElement->createNewChildElement("REALTIMEERP");

    // ------ Save Trigger Channels ------ //
    XmlElement* ttlNode = mainNode->createNewChildElement("ttl");
    
    for (int i = 0; i < triggerChannels.size(); i++)
    {
        int triggerIndex = eventSourceArray.indexOf(triggerChannels[i]);
        ttlNode->setAttribute("ttl" + String(i), triggerIndex);
    }

    // ------ Save Other Params ------ //
    mainNode->setAttribute("alpha", alpha);
    mainNode->setAttribute("ERPLen", ERPLenSec);
}

void Node::loadCustomParametersFromXml()
{
 
    if (parametersAsXml)
    {
        forEachXmlChildElementWithTagName(*parametersAsXml, mainNode, "REALTIMEERP")
        {
            // Load trigger channels
            forEachXmlChildElementWithTagName(*mainNode, node, "ttl")
            {
                triggerChannels.clear();
                for (int i = 0; i < node->getNumAttributes(); i++)
                {
                    int index = node->getIntAttribute("ttl" + String(i), -1);
                    if (index != -1)
                    {
                        triggerChannels.addIfNotAlreadyThere(eventSourceArray[index]);      
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