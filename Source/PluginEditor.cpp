#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
constexpr auto background = 0xff141413;
constexpr auto panel = 0xff222320;
constexpr auto outline = 0xff55564c;
constexpr auto blue = 0xffd9b36a;
constexpr auto red = 0xffeb565e;
constexpr auto silver = 0xffeee8d8;
constexpr auto muted = 0xffb2ae9f;
}

ANKSModularEditor::ANKSModularEditor (ANKSModularProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      jacks {{
          {anks::Port::seqPitch, {262.f, 365.f}, "PITCH OUT", 1},
          {anks::Port::seqGate, {318.f, 365.f}, "GATE OUT", 2},
          {anks::Port::oscPitch, {372.f, 365.f}, "PITCH IN", 1},
          {anks::Port::oscOut, {507.f, 365.f}, "AUDIO OUT", 0},
          {anks::Port::filterIn, {550.f, 365.f}, "AUDIO IN", 0},
          {anks::Port::filterOut, {705.f, 365.f}, "AUDIO OUT", 0},
          {anks::Port::filterCV, {614.f, 365.f}, "CUTOFF CV", 3},
          {anks::Port::vcaIn, {752.f, 365.f}, "AUDIO IN", 0},
          {anks::Port::vcaCV, {808.f, 365.f}, "AMP CV", 3},
          {anks::Port::vcaOut, {876.f, 365.f}, "AUDIO OUT", 0},
          {anks::Port::envGate, {45.f, 599.f}, "GATE IN", 2},
          {anks::Port::envCV, {407.f, 599.f}, "ENV CV", 3},
          {anks::Port::lfoCV, {672.f, 599.f}, "LFO CV", 3},
          {anks::Port::outputIn, {926.f, 365.f}, "AUDIO IN", 0},
          {anks::Port::osc2Pitch, {731.f, 599.f}, "PITCH IN", 1},
          {anks::Port::osc2Out, {866.f, 599.f}, "AUDIO OUT", 0},
          {anks::Port::mixerIn1, {930.f, 599.f}, "INPUT 1", 0},
          {anks::Port::mixerIn2, {995.f, 599.f}, "INPUT 2", 0},
          {anks::Port::mixerOut, {1070.f, 599.f}, "MIX OUT", 0},
          {anks::Port::drumOut, {604.f, 901.f}, "DRUM OUT", 0},
          {anks::Port::drumMixIn, {1080.f, 365.f}, "DRUM IN", 0}
      }}
{
    setResizable (true, true);
    setResizeLimits (784, 665, 1344, 1140);
    getConstrainer()->setFixedAspectRatio (1120.0 / 950.0);
    setSize (1120, 950);
    setLookAndFeel (&skin);
    skin.setColour (juce::Slider::thumbColourId, juce::Colour (blue));
    skin.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (blue));
    skin.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (outline));
    skin.setColour (juce::Slider::textBoxTextColourId, juce::Colour (silver));
    skin.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (background));
    skin.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    skin.setColour (juce::ComboBox::backgroundColourId, juce::Colour (background));
    skin.setColour (juce::ComboBox::textColourId, juce::Colour (silver));
    skin.setColour (juce::ComboBox::outlineColourId, juce::Colour (outline));
    skin.setColour (juce::TextButton::buttonColourId, juce::Colour (panel));
    skin.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff225b7e));
    skin.setColour (juce::TextButton::textColourOffId, juce::Colour (silver));
    skin.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    skin.setColour (juce::ToggleButton::tickColourId, juce::Colour (blue));

    runButton.setButtonText ("RUN / STOP");
    runButton.setClickingTogglesState (true);
    runButton.setBounds (22, 39, 124, 31);
    addAndMakeVisible (runButton);
    runAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.parameters, "run", runButton);
    seqButton.setButtonText ("SEQ / MIDI");
    seqButton.setClickingTogglesState (true);
    seqButton.setBounds (155, 39, 124, 31);
    addAndMakeVisible (seqButton);
    seqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.parameters, "seqMode", seqButton);
    previewButton.setButtonText ("TEST NOTE");
    previewButton.setBounds (703, 39, 115, 31);
    previewButton.onClick = [this] { processor.requestPreview(); };
    addAndMakeVisible (previewButton);
    resetButton.setButtonText ("RESET CABLES");
    resetButton.setBounds (828, 39, 145, 31);
    resetButton.onClick = [this] { processor.resetPatch(); selectedJack = -1; repaint(); };
    addAndMakeVisible (resetButton);

    addKnob ("bpm", "BPM", 310, 23, 84, 62);
    // Eight note steps, each with a switch. A slider changes semitone offset from C3.
    for (int i = 0; i < 8; ++i)
    {
        auto slider = std::make_unique<juce::Slider>();
        slider->setSliderStyle (juce::Slider::LinearVertical);
        slider->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider->setBounds (35 + i * 37, 177, 28, 126);
        slider->setColour (juce::Slider::trackColourId, juce::Colour (blue));
        slider->setColour (juce::Slider::backgroundColourId, juce::Colour (outline));
        slider->setTooltip ("Step " + juce::String (i + 1) + " / semitones relative to C3");
        addAndMakeVisible (*slider);
        stepAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                                  (processor.parameters, "note" + juce::String (i + 1), *slider));
        stepSliders[static_cast<size_t> (i)] = std::move (slider);
        auto on = std::make_unique<juce::ToggleButton>();
        on->setBounds (40 + i * 37, 332, 20, 20);
        on->setTooltip ("Enable step " + juce::String (i + 1));
        addAndMakeVisible (*on);
        stepButtonAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
                                         (processor.parameters, "on" + juce::String (i + 1), *on));
        stepButtons[static_cast<size_t> (i)] = std::move (on);
    }

    wave.addItemList ({"SINE", "TRIANGLE", "SAW", "SQUARE"}, 1);
    wave.setBounds (365, 170, 148, 25);
    addAndMakeVisible (wave);
    waveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.parameters, "wave", wave);
    wave2.addItemList ({"SINE", "TRIANGLE", "SAW", "SQUARE"}, 1);
    wave2.setBounds (719, 467, 160, 25);
    addAndMakeVisible (wave2);
    wave2Attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.parameters, "wave2", wave2);
    addKnob ("tune2", "SEMITONES", 713, 498, 82, 83);
    addKnob ("fine2", "FINE / CENTS", 801, 498, 82, 83);
    addKnob ("mix1", "IN 1", 913, 475, 60, 106);
    addKnob ("mix2", "IN 2", 975, 475, 60, 106);
    addKnob ("mixLevel", "OUT", 1037, 475, 60, 106);

    addKnob ("tune", "TUNE", 400, 214, 85, 105);
    addKnob ("cutoff", "CUTOFF", 549, 165, 98, 135);
    addKnob ("res", "RESONANCE", 646, 165, 76, 135);
    addKnob ("level", "LEVEL", 764, 194, 110, 135);
    addKnob ("master", "MASTER", 958, 196, 100, 128);
    addKnob ("attack", "ATTACK", 49, 466, 80, 114);
    addKnob ("decay", "DECAY", 137, 466, 80, 114);
    addKnob ("sustain", "SUSTAIN", 230, 466, 80, 114);
    addKnob ("release", "RELEASE", 325, 466, 82, 114);
    addKnob ("lfoRate", "RATE", 469, 466, 90, 114);
    addKnob ("lfoDepth", "DEPTH", 574, 466, 90, 114);

    drumRunButton.setButtonText ("DRUM RUN");
    drumRunButton.setClickingTogglesState (true);
    drumRunButton.setBounds (250, 658, 100, 29);
    addAndMakeVisible (drumRunButton);
    drumRunAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.parameters, "drumRun", drumRunButton);
    drumTriggerButton.setButtonText ("HIT");
    drumTriggerButton.setBounds (359, 658, 80, 29);
    drumTriggerButton.onClick = [this] { processor.requestDrumTrigger(); };
    addAndMakeVisible (drumTriggerButton);
    drumFollowButton.setButtonText ("LINK BPM");
    drumFollowButton.setClickingTogglesState (true);
    drumFollowButton.setTooltip ("Use the core BPM. DRUM RUN remains independent; this does not sync to the DAW transport.");
    drumFollowButton.setBounds (448, 658, 110, 29);
    addAndMakeVisible (drumFollowButton);
    drumFollowAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.parameters, "drumFollow", drumFollowButton);
    drumWave.addItemList ({"TRIANGLE", "SQUARE"}, 1);
    drumWave.setBounds (570, 658, 112, 29);
    addAndMakeVisible (drumWave);
    drumWaveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.parameters, "drumWave", drumWave);

    addKnob ("drumBpm", "FREE BPM", 30, 715, 84, 82);
    addKnob ("drumPitch", "BASE Hz", 126, 715, 84, 82);
    addKnob ("drumRatio", "VCO2 RATIO", 222, 715, 84, 82);
    addKnob ("drumTone", "VCO2 MIX", 318, 715, 84, 82);
    addKnob ("drumNoise", "NOISE", 414, 715, 84, 82);
    addKnob ("drumFm", "FM", 510, 715, 84, 82);
    addKnob ("drumDecay", "AMP DECAY", 30, 810, 84, 82);
    addKnob ("drumPitchDecay", "PITCH DECAY", 126, 810, 84, 82);
    addKnob ("drumPitchDepth", "PITCH DEPTH", 222, 810, 84, 82);
    addKnob ("drumCutoff", "CUTOFF", 318, 810, 84, 82);
    addKnob ("drumRes", "RESONANCE", 414, 810, 84, 82);
    addKnob ("drumLevel", "DRUM LEVEL", 510, 810, 84, 82);
    for (int i = 0; i < 8; ++i)
    {
        const auto suffix = juce::String (i + 1);
        for (int row = 0; row < 2; ++row)
        {
            auto slider = std::make_unique<juce::Slider>();
            slider->setSliderStyle (juce::Slider::LinearVertical);
            slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 43, 18);
            slider->setBounds (645 + i * 54, row == 0 ? 731 : 828, 43, 75);
            slider->setNumDecimalPlacesToDisplay (row == 0 ? 0 : 2);
            slider->setTooltip ((row == 0 ? "Pitch in semitones, step " : "Velocity (0 = rest), step ") + suffix);
            addAndMakeVisible (*slider);
            const auto id = (row == 0 ? "drumNote" : "drumVel") + suffix;
            stepAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.parameters, id, *slider));
            (row == 0 ? drumPitchSliders : drumVelocitySliders)[static_cast<size_t> (i)] = std::move (slider);
        }
    }

    startTimerHz (12);
}

