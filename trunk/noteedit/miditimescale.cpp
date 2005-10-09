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

#include "config.h"
#ifdef WITH_TSE3
#include "miditimescale.h"
#include "resource.h"
#include "staff.h"
#include "clef.h"
#include "chord.h"
#include "rest.h"
#include "sign.h"
#include <stdlib.h>
#include <stdio.h>
//#include <alloca.h>
#include <math.h>

#define SEGMENT_LEN 1024
#define TSE3TIME2MYMIDITIME(t) ((int) ((double) (t) * (double) QUARTER_LENGTH / (double) (TSE3::Clock::PPQN)))
#define TRI16 (NOTE8_LENGTH / 3)
#define M3 (MULTIPLICATOR / 3)

#define TIM(x) x / M3, x / TRI16, x

NMidiTimeScale::NMidiTimeScale(int noteSnap, int smallestTupletNote) : noteSnap_(noteSnap), smallestTupletNote_(smallestTupletNote) {
	array_len_ = 0;
	alloc_len_ = 0;
	unrolled_midi_events_ = 0;
	midi_program_ = -1;
	time_array_ = 0;
	time_array_len_ = 0;
	time_array_alloc_len_ = 0;
	switch (smallestTupletNote) {
		case HALF_LENGTH: mink_ = 8; break;
		case QUARTER_LENGTH: mink_ = 4; break;
		case NOTE8_LENGTH: mink_ = 2; break;
		case -2:
		case NOTE16_LENGTH: mink_ = 1; break;
		default: mink_ = 1; break;
	}
}

NMidiTimeScale::~NMidiTimeScale() {
	if (unrolled_midi_events_) {
		free(unrolled_midi_events_);
	}
	if (time_array_) {
		free(time_array_);
	}
	unrolled_midi_events_ = 0;
	array_len_ = 0;
	alloc_len_ = 0;
	time_array_ = 0;
	time_array_len_ = 0;
	time_array_alloc_len_ = 0;
}

int NMidiTimeScale::getMidiProgram() {
	if (midi_program_ < 0) return 0;
	return midi_program_;
}

void NMidiTimeScale::removeEvent(unsigned int idx) {
	if (idx >= array_len_) {
		NResource::abort("NMidiTimeScale::remove");
	}
	for (;idx < array_len_ - 1; idx++) {
		unrolled_midi_events_[idx] = unrolled_midi_events_[idx + 1];
	}
	array_len_--;
}

void NMidiTimeScale::insertEvent(struct unrolled_midi_events_str *ptr) {
	unsigned int i, j, k, l, len, len1;
	unrolled_midi_events_str *ptr1;
	bool inserted, equal;

	l = array_len_;
	len = ptr->stop_time - ptr->start_time;
	for (i = 0, ptr1 = unrolled_midi_events_; i < l && ptr1->start_time < ptr->start_time; i++, ptr1++);
	inserted = false;
	if (i < l && (ptr1->eventType & EVT_CLASS_REST) == 0 && (ptr1->eventType & EVT_NORMAL_EVENT) && (ptr->eventType & EVT_NORMAL_EVENT)) {
		len1 = ptr->stop_time - ptr->start_time;
		for (;!inserted && i < l && ptr1->start_time == ptr->start_time; i++, ptr1++) {
			if (ptr1->stop_time >= ptr->stop_time - len / 2 && ptr1->stop_time <= ptr->stop_time + len / 2 ||
				ptr->stop_time >= ptr1->stop_time - len1 / 2 && ptr->stop_time <= ptr1->stop_time + len1 / 2
) {
				inserted = true;
				equal = false; // Rosegarden produces some MIDI notes n times (?)
				for (k = 0; !equal && k < ptr1->U.norm_evt.num_pitches; k++) {
					if (ptr1->U.norm_evt.pitches[k] == ptr->U.norm_evt.pitches[0]) equal = true;
				}
				if (!equal) {
					if (ptr1->U.norm_evt.num_pitches < MAX_PITCHES) {
						ptr1->U.norm_evt.pitches[ptr1->U.norm_evt.num_pitches] = ptr->U.norm_evt.pitches[0];
						ptr1->U.norm_evt.num_pitches++;
					}
					else {
						fprintf(stderr, "too many pitchs\n");
					}
				}
			}
		}
	}
	if (inserted) return;
	if (array_len_ >= alloc_len_) {
		if (unrolled_midi_events_) {
			alloc_len_ += SEGMENT_LEN;
			if ((unrolled_midi_events_ = (struct unrolled_midi_events_str *)
				realloc(unrolled_midi_events_, alloc_len_ * sizeof(struct unrolled_midi_events_str))) == NULL) {
					NResource::abort("NMidiTimeScale::insertMidiEvent", 1);
			}
		}
		else {
			alloc_len_ = SEGMENT_LEN;
			if ((unrolled_midi_events_ = (struct unrolled_midi_events_str *)
				malloc(alloc_len_ * sizeof(struct unrolled_midi_events_str))) == NULL) {
					NResource::abort("NMidiTimeScale::insertMidiEvent", 2);
			}
		}
	}
	for (j = array_len_; j > i; j--) {
		unrolled_midi_events_[j] = unrolled_midi_events_[j-1];
	}
	unrolled_midi_events_[i] = *ptr;
	array_len_++;
}

int NMidiTimeScale::determine_snap(int length) {
	int compare;

	compare = NOTE8_LENGTH;

	do {
		if (compare < length) {
			return (compare >> 1);

		}
		compare >>= 1;
	}
	while (compare >= NOTE64_LENGTH);
	return (NOTE64_LENGTH >> 1);
}

void NMidiTimeScale::insertTimeOfTimesig(unsigned int timeSigtime) {
	unsigned int i, j;
	unsigned int *iptr;

	for (i = 0, iptr = time_array_; i < time_array_len_ && *iptr < timeSigtime; i++, iptr++);
	if (time_array_len_ >= time_array_alloc_len_) {
		if (time_array_) {
			time_array_alloc_len_ += SEGMENT_LEN;
			if ((time_array_ = (unsigned int *) realloc(time_array_, time_array_alloc_len_ * sizeof(unsigned int))) == NULL) {
					NResource::abort("NMidiTimeScale::insertTimeOfTimesig", 1);
			}
		}
		else {
			time_array_alloc_len_ = SEGMENT_LEN;
			if ((time_array_ = (unsigned int *) malloc(time_array_alloc_len_ * sizeof(unsigned int))) == NULL) {
					NResource::abort("NMidiTimeScale::insertTimeOfTimesig", 2);
			}
		}
	}
	for (j = time_array_len_; j > i; j--) {
		time_array_[j] = time_array_[j-1];
	}
	time_array_[i] = timeSigtime;
	time_array_len_++;
}

unsigned int NMidiTimeScale::lastTimeSigTime(unsigned int currentTime) {
	unsigned int i;
	unsigned int *iptr, *last_iptr = 0;

	for (i = 0, iptr = time_array_; i < time_array_len_ && *iptr < currentTime; last_iptr = iptr, i++, iptr++);
	if (last_iptr) {
		return *last_iptr;
	}
	return 0;
}


int NMidiTimeScale::quantTriplet(int l, struct unrolled_midi_events_str *ptr, bool *addRest, int maxlength) {
	unsigned int testlength;
	unsigned int deltamin1 = (1<<30);
	int i, shifts;
	int ret;

	if (l > maxlength) {
		NResource::abort("NMidiTimeScale::quantTriplet");
	}
	*addRest = (ptr->eventType & EVT_CLASS_REST);

	maxlength /= MULTIPLICATOR / 3;
	l /= MULTIPLICATOR / 3;
	for (shifts = 0; shifts < 9 && (0x3 << shifts) <  maxlength; shifts++);

	testlength = (0x2 << (shifts));
	for (i = shifts; i > 0; i--) {
		if (testlength > (unsigned int) l) {
			testlength >>= 1;
		}
		else {
			deltamin1 = l - testlength;
			break;
		}
	}
	return (MULTIPLICATOR << i);
}


