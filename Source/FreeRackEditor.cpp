#include "FreeRackEditor.h"
#include "PluginProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>

namespace
{
constexpr int cardWidth = 280, cardHeight = 280, cellWidth = 296, cellHeight = 296;
const juce::Colour background (0xff121411), panel (0xff242720), ink (0xffeee9d9), gold (0xffd9b36a);
juce::Colour signalColour (mooger::Signal signal)
{
    switch (signal) { case mooger::Signal::audio: return juce::Colour (0xffe9a44b);
        case mooger::Signal::gate: return juce::Colour (0xffeb565e);
        case mooger::Signal::pitch: return juce::Colour (0xffb99cff);
        default: return juce::Colour (0xff51d6de); }
}
struct Address { int slot = -1, port = -1; bool output = false; bool valid() const { return slot >= 0; } };
class Canvas;
class Card final : public juce::Component
{
public:
    Card (Canvas&, ANKSModularProcessor&, int, const mooger::Module&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void refresh();
    void setRear (bool show) { rear = show; for (auto* control : music) control->setVisible (! rear); repaint(); }
    int visibleControls() const { int count = 0; for (auto* c : music) if (c->isVisible()) ++count; return count; }
    Address hit (juce::Point<float>) const;
    juce::Point<float> socket (int port, bool output) const { return {output ? float (cardWidth - 14) : 14.0f, 68.0f + port * 43.0f}; }
    std::uint32_t uid;
    mooger::Type type;
private:
    Canvas& canvas;
    ANKSModularProcessor& processor;
    int slot;
    bool rear = false, dragging = false;
    juce::Point<int> origin;
    std::vector<std::unique_ptr<juce::Component>> owned;
    std::vector<juce::Component*> music;
    std::array<juce::Slider*, mooger::maxParameters> sliders {};
    std::array<juce::ComboBox*, mooger::maxParameters> choices {};
    std::array<juce::TextButton*, mooger::maxParameters> toggles {};
    juce::TextButton removeButton {"X"}, duplicateButton {"2x"}, triggerButton {"IMPULSION"};
};
class Canvas final : public juce::Component, public juce::DragAndDropTarget
{
public:
    explicit Canvas (ANKSModularProcessor& p) : processor (p) { setSize (1200, 900); }
    ANKSModularProcessor& processor;
    std::array<std::unique_ptr<Card>, mooger::maxModules> cards;
    mooger::RackState model;
    bool rear = false;
    int selected = -1, draggingSlot = -1;
    Address cableStart;
    std::uint32_t cableStartUid = 0;
    bool cableDragged = false;
    juce::Point<float> cableEnd;
    std::function<void (juce::String)> status;
    void message (juce::String text) { if (status) status (text); }
    void sync()
    {
        if (! processor.isFreeRack()) { for (auto& card : cards) card.reset(); repaint(); return; }
        model = processor.rackState(); int width = 1200, height = 900;
        if (cableStart.valid() && (! model.modules[size_t (cableStart.slot)].active || model.modules[size_t (cableStart.slot)].uid != cableStartUid)) cableStart = {};
        for (int i = 0; i < mooger::maxModules; ++i)
        {
            const auto& node = model.modules[size_t (i)]; auto& card = cards[size_t (i)];
            if (! node.active) { card.reset(); continue; }
            if (! card || card->uid != node.uid || card->type != node.type)
            { card = std::make_unique<Card> (*this, processor, i, node); addAndMakeVisible (*card); card->setRear (rear); }
            if (draggingSlot != i) card->setBounds (12 + node.column * cellWidth, 12 + node.row * cellHeight, cardWidth, cardHeight);
            card->refresh(); width = std::max (width, card->getRight() + 20); height = std::max (height, card->getBottom() + 20);
        }
        if (selected >= 0 && ! model.modules[size_t (selected)].active) selected = -1;
        setSize (width, height); repaint();
    }
    void setRear (bool show) { rear = show; cableStart = {}; for (auto& card : cards) if (card) card->setRear (show); repaint(); }
    void select (int slot) { selected = slot; repaint(); }
    juce::Point<float> point (Address address) const
    { return cards[size_t (address.slot)]->getPosition().toFloat() + cards[size_t (address.slot)]->socket (address.port, address.output); }
    Address hit (juce::Point<float> position) const
    {
        if (! rear) return {};
        for (const auto& card : cards) if (card) { const auto address = card->hit (position - card->getPosition().toFloat()); if (address.valid()) return address; }
        return {};
    }
    void socketDown (Address address)
    {
        if (address.output) { cableStart = address; cableStartUid = model.modules[size_t (address.slot)].uid; cableEnd = point (address); cableDragged = false; }
        else if (cableStart.valid()) { processor.connectRack ({cableStart.slot, cableStart.port, address.slot, address.port}); cableStart = {}; message ("Connexion créée."); }
        else { processor.disconnectRack (address.slot, address.port); message ("Entrée débranchée."); }
        model = processor.rackState(); repaint();
    }
    void endCable (juce::Point<float> at)
    {
        if (cableDragged && cableStart.valid())
        {
            const auto target = hit (at);
            if (target.valid() && ! target.output) { processor.connectRack ({cableStart.slot, cableStart.port, target.slot, target.port}); message ("Connexion créée."); }
            cableStart = {}; model = processor.rackState(); repaint();
        }
    }
    void paint (juce::Graphics& g) override
    {
        g.fillAll (background);
        g.setColour (juce::Colour (0xff2b3028));
        for (int y = 8; y < getHeight(); y += 24) for (int x = 8; x < getWidth(); x += 24) g.fillRect (x, y, 1, 1);
        if (! processor.isFreeRack())
        {
            g.setColour (ink); g.setFont (juce::FontOptions (18));
            g.drawFittedText ("Ancien patch conservé en mode classique.\nFermez puis rouvrez l'éditeur pour ses commandes,\nou choisissez CRÉER pour construire un rack libre.", getLocalBounds().reduced (30), juce::Justification::centred, 4);
        }
    }
    void paintOverChildren (juce::Graphics& g) override
    {
        if (! rear || ! processor.isFreeRack()) return;
        auto cable = [&] (juce::Point<float> a, juce::Point<float> b, juce::Colour colour)
        {
            juce::Path path; path.startNewSubPath (a); const float bend = std::max (45.0f, std::min (170.0f, std::abs (b.x - a.x) * 0.45f));
            path.cubicTo (a.x + bend, a.y + 22, b.x - bend, b.y + 22, b.x, b.y);
            g.setColour (juce::Colours::black.withAlpha (0.85f)); g.strokePath (path, juce::PathStrokeType (6));
            g.setColour (colour.withAlpha (0.88f)); g.strokePath (path, juce::PathStrokeType (3));
        };
        for (int i = 0; i < model.cableCount; ++i)
        {
            const auto c = model.cables[size_t (i)]; if (! cards[size_t (c.from)] || ! cards[size_t (c.to)]) continue;
            cable (point ({c.from, c.output, true}), point ({c.to, c.input, false}), signalColour (mooger::info (model.modules[size_t (c.from)].type).outputs[size_t (c.output)].signal));
        }
        if (cableStart.valid() && cards[size_t (cableStart.slot)]) cable (point (cableStart), cableEnd, gold);
    }
    void mouseDown (const juce::MouseEvent&) override { selected = -1; cableStart = {}; repaint(); }
    bool isInterestedInDragSource (const SourceDetails& d) override { return d.description.toString().startsWith ("catalog:"); }
    void itemDropped (const SourceDetails& d) override
    {
        const auto type = mooger::Type (d.description.toString().fromFirstOccurrenceOf (":", false, false).getIntValue());
        const int slot = processor.addRackModule (type, d.localPosition.x / cellWidth, d.localPosition.y / cellHeight);
        selected = slot; message (slot >= 0 ? "Module ajouté. Reliez ses prises à l'arrière." : "Le rack contient déjà 32 modules."); sync();
    }
};

Card::Card (Canvas& c, ANKSModularProcessor& p, int index, const mooger::Module& module)
    : uid (module.uid), type (module.type), canvas (c), processor (p), slot (index)
{
    setComponentID ("rack-module-" + juce::String (slot));
    setSize (cardWidth, cardHeight);
    removeButton.setBounds (250, 4, 23, 24); duplicateButton.setBounds (218, 4, 29, 24);
    removeButton.setTooltip ("Retirer le module et ses câbles"); duplicateButton.setTooltip ("Ajouter un exemplaire indépendant sans recopier les câbles");
    removeButton.onClick = [this] { processor.removeRackModule (slot); canvas.message ("Module retiré."); };
    duplicateButton.onClick = [this] { const int added = processor.duplicateRackModule (slot); canvas.message (added >= 0 ? "Module dupliqué." : "Le rack est plein."); };
    addAndMakeVisible (removeButton); addAndMakeVisible (duplicateButton);
    const auto& d = mooger::info (type);
    for (int i = 0; i < d.parameterCount; ++i)
    {
        const auto parameter = d.parameters[size_t (i)]; const int x = 7 + (i % 4) * 67, y = 43 + (i / 4) * 65;
        auto name = std::make_unique<juce::Label>(); name->setText (parameter.name, juce::dontSendNotification);
        name->setFont (juce::FontOptions (9.0f)); name->setJustificationType (juce::Justification::centred);
        name->setColour (juce::Label::textColourId, ink); name->setBounds (x, y, 65, 14);
        music.push_back (name.get()); addAndMakeVisible (*name); owned.push_back (std::move (name));
        if (parameter.kind == mooger::ParamKind::toggle)
        {
            auto control = std::make_unique<juce::TextButton> (parameter.name); control->setClickingTogglesState (true);
            control->setBounds (x + 2, y + 22, 61, 29); auto* raw = control.get(); toggles[size_t (i)] = raw;
            control->onClick = [this, raw, i] { canvas.select (slot); processor.setRackValue (slot, i, raw->getToggleState() ? 1 : 0); };
            music.push_back (raw); addAndMakeVisible (*raw); owned.push_back (std::move (control));
        }
        else if (parameter.kind == mooger::ParamKind::choice)
        {
            auto control = std::make_unique<juce::ComboBox>(); control->addItemList (juce::StringArray::fromTokens (parameter.choices, "|", ""), 1);
            control->setBounds (x + 1, y + 22, 64, 27); auto* raw = control.get(); choices[size_t (i)] = raw;
            control->onChange = [this, raw, i, parameter] { canvas.select (slot); processor.setRackValue (slot, i, float (raw->getSelectedItemIndex()) / parameter.high); };
            music.push_back (raw); addAndMakeVisible (*raw); owned.push_back (std::move (control));
        }
        else
        {
            auto control = std::make_unique<juce::Slider>(); control->setRange (0, 1, 0);
            control->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); control->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
            control->setBounds (x, y + 14, 65, 51); auto* raw = control.get(); sliders[size_t (i)] = raw;
            control->textFromValueFunction = [parameter] (double v) { return juce::String (parameter.physical (float (v)), parameter.decimals); };
            control->valueFromTextFunction = [parameter] (const juce::String& text) { return parameter.normalized (text.getFloatValue()); };
            control->onValueChange = [this, raw, i] { canvas.select (slot); processor.setRackValue (slot, i, float (raw->getValue())); };
            control->onDragStart = [this, i] { processor.rackParameter (slot, i)->beginChangeGesture(); };
            control->onDragEnd = [this, i] { processor.rackParameter (slot, i)->endChangeGesture(); };
            music.push_back (raw); addAndMakeVisible (*raw); owned.push_back (std::move (control));
        }
    }
    if (d.trigger)
    {
        triggerButton.setBounds (10, 246, 110, 25); triggerButton.onClick = [this] { processor.triggerRackModule (slot); };
        addAndMakeVisible (triggerButton); music.push_back (&triggerButton);
    }
    refresh();
}
void Card::refresh()
{
    const auto& d = mooger::info (type);
    for (int i = 0; i < d.parameterCount; ++i)
    {
        const float value = processor.rackValue (slot, i);
        if (sliders[size_t (i)] && ! sliders[size_t (i)]->isMouseButtonDown()) sliders[size_t (i)]->setValue (value, juce::dontSendNotification);
        if (toggles[size_t (i)]) toggles[size_t (i)]->setToggleState (value > 0.5f, juce::dontSendNotification);
        if (choices[size_t (i)]) choices[size_t (i)]->setSelectedItemIndex (int (d.parameters[size_t (i)].physical (value)), juce::dontSendNotification);
    }
    repaint();
}
Address Card::hit (juce::Point<float> at) const
{
    if (! rear) return {};
    const auto& d = mooger::info (type);
    for (int port = 0; port < d.inputCount; ++port) if (at.getDistanceFrom (socket (port, false)) < 13) return {slot, port, false};
    for (int port = 0; port < d.outputCount; ++port) if (at.getDistanceFrom (socket (port, true)) < 13) return {slot, port, true};
    return {};
}
void Card::paint (juce::Graphics& g)
{
    g.setColour (panel); g.fillRoundedRectangle (getLocalBounds().toFloat(), 7);
    g.setColour (canvas.selected == slot ? gold : juce::Colour (0xff555b4c)); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1), 7, 1.5f);
    g.setColour (ink); g.setFont (juce::FontOptions (14, juce::Font::bold));
    g.drawText (juce::String (mooger::info (type).name) + " #" + juce::String (uid), 10, 3, 205, 26, juce::Justification::left);
    g.setColour (juce::Colour (0xff4a4e43)); g.drawLine (5, 32, cardWidth - 5, 32);
    const auto& d = mooger::info (type);
    if (rear)
    {
        g.setFont (juce::FontOptions (10)); g.setColour (gold);
        g.drawText ("ENTREES", 22, 37, 95, 16, juce::Justification::left);
        g.drawText ("SORTIES", cardWidth - 116, 37, 95, 16, juce::Justification::right);
        for (int side = 0; side < 2; ++side) for (int i = 0; i < (side ? d.outputCount : d.inputCount); ++i)
        {
            const auto port = side ? d.outputs[size_t (i)] : d.inputs[size_t (i)]; const auto at = socket (i, side != 0);
            g.setColour (signalColour (port.signal)); g.fillEllipse (at.x - 9, at.y - 9, 18, 18);
            g.setColour (background); g.fillEllipse (at.x - 5, at.y - 5, 10, 10);
            g.setColour (ink); g.drawText (port.name, side ? cardWidth - 138 : 28, int (at.y) - 9, 110, 18,
                                         side ? juce::Justification::right : juce::Justification::left);
        }
        g.setColour (juce::Colour (0xffa9ad9d)); g.drawText ("AUDIO / CV / GATE / PITCH", 10, 251, 260, 16, juce::Justification::centred);
    }
    else if (type == mooger::Type::midi)
    {
        g.setFont (juce::FontOptions (16)); g.drawFittedText ("Entree MIDI du plugin\nNotes, gate et velocite\nvers les prises arriere", 20, 75, 240, 120, juce::Justification::centred, 4);
    }
    else if (type == mooger::Type::sequencer)
    {
        const int active = processor.rackDisplayedStep (slot);
        for (int i = 0; i < 8; ++i) { g.setColour (i == active ? juce::Colour (0xffeb565e) : juce::Colour (0xff555b4c)); g.fillEllipse (18.0f + i * 31, 253, 10, 10); }
    }
}
void Card::mouseDown (const juce::MouseEvent& e)
{
    canvas.select (slot); const auto address = hit (e.position);
    if (address.valid()) { canvas.socketDown (address); return; }
    if (e.y < 32) { dragging = true; origin = getPosition(); canvas.draggingSlot = slot; }
}
void Card::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging)
    {
        const auto at = origin + e.getScreenPosition() - e.getMouseDownScreenPosition();
        setTopLeftPosition (std::max (0, at.x), std::max (0, at.y)); canvas.repaint();
    }
    else if (canvas.cableStart.valid()) { canvas.cableEnd = e.getEventRelativeTo (&canvas).position; canvas.cableDragged = true; canvas.repaint(); }
}
void Card::mouseUp (const juce::MouseEvent& e)
{
    if (dragging)
    {
        dragging = false; canvas.draggingSlot = -1;
        processor.moveRackModule (slot, int (std::round (double (getX() - 12) / cellWidth)), int (std::round (double (getY() - 12) / cellHeight))); canvas.sync();
    }
    else canvas.endCable (e.getEventRelativeTo (&canvas).position);
}
} // namespace

