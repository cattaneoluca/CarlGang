# FM Synthesizer controlled by hand movement
## Introduction
Implementation of a FM synthesizer, designed in **SUPERCOLLIDER**, controlled by tracking hand movements and visualization through geometric forms. 

### What is a FM synthesizer?

## SuperCollider Implementation


### Plug-in parameters
These are the parameters we chose for our Flanger plugin that apper in the GUI and could be modify by the user:
- **Feedforward** (Mix) (knob): It represents the amount of delayed signal that is mixed in with the
original one. Having a value of 0 means that we are considering only the
dry signal. Increasing this value (up to 1), one can adjust the balance
between the processed signal and the dry signal.

- **Feedback** (knob): Through this parameter it's possible to control, in practice, how much
of the output signal from the delay line we want to send back through
the device input. The range of possible values for this plugin is [0; 0:50].
We decided to set the end of the range at half of his theoretical maximum for personal tastes to reach a specific sound effect, because the
more it approaches 1 the more emerges a metallic sound due to the
sharpening of peaks and notches in the frequency response.
- **Delay** (knob): It lets the user adjusts the minimum amount of delay of the LFO, in a
range of [1:00; 5:00][ms]. For higher delay times our 
Flanger behaved as
a chorus, so we fixed the max at 5 ms.
- **LFO Width** (Sweep Width)  (knob): It allows the user to control the total amplitude of waveform of the
LFO, in a range of [1:00; 20:00][ms].
- **LFO Frequency** (Speed) (knob): The LFO frequency can be set in a range of [0:05; 2:00][Hz].
- **Shape of the LFO Envelope**  (Combo Box): It allows one to select which shape use for the LFO. For this plugin there
are three possible waveform shapes: Sine, Triangle and Sawtooth.
## Juce Implementation: the Audio Processor
The first thing we decided to implement is the **Value Tree State**, a class used
to manage all the parameters of the plugin, or so to say, the entire plugin's
state. It is very helpful in order to handle the connection between the objects
in the Editor and the Processor via the instantiation of specific classes called
*Attachments*, one for each type of graphical objects (i.e., slider, comboBox).
An identification string is used for retrieving a parameter, and by using the
Value Tree State, the post-condition of the get method ensures that the pa-
rameter is the newest value up to date with the user interface.

As a delay-based effect, the  Flanger is implemented using circular buffers,
which can be considered as FIFO buffers (First In First Out). The dimen-
sion of the buffer is fixed and it is large enough to fit the amount of the
maximum delay (sweep width + delay parameters) at any point in the LFO cycle. 

The actual length of delay at any time is controlled by the distance
between the read pointer and the write pointer in the buffer. The increase
or decrease of the delay is represented by the speed of the read pointer with
respect to the movement of the write pointer. If the read pointer moves faster
(slower), the amount of delay will decrease (increase).

In addition, we need to use low order polynomial interpolation to calculate
the output of the delay line, including the case in which the delay length,
expressed by the function M[n], is not an integer. First, we tried to imple-
ment a simple linear interpolation, but then we opted instead for the **cubic
interpolation** because the former was causing some artefacts. 
In either case, we left the linear interpolation commented in the code for a quick comparison.

In order to handle the multiple waveforms of the LFO, we deffined the wave
functions through a switch case in which the phase varies incrementally.
Given the wave functions, it easy to compute the current delay with the
user-defined parameters *width* and *delay* and hence the delay read pointer:

`float currentDelay = delay + sweepWidth * waveformFunction;`

### GUI Implementation
In order to create the knobs, we created two custom classes: *BlueKnob-
Style, MagentaKnobStyle* in the *PluginEditor.h* that use the juce method
*LookAndFeel V4*. 

The class draw the knob starting from a rotary slider,
drawing two concentric circles and rotating a rectangle using the method

`juce::AffineTransform`

Then we defined two elements: *blueKnob, mageKnob* of the classes in the
*AudioProcessorEditor* that gives the style defined to the slider using the
function *setLookAndFeel* in the editor compiler.

## Result and demo
Here is the [VST-3 file](https://github.com/EllDy96/CarlGang/blob/Homework2/Hw_2/Flanger/VST3/Flanger.vst3) that you can download If you want to try our plug-in in any DAW.

To recreate the famous *jet passing overhead* characteristic sound of the
Flanger, we recorded a clean guitar and applied some distortion (adding higher
frequencies enhance the effect). And finally, we fed the guitar recording to
delay with the following parameters:
- Feedforward = 1
- Feedback = 0:25
- Delay = 1 ms
- LFO Width = 4:5 ms
- LFO Freq = 0:1 Hz
- Waveform = sine

The result can be played at the following link: [Audio Demo](https://polimi365-my.sharepoint.com/:u:/g/personal/10751438_polimi_it/ESSG1VdlCZVMsWpJDyI5JisBTeKSS_7I16fRfVOw2sIelg?e=JC0Wi9)
or can be downloaded [here](https://github.com/EllDy96/CarlGang/blob/Homework2/Hw_2/flangerAudioTest.mp3).

Here below you can see the plug-in User Interface start-up window with all the
default parameters:

![User Interface](https://github.com/EllDy96/CarlGang/blob/Homework2/Hw_2/report%20HW2/ui.png)

For a further explanation please see the [report](https://github.com/EllDy96/CarlGang/blob/Homework2/Hw_2/report%20HW2/HW2.pdf).