int NMidiTimeScale::quantNote(int l, int *dotcount, int maxlength) {
	unsigned int testlength;
	unsigned int deltamin3 = (1<<30), deltamin9 = (1<<30);
	int i, j, shifts;
	int ret;
	*dotcount = 0;

	if (l > maxlength) return maxlength;

	maxlength /= MULTIPLICATOR / 3;
	l /= MULTIPLICATOR / 3;
	for (shifts = 0; shifts < 9 && (0x3 << shifts) <  maxlength; shifts++);

	testlength = (0x3 << shifts);
	for (i = shifts; i > 0; i--) {
		if (testlength > (unsigned int) l) {
			testlength >>= 1;
		}
		else {
			deltamin3 = l - testlength;
			break;
		}
	}

	testlength = (0x9 << (shifts - 1));
	for (j = shifts; j > 0; j--) {
		if (testlength > (unsigned int) l) {
			testlength >>= 1;
		}
		else {
			deltamin9 = l - testlength;
			break;
		}
	}

	if (deltamin9 < deltamin3) {
		*dotcount = 1;
		ret = (MULTIPLICATOR << j);
		return ret;
	}
	ret = (MULTIPLICATOR << i);
	return ret;
}



bool NMidiTimeScale::overlapping(unsigned int idx, unrolled_midi_events_str *ptr) {
	unsigned int i, l;
	unsigned int start, stop;
	unrolled_midi_events_str *ptr1;

	if ((ptr->eventType & (EVT_NORMAL_EVENT | EVT_PSEUDO_TRIPLET_NOTE)) == 0) return true;
	l = array_len_;
	for (ptr1 = &(unrolled_midi_events_[0]), i = 0; i < l; i++, ptr1++) {
		if (ptr1->eventType & EVT_PROGRAM_CHANGE) continue;
		if (idx == i) continue;
		if (ptr1->start_time > ptr->stop_time && ptr1->U.norm_evt.triplet_start_time > ptr->stop_time) {
		    	return false;
		}
		start = ((ptr1->eventType & EVT_PSEUDO_TRIPLET_NOTE) == 0 && ptr1->U.norm_evt.triplet_start_time < ptr1->start_time) ? ptr1->U.norm_evt.triplet_start_time : ptr1->start_time;
		stop = ((ptr1->eventType & EVT_PSEUDO_TRIPLET_NOTE) == 0 && ptr1->U.norm_evt.triplet_stop_time > ptr1->stop_time) ? ptr1->U.norm_evt.triplet_stop_time : ptr1->stop_time;
		if (start < ptr->stop_time && stop >= ptr->stop_time) return true;
		if (start < ptr->U.norm_evt.triplet_stop_time && stop >= ptr->U.norm_evt.triplet_stop_time) return true;
	}
	return false;
}


unsigned int NMidiTimeScale::findNextChunkEnd(bool *chukOk, unsigned int *chunkStartIdx) {
	unsigned int chunkEndIdx;
	unsigned int chunkStartTime, chunkEndTime;
	unsigned int i, l;
	unsigned int stop;
	unrolled_midi_events_str *ptr;


	l = array_len_;
	for (i = *chunkStartIdx; i < l && ((unrolled_midi_events_[i].eventType & (EVT_NORMAL_EVENT | EVT_PSEUDO_TRIPLET_NOTE)) == 0); i++);
	if (i >= l) {
		*chukOk = false;
		return 0;
	}
	*chunkStartIdx = i;
	*chukOk = true;
	ptr = &(unrolled_midi_events_[i]);
	chunkEndTime = ptr->stop_time;
	chunkStartTime = ptr->start_time;
	while (chunkEndTime < chunkStartTime + WHOLE_LENGTH) {
		i++; ptr++;
		if (i < l) {
			if ((ptr->eventType & (EVT_NORMAL_EVENT | EVT_PSEUDO_TRIPLET_NOTE)) == 0) continue;
			chunkEndTime = ptr->stop_time;
		}
		else {
			chunkEndIdx = i = *chunkStartIdx;
			ptr = &(unrolled_midi_events_[i]);
			chunkEndTime = ptr->stop_time;

			for (; i < l; i++, ptr++) {
				if (ptr->stop_time > chunkEndTime && (ptr->eventType & (EVT_NORMAL_EVENT | EVT_PSEUDO_TRIPLET_NOTE)) ) {
					chunkEndIdx = i;
					chunkEndTime = ptr->stop_time;
				}
			}
			return chunkEndIdx;
		}
	}
	while(overlapping(i, ptr)) {
		i++; ptr++;
		if (i >= l) {
			chunkEndIdx = i = *chunkStartIdx;
			ptr = &(unrolled_midi_events_[i]);
			chunkEndTime = ptr->stop_time;
			for (; i < l; i++, ptr++) {
				if (ptr->stop_time >= chunkEndTime && (ptr->eventType & (EVT_NORMAL_EVENT | EVT_PSEUDO_TRIPLET_NOTE)) ) {
					chunkEndIdx = i;
					chunkEndTime = ptr->stop_time;
				}
			}
			return chunkEndIdx;
		}
	}
	stop = ptr->stop_time;
	while (ptr->start_time < stop || ((ptr->eventType & (EVT_NORMAL_EVENT | EVT_PSEUDO_TRIPLET_NOTE)) == 0)) { // the next event(s) can have a later start time but a earlier stop time
		i++; ptr++;
		if (i >= l) {
			chunkEndIdx = i = *chunkStartIdx;
			ptr = &(unrolled_midi_events_[i]);
			chunkEndTime = ptr->stop_time;
			for (; i < l; i++, ptr++) {
				if (ptr->stop_time > chunkEndTime && (ptr->eventType & (EVT_NORMAL_EVENT | EVT_PSEUDO_TRIPLET_NOTE)) ) {
					chunkEndIdx = i;
					chunkEndTime = ptr->stop_time;
				}
			}
			return chunkEndIdx;
		}
	}
	return i-1;
}

int NMidiTimeScale::findFirstUclassified(unsigned int chunkStartIdx, unsigned int len) {
	unsigned int i;
	unrolled_midi_events_str *ptr;

	for (i = 0, ptr = &(unrolled_midi_events_[chunkStartIdx]); i < len; i++, ptr++) {
		if ((ptr->eventType & (EVT_NORMAL_EVENT | EVT_PSEUDO_TRIPLET_NOTE)) == 0) continue;
		if (ptr->voice_nr == -1) return (chunkStartIdx + i);
	}
	return -1;
}
	

int NMidiTimeScale::findLastUclassified(unsigned int chunkStartIdx, unsigned int len) {
	unrolled_midi_events_str *ptr;
	unsigned int i, max_stop_time = 0;
	int maxidx = -1;

	for (i = 0, ptr = &(unrolled_midi_events_[chunkStartIdx]); i < len; i++, ptr++) {
		if ((ptr->eventType & (EVT_NORMAL_EVENT | EVT_PSEUDO_TRIPLET_NOTE)) == 0) continue;
		if (ptr->voice_nr >= 0) continue;
		if (ptr->stop_time > max_stop_time) {
			max_stop_time = ptr->stop_time;
			maxidx = chunkStartIdx + i;
		}
		if (ptr->eventType & EVT_NORMAL_EVENT) {
			if (ptr->U.norm_evt.triplet_stop_time > max_stop_time) {
				max_stop_time = ptr->U.norm_evt.triplet_stop_time;
				maxidx = chunkStartIdx + i;
			}
		}
	}
	return maxidx;
}