struct FreeRackEditor::Impl final : public juce::ListBoxModel, public juce::Timer
{
    FreeRackEditor& owner;
    ANKSModularProcessor& processor;
    juce::LookAndFeel_V4 skin;
    juce::ListBox bank {"Banque", this};
    juce::Viewport viewport;
    Canvas canvas;
    juce::TextButton flip {"RETOURNER LE RACK"}, add {"AJOUTER"}, replace {"REMPLACER"}, create {"CREER"}, play {"SEQ RUN / STOP"}, save {"ENREGISTRER"}, load {"OUVRIR"};
    juce::ComboBox preset;
    juce::Label status;
    std::unique_ptr<juce::FileChooser> chooser;
    Impl (FreeRackEditor& o, ANKSModularProcessor& p) : owner (o), processor (p), canvas (p)
    {
        skin.setColour (juce::Slider::thumbColourId, gold); skin.setColour (juce::Slider::rotarySliderFillColourId, gold);
        skin.setColour (juce::Slider::textBoxTextColourId, ink); skin.setColour (juce::Slider::textBoxBackgroundColourId, background);
        skin.setColour (juce::TextButton::buttonColourId, panel); skin.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff4a614a));
        skin.setColour (juce::ComboBox::backgroundColourId, background); skin.setColour (juce::ListBox::backgroundColourId, background);
        owner.setLookAndFeel (&skin);
        for (auto* c : std::initializer_list<juce::Component*> {&bank, &viewport, &flip, &add, &replace, &create, &play, &save, &load, &preset, &status}) owner.addAndMakeVisible (*c);
        viewport.setViewedComponent (&canvas, false); bank.setRowHeight (34); bank.selectRow (0);
        canvas.setComponentID ("rack-canvas"); bank.setComponentID ("rack-bank");
        canvas.status = [this] (juce::String text) { status.setText (text, juce::dontSendNotification); };
        preset.addItemList ({"Initial sequenceur", "Initial MIDI", "Rack vide"}, 1); preset.setSelectedId (1);
        flip.onClick = [this] { owner.setRear (! canvas.rear); };
        add.onClick = [this] { const int slot = processor.addRackModule (mooger::Type (std::max (0, bank.getSelectedRow()))); canvas.select (slot); canvas.message (slot < 0 ? "Limite de 32 modules atteinte." : "Module ajoute, sans cables. Retournez le rack pour le relier."); canvas.sync(); };
        replace.onClick = [this] { const bool ok = processor.replaceRackModule (canvas.selected, mooger::Type (std::max (0, bank.getSelectedRow()))); canvas.message (ok ? "Module remplace ; prises compatibles conservees." : "Selectionnez d'abord l'en-tete d'un module du rack."); canvas.sync(); };
        create.onClick = [this] { processor.newFreeRack (preset.getSelectedId() == 3 ? 0 : preset.getSelectedId()); canvas.sync(); canvas.message ("Nouveau rack cree."); };
        play.onClick = [this]
        {
            const auto model = processor.rackState(); bool anyRunning = false;
            for (int i = 0; i < mooger::maxModules; ++i) if (model.modules[size_t (i)].active)
            { const auto t = model.modules[size_t (i)].type; if (t == mooger::Type::sequencer) anyRunning |= processor.rackValue (i, 1) > 0.5f; if (t == mooger::Type::percussion) anyRunning |= processor.rackValue (i, 7) > 0.5f; }
            for (int i = 0; i < mooger::maxModules; ++i) if (model.modules[size_t (i)].active)
            { const auto t = model.modules[size_t (i)].type; if (t == mooger::Type::sequencer) processor.setRackValue (i, 1, anyRunning ? 0 : 1); if (t == mooger::Type::percussion) processor.setRackValue (i, 7, anyRunning ? 0 : 1); }
            canvas.sync();
        };
        save.onClick = [this] { chooseFile (true); }; load.onClick = [this] { chooseFile (false); };
        canvas.sync(); canvas.message ("Banque integree : glisser un module dans le rack, ou AJOUTER. Les exemplaires sont independants."); startTimerHz (20);
    }
    ~Impl() override { stopTimer(); viewport.setViewedComponent (nullptr, false); owner.setLookAndFeel (nullptr); }
    int getNumRows() override { return int (mooger::Type::count); }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (selected) { g.setColour (juce::Colour (0xff3d4636)); g.fillRect (0, 0, w, h); }
        g.setColour (ink); g.setFont (juce::FontOptions (13)); g.drawText (mooger::info (mooger::Type (row)).name, 10, 0, w - 15, h, juce::Justification::left);
    }
    juce::var getDragSourceDescription (const juce::SparseSet<int>& rows) override { return rows.size() == 1 ? juce::var ("catalog:" + juce::String (rows[0])) : juce::var(); }
    void listBoxItemDoubleClicked (int, const juce::MouseEvent&) override { if (add.onClick) add.onClick(); }
    void timerCallback() override
    {
        const bool free = processor.isFreeRack();
        bank.setEnabled (free); add.setEnabled (free); replace.setEnabled (free); play.setEnabled (free); save.setEnabled (free);
        canvas.sync();
    }
    void chooseFile (bool writing)
    {
        chooser = std::make_unique<juce::FileChooser> (writing ? "Enregistrer le rack" : "Ouvrir un rack", juce::File(), "*.mooger");
        const auto safe = juce::Component::SafePointer<FreeRackEditor> (&owner);
        chooser->launchAsync (juce::FileBrowserComponent::canSelectFiles | (writing ? juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting : juce::FileBrowserComponent::openMode),
            [safe, writing] (const juce::FileChooser& dialog)
            {
                if (! safe || ! safe->impl) return; auto& self = *safe->impl; const auto chosen = dialog.getResult(); if (chosen == juce::File()) return;
                if (writing)
                {
                    juce::MemoryBlock block; self.processor.getStateInformation (block);
                    self.canvas.message (chosen.withFileExtension (".mooger").replaceWithData (block.getData(), block.getSize()) ? "Rack enregistre." : "Echec d'enregistrement.");
                }
                else
                {
                    juce::MemoryBlock block;
                    if (chosen.getSize() > 1024 * 1024 || ! chosen.loadFileAsData (block)) { self.canvas.message ("Fichier illisible."); return; }
                    auto xml = juce::AudioProcessor::getXmlFromBinary (block.getData(), int (block.getSize()));
                    if (! xml || ! self.processor.restoreRack (juce::ValueTree::fromXml (*xml).getChildWithName ("FreeRack"))) { self.canvas.message ("Ce fichier n'est pas un rack libre valide."); return; }
                    self.canvas.sync(); self.canvas.message ("Rack restaure.");
                }
            });
    }
};