ANKSModularEditor::~ANKSModularEditor()
{
    stopTimer();
    // Attachments must be destroyed before widgets and before LookAndFeel.
    runAttachment.reset(); seqAttachment.reset(); waveAttachment.reset(); wave2Attachment.reset();
    drumRunAttachment.reset(); drumFollowAttachment.reset(); drumWaveAttachment.reset();
    stepAttachments.clear(); stepButtonAttachments.clear(); knobAttachments.clear();
    setLookAndFeel (nullptr);
}

void ANKSModularEditor::resized()
{
    const auto transform = juce::AffineTransform::scale (getWidth() / 1120.0f);
    for (auto* child : getChildren())
        if (dynamic_cast<juce::ResizableCornerComponent*> (child) == nullptr)
            child->setTransform (transform);
}

void ANKSModularEditor::addKnob (const juce::String& id, const juce::String& title,
                                  int x, int y, int w, int h)
{
    auto label = std::make_unique<juce::Label>();
    label->setText (title, juce::dontSendNotification);
    label->setFont (juce::FontOptions (11.0f));
    label->setJustificationType (juce::Justification::centred);
    label->setColour (juce::Label::textColourId, juce::Colour (muted));
    label->setBounds (x, y, w, 16);
    addAndMakeVisible (*label);
    knobNames.push_back (std::move (label));

    auto s = std::make_unique<juce::Slider>();
    s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, w, 19);
    s->setBounds (x, y + 16, w, h - 16);
    s->setDoubleClickReturnValue (false, 0);
    addAndMakeVisible (*s);
    knobAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                               (processor.parameters, id, *s));
    knobs.push_back (std::move (s));
}

