/* ManipulationEditor.cpp
 *
 * Copyright (C) 1992-2022 Paul Boersma
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This code is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this work. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ManipulationEditor.h"
#include "PitchTier_to_PointProcess.h"
#include "Sound_to_PointProcess.h"
#include "Sound_to_Pitch.h"
#include "Pitch_to_PitchTier.h"
#include "Pitch_to_PointProcess.h"
#include "EditorM.h"

Thing_implement (ManipulationPitchTierArea, PitchTierArea, 0);

Thing_implement (ManipulationDurationTierArea, DurationTierArea, 0);

#include "enums_getText.h"
#include "ManipulationEditor_enums.h"
#include "enums_getValue.h"
#include "ManipulationEditor_enums.h"

Thing_implement (ManipulationEditor, FunctionEditor, 0);

#include "Prefs_define.h"
#include "ManipulationEditor_prefs.h"
#include "Prefs_install.h"
#include "ManipulationEditor_prefs.h"
#include "Prefs_copyToInstance.h"
#include "ManipulationEditor_prefs.h"

/*
 * How to add a synthesis method (in an interruptable order):
 * 1. add an Manipulation_ #define in Manipulation.h;
 * 2. add a synthesize_ routine in Manipulation.cpp, and a reference to it in Manipulation_to_Sound;
 * 3. add a button in ManipulationEditor.h;
 * 4. add a cb_Synth_ callback.
 * 5. create the button in createMenus and update updateMenus;
 */

static const conststring32 units_strings [] = { 0, U"Hz", U"st" };

static int prefs_synthesisMethod = Manipulation_OVERLAPADD;   /* Remembered across editor creations, not across Praat sessions. */

static void updateMenus (ManipulationEditor me) {
	Melder_assert (my synthPulsesButton);
	GuiMenuItem_check (my synthPulsesButton, my synthesisMethod == Manipulation_PULSES);
	Melder_assert (my synthPulsesHumButton);
	GuiMenuItem_check (my synthPulsesHumButton, my synthesisMethod == Manipulation_PULSES_HUM);
	Melder_assert (my synthPulsesLpcButton);
	GuiMenuItem_check (my synthPulsesLpcButton, my synthesisMethod == Manipulation_PULSES_LPC);
	Melder_assert (my synthPitchButton);
	GuiMenuItem_check (my synthPitchButton, my synthesisMethod == Manipulation_PITCH);
	Melder_assert (my synthPitchHumButton);
	GuiMenuItem_check (my synthPitchHumButton, my synthesisMethod == Manipulation_PITCH_HUM);
	Melder_assert (my synthPulsesPitchButton);
	GuiMenuItem_check (my synthPulsesPitchButton, my synthesisMethod == Manipulation_PULSES_PITCH);
	Melder_assert (my synthPulsesPitchHumButton);
	GuiMenuItem_check (my synthPulsesPitchHumButton, my synthesisMethod == Manipulation_PULSES_PITCH_HUM);
	Melder_assert (my synthOverlapAddButton);
	GuiMenuItem_check (my synthOverlapAddButton, my synthesisMethod == Manipulation_OVERLAPADD);
	Melder_assert (my synthPitchLpcButton);
	GuiMenuItem_check (my synthPitchLpcButton, my synthesisMethod == Manipulation_PITCH_LPC);
}

/*
	The "sound area" contains the original sound and the pulses.
 */
static bool getSoundArea (ManipulationEditor me, double *ymin, double *ymax) {
	*ymin = 0.67;
	*ymax = 1.00;
	return my sound() || my pulses();
}

/********** MENU COMMANDS **********/

/***** FILE MENU *****/

static void CONVERT_DATA_TO_ONE__ExtractOriginalSound (ManipulationEditor me, EDITOR_ARGS_DIRECT_WITH_OUTPUT) {
	CONVERT_DATA_TO_ONE
		if (! my sound())
			return;
		autoSound result = Data_copy (my sound());
	CONVERT_DATA_TO_ONE_END (U"untitled")
}

static void CONVERT_DATA_TO_ONE__ExtractPulses (ManipulationEditor me, EDITOR_ARGS_DIRECT_WITH_OUTPUT) {
	CONVERT_DATA_TO_ONE
		if (! my pulses())
			return;
		autoPointProcess result = Data_copy (my pulses());
	CONVERT_DATA_TO_ONE_END (U"untitled")
}

static void CONVERT_DATA_TO_ONE__ExtractPitchTier (ManipulationEditor me, EDITOR_ARGS_DIRECT_WITH_OUTPUT) {
	CONVERT_DATA_TO_ONE
		if (! my pitch())
			return;
		autoPitchTier result = Data_copy (my pitch());
	CONVERT_DATA_TO_ONE_END (U"untitled")
}

static void CONVERT_DATA_TO_ONE__ExtractDurationTier (ManipulationEditor me, EDITOR_ARGS_DIRECT_WITH_OUTPUT) {
	CONVERT_DATA_TO_ONE
		if (! my duration())
			return;
		autoDurationTier result = Data_copy (my duration());
	CONVERT_DATA_TO_ONE_END (U"untitled")
}