FreeRackEditor::FreeRackEditor (ANKSModularProcessor& p, bool embedded) : AudioProcessorEditor (&p)
{
    impl = std::make_unique<Impl> (*this, p);
    setResizable (! embedded, ! embedded); setResizeLimits (1000, 700, 1800, 1200); setSize (1440, 900);
}
FreeRackEditor::~FreeRackEditor() { impl.reset(); }
void FreeRackEditor::paint (juce::Graphics& g)
{
    g.fillAll (background); g.setColour (gold); g.setFont (juce::FontOptions (24, juce::Font::bold)); g.drawText ("MOOGER", 12, 2, 155, 40, juce::Justification::left);
    g.setColour (ink); g.setFont (juce::FontOptions (13)); g.drawText (impl->canvas.rear ? "FACE ARRIERE - CONNEXIONS" : "FACE AVANT - COMMANDES", 180, 4, 360, 34, juce::Justification::left);
    g.setColour (gold); g.setFont (juce::FontOptions (13, juce::Font::bold)); g.drawText ("BANQUE DE MODULES", 12, 86, 190, 24, juce::Justification::left);
}
void FreeRackEditor::resized()
{
    if (! impl) return; auto& ui = *impl;
    ui.flip.setBounds (getWidth() - 202, 8, 190, 30); ui.play.setBounds (getWidth() - 382, 8, 170, 30);
    ui.preset.setBounds (215, 48, 165, 28); ui.create.setBounds (385, 48, 75, 28);
    ui.save.setBounds (470, 48, 120, 28); ui.load.setBounds (600, 48, 90, 28);
    ui.add.setBounds (12, 48, 90, 28); ui.replace.setBounds (108, 48, 94, 28);
    ui.bank.setBounds (12, 112, 190, getHeight() - 150); ui.viewport.setBounds (214, 86, getWidth() - 226, getHeight() - 124);
    ui.status.setBounds (10, getHeight() - 31, getWidth() - 20, 25);
}
void FreeRackEditor::setRear (bool show) { impl->canvas.setRear (show); repaint(); }
bool FreeRackEditor::isRear() const { return impl->canvas.rear; }
int FreeRackEditor::visibleMusicalControls() const { int n = 0; for (const auto& card : impl->canvas.cards) if (card) n += card->visibleControls(); return n; }