void NMidiTimeScale::findShortestPath(struct path_elem_str* shortest_path, int fidx, unsigned int chunkStartIdx, unsigned int chunkEndIdx, unsigned int len) {
	unrolled_midi_events_str *ptr, *ptr2;
	unsigned int i, k;
	int minimum_costs, minimum_idx;
	unsigned int idx;
	int idx2;
	int pidx, len2;
	int costs;

	len2 = len;

	if (unrolled_midi_events_[fidx].voice_nr >= 0) {
		NResource::abort("NMidiTimeScale::findShortestPath", 1);
	}
	if (unrolled_midi_events_[fidx].eventType & EVT_PROGRAM_CHANGE) {
		NResource::abort("NMidiTimeScale::findShortestPath", 2);
	}
	pidx = unrolled_midi_events_[fidx].path_idx;
	if (pidx < 0 || pidx >= len2) {
		NResource::abort("NMidiTimeScale::findShortestPath", 3);
	}
	for (i = 0; i <= len; i++) {
		shortest_path[i].costs = -1;
		shortest_path[i].ready = false;
		shortest_path[i].prev_idx = -1;
	}
	shortest_path[pidx].costs = 0;

	while (1) {
		minimum_idx = -1;
		minimum_costs = (1 << 30);
		for (i = 0; i < len; i++) {
			if (!shortest_path[i].ready && shortest_path[i].costs >= 0 && shortest_path[i].costs < minimum_costs) {
				minimum_costs = shortest_path[i].costs;
				minimum_idx = i;
			}
		}
		if (minimum_idx < 0) return;
		if (minimum_idx >= len2) {
			NResource::abort("NMidiTimeScale::findShortestPath", 4);
		}
		idx = shortest_path[minimum_idx].idx;
		if (idx < chunkStartIdx || idx > chunkEndIdx) {
			NResource::abort("NMidiTimeScale::findShortestPath", 5);
		}
		ptr = &(unrolled_midi_events_[idx]);
		if (ptr->voice_nr >= 0) {
			NResource::abort("NMidiTimeScale::findShortestPath", 6);
		}
		if ((ptr->eventType & (EVT_NORMAL_EVENT | EVT_PSEUDO_TRIPLET_NOTE)) == 0) {
			NResource::abort("NMidiTimeScale::findShortestPath", 7);
		}
		for (k = 0; k < len - 1; k++) {
			if (ptr->decision_tree[k].costs >= 0) {
				ptr2 = ptr->decision_tree[k].next_event;
				if (ptr2->voice_nr < 0) {
					costs = minimum_costs + ptr->decision_tree[k].costs;
					idx2 = ptr2->path_idx;
					if (idx2 < 0 || idx2 >= len2) {
						NResource::abort("NMidiTimeScale::findShortestPath", 8);
					}
					if (!shortest_path[idx2].ready && (shortest_path[idx2].costs < 0 || shortest_path[idx2].costs > costs)) {
						shortest_path[idx2].costs = costs;
						shortest_path[idx2].prev_idx = minimum_idx;
					}
				}
			}
		}
		shortest_path[minimum_idx].ready = true;
	}
}

	
void NMidiTimeScale::findPathsInChunk(unsigned int chunkStartIdx, unsigned int chunkEndIdx) {
	unrolled_midi_events_str *ptr, *ptr2;
	unsigned int len = chunkEndIdx - chunkStartIdx;
	unsigned int i, k;
	int j, max_ave_idx, tmp;
	int pitch_sum;
	bool endof_path;
	int num_pitches;
	int voice_map[MAX_VOICES];
	double average_pitches[MAX_VOICES], max_pitch;
	int fidx, lidx, pidx, len2;
	struct path_elem_str *shortest_path;
	int voice_nr = 0;


	len2 = len;

	for (i = chunkStartIdx, ptr = &(unrolled_midi_events_[chunkStartIdx]); i <= chunkEndIdx; i++, ptr++) {
		if (ptr->eventType & (EVT_PROGRAM_CHANGE | EVT_PSEUDO_TRIPLET_NOTE)) continue;
		pitch_sum = 0;
		for (k = 0; k < ptr->U.norm_evt.num_pitches; k++) {
			pitch_sum += ptr->U.norm_evt.pitches[k];
		}
		ptr->ave_pitch = (double)  pitch_sum / (double)  ptr->U.norm_evt.num_pitches;
	}

	for (i = chunkStartIdx, ptr = &(unrolled_midi_events_[chunkStartIdx]); i <= chunkEndIdx; i++, ptr++) {
		if ((ptr->eventType & (EVT_NORMAL_EVENT | EVT_PSEUDO_TRIPLET_NOTE)) == 0) continue;
		ptr->decision_tree = (struct decision_tree_str *) alloca(len * sizeof(struct decision_tree_str));
		initialize_desicion_tree(ptr, i, chunkStartIdx, chunkEndIdx, len);
	}

	shortest_path = (struct path_elem_str *) alloca((len + 1) * sizeof(struct path_elem_str));
		
	voice_nr = 0;
	for (i = 0; i <= len; i++) {
		shortest_path[i].idx = chunkStartIdx + i;
		unrolled_midi_events_[chunkStartIdx + i].path_idx = i;
	}
	while ((fidx = findFirstUclassified(chunkStartIdx, len + 1)) >= 0) {
		if (voice_nr >= MAX_VOICES) {
			fprintf(stderr, "too many voices, notes omitted\n"); fflush(stderr);
			//NResource::abort("NMidiTimeScale::findPathsInChunk", 1);
		}
		num_pitches = 0;
		if (voice_nr < MAX_VOICES) average_pitches[voice_nr] = 0.0;
		if (len == 0) {
			unrolled_midi_events_[fidx].voice_nr = voice_nr;
			if (voice_nr < MAX_VOICES) average_pitches[voice_nr] = unrolled_midi_events_[fidx].ave_pitch;
			voice_nr++;
			continue;
		}
		findShortestPath(shortest_path, fidx, chunkStartIdx, chunkEndIdx, len + 1);
		if ((lidx = findLastUclassified(chunkStartIdx, len+1)) < 0) {
			NResource::abort("NMidiTimeScale::findPathsInChunk", 2);
		}
		endof_path = false;
		pidx = unrolled_midi_events_[lidx].path_idx;
		if (pidx < 0 || pidx > len2) {
			NResource::abort("NMidiTimeScale::findPathsInChunk", 3);
		}
		while (!endof_path) {
			if (unrolled_midi_events_[lidx].voice_nr >= 0) {
				 NResource::abort("NMidiTimeScale::findPathsInChunk", 4);
			}
			unrolled_midi_events_[lidx].voice_nr = voice_nr;
			if (voice_nr < MAX_VOICES) average_pitches[voice_nr] += unrolled_midi_events_[fidx].ave_pitch;
			num_pitches++;
			if (lidx == fidx) {
				endof_path = true;
			}
			else {
				pidx = shortest_path[pidx].prev_idx;
				
				if (pidx == -1) {
					endof_path = true;
				}
				else {
					if (pidx < -1 || pidx > len2) {
						NResource::abort("NMidiTimeScale::findPathsInChunk", 5);
					}
					lidx = shortest_path[pidx].idx;
					if (lidx < chunkStartIdx || lidx > chunkEndIdx) {
						NResource::abort("NMidiTimeScale::findPathsInChunk", 6);
					}
				}
			}
		}
#define NUMBER_OF_NOTES_FAC 0.5
		if (voice_nr < MAX_VOICES) {
			average_pitches[voice_nr] /= num_pitches;
			average_pitches[voice_nr] += NUMBER_OF_NOTES_FAC * num_pitches;
		}
		voice_nr++;
	}
	if (voice_nr > max_voices_ && voice_nr < MAX_VOICES) max_voices_ = voice_nr;

	if (voice_nr < MAX_VOICES) {
		for (i = 0; i < voice_nr; i++) {
			voice_map[i] = i;
	
		}
	
		for (i = 0; i < voice_nr - 1; i++) {
			max_pitch = average_pitches[i];
			max_ave_idx = i;
			for (j = i + 1; j < voice_nr; j++) {
				if (average_pitches[j] > max_pitch) {
					max_pitch = average_pitches[j];
					max_ave_idx = j;
				}
			}
			if (max_ave_idx != i) {
				tmp = voice_map[i];
				voice_map[i] = voice_map[max_ave_idx];
				voice_map[max_ave_idx] = tmp;
			}
		}
	}

	for (i = chunkStartIdx, ptr = &(unrolled_midi_events_[chunkStartIdx]); i <= chunkEndIdx; i++, ptr++) {
		if ((ptr->eventType & (EVT_NORMAL_EVENT | EVT_PSEUDO_TRIPLET_NOTE)) == 0) continue;
		ptr->voice_nr = voice_map[ptr->voice_nr];
		ptr->stem_policy = (voice_nr == 1) ? STEM_POL_INDIVIDUAL : ((ptr->voice_nr < voice_nr / 2) ? STEM_POL_UP : STEM_POL_DOWN);
	}
}

