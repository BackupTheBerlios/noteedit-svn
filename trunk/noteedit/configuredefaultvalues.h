/****************************************************************************/
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the GNU General Public License as published by the    */
/* Free Software Foundation; either version 2 of the License, or (at your   */
/* option) any later version.                                               */
/*                                                                          */
/* This program is distributed in the hope that it will be useful, but      */
/* WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General */
/* Public License for more details.					    */
/*                                                                          */
/* You should have received a copy of the GNU General Public License along  */
/* with this program; (See "LICENSE.GPL"). If not, write to the Free        */
/* Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA       */
/* 02111-1307, USA. 							    */
/*			                                                    */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*		Erik Sigra, SWEDEN                                          */
/*    sigra@home.se                                                         */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#ifndef CONFIGUREDEFAULTVALUES_H
#define CONFIGUREDEFAULTVALUES_H

//  GENERAL

//  Autosave
#define AUTOSAVE_ENABLE   true
#define AUTOSAVE_INTERVAL 4

//  Startup
#define MUSIX_WARN              true
#define MIDI_PEDAL              true
#define STARTUP_TIP             true
#define STARTUP_LOAD_LAST_SCORE true


//  EDITING

//  [unnamed a]
#define EDITING_ALLOW_AUTO_BEAMING    true
#define EDITING_ALLOW_KEYBOARD_INSERT false
#define EDITING_INSERT_ECHO true
#define EDITING_MOVE_ACCORDING_KEYSIG true
#define EDITING_AUTOMATIC_BAR_INSERTION true

#ifdef WITH_DIRECT_PRINTING

//  PRINTING
#define PRINTING_DEFAULT_OPTIONS "%s"

//  Typesetting program
#define PRINTING_TYPESETTING_PROGRAM 2
#define PRINTING_TYPESETTING_PROGRAM_FORMAT 0
#define PRINTING_TYPESETTING_PROGRAM_INVOKATION "lilypond"

//  Preview program
#define PRINTING_PREVIEW_PROGRAM 0
#define PRINTING_PREVIEW_PROGRAM_INVOKATION "gv"

#endif

//  COLORS

#define COLORS_BACKGROUND                Qt::white
#define COLORS_SELECTION_BACKGROUND      QColor(255, 80, 255)
#define COLORS_STAFF                     Qt::black
#define COLORS_SELECTED_STAFF            Qt::blue
#define COLORS_BAR                       Qt::black
#define COLORS_SELECTED_BAR              Qt::red
#define COLORS_BAR_NUMBER                Qt::black
#define COLORS_SELECTED_BAR_NUMBER       Qt::red
#define COLORS_TEMPO_SIGNATURE           Qt::black
#define COLORS_SELECTED_TEMPO_SIGNATURE  Qt::red
#define COLORS_VOLUME_SIGNATURE          Qt::black
#define COLORS_SELECTED_VOLUME_SIGNATURE Qt::red
#define COLORS_PROGRAM_CHANGE            Qt::black
#define COLORS_SELECTED_PROGRAM_CHANGE   Qt::red
#define COLORS_SPECIAL_ENDING            Qt::black
#define COLORS_SELECTED_SPECIAL_ENDING   Qt::red
#define COLORS_STAFF_NAME                Qt::black
#define COLORS_SELECTED_STAFF_NAME       Qt::blue
#define COLORS_LYRIC                     Qt::black
#define COLORS_CONTEXT_BRUSH		 QColor(219, 243, 255)


//  SOUND

//  Sequencers
#define SEQUENCERS_ALSA true
#define SEQUENCERS_OSS  true

//  MIDI devices
#define DEFAULT_MIDI_PORT 0


// CHORD NAMES

#define DEFAULT_CHORD_NAME_SET   0
#define DEFAULT_DOM7_ID          0
#define DEFAULT_ALTERATION_SIGN  0

#endif //  CONFIGUREDEFAULTVALUES_H
