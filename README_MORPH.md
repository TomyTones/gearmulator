# Prototype: Gearmulator — JP-8000 Morph Synth

## What This Is

A fork of [Gearmulator](https://github.com/dsp56300/gearmulator) — a low-level hardware emulator for classic VA synthesizers. Instead of recreating synth sounds from scratch, Gearmulator emulates the original silicon chips (ICs) and loads the synth's actual firmware (ROM). The result is the real synth running inside your DAW.

This prototype focuses on the **JE8086** plugin, which emulates the **Roland JP-8000** using its original firmware.

The end goal of this fork is a **Morph Synth** — two JP-8000 engines running side by side inside a single plugin, with a fader to morph between them.

---

## Status

| Phase | Description | Status |
|---|---|---|
| 1a | Fork, build, install JE8086 | ✅ Done |
| 1b | Dual-engine audio crossfade plugin (JE8086 Morph) | ✅ Done |
| 1c | Dual-panel side-by-side UI with morph slider | ✅ Done |
| 2 | Custom Figma-designed skin | Planned |

---

## What's Been Built

### JE8086 Morph Plugin (`jeMorphPlugin`)

A new AU/VST3/CLAP plugin (`JE8086Morph`, 4CC: `Tjpm`) that:

- Runs **two independent JP-8000 emulation engines** inside one plugin
- Blends their audio output via a **crossfade formula**: `output = A × (1-morph) + B × morph`
- Exposes **"Morph A↔B"** as a fully automatable JUCE parameter (visible in Logic's automation lane)
- Shows the **full jeTrancy UI twice, side by side**, with a vertical morph slider between them

```
┌────────────────────────────────────────────────────────────┐
│  [ Engine A — full JP-8000 UI ]  │▲B│  [ Engine B — full JP-8000 UI ]  │
│                                  │  │                                   │
│                                  │──│  ← drag to morph                 │
│                                  │  │                                   │
│                                  │▼A│                                   │
└────────────────────────────────────────────────────────────┘
```

### Key Architecture Changes

| File | What changed |
|---|---|
| `source/ronaldo/je8086/jeMorphPlugin/` | New plugin folder |
| `jeMorphPlugin/jeMorphProcessor.h/.cpp` | `MorphProcessor` — dual engine, crossfade `processBlock` |
| `jeMorphPlugin/jeMorphEditorState.h/.cpp` | `MorphEditorState` + `MorphContainer` — dual-panel JUCE layout |
| `jeMorphPlugin/CMakeLists.txt` | Build target `JE8086Morph` with jeTrancy skin |
| `jeMorphPlugin/serverPlugin.cpp` | Bridge device factory (required by build macro) |
| `jeJucePlugin/jeCreatePlugin.cpp` | Extracted `createPluginFilter()` from `jePluginProcessor.cpp` |
| `jeJucePlugin/CMakeLists.txt` | Added `jeCreatePlugin.cpp` to source list |
| `je8086/CMakeLists.txt` | Added `jeMorphPlugin` subdirectory |
| `jucePluginLib/processor.h` | Moved `prepareToPlay/processBlock/processBlockBypassed` to `protected:` |
| `jucePluginEditorLib/pluginEditorState.h` | Made `getUiRoot/getWidth/getHeight/resizeEditor` virtual |

---

## Project Structure

```
Prototype-Gearmulator/
├── docs/
│   └── JP8000v1.05/              ← JP-8000 firmware ROM (8 .MID files)
├── README.md
├── PLAN.md
└── gearmulator/                  ← The cloned fork
    ├── source/
    │   ├── ronaldo/je8086/
    │   │   ├── jeLib/            ← Synthesis engine (jeLib::Device)
    │   │   ├── jeJucePlugin/     ← Original JE8086 plugin + jeTrancy skin
    │   │   └── jeMorphPlugin/    ← NEW: Dual-engine morph plugin
    │   ├── synthLib/             ← Base device/plugin abstraction
    │   ├── jucePluginLib/        ← JUCE processor + parameter system
    │   ├── jucePluginEditorLib/  ← JUCE editor + RmlUi integration
    │   ├── dsp56300/             ← DSP56300 chip emulator (submodule)
    │   └── mc68k/                ← MC68000 CPU emulator (submodule)
    └── bin/plugins/Release/
        ├── AU/JE8086.component
        ├── AU/JE8086Morph.component   ← NEW
        ├── VST3/JE8086.vst3
        └── VST3/JE8086Morph.vst3      ← NEW
```

---

## Architecture Overview

```
MorphProcessor (AudioProcessor)
  └── jucePluginEditorLib::Processor
        └── jeJucePlugin::AudioPluginAudioProcessor
              ├── Engine A: jeLib::Device + synthLib::Plugin  ← original engine
              │     ├── MC68000 CPU emulation
              │     ├── DSP56300 chip emulation
              │     └── JP-8000 ROM (firmware)
              ├── Engine B: jeLib::Device + synthLib::Plugin  ← second engine (jeMorphPlugin)
              │     └── (same ROM, independent state)
              └── Controller  ← parameter management (shared, controls Engine A)
                    └── parameters → MIDI SysEx → Device

MorphEditorState
  ├── Editor A (jeJucePlugin::Editor) → jeTrancy RmlUi context
  ├── MorphContainer (juce::Component)
  │     ├── Engine A RmlComponent (left panel)
  │     ├── JUCE Slider + SliderParameterAttachment → "Morph A↔B" param
  │     └── Engine B RmlComponent (right panel)
  └── Editor B (jeJucePlugin::Editor) → jeTrancy RmlUi context
```

**Key technical notes:**
- Parameters are sent as **MIDI SysEx** to emulated hardware (no direct C++ parameter API)
- Engine B shares Engine A's Controller — its patch state is independent (audio-only morph at this stage)
- The morph slider is bidirectionally synced with the automation parameter via `juce::SliderParameterAttachment`
- MIDI note/CC events are forwarded to both engines; SysEx (patch data) goes only to Engine A

---

## Build

**Prerequisites:** Homebrew, cmake, Xcode 14+

```bash
cd gearmulator
git submodule update --init --recursive
bash build_mac.sh
```

**Install:**
```bash
cp -r bin/plugins/Release/AU/JE8086Morph.component ~/Library/Audio/Plug-Ins/Components/
```

**Validate:**
```bash
auval -v aumu Tjpm TusP
```

---

## Phase 2 — Custom Skin (Planned)

See [PLAN.md](PLAN.md) for the full plan.

Design a completely new UI in **Figma** (using the Figma MCP connector), then implement it as a new RmlUi skin:

```
jeMorphPlugin/skins/jeMorph/
├── jeMorph.rml     ← layout, binding to param="" attributes
├── jeMorph.rcss    ← sizing and style
└── *.png           ← exported from Figma
```
