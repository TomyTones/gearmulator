# Morph Synth — Implementation Plan

## Concept

Two JP-8000 emulation engines inside one AU plugin. A central morph fader blends their audio output. Move the fader and you hear a smooth transition from patch A to patch B.

```
┌────────────────────────────────────────────────────────────┐
│  [ JP-8000  Engine A ]   [▲B]   [ JP-8000  Engine B ]     │
│    (patch, knobs, UI)    [──]   (patch, knobs, UI)         │
│                          [▼A]   ← morph slider             │
└────────────────────────────────────────────────────────────┘
```

---

## Phase 1 — Audio Crossfade Morph ✅ Complete

### 1a — Fork, build, install JE8086 ✅

- Forked Gearmulator, cloned with all submodules
- Built all plugins via `bash build_mac.sh`
- Installed `JE8086.component` to `~/Library/Audio/Plug-Ins/Components/`
- Confirmed JP-8000 ROM loads correctly

### 1b — Dual-engine plugin (jeMorphPlugin) ✅

**New plugin:** `JE8086Morph` (AU/VST3/CLAP, 4CC: `Tjpm`)

**What was built:**
- `MorphProcessor` — extends `AudioPluginAudioProcessor`, owns two independent `jeLib::Device` + `synthLib::Plugin` instances
- Crossfade formula: `output = A × (1-morph) + B × morph`
- `m_morphParam` — automatable `AudioParameterFloat` ("Morph A↔B", 0.0–1.0)
- MIDI routing: note/CC events forwarded to both engines; SysEx goes only to Engine A
- `createEditorState()` override returns `MorphEditorState`

**Infrastructure changes to base classes:**
- `jucePluginLib/processor.h`: moved `prepareToPlay`, `processBlock`, `processBlockBypassed` from `private:` to `protected:` so `MorphProcessor` can call the base explicitly
- `jucePluginEditorLib/pluginEditorState.h`: made `getUiRoot`, `getWidth`, `getHeight`, `resizeEditor` virtual so `MorphEditorState` can override them

**Extracted from jeJucePlugin:**
- `jeCreatePlugin.cpp` — separated `createPluginFilter()` from `jePluginProcessor.cpp` to avoid duplicate symbol when both plugins compile the shared sources

### 1c — Dual-panel side-by-side UI ✅

**What was built:**
- `MorphEditorState` — creates two `jeJucePlugin::Editor` instances (one per engine view), overrides sizing/layout
- `MorphContainer` — a JUCE Component holding:
  - Engine A's RmlComponent (left, full jeTrancy skin)
  - A vertical JUCE `Slider` (center, 80px wide, dark strip)
  - Engine B's RmlComponent (right, full jeTrancy skin)
- `SliderParameterAttachment` — bidirectional sync between the JUCE slider and the "Morph A↔B" automation parameter
- The window is `2 × skin_width + 80px` wide and automatically resizable

**Validated:**
- `auval -v aumu Tjpm TusP` → PASS
- Plugin opens in Logic Pro
- Morph slider visible in Logic's automation lane as "Morph A↔B"
- Sweeping the slider blends the audio between Engine A and Engine B

---

## Phase 2 — Custom Skin (Next)

### Why a new skin?

The current prototype shows the jeTrancy skin twice. Both panels share the same Controller — you can only control Engine A from the UI. This is honest as a prototype but not the final product.

The new skin should be purpose-built to clearly show:
- Which engine is active / dominant
- The morph position
- (Future) Parameter state of each engine independently

### Step 6 — Decide look & feel

- Review the Roland JP-8000 hardware panel for visual language reference
- Sketch the dual-engine layout concept:
  - One unified panel (not two separate skins)
  - Prominent morph arc/fader in the center
  - A/B patch name display
- Define the color palette and typography

### Step 7 — Design in Figma

Using the **Figma MCP connector** (available in Claude Code):
- Design the full plugin panel at 2772×1460 dp (rootScale 0.5 → 1386×730 px rendered)
- Create all component assets: background, knobs, buttons, sliders, LEDs
- Export PNGs for the RmlUi skin system

### Step 8 — Implement the skin

Create a new skin folder:
```
source/ronaldo/je8086/jeMorphPlugin/skins/jeMorph/
├── jeMorph.rml       ← layout, binding to param="" attributes
├── jeMorph.rcss      ← sizing and style rules
└── *.png             ← exported from Figma
```

Add the skin to `jeMorphPlugin/CMakeLists.txt`:
```cmake
addSkin("JE8086Morph" "jeMorph" "skins/jeMorph" "jeMorph.rml")
```

The RML `param="..."` attribute names do not change — only the visuals change. All existing knob/button bindings carry over automatically.

---

## Phase 3 — Parameter Morphing (Future)

After the custom skin is in place, consider interpolating synth parameters between A and B in addition to audio crossfade. This requires:
- A way to store Engine B's full parameter state independently (separate Controller, or snapshot mechanism)
- Sending interpolated SysEx values to each engine at each morph position
- UI to load/save "Patch A" and "Patch B" independently

This is explicitly out of scope until Phase 2 is complete.

---

## Open Questions

1. **Engine B parameter control:** Currently Engine B's patch is whatever state it initialized with (same ROM default). To make the morph musically useful, we need a way to load a different patch into Engine B. Options: separate Controller, or a "snapshot" system. → Investigate in Phase 2/3.

2. **CPU budget:** Two full JP-8000 emulations on Apple Silicon — initial tests show it's acceptable, but add a bypass for Engine B when `morph == 0` as an optimization if needed.

3. **Skin reload:** Changing skins via right-click while the dual-panel UI is open resets to single-panel view (known prototype limitation). Fix in Phase 2 when implementing the dedicated skin.
