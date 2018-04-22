#ifndef REPL_STATE_H
#define REPL_STATE_H

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This file is distributed as part of the Cache Replacement Championship     //
// workshop held in conjunction with ISCA'2010.                               //
//                                                                            //
//                                                                            //
// Everyone is granted permission to copy, modify, and/or re-distribute       //
// this software.                                                             //
//                                                                            //
// Please contact Aamer Jaleel <ajaleel@gmail.com> should you have any        //
// questions                                                                  //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <map>
#include "utils.h"
#include "crc_cache_defs.h"

// Replacement Policies Supported
typedef enum 
{
    CRC_REPL_LRU        = 0,
    CRC_REPL_RANDOM     = 1,
    CRC_REPL_CONTESTANT = 2,
    CRC_REPL_MLRU        = 3
} ReplacemntPolicy;

// Replacement State Per Cache Line
typedef struct
{
    UINT32  LRUstackposition;

    // CONTESTANTS: Add extra state per cache line here
	double location;

	UINT32 heat;
	UINT32 pq;
	UINT32 cnt_hit;
	Addr_t pa, pc;
	INT32 type;
} LINE_REPLACEMENT_STATE, LRS;

// The implementation for the cache replacement policy
class CACHE_REPLACEMENT_STATE
{

  private:
    UINT32 numsets;
    UINT32 assoc;
    UINT32 replPolicy;
    
    LINE_REPLACEMENT_STATE   **repl;

    COUNTER mytimer;  // tracks # of references to the cache

    // CONTESTANTS:  Add extra state for cache here

  public:

    // The constructor CAN NOT be changed
    CACHE_REPLACEMENT_STATE( UINT32 _sets, UINT32 _assoc, UINT32 _pol );

    INT32  GetVictimInSet( UINT32 tid, UINT32 setIndex, const LINE_STATE *vicSet, UINT32 assoc, Addr_t PC, Addr_t paddr, UINT32 accessType );
    void   UpdateReplacementState( UINT32 setIndex, INT32 updateWayID );

    void   SetReplacementPolicy( UINT32 _pol ) { replPolicy = _pol; } 
    void   IncrementTimer() { mytimer++; } 

    void   UpdateReplacementState( UINT32 setIndex, INT32 updateWayID, const LINE_STATE *currLine, 
                                   UINT32 tid, Addr_t PC, UINT32 accessType, bool cacheHit );

    ostream&   PrintStats( ostream &out);

  private:
    
    void   InitReplacementState();
    INT32  Get_Random_Victim( UINT32 setIndex );

    INT32  Get_LRU_Victim( UINT32 setIndex );
    void   UpdateLRU( UINT32 setIndex, INT32 updateWayID );

	void updateStride(UINT32 setIndex, INT32 updateWayID);
    INT32  getStrideVictim( UINT32 setIndex, Addr_t pc, Addr_t pa );

	UINT32* cnt_hot;
	int cnt_miss, last_vic;
	void updateMLRU(UINT32 setIndex, INT32 updateWayID, Addr_t);
    INT32  getMLRUVictim( UINT32 setIndex , Addr_t, Addr_t);

	class LIRSplus* lirs;
	void updateLIRSplus(UINT32 setIndex, INT32 updateWayID, Addr_t);
    INT32  getLIRSplusVictim( UINT32 setIndex , Addr_t, Addr_t, UINT32 );
};

class LIRSplus {
	private:
		int *q, qh, qt, cnt, temp_thres;
		int temp_cnt[67];
	public:
		LIRSplus() {
			qh = qt = 0, cnt = 0;
			temp_thres = 38;
			memset(temp_cnt, 0, sizeof(temp_cnt));
		}
		~LIRSplus() {
			delete [] this->q;
		}
		void init(int);
		INT32 getVictim(LRS*, Addr_t);
		void update(LRS*, INT32);
	private:
		int n, qsz;
		Addr_t *stc;
		int	*stp;
		int tst;
		void rmHIR(LRS*);
		void moveToTop(LRS*, int);
		void rmFromQueue(LRS*, int);
		void rmButton(LRS*, int = 0);
		int getTempreture(Addr_t);
		void adjustTempreture(int);
		inline int qprev(int p) {
			return p == 0 ? qsz - 1 : p - 1;
		}
		inline int qnext(int p) {
			return p == qsz - 1 ? 0 : p + 1;
		}
};

#endif
