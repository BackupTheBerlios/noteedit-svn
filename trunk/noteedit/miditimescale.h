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

#ifndef MIDITIMESCALE_H

#define MIDITIMESCALE_H
#include "config.h"
#ifdef WITH_TSE3
#include <tse3/Midi.h>
#define MAX_VOICES 9
/* #define VOICE_DEBUG */

class NStaff;
class NVoice;
class NClef;
class main_props_str;
class staff_props_str;
class NMainFrameWidget;

class NMidiTimeScale {
	public:
		NMidiTimeScale(int noteSnap, int smallestTupletNote);
		~NMidiTimeScale();
		void insertMidiEvent(TSE3::MidiEvent *midiEvent,  unsigned int min, unsigned int max);
		int getMidiProgram();
		int getMaxVoices() {return max_voices_;}
		void createStaff(NStaff *staff, bool drum_channel, int volmindist, bool computeAverageVolume,
			unsigned int actualVolume, double averageVolume, double dynamic);
		void insertTimeOfTimesig(unsigned int timeSigtime);
	private:
		struct unrolled_midi_events_str;
		struct path_elem_str {
			int costs;
			int idx, prev_idx;
			bool ready;
		};
		struct decision_tree_str {
			unrolled_midi_events_str *next_event;
			int costs;
		};
#define MAX_PITCHES 16
#define TLIST_MAX 10
		struct unrolled_midi_events_str {
			unsigned int eventType;
#define EVT_NORMAL_EVENT	(1 << 0)
#define EVT_PROGRAM_CHANGE	(1 << 1)
#define EVT_FIRST_IN_TRIPLET	(1 << 2)
#define EVT_PART_OF_TRIPLET	(1 << 3)
#define EVT_LAST_IN_TRIPLET	(1 << 4)
#define EVT_PSEUDO_TRIPLET_NOTE	(1 << 5)
#define EVT_NOTE_BEFORE_TRIPLET (1 << 6)
#define EVT_NOTE_AFTER_TRIPLET  (1 << 7)


#define EVT_CLASS_NOTE		(1 << 8)
#define EVT_CLASS_REST		(1 << 9)

	
			unsigned int start_time, stop_time, split_time;
			union {
				struct {
					unsigned int triplet_start_time, triplet_stop_time;
					unsigned int volume, num_pitches;
					unsigned int triplet_border, used_part_time;
					unsigned char pitches[MAX_PITCHES];
					int sta2diff, sta3diff;
				} norm_evt;
				struct {
					int num_triplet_members;
					bool members_inserted_again;
					struct unrolled_midi_events_str *triplet_members[TLIST_MAX];
				} pseudo_evt;
				int program_nr;
			} U;
			int voice_nr;
			int path_idx;
			int stem_policy;
			struct decision_tree_str *decision_tree;
			double ave_pitch;
		};
		unsigned int *time_array_, time_array_len_, time_array_alloc_len_;
#ifdef VOICE_DEBUG
		void outputDistribution();
#endif
		void findVoices();
		void initialize_desicion_tree(unrolled_midi_events_str *ptr, unsigned int idx, unsigned int chunkStartIdx, unsigned int chunkEndIdx, int len);
		unsigned int findNextChunkEnd(bool *chukOk, unsigned int *chunkStartIdx);
		void findPathsInChunk(unsigned int chunkStartIdx, unsigned int chunkEndIdx);
		int findFirstUclassified(unsigned int chunkStartIdx, unsigned int len);
		int findLastUclassified(unsigned int chunkStartIdx, unsigned int len);
		void searchForTriplet(int idx, int tripletLength, int snapedTripletPosition, int typeFound);
#define TRIPLET_SNAP 0
#define SYSTEM2_SNAP  1
		int search_for_event_enlarge(int start_type, unsigned int start_time, int stop_type, unsigned int stop_time);
		bool is_nearby(unsigned int test_time, unsigned int target_time, int dist);
		void findTriplets();
		unsigned int lastTimeSigTime(unsigned int currentTime);
		void findShortestPath(struct path_elem_str* shortest_path, int fidx, unsigned int chunkStartIdx, unsigned int chunkEndIdx, unsigned int len);
		bool overlapping(unsigned int idx, unrolled_midi_events_str *ptr);
		bool detectOverlapping(int voice, unsigned int idx);
		void createVoice(int nr, main_props_str *main_props, staff_props_str *staff_props, NClef *clef,
			NVoice *voice,  bool first, bool drum_channel, int volmindist, bool computeAverageVolume,
			unsigned int actualVolume, double averageVolume, double dynamic);
		int quantNote(int l, int *dotcount, int maxlength);
		int quantTriplet(int l, struct unrolled_midi_events_str *ptr, bool *addRest, int maxlength);

		struct tripletMemberStr {
			int evt_class;
			int idx;
			unsigned int start_time, stop_time, split_time;
		} TList_[TLIST_MAX], MinTList_[TLIST_MAX];
		int MinTListPtr_, TListPtr_;
		void prependTidx(int idx);
		void appendTidx(int idx);
		void prependTRest(unsigned int start_time, unsigned int stop_time);
		void appendTRest(unsigned int start_time, unsigned int stop_time);
		int search_cuttable_note_left(unsigned int split_time, unsigned int stop_time);
		int search_cuttable_note_right(unsigned int split_time, unsigned int start_time);
		bool is_a_cuttable_right_note(int idx, unsigned int target_time);
		void append_cuttable_note(int idx, unsigned int split_time);
		void prepend_cuttable_note(int idx, unsigned int split_time);
		int findBigRightTripletPartSloppy(int idx, unsigned int T1, int divis);
		int findBigLeftTripletPartSloppy(unsigned int T0, int divis);
		int findSmallLeftTripletPartSloppy(unsigned int T0, int divis);
		int findSmallRightTripletPartSloppy(int idx, unsigned int T1, int divis);
		void removeEvent(unsigned int idx);
		void insertEvent(struct unrolled_midi_events_str *ptr);
		void findFreeSlot(unrolled_midi_events_str *ptr);
		void divideOverlapping();
		int determine_snap(int length);
		void addEvent(int nr, struct unrolled_midi_events_str *ptr);
		unsigned int array_len_, alloc_len_;
		struct unrolled_midi_events_str *unrolled_midi_events_;
		int midi_program_;
		int max_voices_;
		int noteSnap_, smallestTupletNote_, mink_;
};

#endif
#endif /* MIDITIMESCALE_H */
