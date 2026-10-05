# Montage Retiming

UE 5.7 plugin for designer/animator retiming in DeckKnight, with playback in Editor, Development and Shipping. No engine changes or animation rebaking are required.

## Use

1. Build/restart the DeckKnight editor. Open an animation montage and select **Section Retiming** in its toolbar. The panel can be docked beside the timeline.
2. Check **Enable Retiming**. This defaults to off for each montage.
3. Edit **Target frames**. Valid numeric edits update playback immediately; finishing the edit creates one undo step. Original frames, source fps, speed multiplier and target seconds are displayed for each section.
4. Select section checkboxes to set a common frame count or scale current target durations by a percentage. For example, 200% doubles duration and halves speed. Batch results round to whole source frames, with a minimum of one frame.
5. Use **Reset section** or **Reset All** to restore authored timing. Reset preserves the montage's enable toggle. Disabling the toggle preserves saved targets. Save the montage normally.
6. Use a row's **Loop** button to preview only that section repeatedly; click **Looping** to restore normal preview routing. Only one section loops at a time. **Jump to section** starts playback at that section and exits section-only looping. These controls affect the transient editor preview, not saved montage links or gameplay.
7. Changing **Target frames** restarts playback at the edited section immediately. Editing the looping section preserves its loop. Committing an unchanged value does not restart playback again. Batch edits and resets restart an affected looping section, or the first changed section in timeline order.
8. Manual scrubbing, preview transport changes, or editing the montage/source animation clears the tool's section loop. Normal authored section links are restored, including any explicitly authored loops. Use **Loop** again to resume section-only looping.
   Explicitly toggling **Looping** off resumes Persona's normal repeating preview, even if a prior transport action cleared its looping flag. Manual cancellation leaves implicit preview looping off. Non-looping previews hold their final animation pose instead of blending to reference pose; saved asset blend-out settings are unchanged.

Targets are saved as seconds on section metadata, so renames retain identity and source-rate changes retain intended duration. Original frame counts can be fractional because markers need not lie on source-frame boundaries. Explicit numeric edits round to whole frames. A target preserved across a sample-rate change can also display fractional frames until edited.

The original duration includes existing segment speeds and the montage's positive Rate Scale. Source fps comes from the animation data model, not game fps or platform resampling. Example: a baseline section of 10 frames at 30 fps retimed to 20 frames plays at 0.5x for 0.6667 seconds. An external Montage Play Rate of 2x halves that elapsed duration again. Speed shown in the panel is the retiming multiplier relative to authored playback.

Timeline-order start seconds are a convenience calculation assuming each earlier section plays once. They update after editing or resetting earlier sections. They are not a prediction of arrival time through jumps and loops. No route-dependent total duration is shown.

## Playback and integration

`UGASCourseAnimInstance` now derives from the plugin's `UMontageRetimingAnimInstance`. Existing `Montage_Play`, Gameplay Ability montage calls and rate changes made between animation updates use the settings automatically. Animation Blueprints using a different native base must derive from this base or integrate `MontageRetiming::Advance` into their existing `Montage_Advance` override.

Persona uses a transient `UAnimPreviewInstance` subclass with the same advancement adapter. Opening the panel installs it immediately; opted-in montage previews are also detected automatically. It is not saved into the asset or a Blueprint. The adapter preserves engine notify traversal, root-motion extraction and montage routing by advancing through the native implementation with the section's rate, splitting at section boundaries and branching events.

- Section duration is its marker to the next marker in timeline order, or montage end.
- Each target applies to one traversal. Existing next-section links and loops remain in control.
- Entering partway through uses that section's speed for the remaining animation.
- Section markers, source segments, curves and notify positions never move.
- Deleting a marker expands the preceding section. Surviving explicit targets remain fixed and speed is recalculated. Untargeted sections retain authored timing.
- Reordering carries section metadata with the section. Duplicated identities, coincident markers and invalid durations produce review warnings. Resetting an affected section creates a fresh identity.

Enabled retiming runs in both Development and Shipping using the same runtime adapter. Only the editor UI module is excluded from packaged games. Saving/cooking caches validated source sample rates on montage section metadata so runtime playback does not depend on stripped editor animation data. An enabled montage with invalid timing data reports a cook error instead of silently packaging different timing.

## First-version boundaries

- Multiple slot tracks are supported within `DefaultGroup`. Each section must use one consistent source sample rate across all overlapping animation sequence segments and slots.
- Mixed source rates show an editor warning. Invalid configurations fall back to authored playback for the whole montage, rather than applying only part of the requested retiming.
- DeckKnight Time Warp notify states (including subclasses and notifies on source sequences) are mutually exclusive with retiming. Remove Time Warp or disable retiming.
- Nested animation composites, marker-synchronized montages and existing montage Time Stretch Curves are rejected with a warning. Ordinary sequence segments can span sections and retain their authored segment play rates/loops.
- `Montage_Play` return values retain Unreal's authored-length semantics; use the panel for retimed durations. This plugin does not replace Unreal's montage replication/synchronization rules.
- Native branching callbacks that explicitly set an absolute rate to exactly the temporary effective rate are indistinguishable from leaving the rate untouched. Rate-changing notify systems should not be combined with this tool. Ordinary external rate changes between updates are supported.
- A 512-substep budget bounds pathological looping/hitch workloads; exceeding it logs a warning and drops the remaining advancement time for that update.

## Validation

The UE 5.7 DeckKnight Editor, Development game and Shipping game targets have been built and linked locally. All 15 tests in `DeckKnight.MontageRetiming` and `GASCourse.MeleeTrace` pass. Coverage includes mixed source rates across slots, cooked source-rate handling, metadata preservation on structural edits, large-tick boundary crossing, external-rate composition, disabled playback, loops, partial entry and notify alignment.

The combined combat test drives real montage advancement and melee notify dispatch, then feeds a deterministic rotating weapon transform to the existing post-animation collision path. It checks spheres, boxes and capsules at 0.5x, 2x and 4x speed with 0.1-second updates: the target is accepted once and trace windows are cleaned up. Existing weapon sockets, shape anchors, hit policies and sweep tests also pass. This checks playback/collision integration; it does not render or evaluate a production character's blended bone poses.

```powershell
Engine\Build\BatchFiles\Build.bat GASCourseEditor Win64 Development -Project=E:\DeckKnight_ExternalSSD\DeckKnight\GASCourse.uproject -MaxParallelActions=2 -NoHotReloadFromIDE -WaitMutex
Engine\Binaries\Win64\UnrealEditor-Cmd.exe E:\DeckKnight_ExternalSSD\DeckKnight\GASCourse.uproject -unattended -nop4 -NullRHI -nosound -DisablePlugins=Fab -NoLoadStartupPackages "-ExecCmds=Automation RunTests DeckKnight.MontageRetiming+GASCourse.MeleeTrace" "-TestExit=Automation Test Queue Empty"
```

Fab is disabled only for the headless test process because this checkout's saved Fab tab crashes during NullRHI startup. No project plugin preferences are changed by this command.

Before adopting the tool for a specific animation set, visually check a representative montage in Persona, PIE and a packaged game: live edits, undo/redo, batch/reset, a root-motion sequence, and its gameplay notify windows. Automated tests do not replace that visual/content check. The existing melee system sweeps between evaluated socket poses; this plugin does not add intermediate skeletal pose evaluations. Extremely short hit windows traversed within one update and complex motion during large hitches need content-specific validation at the minimum supported frame rate.