void NMidiTimeScale::initialize_desicion_tree(unrolled_midi_events_str *ptr, unsigned int idx, unsigned int chunkStartIdx, unsigned int chunkEndIdx, int len) {
#define PITCH_DIST_COST_FAC 2.0
#define START_DIST_FAC 20.0
	unrolled_midi_events_str *ptr1;
	unsigned int i;
	int j;

	for (j = 0, i = chunkStartIdx, ptr1 = &(unrolled_midi_events_[chunkStartIdx]); i <= chunkEndIdx; i++, ptr1++) {
		if (i == idx) continue;
		if (j >= len) {
			NResource::abort("NMidiTimeScale::initialize_desicion_tree");
		}
		ptr->decision_tree[j].next_event = ptr1;
		if (ptr1->start_time < ptr->stop_time || ((ptr1->eventType & ( EVT_NORMAL_EVENT | EVT_PSEUDO_TRIPLET_NOTE)) == 0)) {
			ptr->decision_tree[j].costs = -1;
		}
		else if ((ptr1->eventType & EVT_CLASS_NOTE) && (ptr->eventType & EVT_CLASS_NOTE)){
			ptr->decision_tree[j].costs = (int) (PITCH_DIST_COST_FAC * fabs(ptr->ave_pitch - ptr1->ave_pitch) +
								START_DIST_FAC * fabs(ptr1->start_time -  ptr->stop_time));
		}
		j++;
	}
}
			

void NMidiTimeScale::findVoices() {
	unsigned int chunkStartIdx = 0, chunkEndIdx;
	bool chunkOk, removed, inserted;
	unrolled_midi_events_str *ptr, *ptr2, *templist[TLIST_MAX];
	unsigned int i, l, vnr, st_policy;
	int j;
	max_voices_ = 0;


	do { // remove all triplet members there are copies of this events in EVT_PSEUDO_TRIPLET_NOTE
		removed = false;
		for (i = 0; i < array_len_; i++) {
			ptr = &(unrolled_midi_events_[i]);
			if ((ptr->eventType & (EVT_PART_OF_TRIPLET | EVT_LAST_IN_TRIPLET | EVT_FIRST_IN_TRIPLET)) == 0) continue;
			removeEvent(i);
			removed = true;
		}
	}
	while (removed);
	do {
		chunkEndIdx = findNextChunkEnd(&chunkOk, &chunkStartIdx);
		if (chunkOk) {
			l = chunkEndIdx - chunkStartIdx + 1;
#ifdef XXX
			if (l > 1000) {
				printf("s = %d, e = %d l = %d\n", chunkStartIdx, chunkEndIdx, chunkEndIdx - chunkStartIdx + 1); fflush(stdout);
				for (i = chunkStartIdx; i <= chunkEndIdx; i++) {
					printf("%d: t = 0x%x l = %d(%d)\n", i, unrolled_midi_events_[i].eventType,
					(unrolled_midi_events_[i].stop_time - unrolled_midi_events_[i].start_time) / MULTIPLICATOR, 
					unrolled_midi_events_[i].stop_time - unrolled_midi_events_[i].start_time);
				}
				fflush(stdout);
			}
#endif
			findPathsInChunk(chunkStartIdx, chunkEndIdx);
			chunkStartIdx = chunkEndIdx + 1;
		}
	}
	while (chunkOk);


	do { // insert the copies of the triplet members in EVT_PSEUDO_TRIPLET_NOTE again, this is necessary because of
	     // the array shifts, neither the index nor the pointer to an event can act as
	     // identificator because the array insert and remove copies the array members
		inserted = false;
		for (i = 0; i < array_len_; i++) {
			ptr = &(unrolled_midi_events_[i]);
			if ((ptr->eventType & EVT_PSEUDO_TRIPLET_NOTE) == 0) continue;
			if (ptr->U.pseudo_evt.members_inserted_again) continue;
			l = ptr->U.pseudo_evt.num_triplet_members;
			vnr = ptr->voice_nr;
			st_policy = ptr->stem_policy;
			ptr->U.pseudo_evt.members_inserted_again = true;
			memcpy(templist, ptr->U.pseudo_evt.triplet_members, l * sizeof(struct unrolled_midi_events_str *));
			for (j = 0; j < l; j++) { // temp vars because "insertEvent" shifts the array
				ptr2 = templist[j];
				ptr2->voice_nr = vnr;
				ptr2->stem_policy = st_policy;
				insertEvent(ptr2); // makes a copy
				free(ptr2);
			}
			inserted = true;
		}
	}
	while (inserted);
}

#define COST_FOR_REST 100
#define COSTS_FOR_SPLIT 0
#define HIGH_COSTS 10000


int NMidiTimeScale::search_for_event_enlarge(int start_type, unsigned int start_time, int stop_type, unsigned int stop_time) {
	unrolled_midi_events_str *ptr;
	unsigned int i;
	int diff;
	bool ok;

	for (i = 0, ptr = &(unrolled_midi_events_[0]); i < array_len_; i++, ptr++) {
		if ((ptr->eventType & EVT_NORMAL_EVENT) == 0) continue;
		ok = false;
		switch (start_type) {
			case TRIPLET_SNAP: if (ptr->U.norm_evt.triplet_start_time > start_time + WHOLE_LENGTH) return -1;
					   if (ptr->U.norm_evt.sta2diff <= ptr->U.norm_evt.sta3diff) break;
					   if (ptr->U.norm_evt.triplet_start_time == start_time) ok = true;
					   break;
			case SYSTEM2_SNAP: if (ptr->start_time > start_time) return -1;
					   if (ptr->start_time  == start_time) ok = true;
					   break;
			default: NResource::abort("NMidiTimeScale::search_for_event_enlarge", 1);
		}
		if (!ok) continue;
		switch (stop_type) {
			case TRIPLET_SNAP: diff = stop_time - ptr->U.norm_evt.triplet_stop_time;
					if (diff < 0) diff = -diff;
#ifdef XXX
					   if (ok) {
					   	printf("diff = %d(%d/%d), stop_time = %d(%d/%d), ptr->U.norm_evt.triplet_stop_time = %d(%d/%d)\n",
						TIM(diff), TIM(stop_time), TIM(ptr->U.norm_evt.triplet_stop_time)); fflush(stdout);
					   }
#endif
					   if (diff <= (stop_time - start_time) / 4) {
					   	ptr->U.norm_evt.triplet_stop_time = stop_time;
						return i;
					   }
					   break;
			case SYSTEM2_SNAP: 
					   diff = stop_time - ptr->stop_time;
					   if (diff < 0) diff = -diff;
					   if (diff <= (stop_time - start_time) / 4) {
					   	ptr->stop_time = stop_time;
						return i;
					   }
					   break;
			default: NResource::abort("NMidiTimeScale::search_for_event_enlarge", 1);
		}
	}
	return -1;
}

int NMidiTimeScale::search_cuttable_note_left(unsigned int split_time, unsigned int stop_time) {
	unrolled_midi_events_str *ptr;
	unsigned int i;
	int diff;

	for (i = 0, ptr = &(unrolled_midi_events_[0]); i < array_len_; i++, ptr++) {
		if ((ptr->eventType & EVT_NORMAL_EVENT) == 0) continue;
		if (ptr->start_time > split_time) return -1;
		diff = stop_time - ptr->U.norm_evt.triplet_stop_time;
		if (diff < 0) diff = -diff;
		if (diff <= (stop_time - split_time) / 4) {
			ptr->U.norm_evt.triplet_stop_time = stop_time;
			return i;
		}
	}
	return -1;
}

int NMidiTimeScale::search_cuttable_note_right(unsigned int split_time, unsigned int start_time) {
	unrolled_midi_events_str *ptr;
	unsigned int i;
	int diff;

	for (i = 0, ptr = &(unrolled_midi_events_[0]); i < array_len_; i++, ptr++) {
		if ((ptr->eventType & EVT_NORMAL_EVENT) == 0) continue;
		if (ptr->start_time > split_time) return -1;
		if (ptr->stop_time <= split_time) continue;
		if (!is_nearby(ptr->U.norm_evt.triplet_start_time, start_time, ptr->stop_time - ptr->U.norm_evt.triplet_start_time)) continue;
		ptr->U.norm_evt.triplet_start_time = start_time;
		return i;
	}
	return -1;
}