static void CONVERT_DATA_TO_ONE__ExtractManipulatedSound (ManipulationEditor me, EDITOR_ARGS_DIRECT_WITH_OUTPUT) {
	CONVERT_DATA_TO_ONE
		autoSound result = Manipulation_to_Sound (my manipulation(), my synthesisMethod);
	CONVERT_DATA_TO_ONE_END (U"untitled")
}

/***** EDIT MENU *****/

void structManipulationEditor :: v_saveData () {
	our previousPulses   = Data_copy (our pulses());     // could be null
	our previousPitch    = Data_copy (our pitch());      // could be null
	our previousDuration = Data_copy (our duration());   // could be null
}

void structManipulationEditor :: v_restoreData () {
	std::swap (our manipulation() -> pulses,   our previousPulses);     // could be null
	std::swap (our manipulation() -> pitch,    our previousPitch);      // could be null
	std::swap (our manipulation() -> duration, our previousDuration);   // could be null
}

/***** PULSES MENU *****/

static void menu_cb_removePulses (ManipulationEditor me, EDITOR_ARGS_DIRECT) {
	if (! my pulses())
		return;
	Editor_save (me, U"Remove pulse(s)");
	if (my startSelection == my endSelection)
		PointProcess_removePointNear (my pulses(), my startSelection);
	else
		PointProcess_removePointsBetween (my pulses(), my startSelection, my endSelection);
	FunctionEditor_redraw (me);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_addPulseAtCursor (ManipulationEditor me, EDITOR_ARGS_DIRECT) {
	if (! my pulses())
		return;
	Editor_save (me, U"Add pulse");
	PointProcess_addPoint (my pulses(), 0.5 * (my startSelection + my endSelection));
	FunctionEditor_redraw (me);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_addPulseAt (ManipulationEditor me, EDITOR_ARGS_FORM) {
	EDITOR_FORM (U"Add pulse", nullptr)
		REAL (position, U"Position (s)", U"0.0")
	EDITOR_OK
		SET_REAL (position, 0.5 * (my startSelection + my endSelection))
	EDITOR_DO
		if (! my pulses())
			return;
		Editor_save (me, U"Add pulse");
		PointProcess_addPoint (my pulses(), position);
		FunctionEditor_redraw (me);
		Editor_broadcastDataChanged (me);
	EDITOR_END
}

/***** PITCH MENU *****/

static void menu_cb_removePitchPoints (ManipulationEditor me, EDITOR_ARGS_DIRECT) {
	if (! my pitch())
		return;
	Editor_save (me, U"Remove pitch point(s)");
	if (my startSelection == my endSelection)
		AnyTier_removePointNear (my pitch()->asAnyTier(), my startSelection);
	else
		AnyTier_removePointsBetween (my pitch()->asAnyTier(), my startSelection, my endSelection);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_addPitchPointAtCursor (ManipulationEditor me, EDITOR_ARGS_DIRECT) {
	if (! my pitch())
		return;
	Editor_save (me, U"Add pitch point");
	RealTier_addPoint (my pitch(), 0.5 * (my startSelection + my endSelection), my pitchTierArea -> ycursor);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_addPitchPointAtSlice (ManipulationEditor me, EDITOR_ARGS_DIRECT) {
	if (! my pulses())
		Melder_throw (U"There are no pulses.");
	if (! my pitch())
		return;
	const integer ileft = PointProcess_getLowIndex (my pulses(), 0.5 * (my startSelection + my endSelection));
	const integer iright = ileft + 1, nt = my pulses() -> nt;
	constVEC t = my pulses() -> t.get();
	double desiredY = my pitchTierArea -> ycursor;   // default
	Editor_save (me, U"Add pitch point");
	if (nt <= 1) {
		/* Ignore. */
	} else if (ileft <= 0) {
		double tright = t [2] - t [1];
		if (tright > 0.0 && tright <= 0.02)
			desiredY = 1.0 / tright;
	} else if (iright > nt) {
		double tleft = t [nt] - t [nt - 1];
		if (tleft > 0.0 && tleft <= 0.02)
			desiredY = 1.0 / tleft;
	} else {   /* Three-period median. */
		double tmid = t [iright] - t [ileft], tleft = 0.0, tright = 0.0;
		if (ileft > 1)
			tleft = t [ileft] - t [ileft - 1];
		if (iright < nt)
			tright = t [iright + 1] - t [iright];
		if (tleft > 0.02)
			tleft = 0;
		if (tmid > 0.02)
			tmid = 0;
		if (tright > 0.02)
			tright = 0;
		/* Bubble-sort. */
		if (tmid < tleft)
			std::swap (tmid, tleft);
		if (tright < tleft)
			std::swap (tright, tleft);
		if (tright < tmid)
			std::swap (tright, tmid);
		if (tleft != 0.0)
			desiredY = 1.0 / tmid;   // median of 3
		else if (tmid != 0.0)
			desiredY = 2.0 / (tmid + tright);   // median of 2
		else if (tright != 0.0)
			desiredY = 1.0 / tright;   // median of 1
	}
	RealTierArea_addPointAt (my pitchTierArea.get(), 0.5 * (my startSelection + my endSelection), desiredY);
	Editor_broadcastDataChanged (me);
}	

static void menu_cb_addPitchPointAt (ManipulationEditor me, EDITOR_ARGS_FORM) {
	EDITOR_FORM (U"Add pitch point", nullptr)
		REAL (time, U"Time (s)", U"0.0")
		REAL (frequency, U"Frequency (Hz or st)", U"100.0")
	EDITOR_OK
		SET_REAL (time, 0.5 * (my startSelection + my endSelection))
		SET_REAL (frequency, my pitchTierArea -> ycursor)
	EDITOR_DO
		if (! my pitch())
			return;
		Editor_save (me, U"Add pitch point");
		RealTierArea_addPointAt (my pitchTierArea.get(), time, frequency);
		Editor_broadcastDataChanged (me);
	EDITOR_END
}

static void menu_cb_stylizePitch (ManipulationEditor me, EDITOR_ARGS_FORM) {
	EDITOR_FORM (U"Stylize pitch", U"PitchTier: Stylize...")
		REAL (frequencyResolution, U"Frequency resolution", my default_pitch_stylize_frequencyResolution ())
		RADIO (units, U"Units", my default_pitch_stylize_useSemitones () + 1)
			RADIOBUTTON (U"Hertz")
			RADIOBUTTON (U"semitones")
	EDITOR_OK
		SET_REAL   (frequencyResolution, my instancePref_pitch_stylize_frequencyResolution())
		SET_OPTION (units,               my instancePref_pitch_stylize_useSemitones() + 1)
	EDITOR_DO
		if (! my pitch())
			return;
		Editor_save (me, U"Stylize pitch");
		my setInstancePref_pitch_stylize_frequencyResolution (frequencyResolution);
		my setInstancePref_pitch_stylize_useSemitones (units - 1);
		PitchTier_stylize (my pitch(), frequencyResolution, my instancePref_pitch_stylize_useSemitones());
		Editor_broadcastDataChanged (me);
	EDITOR_END
}

static void menu_cb_stylizePitch_2st (ManipulationEditor me, EDITOR_ARGS_DIRECT) {
	if (! my pitch())
		return;
	Editor_save (me, U"Stylize pitch");
	PitchTier_stylize (my pitch(), 2.0, true);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_interpolateQuadratically (ManipulationEditor me, EDITOR_ARGS_FORM) {
	EDITOR_FORM (U"Interpolate quadratically", nullptr)
		NATURAL (numberOfPointsPerParabola, U"Number of points per parabola", my default_pitch_interpolateQuadratically_numberOfPointsPerParabola ())
		RADIO (units, U"Units", my default_pitch_stylize_useSemitones () + 1)
			RADIOBUTTON (U"Hertz")
			RADIOBUTTON (U"semitones")
	EDITOR_OK
		SET_INTEGER (numberOfPointsPerParabola, my instancePref_pitch_interpolateQuadratically_numberOfPointsPerParabola())
		SET_OPTION  (units,                     my instancePref_pitch_stylize_useSemitones() + 1)
	EDITOR_DO
		if (! my pitch())
			return;
		Editor_save (me, U"Interpolate quadratically");
		my setInstancePref_pitch_interpolateQuadratically_numberOfPointsPerParabola (numberOfPointsPerParabola);
		my setInstancePref_pitch_stylize_useSemitones (units - 1);
		RealTier_interpolateQuadratically (my pitch(), numberOfPointsPerParabola, my instancePref_pitch_stylize_useSemitones());
		Editor_broadcastDataChanged (me);
	EDITOR_END
}

static void menu_cb_shiftPitchFrequencies (ManipulationEditor me, EDITOR_ARGS_FORM) {
	EDITOR_FORM (U"Shift pitch frequencies", nullptr)
		REAL (frequencyShift, U"Frequency shift", U"-20.0")
		OPTIONMENU (unit_i, U"Unit", 1)
			OPTION (U"Hertz")
			OPTION (U"mel")
			OPTION (U"logHertz")
			OPTION (U"semitones")
			OPTION (U"ERB")
	EDITOR_OK
	EDITOR_DO
		kPitch_unit unit =
			unit_i == 1 ? kPitch_unit::HERTZ :
			unit_i == 2 ? kPitch_unit::MEL :
			unit_i == 3 ? kPitch_unit::LOG_HERTZ :
			unit_i == 4 ? kPitch_unit::SEMITONES_1 :
			kPitch_unit::ERB;
		if (! my pitch())
			return;
		Editor_save (me, U"Shift pitch frequencies");
		try {
			PitchTier_shiftFrequencies (my pitch(), my startSelection, my endSelection, frequencyShift, unit);
			Editor_broadcastDataChanged (me);
		} catch (MelderError) {
			// the PitchTier may have partially changed
			Editor_broadcastDataChanged (me);
			throw;
		}
	EDITOR_END
}

static void menu_cb_multiplyPitchFrequencies (ManipulationEditor me, EDITOR_ARGS_FORM) {
	EDITOR_FORM (U"Multiply pitch frequencies", nullptr)
		POSITIVE (factor, U"Factor", U"1.2")
		LABEL (U"The multiplication is always done in hertz.")
	EDITOR_OK
	EDITOR_DO
		if (! my pitch())
			return;
		Editor_save (me, U"Multiply pitch frequencies");
		PitchTier_multiplyFrequencies (my pitch(), my startSelection, my endSelection, factor);
		Editor_broadcastDataChanged (me);
	EDITOR_END
}

static void menu_cb_setPitchRange (ManipulationEditor me, EDITOR_ARGS_FORM) {
	EDITOR_FORM (U"Set pitch range", nullptr)
		REAL (dataFreeMinimum, U"Data-free minimum (Hz)", my pitchTierArea -> default_dataFreeMinimum())
		REAL (dataFreeMaximum, U"Data-free maximum (Hz)", my pitchTierArea -> default_dataFreeMaximum())
	EDITOR_OK
		SET_REAL (dataFreeMinimum, my pitchTierArea -> instancePref_dataFreeMinimum())
		SET_REAL (dataFreeMaximum, my pitchTierArea -> instancePref_dataFreeMaximum())
	EDITOR_DO
		my pitchTierArea -> setInstancePref_dataFreeMinimum (dataFreeMinimum);
		my pitchTierArea -> setInstancePref_dataFreeMaximum (dataFreeMaximum);
		my pitchTierArea -> v_updateScaling ();   // BUG: should be viewChanged()
		FunctionEditor_redraw (me);
	EDITOR_END
}

/***** DURATION MENU *****/

static void menu_cb_setDurationRange (ManipulationEditor me, EDITOR_ARGS_FORM) {
	EDITOR_FORM (U"Set duration range", nullptr)
		REAL (dataFreeMinimum, U"Data-free minimum", my durationTierArea -> default_dataFreeMinimum())
		REAL (dataFreeMaximum, U"Data-free maximum", my durationTierArea -> default_dataFreeMaximum())
	EDITOR_OK
		SET_REAL (dataFreeMinimum, my durationTierArea -> instancePref_dataFreeMinimum())
		SET_REAL (dataFreeMaximum, my durationTierArea -> instancePref_dataFreeMaximum())
	EDITOR_DO
		if (dataFreeMinimum > 1.0)
			Melder_throw (U"Minimum relative duration should not be greater than 1.");
		if (dataFreeMaximum < 1.0)
			Melder_throw (U"Maximum relative duration should not be less than 1.");
		if (dataFreeMinimum >= dataFreeMaximum)
			Melder_throw (U"Maximum relative duration should be greater than minimum.");
		my durationTierArea -> setInstancePref_dataFreeMinimum (dataFreeMinimum);
		my durationTierArea -> setInstancePref_dataFreeMaximum (dataFreeMaximum);
		my durationTierArea -> v_updateScaling ();
		FunctionEditor_redraw (me);
	EDITOR_END
}

static void menu_cb_setDraggingStrategy (ManipulationEditor me, EDITOR_ARGS_FORM) {
	EDITOR_FORM (U"Set dragging strategy", U"ManipulationEditor")
		RADIO_ENUM (kManipulationEditor_draggingStrategy, draggingStrategy,
				U"Dragging strategy", my default_pitch_draggingStrategy ())
	EDITOR_OK
		SET_ENUM (draggingStrategy, kManipulationEditor_draggingStrategy, my instancePref_pitch_draggingStrategy())
	EDITOR_DO
		my setInstancePref_pitch_draggingStrategy (draggingStrategy);
	EDITOR_END
}

static void menu_cb_removeDurationPoints (ManipulationEditor me, EDITOR_ARGS_DIRECT) {
	if (! my duration())
		return;
	Editor_save (me, U"Remove duration point(s)");
	if (my startSelection == my endSelection)
		AnyTier_removePointNear (my duration()->asAnyTier(), 0.5 * (my startSelection + my endSelection));
	else
		AnyTier_removePointsBetween (my duration()->asAnyTier(), my startSelection, my endSelection);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_addDurationPointAtCursor (ManipulationEditor me, EDITOR_ARGS_DIRECT) {
	if (! my duration())
		return;
	Editor_save (me, U"Add duration point");
	RealTier_addPoint (my duration(), 0.5 * (my startSelection + my endSelection), my durationTierArea -> ycursor);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_addDurationPointAt (ManipulationEditor me, EDITOR_ARGS_FORM) {
	EDITOR_FORM (U"Add duration point", nullptr)
		REAL (time, U"Time (s)", U"0.0");
		REAL (relativeDuration, U"Relative duration", U"1.0");
	EDITOR_OK
		SET_REAL (time, 0.5 * (my startSelection + my endSelection))
	EDITOR_DO
		if (! my duration())
			return;
		Editor_save (me, U"Add duration point");
		RealTierArea_addPointAt (my durationTierArea.get(), time, relativeDuration);
		Editor_broadcastDataChanged (me);
	EDITOR_END
}

static void menu_cb_newDuration (ManipulationEditor me, EDITOR_ARGS_DIRECT) {
	Editor_save (me, U"New duration");
	my manipulation() -> duration = DurationTier_create (my manipulation() -> xmin, my manipulation() -> xmax);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_forgetDuration (ManipulationEditor me, EDITOR_ARGS_DIRECT) {
	Editor_save (me, U"Forget duration");
	my manipulation() -> duration = autoDurationTier();
	Editor_broadcastDataChanged (me);
}
	
static void menu_cb_ManipulationEditorHelp (ManipulationEditor, EDITOR_ARGS_DIRECT) { Melder_help (U"ManipulationEditor"); }
static void menu_cb_ManipulationHelp (ManipulationEditor, EDITOR_ARGS_DIRECT) { Melder_help (U"Manipulation"); }

#define menu_cb_Synth_common(menu_cb,meth) \
static void menu_cb (ManipulationEditor me, EDITOR_ARGS_DIRECT) { \
	prefs_synthesisMethod = my synthesisMethod = meth; \
	updateMenus (me); \
}
menu_cb_Synth_common (menu_cb_Synth_Pulses, Manipulation_PULSES)
menu_cb_Synth_common (menu_cb_Synth_Pulses_hum, Manipulation_PULSES_HUM)
menu_cb_Synth_common (menu_cb_Synth_Pulses_Lpc, Manipulation_PULSES_LPC)
menu_cb_Synth_common (menu_cb_Synth_Pitch, Manipulation_PITCH)
menu_cb_Synth_common (menu_cb_Synth_Pitch_hum, Manipulation_PITCH_HUM)
menu_cb_Synth_common (menu_cb_Synth_Pulses_Pitch, Manipulation_PULSES_PITCH)
menu_cb_Synth_common (menu_cb_Synth_Pulses_Pitch_hum, Manipulation_PULSES_PITCH_HUM)
menu_cb_Synth_common (menu_cb_Synth_OverlapAdd_nodur, Manipulation_OVERLAPADD_NODUR)
menu_cb_Synth_common (menu_cb_Synth_OverlapAdd, Manipulation_OVERLAPADD)
menu_cb_Synth_common (menu_cb_Synth_Pitch_Lpc, Manipulation_PITCH_LPC)

void structManipulationEditor :: v_createMenus () {
	ManipulationEditor_Parent :: v_createMenus ();

	Editor_addCommand (this, U"File", U"Extract original sound", 0,
			CONVERT_DATA_TO_ONE__ExtractOriginalSound);
	Editor_addCommand (this, U"File", U"Extract pulses", 0,
			CONVERT_DATA_TO_ONE__ExtractPulses);
	Editor_addCommand (this, U"File", U"Extract pitch tier", 0,
			CONVERT_DATA_TO_ONE__ExtractPitchTier);
	Editor_addCommand (this, U"File", U"Extract duration tier", 0,
			CONVERT_DATA_TO_ONE__ExtractDurationTier);
	Editor_addCommand (this, U"File", U"Publish resynthesis", 0,
			CONVERT_DATA_TO_ONE__ExtractManipulatedSound);
	Editor_addCommand (this, U"File", U"-- close --", 0, nullptr);

	Editor_addMenu (this, U"Pulse", 0);
	Editor_addCommand (this, U"Pulse", U"Add pulse at cursor", 'P', menu_cb_addPulseAtCursor);
	Editor_addCommand (this, U"Pulse", U"Add pulse at...", 0, menu_cb_addPulseAt);
	Editor_addCommand (this, U"Pulse", U"-- remove pulses --", 0, nullptr);
	Editor_addCommand (this, U"Pulse", U"Remove pulse(s)", GuiMenu_OPTION | 'P', menu_cb_removePulses);

	Editor_addMenu (this, U"Pitch", 0);
	Editor_addCommand (this, U"Pitch", U"Add pitch point at cursor", 'T', menu_cb_addPitchPointAtCursor);
	Editor_addCommand (this, U"Pitch", U"Add pitch point at time slice", 0, menu_cb_addPitchPointAtSlice);
	Editor_addCommand (this, U"Pitch", U"Add pitch point at...", 0, menu_cb_addPitchPointAt);
	Editor_addCommand (this, U"Pitch", U"-- remove pitch --", 0, nullptr);
	Editor_addCommand (this, U"Pitch", U"Remove pitch point(s)", GuiMenu_OPTION | 'T', menu_cb_removePitchPoints);
	Editor_addCommand (this, U"Pitch", U"-- pitch prefs --", 0, nullptr);
	Editor_addCommand (this, U"Pitch", U"Set pitch range...", 0, menu_cb_setPitchRange);
	Editor_addCommand (this, U"Pitch", U"Set pitch dragging strategy...", 0, menu_cb_setDraggingStrategy);
	Editor_addCommand (this, U"Pitch", U"-- modify pitch --", 0, nullptr);
	Editor_addCommand (this, U"Pitch", U"Shift pitch frequencies...", 0, menu_cb_shiftPitchFrequencies);
	Editor_addCommand (this, U"Pitch", U"Multiply pitch frequencies...", 0, menu_cb_multiplyPitchFrequencies);
	Editor_addCommand (this, U"Pitch", U"All:", GuiMenu_INSENSITIVE, menu_cb_stylizePitch);
	Editor_addCommand (this, U"Pitch", U"Stylize pitch...", 0, menu_cb_stylizePitch);
	Editor_addCommand (this, U"Pitch", U"Stylize pitch (2 st)", '2', menu_cb_stylizePitch_2st);
	Editor_addCommand (this, U"Pitch", U"Interpolate quadratically...", 0, menu_cb_interpolateQuadratically);

	Editor_addMenu (this, U"Dur", 0);
	Editor_addCommand (this, U"Dur", U"Add duration point at cursor", 'D', menu_cb_addDurationPointAtCursor);
	Editor_addCommand (this, U"Dur", U"Add duration point at...", 0, menu_cb_addDurationPointAt);
	Editor_addCommand (this, U"Dur", U"-- remove duration --", 0, nullptr);
	Editor_addCommand (this, U"Dur", U"Remove duration point(s)", GuiMenu_OPTION | 'D', menu_cb_removeDurationPoints);
	Editor_addCommand (this, U"Dur", U"-- duration prefs --", 0, nullptr);
	Editor_addCommand (this, U"Dur", U"Set duration range...", 0, menu_cb_setDurationRange);
	Editor_addCommand (this, U"Dur", U"-- refresh duration --", 0, nullptr);
	Editor_addCommand (this, U"Dur", U"New duration", 0, menu_cb_newDuration);
	Editor_addCommand (this, U"Dur", U"Forget duration", 0, menu_cb_forgetDuration);

	Editor_addMenu (this, U"Synth", 0);
	our synthPulsesButton = Editor_addCommand (this, U"Synth", U"Pulses --", GuiMenu_RADIO_FIRST, menu_cb_Synth_Pulses);
	our synthPulsesHumButton = Editor_addCommand (this, U"Synth", U"Pulses (hum) --", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pulses_hum);

	our synthPulsesLpcButton = Editor_addCommand (this, U"Synth", U"Pulses & LPC -- (\"LPC resynthesis\")", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pulses_Lpc);
	Editor_addCommand (this, U"Synth", U"-- pitch resynth --", 0, nullptr);
	our synthPitchButton = Editor_addCommand (this, U"Synth", U" -- Pitch", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pitch);
	our synthPitchHumButton = Editor_addCommand (this, U"Synth", U" -- Pitch (hum)", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pitch_hum);
	our synthPulsesPitchButton = Editor_addCommand (this, U"Synth", U"Pulses -- Pitch", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pulses_Pitch);
	our synthPulsesPitchHumButton = Editor_addCommand (this, U"Synth", U"Pulses -- Pitch (hum)", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pulses_Pitch_hum);
	Editor_addCommand (this, U"Synth", U"-- full resynth --", 0, nullptr);
	our synthOverlapAddButton = Editor_addCommand (this, U"Synth", U"Sound & Pulses -- Pitch & Duration  (\"Overlap-add manipulation\")", GuiMenu_RADIO_NEXT | GuiMenu_TOGGLE_ON, menu_cb_Synth_OverlapAdd);
	our synthPitchLpcButton = Editor_addCommand (this, U"Synth", U"LPC -- Pitch  (\"LPC pitch manipulation\")", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pitch_Lpc);
}

void structManipulationEditor :: v_createHelpMenuItems (EditorMenu menu) {
	ManipulationEditor_Parent :: v_createHelpMenuItems (menu);
	EditorMenu_addCommand (menu, U"ManipulationEditor help", '?', menu_cb_ManipulationEditorHelp);
	EditorMenu_addCommand (menu, U"Manipulation help", 0, menu_cb_ManipulationHelp);
}

/********** DRAWING AREA **********/

void structManipulationEditor :: v_distributeAreas () {
	our pitchTierArea -> setGlobalYRange_fraction (( our duration() ? 0.17 : 0.0 ), 0.67);
	if (our duration())
		our durationTierArea -> setGlobalYRange_fraction (0.0, 0.17);
}

static void drawSoundArea (ManipulationEditor me, double ymin, double ymax) {
	Graphics_Viewport viewport = Graphics_insetViewport (my graphics.get(), 0.0, 1.0, ymin, ymax);
	Graphics_setWindow (my graphics.get(), 0.0, 1.0, 0.0, 1.0);
	Graphics_setColour (my graphics.get(), Melder_WHITE);
	Graphics_fillRectangle (my graphics.get(), 0.0, 1.0, 0.0, 1.0);
	Graphics_setColour (my graphics.get(), Melder_BLACK);
	Graphics_rectangle (my graphics.get(), 0.0, 1.0, 0.0, 1.0);
	Graphics_setTextAlignment (my graphics.get(), Graphics_RIGHT, Graphics_TOP);
	Graphics_setFont (my graphics.get(), kGraphics_font::TIMES);
	Graphics_text (my graphics.get(), 1.0, 1.0, U"%%Sound");
	Graphics_setColour (my graphics.get(), Melder_BLUE);
	Graphics_text (my graphics.get(), 1.0, 1.0 - Graphics_dyMMtoWC (my graphics.get(), 3), U"%%Pulses");
	Graphics_setFont (my graphics.get(), kGraphics_font::HELVETICA);

	/*
		Draw blue pulses.
	*/
	if (my pulses()) {
		Graphics_setWindow (my graphics.get(), my startWindow, my endWindow, 0.0, 1.0);
		Graphics_setColour (my graphics.get(), Melder_BLUE);
		for (integer i = 1; i <= my pulses() -> nt; i ++) {
			const double t = my pulses() -> t [i];
			if (t >= my startWindow && t <= my endWindow)
				Graphics_line (my graphics.get(), t, 0.05, t, 0.95);
		}
	}

	/*
		Draw sound.
	*/
	integer first, last;
	if (my sound() && Sampled_getWindowSamples (my sound(), my startWindow, my endWindow, & first, & last) > 1) {
		double minimum, maximum, scaleMin, scaleMax;
		Matrix_getWindowExtrema (my sound(), first, last, 1, 1, & minimum, & maximum);
		if (minimum == maximum) {
			minimum = -0.5;
			maximum = +0.5;
		}
		/*
			Scaling.
		*/
		scaleMin = 0.83 * minimum + 0.17 * my soundmin;
		scaleMax = 0.83 * maximum + 0.17 * my soundmax;
		Graphics_setWindow (my graphics.get(), my startWindow, my endWindow, scaleMin, scaleMax);
		FunctionEditor_drawRangeMark (me, scaleMin, Melder_float (Melder_half (scaleMin)), U"", Graphics_BOTTOM);
		FunctionEditor_drawRangeMark (me, scaleMax, Melder_float (Melder_half (scaleMax)), U"", Graphics_TOP);
		/*
			Draw dotted zero line.
		*/
		if (minimum < 0.0 && maximum > 0.0) {
			Graphics_setColour (my graphics.get(), Melder_CYAN);
			Graphics_setLineType (my graphics.get(), Graphics_DOTTED);
			Graphics_line (my graphics.get(), my startWindow, 0.0, my endWindow, 0.0);
			Graphics_setLineType (my graphics.get(), Graphics_DRAWN);
		}
		/*
			Draw samples.
		*/
		Graphics_setColour (my graphics.get(), Melder_BLACK);
		Graphics_function (my graphics.get(), & my sound() -> z [1] [0], first, last,
				Sampled_indexToX (my sound(), first), Sampled_indexToX (my sound(), last));
	}

	Graphics_resetViewport (my graphics.get(), viewport);
}

void structManipulationPitchTierArea :: v_drawInside () {
	const bool cursorVisible = (
		our startSelection() == our endSelection() &&
		our startSelection() >= our startWindow() &&
		our startSelection() <= our endWindow()
	);

	Graphics_setWindow (graphics(), 0.0, 1.0, 0.0, 1.0);
	Graphics_setColour (graphics(), Melder_BLUE);
	Graphics_setFont (graphics(), kGraphics_font::TIMES);
	Graphics_setTextAlignment (graphics(), Graphics_RIGHT, Graphics_TOP);
	Graphics_text (graphics(), 1.0, 1.0, U"%%Pitch manip");
	Graphics_setColour (graphics(), MelderColour (0.7));
	Graphics_text (graphics(), 1.0, 1.0 - Graphics_dyMMtoWC (graphics(), 3.0), U"%%Pitch from pulses");
	Graphics_setFont (graphics(), kGraphics_font::HELVETICA);

	Graphics_setWindow (graphics(), startWindow(), endWindow(), our ymin, our ymax);

	/*
		Draw pitch contour based on pulses.
	*/
	Graphics_setGrey (graphics(), 0.7);
	/* BUG: should inlcude grey dots */
	constPointProcess pulses = ((ManipulationEditor) functionEditor()) -> pulses();
	if (pulses) for (integer i = 1; i < pulses -> nt; i ++) {
		const double tleft = pulses -> t [i], tright = pulses -> t [i + 1], t = 0.5 * (tleft + tright);
		if (t >= startWindow() && t <= endWindow()) {
			if (tleft != tright) {
				const double y = 1.0 / (tright - tleft);
				if (y >= our ymin && y <= our ymax)
					Graphics_fillCircle_mm (graphics(), t, y, 1.0);
			}
		}
	}
	Graphics_setGrey (graphics(), 0.0);

	constexpr double minimumFrequency = 50.0;
	FunctionEditor_drawGridLine (functionEditor(), minimumFrequency);

	const integer n = ( our pitchTier() ? our pitchTier() -> points.size : 0 );
	if (cursorVisible && n > 0) {
		const double y = RealTier_getValueAtTime (pitchTier(), startSelection());
		FunctionEditor_insertCursorFunctionValue (functionEditor(), y,
			Melder_fixed (y, 1), U" Hz",
			our ymin, our ymax);
	}
	ManipulationPitchTierArea_Parent :: v_drawInside ();
	if (isdefined (our anchorTime))
		RealTierArea_drawWhileDragging (this);
}

void structManipulationDurationTierArea :: v_drawInside () {
	const bool cursorVisible = (
		startSelection() == endSelection() &&
		startSelection() >= startWindow() &&
		startSelection() <= endWindow()
	);

	Graphics_setWindow (graphics(), 0.0, 1.0, 0.0, 1.0);
	Graphics_setColour (graphics(), Melder_BLUE);
	Graphics_setFont (graphics(), kGraphics_font::TIMES);
	Graphics_setTextAlignment (graphics(), Graphics_RIGHT, Graphics_TOP);
	Graphics_text (graphics(), 1.0, 1.0, U"%%Duration manip");
	Graphics_setFont (graphics(), kGraphics_font::HELVETICA);

	Graphics_setWindow (graphics(), startWindow(), endWindow(), our ymin, our ymax);
	FunctionEditor_drawGridLine (functionEditor(), 1.0);   // BUG: should move to FunctionArea
	if (cursorVisible && durationTier() -> points.size > 0) {
		const double y = RealTier_getValueAtTime (durationTier(), startSelection());
		FunctionEditor_insertCursorFunctionValue (functionEditor(), y,   // BUG: should move to FunctionArea
				Melder_fixed (y, 3), U"", our ymin, our ymax);
	}
	ManipulationDurationTierArea_Parent :: v_drawInside ();
	if (isdefined (our anchorTime))
		RealTierArea_drawWhileDragging (this);
}

void structManipulationEditor :: v_draw () {
	double ysoundmin, ysoundmax;
	(void) getSoundArea (this, & ysoundmin, & ysoundmax);
	if (our sound())
		drawSoundArea (this, ysoundmin, ysoundmax);
	if (our pitch())
		FunctionArea_drawOne (our pitchTierArea.get());
	if (our duration())
		FunctionArea_drawOne (our durationTierArea.get());
	updateMenus (this);
}

bool structManipulationEditor :: v_mouseInWideDataView (GuiDrawingArea_MouseEvent event, double x_world, double globalY_fraction) {
	if (event -> isClick()) {
		our clickedInWidePitchArea = our pitchTierArea -> y_fraction_globalIsInside (globalY_fraction);
		our clickedInWideDurationArea = our durationTierArea -> y_fraction_globalIsInside (globalY_fraction);
	}
	bool result = false;
	if (our clickedInWidePitchArea) {
		result = RealTierArea_mouse (our pitchTierArea.get(), event, x_world, globalY_fraction);
		our pitchTierArea -> v_updateScaling ();
	} else if (our clickedInWideDurationArea) {
		result = RealTierArea_mouse (our durationTierArea.get(), event, x_world, globalY_fraction);
		our durationTierArea -> v_updateScaling ();   // BUG: this is a method instead of a message
	} else {
		result = our ManipulationEditor_Parent :: v_mouseInWideDataView (event, x_world, globalY_fraction);
	}
	if (event -> isDrop()) {
		our clickedInWidePitchArea = false;
		our clickedInWideDurationArea = false;
	}
	return result;
}

void structManipulationEditor :: v_play (double startTime, double endTime) {
	if (our clickWasModifiedByShiftKey) {
		if (our sound())
			Sound_playPart (our sound(), startTime, endTime, theFunctionEditor_playCallback, this);
	} else {
		Manipulation_playPart (our manipulation(), startTime, endTime, our synthesisMethod);
	}
}

autoManipulationEditor ManipulationEditor_create (conststring32 title, Manipulation manipulation) {
	try {
		autoManipulationEditor me = Thing_new (ManipulationEditor);
		my pitchTierArea = ManipulationPitchTierArea_create (true, nullptr, me.get());
		my durationTierArea = ManipulationDurationTierArea_create (true, nullptr, me.get());
		my durationTierArea -> ycursor = 1.0;   // BUG: should be in v1_dataChanged() or in member initialization (undefined there, perhaps?)
		FunctionEditor_init (me.get(), title, manipulation);

		my synthesisMethod = prefs_synthesisMethod; // BUG: should be in v1_dataChanged()
		if (manipulation -> sound)
			Matrix_getWindowExtrema (manipulation -> sound.get(), 0, 0, 0, 0, & my soundmin, & my soundmax);
		if (my soundmin == my soundmax) {
			my soundmin = -1.0;
			my soundmax = +1.0;
		}
		updateMenus (me.get());
		return me;
	} catch (MelderError) {
		Melder_throw (U"Manipulation window not created.");
	}
}

/* End of file ManipulationEditor.cpp */
