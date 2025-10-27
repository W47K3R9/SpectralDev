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
- [x] Rework Architecture
 
## Bugs

- [x] The Plugin can't be deleted without issueing an error to Logic. This might be because the thread is not stopped
and destruction of the instance freezes and waits infinitely...
Fix: Juce creates unique pointers out of all arguments in addParam() since I used a unique pointer too this resulted
in undefined behaviour.
- [ ] The Plugin does not playback sound after moving it back and forth from and to FX slots. It just stays silent,
at least unless you turn feedback up which is really strange! This needs some further investigation.

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
m_plugin_instance.process_samples(audio_buffer);
```

## Concurrency

In order to decouple the calculation of the spectral map from the actual processing of the output samples,
the FFT calculation is triggered concurrently to the rest of the program.

That has the consequence, that everything related to the tuning of the oscillators has to be protected and made thread
safe.

## Setup and function calls

-> This needs to be evaluated and checked.

1. Prepare to play: The DAW sends this signal right before transmitting the first buffers or on DAW config changes. 
2. Process buffer: The DAW provides the plugin with an audio stream (a buffer filled with floats)
  Further the update parameters block will be called sequentially in that function too.
3. Release resources: Called when unloading the plugin. Since I manage everything via RAII nothing has to be done here.
4. Reset: Can be overwritten and will stop any playback of ongoing sounds. Although the documentation says that this
  function *may* be called by the host and LogicPro doesn't call it...

All functions will (hopefully) only be called sequentially.

According to a first research the actual call sequence is as follows:
- prepareToPlay(maxBufferSize): called before Playback (according to some users depending on the DAW even several times 
- processBuffer(samples, numSamples): repeatedly called during Playback numSamples doesn't have to be the same as
  maxBufferSize.
- releaseResources(): no default behaviour, could be called after playback or when unloading (or never?). The plugin
  should be able to manage it's resources on destruction.

For me that means that I will have to focus on prepare to play and process buffer.

releaseResources can essentially be left empty because I release all resources during destructuion.

**Concrete setup requirements**

1. Prepare to play:
- Reset all buffers!
- Mute the oscillators!
- Set action done flags to true to trigger the first playback

2. Process buffer:
- Update parameters
- Will call the main processing loop that fills the calculation input buffer and the output buffer that the DAW
receives.
- Triggers calculation (and therefore checks for calculation sync primitives action done flag)

That means setting the action_done flags of the sync primitives to true may at worst be called twice which has no
negative side effects.