void NMidiTimeScale::appendTRest(unsigned int start_time, unsigned int stop_time) {

	if (TListPtr_ >= TLIST_MAX) {
		NResource::abort("appendTRest"); 
	}
	TList_[TListPtr_  ].evt_class = EVT_CLASS_REST;
	TList_[TListPtr_  ].start_time = start_time;
	TList_[TListPtr_++].stop_time = stop_time;
}

void NMidiTimeScale::prependTRest(unsigned int start_time, unsigned int stop_time) {
	int i;

	if (TListPtr_ >= TLIST_MAX) {
		NResource::abort("prependTRest"); 
	}
	TListPtr_++;
	for (i = TListPtr_ - 1; i > 0; i--) {
		TList_[i] = TList_[i-1];
	}
	TList_[0].evt_class = EVT_CLASS_REST;
	TList_[0].start_time = start_time;
	TList_[0].stop_time = stop_time;
}

void NMidiTimeScale::appendTidx(int idx) {
	if (TListPtr_ >= TLIST_MAX) {
		NResource::abort("appendTidx"); 
	}
	TList_[TListPtr_  ].evt_class = EVT_CLASS_NOTE;
	TList_[TListPtr_++].idx = idx;
}

void NMidiTimeScale::prependTidx(int idx) {
	int i;

	if (TListPtr_ >= TLIST_MAX) {
		NResource::abort("prependTidx"); 
	}
	TListPtr_++;
	for (i = TListPtr_ - 1; i > 0; i--) {
		TList_[i] = TList_[i-1];
	}
	TList_[0].evt_class = EVT_CLASS_NOTE;
	TList_[0].idx = idx;
}

bool NMidiTimeScale::is_nearby(unsigned int test_time, unsigned int target_time, int dist) {
	int diff;

	diff = test_time - target_time;
	if (diff < 0) diff = -diff;
	return diff <= dist / 2;
}

bool NMidiTimeScale::is_a_cuttable_right_note(int idx, unsigned int target_time) {
	if (unrolled_midi_events_[idx].stop_time <= target_time) return false;
	return true;
}

void NMidiTimeScale::append_cuttable_note(int idx, unsigned int split_time) {
	if (TListPtr_ >= TLIST_MAX) {
		NResource::abort("append_cuttable_note"); 
	}
	TList_[TListPtr_  ].evt_class = EVT_NOTE_AFTER_TRIPLET;
	TList_[TListPtr_  ].split_time = split_time;
	TList_[TListPtr_++].idx = idx;
}

void NMidiTimeScale::prepend_cuttable_note(int idx, unsigned int split_time) {
	int i;

	if (TListPtr_ >= TLIST_MAX) {
		NResource::abort("prepend_cuttable_note"); 
	}
	TListPtr_++;
	for (i = TListPtr_ - 1; i > 0; i--) {
		TList_[i] = TList_[i-1];
	}
	TList_[0].evt_class = EVT_NOTE_BEFORE_TRIPLET;
	TList_[0].split_time = split_time;
	TList_[0].idx = idx;
}

int NMidiTimeScale::findBigRightTripletPartSloppy(int idx, unsigned int T1, int divis) {
	int idx1;

	if (is_nearby(unrolled_midi_events_[idx].stop_time, T1, unrolled_midi_events_[idx].stop_time -  unrolled_midi_events_[idx].U.norm_evt.triplet_start_time)) {
		unrolled_midi_events_[idx].stop_time = T1;
		appendTidx(idx);
		return 0;
	}

	if (is_a_cuttable_right_note(idx, T1)) {
		append_cuttable_note(idx, T1);
		return COSTS_FOR_SPLIT;
	}

	if (is_nearby(unrolled_midi_events_[idx].U.norm_evt.triplet_stop_time, T1 - divis, unrolled_midi_events_[idx].U.norm_evt.triplet_stop_time - unrolled_midi_events_[idx].U.norm_evt.triplet_start_time)) {
		unrolled_midi_events_[idx].U.norm_evt.triplet_stop_time = T1 - divis;
		if ((idx1 = search_for_event_enlarge(TRIPLET_SNAP, T1 - divis, SYSTEM2_SNAP, T1)) >= 0) {
			appendTidx(idx);
			appendTidx(idx1);
			return 0;
		}
		if ((idx1 = search_cuttable_note_right(T1, T1 - divis)) >= 0) {
			appendTidx(idx);
			append_cuttable_note(idx1, T1);
			return COSTS_FOR_SPLIT;
		}
		appendTidx(idx);
		appendTRest(T1 - divis, T1);
		return COST_FOR_REST;
	}
	return HIGH_COSTS;
}


int NMidiTimeScale::findSmallLeftTripletPartSloppy(unsigned int T0, int divis) {
	int idx1;

	if ((idx1 = search_for_event_enlarge(SYSTEM2_SNAP, T0, TRIPLET_SNAP, T0 + divis)) >= 0) {
		prependTidx(idx1);
		return 0;
	}
	if ((idx1 = search_cuttable_note_left(T0, T0 + divis)) >= 0) {
		prepend_cuttable_note(idx1, T0);
		return COSTS_FOR_SPLIT;
	}
	prependTRest(T0, T0 + divis);
	return COST_FOR_REST;
}

int NMidiTimeScale::findBigLeftTripletPartSloppy(unsigned int T0, int divis) {
	int idx1, idx2;

	if ((idx1 = search_for_event_enlarge(SYSTEM2_SNAP, T0, TRIPLET_SNAP, T0 + 2 * divis)) >= 0) {
		appendTidx(idx1);
		return 0;
	}

	if ((idx1 = search_for_event_enlarge(SYSTEM2_SNAP, T0, TRIPLET_SNAP, T0 + divis)) >= 0) {
		if ((idx2 = search_for_event_enlarge(TRIPLET_SNAP, T0 + divis, TRIPLET_SNAP, T0 + 2 * divis)) >= 0) {
			appendTidx(idx1);
			appendTidx(idx2);
			return 0;
		}
		appendTidx(idx1);
		appendTRest(T0 + divis, T0 + 2 * divis);
		return COST_FOR_REST;
        }
	if ((idx1 = search_cuttable_note_left(T0, T0 + 2 * divis)) >= 0) {
		prepend_cuttable_note(idx1, T0 + 2 * divis);
		return COSTS_FOR_SPLIT;
	}

	if ((idx1 = search_for_event_enlarge(TRIPLET_SNAP, T0 + divis, TRIPLET_SNAP, T0 + 2 * divis)) >= 0) {
                appendTRest(T0, T0 + divis);
                appendTidx(idx1);
		return COST_FOR_REST;
        }
	appendTRest(T0, T0 + 2 * divis);
	return COST_FOR_REST;
}

int NMidiTimeScale::findSmallRightTripletPartSloppy(int idx, unsigned int T1, int divis) {
	if (is_nearby(unrolled_midi_events_[idx].stop_time, T1, unrolled_midi_events_[idx].stop_time - unrolled_midi_events_[idx].U.norm_evt.triplet_start_time)) {
		unrolled_midi_events_[idx].stop_time = T1;
		appendTidx(idx);
		return 0;
	}
	if (is_a_cuttable_right_note(idx, T1)) {
		append_cuttable_note(idx, T1);
		return COSTS_FOR_SPLIT;
	}
	return HIGH_COSTS;
}

