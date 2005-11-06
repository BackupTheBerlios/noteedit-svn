/****************************************************************************************/
/*											*/
/* This program is free software; you can redistribute it and/or modify it under the	*/
/* terms of the GNU General Public License as published by the Free Software		*/
/* Foundation; either version 2 of the License, or (at your option) any later version.	*/
/*											*/
/* This program is distributed in the hope that it will be useful, but WITHOUT ANY	*/
/* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A	*/
/* PARTICULAR PURPOSE. See the GNU General Public License for more details.		*/
/*											*/
/* You should have received a copy of the GNU General Public License along with this	*/
/* program; (See "LICENSE.GPL"). If not, write to the Free Software Foundation, Inc.,	*/
/* 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.				*/
/*											*/
/*--------------------------------------------------------------------------------------*/
/*											*/
/*		Joerg Anders, TU Chemnitz, Fakultaet fuer Informatik, GERMANY		*/
/*		ja@informatik.tu-chemnitz.de						*/
/*											*/
/*											*/
/****************************************************************************************/

#include "resource.h"
#include "constants.h"
#include "internals.h"
#include "transpainter.h"

main_props_str::main_props_str() : scaledBoldItalicMetrics_(QFont()) /* dummy */ {
			dotcount = 0;
			pedal_on = pedal_off = triplet =
			tied = staccato = sforzato = portato = strong_pizzicato = 
			arpeggio = grace = sforzando = fermate = hidden = false;
			noteBody = 0;
			actualLength = -1;
			actualStemDir = STEM_DIR_AUTO;
			tp              = new NTransPainter();
			tp->setXPosition(-DEFAULT_LEFT_PAGE_BORDER);
			tp->setYPosition(-TOP_BOTTOM_BORDER);
			directPainter              = new NTransPainter();
			directPainter->setXPosition(-(int) ((float) DEFAULT_LEFT_PAGE_BORDER/zoom));
			directPainter->setYPosition(-TOP_BOTTOM_BORDER);
			p                = new NTransPainter();
			p->setYPosition(-TOP_BOTTOM_BORDER);
			left_page_border = DEFAULT_LEFT_PAGE_BORDER;
			context_clef_xpos = DEFAULT_CONTEXT_CLEF_X_POS;
			context_keysig_xpos = DEFAULT_CONTEXT_KEYSIG_X_POS;
		}

main_props_str::~main_props_str() {
			delete tp;
			delete p;
			delete directPainter;
		}

