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

#ifndef MULTISTAFFINFO_H

#define MULTISTAFFINFO_H

#include <qlist.h>
#include <qstring.h>
#include "config.h"
#if GCC_MAJ_VERS > 2
#include <fstream>
#else
#include <fstream.h>
#endif

using namespace std;

class NMainFrameWidget;
class NStaff;
class NMultistaffInfo {
	public:
		NMultistaffInfo(NMainFrameWidget *mainWidget, QList<NStaff> *stafflist, int staff_count);
		~NMultistaffInfo();
		int multistaffIdxOfStaff(int staff_nr, int *multistaffnr, int *numStaffsInMultistaff);
		void writeAkkoladen(ofstream *out, bool pmxstyle);
		QString computeTexClef(int multistaffnr);
		bool DiscontOutsidePiano() {return discontBarsOutsidePiano_;}
		bool ContinuedBarLines() {return continuedBarLines_;}
		void noticeClefChange(int staff_nr, int newTeXClef);
		int getMultiStaffCount() {return multiStaffCount_;}
		int getStaffCount(int multistaffnr);
		bool clefChanged(int multistaffnr, bool reset);
		bool hasDisconnectedPianoBars() {return disconnectedPianoBars_;}
		int getfirstStaffInMultistaff(int multistaffnr);
	private:
		struct multistaffinfo {
			int staffcount, fiststaffnr;
			int *clefinfo;
			bool clefchange;
		};
		struct multistaffinfo *multiStaffInfo_;
		int multiStaffCount_;
		int staffCount_;
		NMainFrameWidget *mainWidget_;
		bool discontBarsOutsidePiano_;
		bool continuedBarLines_;
		bool disconnectedPianoBars_;
};

#endif /* MULTISTAFFINFO_H */