void NMidiTimeScale::searchForTriplet(int idx, int tripletLength, int snapedTripletPosition, int typeFound) {
	int i, j, k, n, divis, costs, minT0, minT1, mincosts;
	struct unrolled_midi_events_str pseudo_note, *ptr2, *ptr3;
	unsigned int T0, T1;
	int num_pitches;
	int Tl, tyF;

#ifdef XXX
	printf("int idx = %d, 1/%d Zerlegung bei (%d/3), n = %d\n", idx, 8 / tripletLength, typeFound + 1, snapedTripletPosition); fflush(stdout);
#endif


	MinTListPtr_ = -1;
	Tl = tripletLength;
	tyF = typeFound;
	mincosts = (1 << 30);
	snapedTripletPosition *= TRI16;
	while (Tl >= 1) {
		TListPtr_ = 0;
		divis = Tl * TRI16;
		if (tyF) {
			T0 = snapedTripletPosition - 2 * divis;
			T1 = snapedTripletPosition + divis;
			if (T0 & 0x80000000) continue; // < 0
			costs  = findBigLeftTripletPartSloppy(T0, divis);
			costs += findSmallRightTripletPartSloppy(idx, T1, divis);
		}
		else {
			T0 = snapedTripletPosition - divis;
			T1 = snapedTripletPosition + 2 * divis;
			if (T0 & 0x80000000) continue; // < 0
			costs  = findBigRightTripletPartSloppy(idx, T1, divis);
			costs += findSmallLeftTripletPartSloppy(T0, divis);
		}
		if (costs < mincosts  && costs < HIGH_COSTS && TListPtr_ > 0) {
			mincosts = costs;
			minT0 = T0;
			minT1 = T1;
			MinTListPtr_ = TListPtr_;
			memcpy (MinTList_, TList_, TListPtr_ * sizeof(struct tripletMemberStr));
		}
		Tl >>= 1;
		tyF = 1 - tyF;
	}

	if (MinTListPtr_ >= 0) {
#ifdef XXX
		printf("Triplet anzahl  = %d minT0 = %d(%d), minT1 = %d(%d)\n", MinTListPtr_, minT0 / MULTIPLICATOR, minT0,
		minT1 / MULTIPLICATOR, minT1);  fflush(stdout);}
#endif
		num_pitches = 0;
		pseudo_note.eventType = EVT_PSEUDO_TRIPLET_NOTE | EVT_CLASS_NOTE;
		pseudo_note.voice_nr = -1;
		pseudo_note.U.pseudo_evt.members_inserted_again = false;
		pseudo_note.ave_pitch = 0.0;
		pseudo_note.start_time = minT0;
		pseudo_note.stop_time = minT1;
		pseudo_note.U.pseudo_evt.num_triplet_members = MinTListPtr_;
		for (i = 0; i < MinTListPtr_; i++) {
			if (MinTList_[i].evt_class & (EVT_CLASS_NOTE | EVT_NOTE_AFTER_TRIPLET | EVT_NOTE_BEFORE_TRIPLET)) {
				ptr2 = &(unrolled_midi_events_[MinTList_[i].idx]);
				if ((ptr2->eventType & EVT_NORMAL_EVENT) == 0) {
					NResource::abort("NMidiTimeScale::checkForTripletMembers", 1);
				}
				if (i == 0) {
					ptr2->eventType = EVT_FIRST_IN_TRIPLET | EVT_CLASS_NOTE;
					if (MinTList_[i].evt_class & EVT_NOTE_BEFORE_TRIPLET) {
						ptr2->eventType |= EVT_NOTE_BEFORE_TRIPLET;
						ptr2->split_time = MinTList_[i].split_time;
					}
					ptr2->U.norm_evt.triplet_border = ptr2->U.norm_evt.used_part_time = minT0;
				}
				else if (i == MinTListPtr_ - 1) {
					ptr2->eventType = EVT_LAST_IN_TRIPLET | EVT_CLASS_NOTE;
					if (MinTList_[i].evt_class & EVT_NOTE_AFTER_TRIPLET) {
						ptr2->eventType |= EVT_NOTE_AFTER_TRIPLET;
						ptr2->split_time = MinTList_[i].split_time;
					}
					ptr2->U.norm_evt.triplet_border = ptr2->U.norm_evt.used_part_time = minT1;
				}
				else {
					ptr2->eventType = EVT_PART_OF_TRIPLET | EVT_CLASS_NOTE;
				}
				if ((ptr3 = (struct unrolled_midi_events_str *) malloc(sizeof(struct unrolled_midi_events_str))) == 0) {
					NResource::abort("NMidiTimeScale::checkForTripletMembers", 2);
				}
				*ptr3 = *ptr2;
				pseudo_note.U.pseudo_evt.triplet_members[i] = ptr3;
				for (j = 0; j < ptr2->U.norm_evt.num_pitches; j++) {
					pseudo_note.ave_pitch += ptr2->U.norm_evt.pitches[j];
					num_pitches++;
				}
			}
			else {
				if ((ptr3 = (struct unrolled_midi_events_str *) malloc(sizeof(struct unrolled_midi_events_str))) == 0) {
					NResource::abort("NMidiTimeScale::checkForTripletMembers", 3);
				}
				if (i == 0) {
					ptr3->eventType = EVT_FIRST_IN_TRIPLET | EVT_CLASS_REST;
					ptr3->U.norm_evt.triplet_border = ptr3->U.norm_evt.used_part_time = minT0;
				}
				else if (i == MinTListPtr_ - 1) {
					ptr3->eventType = EVT_LAST_IN_TRIPLET | EVT_CLASS_REST;
					ptr3->U.norm_evt.triplet_border = ptr3->U.norm_evt.used_part_time = minT1;
				}
				else {
					ptr3->eventType = EVT_PART_OF_TRIPLET | EVT_CLASS_REST;
				}
				ptr3->start_time = MinTList_[i].start_time;
				ptr3->stop_time = MinTList_[i].stop_time;
				pseudo_note.voice_nr = -1;
				pseudo_note.U.pseudo_evt.triplet_members[i] = ptr3;
			}
				
		}
		pseudo_note.ave_pitch /= (double) num_pitches;
		insertEvent(&pseudo_note);
	}
}


void NMidiTimeScale::findTriplets() {
	unrolled_midi_events_str *ptr;
	unsigned int i, snapedTipletPosition;
	int typeFound;
	int k;

#ifdef XXX
	for (i = 0; i < array_len_; i++) {
		ptr = &(unrolled_midi_events_[i]);
		if ((ptr->eventType & EVT_NORMAL_EVENT) == 0) continue;
		if (ptr->U.norm_evt.sta3diff < ptr->U.norm_evt.sta2diff) {
			printf("idx = %d pitch = %d trime = %d(%d/%d)-%d(%d/%d), sta_kind = T\n", i,
			ptr->U.norm_evt.pitches[0],
			ptr->U.norm_evt.triplet_start_time / TRI16, ptr->U.norm_evt.triplet_start_time / MULTIPLICATOR, 
			ptr->U.norm_evt.triplet_start_time , ptr->U.norm_evt.triplet_stop_time  / TRI16,  ptr->U.norm_evt.triplet_stop_time / MULTIPLICATOR,
			ptr->U.norm_evt.triplet_stop_time);
		}
		else {
			printf("idx = %d pitch = %d time = %d(%d/%d) - %d(%d/%d), sta_kind = N\n", i,
			ptr->U.norm_evt.pitches[0],
			ptr->start_time / TRI16, ptr->start_time / MULTIPLICATOR, 
			ptr->start_time , ptr->stop_time  / TRI16,  ptr->stop_time / MULTIPLICATOR,
			ptr->stop_time);
		}
	}
	fflush(stdout);
#endif

	for (k = mink_; k <= 8; k <<= 1) {
		for (i = 0; i < array_len_; i++) {
			ptr = &(unrolled_midi_events_[i]);
			if ((ptr->eventType & EVT_NORMAL_EVENT) == 0) continue;
			if (ptr->U.norm_evt.sta3diff >= ptr->U.norm_evt.sta2diff) continue;
			//snapedTipletPosition = (ptr->U.norm_evt.triplet_start_time - lastTimeSigTime(ptr->U.norm_evt.triplet_start_time)) / TRI16;
			snapedTipletPosition = ptr->U.norm_evt.triplet_start_time / TRI16;
			if ((snapedTipletPosition % 3) && !(snapedTipletPosition % k) && ((snapedTipletPosition % (k << 1)) || (k == 8))) {
				typeFound = ((snapedTipletPosition - k) % 3) ? 1 : 0;
				searchForTriplet(i, k, snapedTipletPosition, typeFound);
			}
		}
	}
}

