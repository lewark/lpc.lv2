# lpc.lv2
LPC analysis + synthesis LV2 plugin for glitchy/robotic vocal effects, built on rt_lpc

![Screenshot of plugin](https://github.com/knector01/lpc.lv2/blob/main/lpc-screenshot.png?raw=true)

## About

Linear predictive coding (LPC) is an algorithm used to approximate audio signals like human speech. The algorithm generates an excitation signal (such as a pulse wave) that then passes through a filter to emulate the vocal tract. Because it can represent speech signals quite efficiently, LPC has seen wide use in speech synthesis and audio compression. Additionally, due to its distinctive sound, LPC has been used in popular music to create robotic-sounding voice effects.

This plugin aims to serve that last use case by providing an LPC engine for use in compatible DAW software. The plugin itself is simply an LV2 wrapper for rt\_lpc, which was written by Misra et al. Detailed credits are included at the end of this README.

For more information on LPC, I recommend the following resources:

* https://en.wikipedia.org/wiki/Linear_predictive_coding
* https://ccrma.stanford.edu/~hskim08/lpc/

I found the CCRMA page especially helpful for understanding the algorithm.

## Compiling

This plugin is intended to be compiled and used on Linux. Other platforms have not been tested.

Dependencies:

* lv2-dev
* build-essential
* libcairo2-dev
* libpango1.0-dev

(there are possibly more that I've forgotten)

To download the source code, compile it, and then install it, run the following commands in a terminal:

```
git clone --recurse-submodules https://github.com/knector01/lpc.lv2
cd lpc.lv2
make
make install
```

By default, the Makefile installs the plugin to the following path:

```
~/.lv2/lpc.lv2
```

## Parameters

### Order
The order of the filter generated by the LPC engine. Higher values can approximate the frequencies of the original sound more closely, but also incur a higher performance cost, so try lowering this setting if popping/xruns occur.

#### ⚠️ Warning ⚠️
If the Order parameter is set high enough, then **loud, high-pitched** resonances can show up. As a result, I **highly** recommend adding a peak limiter after this plugin in your DAW.

### Buffer Size
Sets the size of the audio buffers used by the LPC engine. Higher values give higher quality output for lower-pitched voices but also significantly increase the performance cost. Try turning this up if the pitched part of the voice is being synthesized as noise. Conversely, if popping/xruns occur then try lowering this setting.

### Whisper
Forces all audio to be treated as unpitched, which causes the excitation signal to always be noise. This creates a whisper-like effect.

### OLA
Enables/disables Overlap-Add (OLA) mode. Can make the audio sound smoother but can also cause some dissonance when used with singing. Also increases the CPU load of the plugin significantly. Works better at higher buffer sizes.

### Glottal Pulse
Instead of using a pulse wave as the LPC excitation signal, use a glottal pop sample. Can make the resulting audio sound slightly more realistic.

### Preemphasis
Emphasizes the input signal using a high-pass filter prior to running the LPC analysis. Also de-emphasizes the signal output from the LPC synthesizer afterwards.

### Pitch Shift
Pitch-shifts the output audio up or down. Can create some neat effects. This setting does nothing if Whisper is enabled.

### MIDI Tuning
Sets the root frequency of the MIDI note scale. (default A440)

### MIDI Bend Range
Sets the range of the MIDI pitch bend.

## TODO

rt_lpc features to re-implement:
* GUI that shows filter response (possibly also show identified pitch): In progress, but very unfinished

Ideas of other possible features:
* Add settings that force unpitched signals to be pitched, or mute them entirely. Maybe add separate gain controls for pitched/unpitched.
* Add a slider to change the window overlap amount to reduce the dissonance when OLA mode is used on singing audio.
* Try other excitation waveforms (square, sawtooth, etc)
* Try other pitch identification algorithms (e.g. FFT-based rather than autocorrelation)
* Allow use of the plugin as a vocoder(?) by swapping out the excitation waveform with an arbitrary audio input (generally a synth instrument)
* Add MIDI portamento
* Try to improve performance. Currently at extreme settings the plugin can attempt to process 4096 samples of data within a time window intended for only 512 samples (the default JACK buffer size), which leads to performance problems.

## Credits

This plugin is heavily based on rt\_lpc, which was written by Ananya Misra, Ge Wang, and Perry R. Cook. For more information, see the rt_lpc homepage at the Princeton Sound Lab, and its wiki page:

* https://soundlab.cs.princeton.edu/software/rt_lpc/
* http://wiki.cs.princeton.edu/index.php/RT_LPC

Additionally, much this plugin's MIDI and GUI code of this plugin was based on code from x42-autotune and Simple Scope by Robin Gareus, and the UI uses Gareus' robtk toolkit.

* https://github.com/x42/fat1.lv2
* https://github.com/x42/sisco.lv2
* https://github.com/x42/robtk

The example plugins from the LV2 project were also instrumental during development, especially the eg-amp and eg-midigate examples written by David Robillard, and the eg-scope example that Simple Scope was built on.

* https://lv2plug.in/book
* https://github.com/lv2/lv2

Thanks!