juce::Colour ANKSModularEditor::signalColor (int type)
{
    switch (type)
    {
        case 0: return juce::Colour (0xffe9a44b);
        case 1: return juce::Colour (0xffb99cff);
        case 2: return juce::Colour (red);
        default: return juce::Colour (0xff51d6de);
    }
}

void ANKSModularEditor::drawModule (juce::Graphics& g, juce::Rectangle<int> box,
                                     const juce::String& name, const juce::String& desc)
{
    const auto r = box.toFloat();
    g.setColour (juce::Colour (panel)); g.fillRoundedRectangle (r, 10.0f);
    g.setColour (juce::Colour (outline)); g.drawRoundedRectangle (r, 10.0f, 1.0f);
    g.setColour (juce::Colour (blue).withAlpha (0.7f));
    g.fillRoundedRectangle (r.getX() + 13, r.getY() + 12, 4, 24, 2.0f);
    g.setColour (juce::Colour (silver)); g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawText (name, box.withTrimmedLeft (25).withTrimmedTop (7).withHeight (26), juce::Justification::left);
    g.setColour (juce::Colour (muted)); g.setFont (juce::FontOptions (11.0f));
    g.drawText (desc, box.withTrimmedLeft (14).withTrimmedTop (42).withHeight (18), juce::Justification::left);
}