#define SNAP1(x, D, L) (((x - L + D / 2) / D) * D + L) 

void NMidiTimeScale::insertMidiEvent(TSE3::MidiEvent *midiEvent,  unsigned int min, unsigned int max) {
	struct unrolled_midi_events_str unr_evt;
	unsigned int lastTimesig;
	int len;
	unsigned int val2, val3, snap2, snap3;
	switch(midiEvent->data.status) {
		case TSE3::MidiCommand_NoteOn:
			if (midiEvent->data.data1 < min || midiEvent->data.data1 > max) break;
			unr_evt.eventType = EVT_NORMAL_EVENT | EVT_CLASS_NOTE;
			unr_evt.U.norm_evt.pitches[0] = midiEvent->data.data1;
			unr_evt.start_time = TSE3TIME2MYMIDITIME(midiEvent->time.pulses);
			unr_evt.stop_time = TSE3TIME2MYMIDITIME(midiEvent->offTime.pulses);

			if (unr_evt.start_time & (1 << 31)) { // negative, appens during import from keyboard
				if (-unr_evt.start_time > 128) {
					unr_evt.start_time = 0;
				}
				else {
					return;
				}
			}
			if (unr_evt.stop_time & (1 << 31)) { // negative, appens during import from keyboard
				return;
			}
			unr_evt.U.norm_evt.volume = midiEvent->data.data2;
			unr_evt.U.norm_evt.num_pitches = 1;
			unr_evt.voice_nr = -1;
			len = unr_evt.stop_time - unr_evt.start_time;
#ifdef XXX
#define M(x) ((x) / MULTIPLICATOR)
			printf("On = %d, Off = %d, len = %d, star = %d, stop = %d ->",
			midiEvent->time.pulses, midiEvent->offTime.pulses, M(len), M(unr_evt.start_time), M(unr_evt.stop_time));
			fflush(stdout);
#endif
			if (noteSnap_ < 0) {
				snap2 = determine_snap(len);
			}
			else {
				snap2 = noteSnap_;
			}
			snap3 = snap2 * 4 / 3;
			lastTimesig = lastTimeSigTime(unr_evt.start_time + snap2);
			val2 = SNAP1(unr_evt.start_time, snap2, lastTimesig);
			val3 = SNAP1(unr_evt.start_time, snap3, lastTimesig);
			/*
			val2 = ((unr_evt.start_time + snap2 / 2) / snap2) * snap2;
			val3 = ((unr_evt.start_time + snap3 / 2) / snap3) * snap3;
			*/
			unr_evt.U.norm_evt.sta2diff = val2 - unr_evt.start_time;
			if (unr_evt.U.norm_evt.sta2diff < 0) unr_evt.U.norm_evt.sta2diff = -unr_evt.U.norm_evt.sta2diff;
			unr_evt.U.norm_evt.sta3diff = val3 - unr_evt.start_time;
			if (unr_evt.U.norm_evt.sta3diff < 0) unr_evt.U.norm_evt.sta3diff = -unr_evt.U.norm_evt.sta3diff;
#ifdef XXX
			printf("Rastung auf %d(%d/%d) 3er Rastung auf %d(%d/%d), qu = %d(%d/%d), sta  = %d(%d/%d) --> 2er = %d(%d/%d), 3er %d(%d/%d)\n",
			snap2 / TRI16, snap2 / MULTIPLICATOR,  snap2,
			snap3 / TRI16, snap3 / MULTIPLICATOR,  snap3,
			QUARTER_LENGTH / TRI16, QUARTER_LENGTH / MULTIPLICATOR,  QUARTER_LENGTH,
			unr_evt.start_time / TRI16, unr_evt.start_time / MULTIPLICATOR, unr_evt.start_time ,
			val2 / TRI16, val2 / MULTIPLICATOR, val2 ,
			val3 / TRI16, val3 / MULTIPLICATOR, val3 ); fflush(stdout);
#endif
			unr_evt.start_time = val2;
			unr_evt.U.norm_evt.triplet_start_time = val3;
			//lastTimesig = lastTimeSigTime(unr_evt.stop_time);
			//snap2 >>= 1;
			//snap3 >>= 1;
			val2 = SNAP1(unr_evt.stop_time, snap2, lastTimesig);
			val3 = SNAP1(unr_evt.stop_time, snap3, lastTimesig);
			/*
			val2 = ((unr_evt.stop_time + snap2 / 2) / snap2) * snap2;
			val3 = ((unr_evt.stop_time + snap3 / 2) / snap3) * snap3;
			*/
			if (val2 == unr_evt.start_time && noteSnap_ > 0) {
				if (unr_evt.stop_time - unr_evt.start_time > noteSnap_ / 4) {
					val2 = unr_evt.start_time + noteSnap_;
				}
			}
			unr_evt.stop_time = val2;
			unr_evt.U.norm_evt.triplet_stop_time = val3;
#ifdef XXX
			printf("len = %d, star = %d, stop = %d\n",
			M(unr_evt.stop_time-unr_evt.start_time), M(unr_evt.start_time), M(unr_evt.stop_time));
			fflush(stdout);
#endif
			insertEvent(&unr_evt);
			break;
		case TSE3::MidiCommand_ProgramChange:
			if (midi_program_ < 0) {
				midi_program_ = midiEvent->data.data1;
				break;
			}
			unr_evt.eventType = EVT_PROGRAM_CHANGE;
			unr_evt.U.program_nr = midiEvent->data.data1;
			unr_evt.start_time = TSE3TIME2MYMIDITIME(midiEvent->time.pulses);
			insertEvent(&unr_evt);
			break;
	}
}


