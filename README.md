# Spectral Madness Development

This repository contains the source code for the LBTS Spectral Madness (concept name) Plug-In.
The PlugIn is written with the C++20 Standard and uses some of it's functionality.
Since this is an Audio PlugIn and not a code library, some OOP concepts like getters / setters
and strict seperation of public and private attributes are intentionally violated.

The folder structure is as follows:

```
.
├── CMakeLists.txt
├── README.md
├── extern
├── inc
│   ├── BufferSizeManager.h
│   ├── ControlPanel.h
│   ├── FourierMap.h
│   ├── Oscillator.h
│   ├── SpectralDomainSpec.h
│   ├── SpectralMain.h
│   ├── SpectralProcessor.h
│   └── VoiceManager.h
├── main.cpp
├── src
├── test
└── tools
    └── build-n-run.sh
```
