//This prevents include loops. We recommend changing the macro to a name suitable for your plugin
#ifndef ERP_H_DEFINED
#define ERP_H_DEFINED

#include <ProcessorHeaders.h>
#include <VisualizerEditorHeaders.h>
#include <string>
#include <vector>

#include "AtomicSynchronizer.h"
#include "CircularArray.h"

//namespace must be an unique name for your plugin
namespace RealTimeERP
{
    struct RealWeightedAccum
    {
        RealWeightedAccum(double alpha = 0)
            : count(0.0)
            , sum(0)
            , alpha(alpha)
        {}

        double getAverage() const
        {
            //std::cout << " GET AVG SUM " << sum << " and COUNT " << count << std::endl;
            return count > 0 ? sum / (double)count : double();
        }

        void addValue(double x)
        {
            sum = x + (1 - alpha) * sum;
            count = 1 + (1 - alpha) * count;
            //std::cout << "SUM " << sum << " and COUNT " << count << std::endl;
        }

    private:
        double sum;
        size_t count;

        double alpha;
    };
    using RWA = RealWeightedAccum;

    struct EventSources
    {
        unsigned int eventIndex;
        unsigned int channel;
        String name;

        bool operator == (const EventSources& ES1) const
        {
            if (eventIndex == ES1.eventIndex && channel == ES1.channel && name == ES1.name)
            {
                return true;
            }
            else
            {
                return false;
            }    
        }      
    };


    // Main Node Class
	class Node : public GenericProcessor
	{
    friend class ERPEditor;
    friend class ERPVisualizer;

    template<typename T>
    using vector = std::vector<T>;  

	public:
		/** The class constructor, used to initialize any members. */
        Node();

		/** The class destructor, used to deallocate memory */
        ~Node();

		/** Indicates if the processor has a custom editor. Defaults to false */
		bool hasEditor() const { return true; }

		/** If the processor has a custom editor, this method must be defined to instantiate it. */
		AudioProcessorEditor* createEditor() override;

		/** Optional method that informs the GUI if the processor is ready to function. If false acquisition cannot start. Defaults to true */
		//bool isReady();

		/** Defines the functionality of the processor.

		The process method is called every time a new data buffer is available.

		Processors can either use this method to add new data, manipulate existing
		data, or send data to an external target (such as a display or other hardware).

		Continuous signals arrive in the "buffer" variable, event data (such as TTLs
		and spikes) is contained in the "events" variable.
		*/
		void process(AudioSampleBuffer& buffer) override;

		/** Handles events received by the processor

		Called automatically for each received event whenever checkForEvents() is called from process()		
		*/
		void handleEvent(const EventChannel* eventInfo, const MidiMessage& event, int samplePosition) override;

		/** Handles spikes received by the processor

		Called automatically for each received event whenever checkForEvents(true) is called from process()
		*/
		//void handleSpike(const SpikeChannel* spikeInfo, const MidiMessage& event, int samplePosition) override;

		/** The method that standard controls on the editor will call.
		It is recommended that any variables used by the "process" function
		are modified only through this method while data acquisition is active. */
		void setParameter(int parameterIndex, float newValue) override;

		/** Saving custom settings to XML. */
		void saveCustomParametersToXml(XmlElement* parentElement) override;

		/** Load custom settings from XML*/
		void loadCustomParametersFromXml() override;

		/** Optional method called every time the signal chain is refreshed or changed in any way.

		Allows the processor to handle variations in the channel configuration or any other parameter
		passed down the signal chain. The processor can also modify here the settings structure, which contains
		information regarding the input and output channels as well as other signal related parameters. Said
		structure shouldn't be manipulated outside of this method.

		*/
		void updateSettings() override;

        Array<int> Node::getActiveInputs();

    private:
        float fs;
        //Array<int> triggerChannels;
        vector<vector<int64>> ttlTimestampBuffer;

        vector<AudioSampleBuffer> curLFP; // Is this better???
        // Calculations to send to visualizer
        AtomicallyShared<vector<vector<RWA>>> avgSum; // Average area under curve (trigger(ttl 1-8) x channel)
        AtomicallyShared<vector<vector<vector<RWA>>>> avgLFP; // Save the average waveform (trigger(ttl 1-8) x channel x vector of waveform)
        AtomicallyShared<vector<vector<RWA>>> avgPeak; // Avg Peak height (trigger(ttl 1-8) x channel)
        AtomicallyShared<vector<vector<RWA>>> avgTimeToPeak; // Avg time to the peak height (trigger(ttl 1-8) x channel)

        vector<vector<RWA>> localAvgSum;
        vector<vector<vector<RWA>>> localAvgLFP;
        vector<vector<RWA>> localAvgPeak;
        vector<vector<RWA>> localAvgTimeToPeak;

        float ERPLenSec;
        float ERPLenSamps;
        vector<uint64> curSamp;
        float alpha;

        Array<EventSources> triggerChannels;
        Array<EventSources> eventSourceArray;
        
        enum Parameter
        {
            ALPHA_E,
            ERP_LEN
        };
	};
}

#endif