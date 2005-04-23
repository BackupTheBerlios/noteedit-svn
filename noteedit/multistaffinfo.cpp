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

#include <istream.h>
#include <cstdlib>
#include "multistaffinfo.h"
#include "mainframewidget.h"
#include "layout.h"
#include "mainframewidget.h"
#include "clef.h"
#include "staff.h"
#include "voice.h"
#include "resource.h"

#define INTERNAL_DRUM_CLEF_MARK 111

NMultistaffInfo::NMultistaffInfo(NMainFrameWidget *mainWidget, QList<NStaff> *stafflist, int staff_count) {
	NClef *clef;
	int i, j, k, l;
	int *clinfo;
	bool isInBraceList;
	bool *contList;
	bool insidePiano;
	int valid_rules;

	mainWidget_ = mainWidget;
	staffCount_ = staff_count;
	if ((multiStaffInfo_ = (struct multistaffinfo *) malloc(staffCount_ * sizeof(struct multistaffinfo))) == NULL) {
		NResource::abort("NMultistaffInfo: interal error");
	}
	clinfo = (int *) alloca(staffCount_ * sizeof(int));
	contList = (bool *) alloca(staffCount_ * sizeof(bool));
	multiStaffCount_ = 0;
	disconnectedPianoBars_ = false;
	for (i = 0; i < staffCount_; i++) {
		if (multiStaffCount_ >= staffCount_) {
			NResource::abort("createMultiInstrumentInfo: internal error", 1);
		}
		if (!NResource::staffSelExport_[i]) continue;
		contList[i] = false;
		for (j = 0; j < staffCount_; j++) {
			if (mainWidget_->barCont_[j].valid && mainWidget_->barCont_[j].beg <= i && mainWidget_->barCont_[j].end > i) {
				contList[i] = true;
			}
		}
		multiStaffInfo_[multiStaffCount_].fiststaffnr = i;
		multiStaffInfo_[multiStaffCount_].staffcount = 1;
		clef = stafflist->at(i)->getVoiceNr(0)->getFirstClef();
		switch (clef->getSubType()) {
			case BASS_CLEF:clinfo[0]=6; break;
			case SOPRANO_CLEF:clinfo[0]=1; break;
			case ALTO_CLEF:clinfo[0]=3; break;
			case TENOR_CLEF:clinfo[0]=4; break;
			case DRUM_BASS_CLEF:
			case DRUM_CLEF:clinfo[0]=INTERNAL_DRUM_CLEF_MARK; break;
			default:clinfo[0]=0; break;
		}
		isInBraceList = false;
		for (j = 0; j < staffCount_; j++) {
			if (mainWidget_->braceMatrix_[j].valid && i >= mainWidget_->braceMatrix_[j].beg && i <= mainWidget_->braceMatrix_[j].end) {
				isInBraceList = true;
				if (!contList[i] && i < mainWidget_->braceMatrix_[j].end) {
					disconnectedPianoBars_ = true;
				}
				break;
			}
		}
		k = 1;
		if (isInBraceList) {
			for (i++; i < staffCount_ && i <= mainWidget_->braceMatrix_[j].end; i++) {
				contList[i] = false;
				for (l = 0; l < staffCount_; l++) {
					if (mainWidget_->barCont_[l].valid && mainWidget_->barCont_[l].beg <= i && mainWidget_->barCont_[l].end > i) {
						contList[i] = true;
					}
				}
				if (!contList[i] && i < mainWidget_->braceMatrix_[j].end) {
					disconnectedPianoBars_ = true;
				}
				if (NResource::staffSelExport_[i]) {
					clef = stafflist->at(i)->getVoiceNr(0)->getFirstClef();
					switch (clef->getSubType()) {
						case BASS_CLEF:clinfo[k]=6; break;
						case SOPRANO_CLEF:clinfo[k]=1; break;
						case ALTO_CLEF:clinfo[k]=3; break;
						case TENOR_CLEF:clinfo[k]=4; break;
						case DRUM_BASS_CLEF:
						case DRUM_CLEF:clinfo[k]=INTERNAL_DRUM_CLEF_MARK; break;
						default:clinfo[k]=0; break;
					}
					k++;
				}
			}
			i--;
		}
		multiStaffInfo_[multiStaffCount_].staffcount = k;
		multiStaffInfo_[multiStaffCount_].clefinfo = (int *) malloc(k * sizeof(int));
		if (multiStaffInfo_[multiStaffCount_].clefinfo == NULL) {
			NResource::abort("createMultiInstrumentInfo: internal error", 2);
		}
		memcpy(multiStaffInfo_[multiStaffCount_].clefinfo, clinfo, k*sizeof(int));
		multiStaffInfo_[multiStaffCount_].clefchange = false;
		multiStaffCount_++;
	}
	if (multiStaffCount_ < 1 || multiStaffCount_ > staffCount_) {
		NResource::abort("createMultiInstrumentInfo: internal error", 4);
	}
	discontBarsOutsidePiano_ = false;
	continuedBarLines_ = false;
	valid_rules = 0;
	for (i = 0; i < staffCount_; i++) {
		if (mainWidget_->barCont_[i].valid) {
			valid_rules++;
			if (mainWidget_->barCont_[i].beg == 0 && mainWidget_->barCont_[i].end == staffCount_ - 1) {
				continuedBarLines_ = true;
			}
		}
	}
	if (continuedBarLines_) return;
	if (!valid_rules) return;
	for (i = 0; i < staffCount_; i++) {
		if (contList[i] && NResource::staffSelExport_[i]) {
			insidePiano = false;
			for (j = 0; !insidePiano && j < staffCount_; j++) {
				if (mainWidget_->braceMatrix_[j].valid && mainWidget_->braceMatrix_[j].beg <= i && mainWidget_->braceMatrix_[j].end > i) {
					insidePiano = true;
				}
			}
			if (!insidePiano) {
				discontBarsOutsidePiano_ = true;
			}
		}
	}
}

