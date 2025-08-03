# Spectral Madness Development

This repository contains the source code for the LBTS Spectral Madness (concept name) Plug-In.
The PlugIn is written with the C++20 Standard and uses some of it's functionality.
Since this is an Audio PlugIn and not a code library, some OOP concepts like getters / setters
and strict seperation of public and private attributes are not followed all too strictly...

## Logic

The logic of the audio plugin is mainly located in the [inc](./inc/) folder since until now (02.08.2025) all classes
are templatized and therefore completely defined in their header.

## Todo's

- [ ] Parallelize the Oscillators. In order to save a few resources, the output of the oscillators could be calculated
concurrently for a few groups of oscillators.
- [ ] Implement MIDI listening for tempo to trigger the FFT calculation according to the current tempo

## Bugs

- [x] The Plugin can't be deleted without issueing an error to Logic. This might be because the thread is not stopped
and destruction of the instance freezes and waits infinitely...
Fix: Juce creates unique pointers out of all arguments in addParam() since I used a unique pointer too this resulted
in undefined behaviour.

