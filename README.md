# Spectral Madness Development

This repository contains the source code for the LBTS Spectral Madness (concept name) Plug-In.
The PlugIn is written with the C++20 Standard and uses some of it's functionality.
Since this is an Audio PlugIn and not a code library, some OOP concepts like getters / setters
and strict seperation of public and private attributes are not followed all too strictly...

## Logic

The logic of the audio plugin is mainly located in the [inc](./inc/) folder since until now (02.08.2025) all classes
are templatized and therefore completely defined in their header.

## Todo's

- [x] Parallelize the Oscillators. In order to save a few resources, the output of the oscillators could be calculated
concurrently for a few groups of oscillators. IGNORE -> performance killer
- [ ] Implement MIDI listening for tempo to trigger the FFT calculation according to the current tempo
- [ ] Rework Architecture
 
## Bugs

- [x] The Plugin can't be deleted without issueing an error to Logic. This might be because the thread is not stopped
and destruction of the instance freezes and waits infinitely...
Fix: Juce creates unique pointers out of all arguments in addParam() since I used a unique pointer too this resulted
in undefined behaviour.

## Architecture

DAW -> sample buffers -> buffer manager -> FFT -> Map -> Oscillators -> gain -> sample buffers -> DAW


The Plugin Parameters could be implemented as a struct that is passed to a central component.
A function within Juce could than be called to update the parameters on each buffer reception.

```cpp
// function in Juce
PluginParameters make_params_for_current_step()
{
  //...
}

// in main processing call
const auto params = make_params_for_current_step();
update_parameters(params);
// ...
process_samples(audio_buffer);
```

Alternatively PluginParameters could be an internal object in the Juce Framework.

```cpp
// function in Juce
// could be boolean if something can go wrong
void update_params_for_current_step(PluginParameters& params) 
{
  //...
}

// in main processing call
update_params_for_current_step(m_plugin_params);
m_plugin_instance.update_parameters(m_plugin_params);
// ...
process_samples(audio_buffer);
```

