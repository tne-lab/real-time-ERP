# Real Time ERP
This repository contains a plugin for the [Open Ephys GUI](https://github.com/open-ephys/plugin-GUI). This plugin provides the user with the ability to visualize and calculate evoked potentials in real time from a variety of event sources. 

Cite this code using the doi above!

![Canvas](canvas.png "Visualizer")
![Editor](editor.png "Editor")

## Settings
For every event source chosen, the **average** for the following four properties will be saved. 
- **Waveform** - Always displays the average waveform of the current event source.
- **Area under curve**
- **Peak height** 
- **Time to the peak** 

Using the editor, the user can determine how long the *window of interest* is after the event is received. The user can also choose whether the moving average will have *linear or exponential decay*. 

The visualizer allows the selection of which event source to view and what calculation to display.


## Installation using CMake

This plugin is to be built outside of the main GUI file tree using CMake. In order to do so, it must be in a sibling directory to plugin-GUI\* and the main GUI must have already been compiled.

See `RealTimeERP/CMAKE_README.txt` and/or the wiki page [here](https://open-ephys.atlassian.net/wiki/spaces/OEW/pages/1259110401/Plugin+CMake+Builds) for build instructions.

\* If you have the GUI built somewhere else, you can specify its location by setting the environment variable `GUI_BASE_DIR` or defining it when calling cmake with the option `-DGUI_BASE_DIR=<location>`.


Currently maintained by Mark Schatza (markschatza@gmail.com)