NMultistaffInfo::~NMultistaffInfo() {
	int i;
	for (i = 0; i < multiStaffCount_; i++) {
		free(multiStaffInfo_[i].clefinfo);
	}
	delete multiStaffInfo_;
}

int NMultistaffInfo::getStaffCount(int multistaffnr) {
	if (multistaffnr < 0 || multistaffnr >= multiStaffCount_) {
		NResource::abort("computeTexClef: internal error");
	}
	return multiStaffInfo_[multistaffnr].staffcount;
}

bool NMultistaffInfo::clefChanged(int multistaffnr, bool reset) {
	bool ret;
	if (multistaffnr < 0 || multistaffnr >= multiStaffCount_) {
		NResource::abort("clefChanged: internal error");
	}
	ret = multiStaffInfo_[multistaffnr].clefchange;
	if (reset) ret = multiStaffInfo_[multistaffnr].clefchange = false;
	return ret;
}

int NMultistaffInfo::getfirstStaffInMultistaff(int multistaffnr) {
	if (multistaffnr < 0 || multistaffnr >= multiStaffCount_) {
		NResource::abort("getfirstStaffInMultistaff: internal error");
	}
	return multiStaffInfo_[multistaffnr].fiststaffnr;
}

void NMultistaffInfo::writeAkkoladen(ofstream *out, bool pmxstyle) {
	int i, j;
	bool isInBracketList;
	struct multistaffinfo *mstaffinfo;
	int bracketCount = 0;
	int multistaffnr;
	int dummy;

	mstaffinfo = (struct multistaffinfo *) alloca(staffCount_ * sizeof(struct multistaffinfo));
	for (i = 0; i < staffCount_; i++) {
		if (bracketCount >= staffCount_) {
			NResource::abort("writeAkkoladen: internal error", 2);
		}
		if (!NResource::staffSelExport_[i]) continue;
		isInBracketList = false;
		for (j = 0; j < staffCount_; j++) {
			if (mainWidget_->bracketMatrix_[j].valid && i >= mainWidget_->bracketMatrix_[j].beg && i <= mainWidget_->bracketMatrix_[j].end) {
				isInBracketList = true; break;
			}
		}
		if (isInBracketList) {
			multistaffIdxOfStaff(i, &multistaffnr, &dummy);
			mstaffinfo[bracketCount].fiststaffnr = multistaffnr;
			mstaffinfo[bracketCount].staffcount = 1;
			for (i++; i < staffCount_ && i <= mainWidget_->bracketMatrix_[j].end; i++) {
				if (NResource::staffSelExport_[i] && multistaffIdxOfStaff(i, &multistaffnr, &dummy) == 0) {
					mstaffinfo[bracketCount].staffcount++;
				}
			}
			i--;
			bracketCount++;
		}
	}
	if (bracketCount > staffCount_) {
		NResource::abort("writeAkkoladen: internal error", 3);
	}
	if (bracketCount) {
		if (pmxstyle) *out << "\\";
		*out << "\\akkoladen{";
		for (i = 0; i < bracketCount; i++) {
			*out << '{' << (multiStaffCount_ - (mstaffinfo[i].fiststaffnr + mstaffinfo[i].staffcount)+1) << '}'
			     << '{' << (multiStaffCount_ - mstaffinfo[i].fiststaffnr) << '}';
		}
		*out << '}';
		if (pmxstyle) *out << "\\";
		*out << endl;
	}
}


void NMultistaffInfo::noticeClefChange(int staff_nr, int newTeXClef) {
	int multistaffnr, idx, dummy;

	idx = multistaffIdxOfStaff(staff_nr, &multistaffnr, &dummy);

	multiStaffInfo_[multistaffnr].clefinfo[idx] = newTeXClef;
	multiStaffInfo_[multistaffnr].clefchange = true;
}

int NMultistaffInfo::multistaffIdxOfStaff(int staff_nr, int *multistaffnr, int *numStaffsInMultistaff) {
	int i;

	for (i = 0; i < multiStaffCount_; i++) {
		if (multiStaffInfo_[i].fiststaffnr <= staff_nr && multiStaffInfo_[i].fiststaffnr + multiStaffInfo_[i].staffcount > staff_nr) {
			*multistaffnr = i;
			*numStaffsInMultistaff = multiStaffInfo_[i].staffcount;
			return (staff_nr - multiStaffInfo_[i].fiststaffnr);
		}
	}
	NResource::abort("multistaffIdxOfStaff: internal error");
	return -1;
}

QString NMultistaffInfo::computeTexClef(int multistaffnr) {
	int i;
	if (multistaffnr < 0 || multistaffnr >= multiStaffCount_) {
		NResource::abort("computeTexClef: internal error");
	}
	QString s, t;
	s.sprintf("\\setclef{%d}{", multiStaffCount_ - multistaffnr);
	for (i = 0; i < multiStaffInfo_[multistaffnr].staffcount; i++) {
		if (multiStaffInfo_[multistaffnr].clefinfo[i] == INTERNAL_DRUM_CLEF_MARK) {
			s.sprintf("\\setclefsymbol{%d}\\drumclef", multiStaffCount_ - multistaffnr);
			return s;
		}
		else {
			t.sprintf("%d", multiStaffInfo_[multistaffnr].clefinfo[multiStaffInfo_[multistaffnr].staffcount-i-1]);
		}
		s += t;
	}
	s += '}';
	return s;
}