void NMidiTimeScale::createVoice(int nr, main_props_str *main_props, staff_props_str *staff_props, NClef *clef,
			NVoice *voice, bool first, bool drum_channel, int volmindist, bool computeAverageVolume, unsigned int actualVolume, double averageVolume, double dynamic) {
	unsigned int i, j, l;
	int len = 0, len2, len3, newlen;
	unsigned int newVolume;
	unsigned int voiceTime = 0;
	unsigned int nextstart = 0, start = 0, stop = 0;
	property_type properties, body;
	bool inTriplet = false;
	int dotcount;
	int voldist;
	int following_rest_len;
	bool next_found;
	bool triplet = false;
	bool addRest;
	bool stacatto;
	bool potential_after_triplet, before_triplet = false;
	int line, offs;
	NChord *chord = 0, *chord2;
	NRest *rest;
	NSign *sign;
	QPtrList<NPlayable> *tupletList = 0;
	struct unrolled_midi_events_str *ptr, *nptr;
	l = array_len_;
	voiceTime = 0;
	for (i = 0, ptr = unrolled_midi_events_; i < l; i++,  ptr++) {
		if (ptr->eventType & EVT_PROGRAM_CHANGE) {
			if (nr == 0) {
				sign = new NSign(main_props, staff_props, PROGRAM_CHANGE);
				sign->setProgram(ptr->U.program_nr);
				voice->appendElem(sign);
			}
			continue;
		}
		if (ptr->voice_nr != nr) continue;
		if (ptr->eventType & EVT_PSEUDO_TRIPLET_NOTE) continue;
		start = ((ptr->eventType & EVT_CLASS_REST) || (ptr->eventType & (EVT_FIRST_IN_TRIPLET | EVT_NORMAL_EVENT))) ? ptr->start_time : ptr->U.norm_evt.triplet_start_time;
		len = start - voiceTime;
		while (len > 0) {
			len2 = quantNote(len, &dotcount, WHOLE_LENGTH);
			if (len2 >= NOTE128_LENGTH) {
			/*
				if (triplet) {
					len3 = 2 * len2 / 3;
				}
				else */ if (dotcount) {
					len3 = 3 * len2 / 2;
				}
				else {
					len3 = len2;
				}
				len -= len3;
				voiceTime += len3;
				properties = 0;
				if (dotcount) {
					properties = PROP_SINGLE_DOT;
				}
				if (!first) {
					properties |= PROP_HIDDEN;
				}
				rest = new NRest(main_props, staff_props, &(voice->yRestOffs_), len2, properties);
				voice->appendElem(rest);
#ifdef YYY
				if (triplet) {
					if (!inTriplet) {
						tupletList = new QPtrList<NPlayable>();
						inTriplet = true;
					}
					tupletList->append(rest);
				}
				else if (inTriplet) {
					if (tupletList->count() < 2) {
						tupletList->clear();
					}
					else {
						NPlayable::computeTuplet(tupletList, 3, 2);
					}
					tupletList = 0;
					inTriplet = false;
				}
#endif
			}
			else {
				voiceTime += len;
				len = 0;
			}
		}
		stop = ((ptr->eventType & EVT_CLASS_REST) || (ptr->eventType & (EVT_NORMAL_EVENT |  EVT_LAST_IN_TRIPLET))) ?  ptr->stop_time : ptr->U.norm_evt.triplet_stop_time;
		properties = 0;
		len = stop - start;
		triplet = (ptr->eventType & (EVT_PART_OF_TRIPLET | EVT_LAST_IN_TRIPLET | EVT_FIRST_IN_TRIPLET));
		stacatto = false;
		following_rest_len = 0;
		if (!triplet && !inTriplet && len < QUARTER_LENGTH) {
			for (next_found = false, nptr = ptr + 1, j = i + 1; j < l; j++,  nptr++) {
				if (nptr->eventType & EVT_PROGRAM_CHANGE) continue;
				if (nptr->voice_nr != nr) continue;
				if (nptr->eventType & EVT_PSEUDO_TRIPLET_NOTE) continue;
				next_found = true;
				break;
			}
			if (next_found) {
				nextstart = ((nptr->eventType & EVT_CLASS_REST) || (nptr->eventType & (EVT_FIRST_IN_TRIPLET | EVT_NORMAL_EVENT))) ? nptr->start_time : nptr->U.norm_evt.triplet_start_time;			     
				following_rest_len = nextstart - stop;
				newlen = following_rest_len + len;
				len2 = quantNote(newlen, &dotcount, DOUBLE_WHOLE_LENGTH);
				len3 = dotcount ? 3 * len2 / 2 : len2;
				if (len3 <= newlen && len3 < 3 * len) {
					stacatto = (len3 >= 2*len);
					stop = start + len;
					len = len3;	
				}
			}
		}
		chord = 0;
		potential_after_triplet = false;
		if (ptr->eventType & EVT_NOTE_BEFORE_TRIPLET) {
			len = ptr->split_time - ptr->start_time;
			before_triplet = true;
		}
		if (ptr->eventType & EVT_NOTE_AFTER_TRIPLET) {
			potential_after_triplet = true;
			len = ptr->split_time - ptr->U.norm_evt.triplet_start_time;
		}
		while (len >= NOTE128_LENGTH) {
			if (triplet && !before_triplet) {
				len2 = quantTriplet(len, ptr, &addRest, WHOLE_LENGTH);
				dotcount = 0;
			}
			else {
				addRest = false;
				len2 = quantNote(len, &dotcount, DOUBLE_WHOLE_LENGTH);

			}
			if (len2 >= NOTE128_LENGTH) {
				if (triplet && !before_triplet) {
					len3 = 2 * len2 / 3;
				}
				else if (dotcount) {
					len3 = 3 * len2 / 2;
				}
				else {
					len3 = len2;
				}
				len -= len3;
				voiceTime += len3;
				if (addRest) {
					if (dotcount) {
						properties = PROP_SINGLE_DOT;
					}
					if (!first) {
						properties |= PROP_HIDDEN;
					}
					rest = new NRest(main_props, staff_props, &(voice->yRestOffs_), len2, properties);
					voice->appendElem(rest);
					if (triplet) {
						if (!inTriplet) {
							tupletList = new QPtrList<NPlayable>();
							inTriplet = true;
						}
						tupletList->append(rest);
					}
				}
				else {
					clef->midi2Line(ptr->U.norm_evt.pitches[0], &line, &offs);
					properties = dotcount ? PROP_SINGLE_DOT : 0;
					if (computeAverageVolume) {
						newVolume = (int) (averageVolume + dynamic / 127.0 * (double) (ptr->U.norm_evt.volume - averageVolume));
					}
					else {
						newVolume = ptr->U.norm_evt.volume;
					}
					voldist = newVolume - actualVolume;
					if (voldist < 0) voldist = -voldist;
					if (voldist > volmindist) {
						actualVolume = newVolume;
						sign = new NSign(main_props, staff_props, VOLUME_SIG);
						sign->setVolume((actualVolume * 6) / 128, actualVolume);
						voice->appendElem(sign);
					}
					if (chord) {
						chord2 = chord->clone();
						chord2->unsetTuplet();
						chord2->tieWith(chord);
						chord2->changeLength(len2);
						if (dotcount) {
							chord2->setDotted(1);
						}
						else {
							chord2->setDotted(0);
						}
						voice->appendElem(chord2);
						if (inTriplet) {
							tupletList->append(chord2);
						}
						chord = chord2;
					}
					else {
						body = 0;
						if (drum_channel) {
							body = (PROP_BODY_CROSS << ((line + LINE_OVERFLOW) % 5));
						}
						if (stacatto) {
							properties |= PROP_STACC;
						}
						chord = new NChord(main_props, staff_props, voice, line, offs, len2, ptr->stem_policy, properties | body);
						for (j = 1; j < ptr->U.norm_evt.num_pitches; j++) {
							clef->midi2Line(ptr->U.norm_evt.pitches[j], &line, &offs);
							body = 0;
							if (drum_channel) {
								body = (PROP_BODY_CROSS << ((line + LINE_OVERFLOW) % 5));
							}
							chord->insertNewNote(line, offs, ptr->stem_policy, properties | body);
						}
						voice->appendElem(chord);
						if (triplet && !before_triplet)  {
							if (!inTriplet) {
								tupletList = new QPtrList<NPlayable>();
								inTriplet = true;
							}
							tupletList->append(chord);
						}
					}
					if (len < NOTE128_LENGTH) {
						if (before_triplet) {
							before_triplet = false;
							len = stop - ptr->split_time;
							tupletList = new QPtrList<NPlayable>();
							inTriplet = true;
						}
						if (potential_after_triplet) {
							potential_after_triplet = false;
							len = stop - ptr->split_time;
							if (tupletList->count() < 2) {
								tupletList->clear();
							}
							else {
								NPlayable::computeTuplet(tupletList, 3, 2);
							}
							tupletList = 0;
							triplet = inTriplet = false;
						}
					}
				}
			}
			else {
				voiceTime += len;
				len = 0;
			}
		}
		if (inTriplet && (ptr->eventType & EVT_LAST_IN_TRIPLET)) {
			if (tupletList->count() < 2) {
				tupletList->clear();
			}
			else {
				NPlayable::computeTuplet(tupletList, 3, 2);
			}
			tupletList = 0;
			inTriplet = false;
		}
	}
	
}


void NMidiTimeScale::createStaff(NStaff *staff, bool drum_channel, int volmindist, bool computeAverageVolume, unsigned int actualVolume, double averageVolume, double dynamic) {
	int i;
	NVoice *voice;
	NClef *clef;
	main_props_str *main_props;
	staff_props_str *staff_props;

	clef = &(staff->actualClef_);
	main_props = staff->getVoiceNr(0)->getMainPropsAddr();
	staff_props = staff->getStaffPropsAddr();

#ifdef YYY
	divideOverlapping();
#endif
#ifdef VOICE_DEBUG
	outputDistribution();
	printf("%d voices\n", voice_count());
	fflush(stdout);
#endif
	if (smallestTupletNote_ != -1) findTriplets();
	findVoices();
	while (staff->voicelist_.at(1)) {
		staff->voicelist_.remove();
	}
	if (max_voices_ > 1) {
		staff->addVoices(max_voices_ - 1);
	}
	for (i = 0; i < max_voices_; i++) {
		voice = staff->getVoiceNr(i);
		createVoice(i, main_props, staff_props, clef, voice, i == 0, drum_channel, volmindist, computeAverageVolume, actualVolume, averageVolume, dynamic);
	}
}
#endif
