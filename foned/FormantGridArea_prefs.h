/* FormantGridArea_prefs.h
 *
 * Copyright (C) 2022 Paul Boersma
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

Prefs_begin (FormantGridArea)

	InstancePrefs_addDouble  (FormantGridArea, formantFloor,                    1, U"0.0")   // Hz
	InstancePrefs_addDouble  (FormantGridArea, formantCeiling,                  1, U"11000.0")   // Hz
	InstancePrefs_addDouble  (FormantGridArea, bandwidthFloor,                  1, U"0.0")   // Hz
	InstancePrefs_addDouble  (FormantGridArea, bandwidthCeiling,                1, U"1000.0")   // Hz

Prefs_end (FormantGridArea)

/* End of file FormantGridArea_prefs.h */