void ANKSModularEditor::drawCable (juce::Graphics& g, juce::Point<float> a,
                                    juce::Point<float> b, int type, bool preview)
{
    const float delta = b.x - a.x;
    const float lift = (std::abs (b.y - a.y) < 6.0f) ? 27.0f : 75.0f;
    juce::Path p;
    p.startNewSubPath (a);
    p.cubicTo ({a.x + std::clamp (delta * 0.30f, -75.0f, 75.0f), a.y + lift},
               {b.x - std::clamp (delta * 0.30f, -75.0f, 75.0f), b.y + lift}, b);
    const auto col = signalColor (type);
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.strokePath (p, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (col.withAlpha (preview ? 0.55f : 0.90f));
    g.strokePath (p, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void ANKSModularEditor::drawJack (juce::Graphics& g, const Jack& jack)
{
    auto col = signalColor (jack.signalType);
    const bool selected = selectedJack >= 0 && jacks[static_cast<size_t> (selected)].id == jack.id;
    g.setColour (selected ? juce::Colours::white : col.withAlpha (0.8f));
    g.fillEllipse (jack.position.x - 12, jack.position.y - 12, 24, 24);
    g.setColour (juce::Colour (0xff080d13));
    g.fillEllipse (jack.position.x - 8, jack.position.y - 8, 16, 16);
    g.setColour (selected ? col : juce::Colour (0xffd5dbe3));
    if (anks::isOutput (jack.id)) g.fillEllipse (jack.position.x - 3, jack.position.y - 3, 6, 6);
    else g.drawEllipse (jack.position.x - 4, jack.position.y - 4, 8, 8, 1.0f);
}

void ANKSModularEditor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (getWidth() / 1120.0f));
    g.fillAll (juce::Colour (background));
    g.setColour (juce::Colour (blue));
    g.setFont (juce::FontOptions (27.0f, juce::Font::bold));
    g.drawText ("MOOGER", 18, 4, 186, 34, juce::Justification::left);
    g.setColour (juce::Colour (red)); g.fillEllipse (210, 20, 7, 7);
    g.setColour (juce::Colour (muted));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("MOTHER CORE  /  DUAL VCO + PERCUSSION  /  NATIVE VST3", 420, 10, 560, 23, juce::Justification::centred);
    g.setColour (juce::Colour (outline)); g.drawLine (18, 86, 1102, 86, 1);

    drawModule (g, {18, 96, 326, 295}, "01   SEQUENCER", "8-step pitch and gate / C3 base");
    drawModule (g, {356, 96, 166, 295}, "02   VCO 1", "Sound generator");
    drawModule (g, {534, 96, 190, 295}, "03   VCF", "Resonant low-pass");
    drawModule (g, {736, 96, 158, 295}, "04   VCA", "Amplitude");
    drawModule (g, {906, 96, 196, 295}, "05   OUTPUT", "Soft-limited stereo");
    drawModule (g, {18, 403, 412, 223}, "06   ENVELOPE", "Attack / decay / sustain / release");
    drawModule (g, {442, 403, 250, 223}, "07   LFO", "Cyclic modulation");
    drawModule (g, {704, 403, 190, 223}, "08   VCO 2", "Independent pitch + wave");
    drawModule (g, {906, 403, 196, 223}, "09   MIXER", "Two inputs / output level");
    drawModule (g, {18, 650, 1084, 272}, "10   PERCUSSION", "Independent voice / 16th-note sequencer / velocity 0 = rest");
    g.setColour (juce::Colour (muted)); g.setFont (juce::FontOptions (11.0f));
    g.drawText ("PITCH / SEMITONES", 645, 710, 425, 18, juce::Justification::centred);
    g.drawText ("VELOCITY", 645, 807, 425, 18, juce::Justification::centred);
    const int drumStep = processor.displayedDrumStep();
    for (int i = 0; i < 8; ++i)
    {
        const auto box = juce::Rectangle<int> (645 + i * 54, 690, 43, 18);
        if (drumStep == i) { g.setColour (juce::Colour (red)); g.fillRoundedRectangle (box.toFloat(), 4); }
        g.setColour (juce::Colour (silver));
        g.drawText (juce::String (i + 1), box, juce::Justification::centred);
    }

    const auto step = processor.displayedStep();
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    for (int i = 0; i < 8; ++i)
    {
        const int x = 35 + i * 37;
        const auto active = step == i;
        g.setColour (active ? juce::Colour (blue) : juce::Colour (muted));
        if (active) g.fillRoundedRectangle (float (x), 144, 28, 21, 4);
        g.setColour (active ? juce::Colour (background) : juce::Colour (muted));
        g.drawText (juce::String (i + 1), x, 143, 28, 23, juce::Justification::centred);
        g.setColour (juce::Colour (silver));
        const int offset = static_cast<int> (processor.parameters.getRawParameterValue ("note" + juce::String (i + 1))->load());
        g.drawText ((offset >= 0 ? "+" : "") + juce::String (offset), x - 2, 307, 32, 19, juce::Justification::centred);
    }
    g.setColour (juce::Colour (muted)); g.setFont (juce::FontOptions (11.0f));
    g.drawText ("WAVEFORM", 364, 145, 155, 21, juce::Justification::centred);
    g.drawText ("Open VCA to hear a drone, or connect ENV CV for notes.", 923, 159, 170, 43,
                juce::Justification::centred, true);

    // Cables are real DSP routing, not only graphical decorations.
    const auto route = processor.getRoutes();
    for (const auto& dst : jacks)
    {
        if (anks::isOutput (dst.id)) continue;
        const int source = route.source[static_cast<size_t> (anks::index (dst.id))];
        if (source < 0) continue;
        for (const auto& src : jacks)
            if (anks::index (src.id) == source)
                drawCable (g, src.position, dst.position, src.signalType);
    }
    if (dragging && selectedJack >= 0)
        drawCable (g, jacks[static_cast<size_t> (selectedJack)].position,
                   dragPosition, jacks[static_cast<size_t> (selectedJack)].signalType, true);

    for (const auto& jack : jacks) drawJack (g, jack);

    g.setColour (juce::Colour (muted));
    g.setFont (juce::FontOptions (10.0f));
    const auto portsLabel = [this, &g] (anks::Port p, const char* s, int x, int y)
    { juce::ignoreUnused (p); g.drawText (s, x, y, 90, 15, juce::Justification::centred); };
    portsLabel (anks::Port::seqPitch, "PITCH", 225, 379);
    portsLabel (anks::Port::seqGate, "GATE", 282, 379);
    portsLabel (anks::Port::oscPitch, "IN", 358, 379);
    portsLabel (anks::Port::oscOut, "OUT", 467, 379);
    portsLabel (anks::Port::filterIn, "IN", 536, 379);
    portsLabel (anks::Port::filterCV, "CV", 598, 379);
    portsLabel (anks::Port::filterOut, "OUT", 678, 379);
    portsLabel (anks::Port::vcaIn, "IN", 735, 379);
    portsLabel (anks::Port::vcaCV, "CV", 789, 379);
    portsLabel (anks::Port::vcaOut, "OUT", 843, 379);
    portsLabel (anks::Port::outputIn, "IN", 903, 379);
    g.drawText ("DRUM IN", 1046, 379, 55, 15, juce::Justification::centred);
    g.drawText ("OUT", 580, 910, 50, 12, juce::Justification::centred);
    g.drawText ("GATE", 26, 610, 60, 16, juce::Justification::centred);
    g.drawText ("CV OUT", 368, 610, 80, 16, juce::Justification::centred);
    g.drawText ("CV OUT", 635, 610, 74, 16, juce::Justification::centred);

    g.drawText ("PITCH IN", 707, 610, 60, 16, juce::Justification::centred);
    g.drawText ("OUT", 841, 610, 50, 16, juce::Justification::centred);
    g.drawText ("IN 1", 906, 610, 50, 16, juce::Justification::centred);
    g.drawText ("IN 2", 971, 610, 50, 16, juce::Justification::centred);
    g.drawText ("OUT", 1045, 610, 50, 16, juce::Justification::centred);
    g.setColour (juce::Colour (muted)); g.setFont (juce::FontOptions (11.0f));
    g.drawText (selectedJack >= 0 ? "PATCH: choose an input socket or drag to it"
                : "MOOGER V0.4  /  Click output then input to patch; click an input to unplug  /  DRUM RUN is independent",
                18, 930, 1085, 18, juce::Justification::centred);
}

int ANKSModularEditor::hitJack (juce::Point<float> mouse) const
{
    mouse /= getWidth() / 1120.0f;
    for (int i = 0; i < static_cast<int> (jacks.size()); ++i)
        if (jacks[static_cast<size_t> (i)].position.getDistanceFrom (mouse) <= 15.0f)
            return i;
    return -1;
}

void ANKSModularEditor::mouseDown (const juce::MouseEvent& e)
{
    const int i = hitJack (e.position);
    if (i < 0) { selectedJack = -1; dragging = false; repaint(); return; }
    const auto p = jacks[static_cast<size_t> (i)].id;
    if (anks::isOutput (p))
    {
        selectedJack = selectedJack == i ? -1 : i;
        dragging = false;
        dragPosition = e.position / (getWidth() / 1120.0f);
    }
    else if (selectedJack >= 0)
    {
        processor.connect (jacks[static_cast<size_t> (selectedJack)].id, p);
        selectedJack = -1;
        dragging = false;
    }
    else processor.disconnect (p);
    repaint();
}
void ANKSModularEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (selectedJack >= 0)
    {
        dragging = true;
        dragPosition = e.position / (getWidth() / 1120.0f);
        repaint();
    }
}
void ANKSModularEditor::mouseUp (const juce::MouseEvent& e)
{
    if (dragging && selectedJack >= 0)
    {
        const int dst = hitJack (e.position);
        if (dst >= 0)
            processor.connect (jacks[static_cast<size_t> (selectedJack)].id,
                               jacks[static_cast<size_t> (dst)].id);
        selectedJack = -1;
        dragging = false;
        repaint();
    }
}
void ANKSModularEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int i = hitJack (e.position);
    if (i >= 0 && ! anks::isOutput (jacks[static_cast<size_t> (i)].id))
    {
        processor.disconnect (jacks[static_cast<size_t> (i)].id);
        selectedJack = -1;
        repaint();
    }
}
void ANKSModularEditor::timerCallback() { repaint(); }
