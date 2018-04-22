#include "replacement_state.h"

#define pst LRUstackposition
#define uid updateWayID

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

/*
** This file implements the cache replacement state. Users can enhance the code
** below to develop their cache replacement ideas.
**
*/


////////////////////////////////////////////////////////////////////////////////
// The replacement state constructor:                                         //
// Inputs: number of sets, associativity, and replacement policy to use       //
// Outputs: None                                                              //
//                                                                            //
// DO NOT CHANGE THE CONSTRUCTOR PROTOTYPE                                    //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
CACHE_REPLACEMENT_STATE::CACHE_REPLACEMENT_STATE( UINT32 _sets, UINT32 _assoc, UINT32 _pol )
{

    numsets    = _sets;
    assoc      = _assoc;
    replPolicy = _pol;

    mytimer    = 0;

    InitReplacementState();
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function initializes the replacement policy hardware by creating      //
// storage for the replacement state on a per-line/per-cache basis.           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::InitReplacementState()
{
    // Create the state for sets, then create the state for the ways
    repl  = new LINE_REPLACEMENT_STATE* [ numsets ];

    // ensure that we were able to create replacement state
    assert(repl);

    // Create the state for the sets
    for(UINT32 setIndex=0; setIndex<numsets; setIndex++) 
    {
        repl[ setIndex ]  = new LINE_REPLACEMENT_STATE[ assoc ];

        for(UINT32 way=0; way<assoc; way++) 
        {
            // initialize stack position (for true LRU)
            repl[ setIndex ][ way ].LRUstackposition = way;
            repl[ setIndex ][ way ].location = 0;
			repl[ setIndex ][ way ].heat = 2 | (way == 0);
			repl[ setIndex ][ way ].cnt_hit = 0;
			repl[ setIndex ][ way ].pa = 0;
			repl[ setIndex ][ way ].pq = way - 1;
        }
    }

    // Contestants:  ADD INITIALIZATION FOR YOUR HARDWARE HERE
	cnt_miss = 0;
	cnt_hot = new UINT32[numsets];
	lirs = new LIRSplus[numsets];
	for (UINT32 i = 0; i < numsets; ++ i) {
		cnt_hot[i] = assoc;
		lirs[i].init(assoc);
	}
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function is called by the cache on every cache miss. The input        //
// arguments are the thread id, set index, pointers to ways in current set    //
// and the associativity.  We are also providing the PC, physical address,    //
// and accesstype should you wish to use them at victim selection time.       //
// The return value is the physical way index for the line being replaced.    //
// Return -1 if you wish to bypass LLC.                                       //
//                                                                            //
// vicSet is the current set. You can access the contents of the set by       //
// indexing using the wayID which ranges from 0 to assoc-1 e.g. vicSet[0]     //
// is the first way and vicSet[4] is the 4th physical way of the cache.       //
// Elements of LINE_STATE are defined in crc_cache_defs.h                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::GetVictimInSet( UINT32 tid, UINT32 setIndex, const LINE_STATE *vicSet, UINT32 assoc,
                                               Addr_t PC, Addr_t paddr, UINT32 accessType )
{
    // If no invalid lines, then replace based on replacement policy
    if( replPolicy == CRC_REPL_LRU ) 
    {
        return Get_LRU_Victim( setIndex );
    }
    else if( replPolicy == CRC_REPL_RANDOM )
    {
        return Get_Random_Victim( setIndex );
    }
    else if( replPolicy == CRC_REPL_CONTESTANT )
    {
        // Contestants:  ADD YOUR VICTIM SELECTION FUNCTION HERE
		// fprintf(stderr, "Miss %lld\n", PC);
		last_vic = 1;
		return getLIRSplusVictim(setIndex, PC, paddr, accessType);
    }
    else if( replPolicy == CRC_REPL_MLRU) {
		return getMLRUVictim(setIndex, PC, paddr);
	}

    // We should never get here
    assert(0);

    return -1; // Returning -1 bypasses the LLC
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function is called by the cache after every cache hit/miss            //
// The arguments are: the set index, the physical way of the cache,           //
// the pointer to the physical line (should contestants need access           //
// to information of the line filled or hit upon), the thread id              //
// of the request, the PC of the request, the accesstype, and finall          //
// whether the line was a cachehit or not (cacheHit=true implies hit)         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdateReplacementState( 
    UINT32 setIndex, INT32 updateWayID, const LINE_STATE *currLine, 
    UINT32 tid, Addr_t PC, UINT32 accessType, bool cacheHit )
{
    // What replacement policy?
    if( replPolicy == CRC_REPL_LRU ) 
    {
        UpdateLRU( setIndex, updateWayID );
    }
    else if( replPolicy == CRC_REPL_RANDOM )
    {
        // Random replacement requires no replacement state update
    }
    else if( replPolicy == CRC_REPL_CONTESTANT )
    {
        // Contestants:  ADD YOUR UPDATE REPLACEMENT STATE FUNCTION HERE
        // Feel free to use any of the input parameters to make
        // updates to your replacement policy
		if (last_vic) {
			last_vic = 0;
			updateLIRSplus(setIndex, updateWayID, PC);
		}
    }
	else if ( replPolicy == CRC_REPL_MLRU ) {
		updateMLRU(setIndex, updateWayID, PC);
	}
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//////// HELPER FUNCTIONS FOR REPLACEMENT UPDATE AND VICTIM SELECTION //////////
//                                                                            //
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function finds the LRU victim in the cache set by returning the       //
// cache block at the bottom of the LRU stack. Top of LRU stack is '0'        //
// while bottom of LRU stack is 'assoc-1'                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::Get_LRU_Victim( UINT32 setIndex )
{
    // Get pointer to replacement state of current set
    LINE_REPLACEMENT_STATE *replSet = repl[ setIndex ];

    INT32   lruWay   = 0;

    // Search for victim whose stack position is assoc-1
    for(UINT32 way=0; way<assoc; way++) 
    {
        if( replSet[way].LRUstackposition == (assoc-1) ) 
        {
            lruWay = way;
            break;
        }
    }

    // return lru way
    return lruWay;
}
// #include <cstdio>
INT32 CACHE_REPLACEMENT_STATE::getStrideVictim( UINT32 setIndex, Addr_t pc, Addr_t pa )
{
    // Get pointer to replacement state of current set
    LINE_REPLACEMENT_STATE *replSet = repl[ setIndex ];
	INT32 shway = 0;
    for(UINT32 way=1; way<assoc; way++) {
		if (replSet[way].location < replSet[shway].location) {
			shway = way;
		}
	}
	// fprintf(stderr, "%llu %llu\n", pc, pa);
	replSet[shway].location = 0.;
	return shway;
}

INT32 CACHE_REPLACEMENT_STATE::getMLRUVictim( UINT32 setIndex, Addr_t pc, Addr_t pa )
{
	++ cnt_miss;
	last_vic = 1;
	UINT32 selw;
	INT32 res = -1;
	LINE_REPLACEMENT_STATE* a = repl[setIndex];
	for (selw = 0; selw < assoc; ++ selw) {
		if (a[selw].heat == 0 && a[selw].LRUstackposition == 0) {
			res = selw;
			a[res].cnt_hit = 0;
			a[selw].LRUstackposition = assoc - cnt_hot[setIndex] - 1;
		} else if (a[selw].heat == 0) {
			-- a[selw].LRUstackposition;
		}
	}
	a[res].location = 0.;
	a[res].pa = pa;
	a[res].pc = pc;
	if (res == -1) {
		fprintf(stderr, "?!!!?\n");
	}
	return res;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function finds a random victim in the cache set                       //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::Get_Random_Victim( UINT32 setIndex )
{
    INT32 way = (rand() % assoc);
    return way;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function implements the LRU update routine for the traditional        //
// LRU replacement policy. The arguments to the function are the physical     //
// way and set index.                                                         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdateLRU( UINT32 setIndex, INT32 updateWayID )
{
    // Determine current LRU stack position
    UINT32 currLRUstackposition = repl[ setIndex ][ updateWayID ].LRUstackposition;

    // Update the stack position of all lines before the current line
    // Update implies incremeting their stack positions by one
    for(UINT32 way=0; way<assoc; way++) 
    {
        if( repl[setIndex][way].LRUstackposition < currLRUstackposition ) 
        {
            repl[setIndex][way].LRUstackposition++;
        }
    }

    // Set the LRU stack position of new line to be zero
    repl[ setIndex ][ updateWayID ].LRUstackposition = 0;
}

void CACHE_REPLACEMENT_STATE::updateMLRU( UINT32 setIndex, INT32 updateWayID, Addr_t pc ) {
	if (last_vic) {
		last_vic = 0;
		return;
	}
	LINE_REPLACEMENT_STATE* a = repl[setIndex];
	UINT32 selw, zero_way;
	// fprintf(stderr, "%d %d %d %d\n", cnt_miss, setIndex, a[updateWayID].heat, a[updateWayID].LRUstackposition);

	for (UINT32 i = 0; i < assoc; ++ i) {
		a[i].location *= .9;
	}
	if ((a[uid].pa ^ pc) & ~0xffu) {
		a[uid].pc = 0;
	}
	if (a[uid].pc) {
		a[uid].location += 10. / (fabs((double)a[uid].pa - pc));
	} else {
		a[uid].location += 1.;
	}
	++ a[updateWayID].cnt_hit;
	if (a[updateWayID].heat == 1) {
		for (selw = 0; selw < assoc; ++ selw) {
			if (a[selw].heat) {
				if (a[selw].LRUstackposition > a[updateWayID].LRUstackposition) {
					-- a[selw].LRUstackposition;
				}
			}
		}
		a[updateWayID].LRUstackposition = cnt_hot[setIndex] - 1;
	} else if ((cnt_hot[setIndex] << 2) < assoc) {
		for (selw = 0; selw < assoc; ++ selw) {
			if (!a[selw].heat && a[selw].pst > a[uid].pst) {
				-- a[selw].pst;
			}
		}
		a[updateWayID].heat = 1;
		a[updateWayID].LRUstackposition = cnt_hot[setIndex] ++;
	} else {
		UINT32 min_hit = 0x3f3f3f3f;
		double min_pos = 1e10;
		for (UINT32 i = 0; i < assoc; ++ i) {
			if (a[i].heat && a[i].cnt_hit < min_hit) {
				min_hit = a[i].cnt_hit;
			}
			if (a[i].heat && a[i].location < min_pos) {
				min_pos = a[i].location;
			}
		}
		if (true || min_pos < a[uid].location) {
			zero_way = 0;
			for (selw = 0; selw < assoc; ++ selw) {
				if (a[selw].heat && a[selw].LRUstackposition == 0) {
					zero_way = selw;
				} else if (a[selw].heat) {
					-- a[selw].LRUstackposition;
				} else if (!a[selw].heat && a[selw].LRUstackposition > a[updateWayID].LRUstackposition) {
					-- a[selw].LRUstackposition;
				}
			}
			a[zero_way].heat = 0;
			a[zero_way].LRUstackposition = assoc - cnt_hot[setIndex] - 1;
			a[updateWayID].heat = 1;
			a[updateWayID].LRUstackposition = cnt_hot[setIndex] - 1;
		} else {
			for (UINT32 i = 0; i < assoc; ++ i) {
				if (a[i].pst > a[uid].pst) {
					-- a[i].pst;
				}
			}
			a[uid].pst = assoc - cnt_hot[setIndex] - 1;
		}
	}
}
void CACHE_REPLACEMENT_STATE::updateStride( UINT32 setIndex, INT32 updateWayID ) {
	for (UINT32 way = 0; way < assoc; ++ way) {
		repl[setIndex][way].location *= .8;
	}
	repl[setIndex][updateWayID].location += 1.;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// The function prints the statistics for the cache                           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
ostream & CACHE_REPLACEMENT_STATE::PrintStats(ostream &out)
{

    out<<"=========================================================="<<endl;
    out<<"=========== Replacement Policy Statistics ================"<<endl;
    out<<"=========================================================="<<endl;

    // CONTESTANTS:  Insert your statistics printing here

    return out;
    
}

INT32 CACHE_REPLACEMENT_STATE::getLIRSplusVictim( UINT32 setIndex, Addr_t pc, Addr_t pa, UINT32 acc_type ) {
	LINE_REPLACEMENT_STATE *a = repl[setIndex];
	int res(lirs[setIndex].getVictim(a, pa));
	a[res].type = acc_type;
	return res;
}

void CACHE_REPLACEMENT_STATE::updateLIRSplus( UINT32 setIndex, INT32 updateWayID, Addr_t pc ) {
	LINE_REPLACEMENT_STATE *a = repl[setIndex];
	return lirs[setIndex].update(a, uid);
}

void LIRSplus::init(int _n) {
	n = _n;
	stc = new Addr_t[n * 3];
	stp = new int[n * 3];
	q = new int[qsz = n * 2];
	for (int i = 0; i + 1 < n; ++ i) {
		q[i] = i + 1;
	}
	qh = 0, qt = n - 1;
	tst = n;
	for (int i = 0; i < n; ++ i) {
		stp[i] = i, stc[i] = 0;
	}
}

INT32 LIRSplus::getVictim(LRS* a, Addr_t pa) {
	int res(q[qh]);
	if (a[res].heat & 2) {
		stp[a[res].pst] = -1;
	}
	qh = qnext(qh);
	a[res].pa = pa;
	for (int i = 0; i < tst; ++ i) {
		if (stc[i] == pa) {
			a[res].pst = i;
			a[res].heat = 3;
			stp[i] = res;
			moveToTop(a, res);
			rmButton(a, 1);
			return res;
		}
	}
	a[res].heat = 2;
	a[res].pst = tst;
	stc[tst] = pa;
	stp[tst ++] = res;
	q[a[res].pq = qt] = res;
	qt = qnext(qt);
	return res;
}

void LIRSplus::moveToTop(LRS* a, int w) {
	int p(a[w].pst);
	for (; p + 1 < tst; ++ p) {
		stc[p] = stc[p + 1];
		stp[p] = stp[p + 1];
		if (stp[p] > -1) {
			a[stp[p]].pst = p;
		}
	}
	stc[a[w].pst = tst - 1] = a[w].pa;
	stp[tst - 1] = w;
}

void LIRSplus::rmFromQueue(LRS* a, int p) {
	for (; qnext(p) != qt; p = qnext(p)) {
		q[p] = q[qnext(p)];
		a[q[p]].pq = p;
	}
	qt = qprev(qt);
}

int LIRSplus::getTempreture(Addr_t pa) {
	int res(63);
	for (int i = 0; i < tst; ++ i) {
		int s;
		for (s = 63; s >= 0; -- s) {
			if (((pa ^ stc[i]) >> s) & 1) {
				break;
			}
		}
		res = std::min(res, s);
	}
	return res;
}

void LIRSplus::rmButton(LRS* a, int frp) {
	int rmi(stp[0]);
	for (int p = 0; p + 1 < tst; ++ p) {
		stc[p] = stc[p + 1];
		stp[p] = stp[p + 1];
		if (stp[p] > -1) {
			a[stp[p]].pst = p;
		}
	}
	-- tst;
	if (rmi > -1) {
		a[q[qt] = rmi].heat = 0;
		int temp(getTempreture(a[rmi].pa));
		adjustTempreture(temp);
		// fprintf(stderr, "%d\n", getTempreture(a[rmi].pa));
		if (temp > temp_thres) {
			a[rmi].pq = qh = qprev(qh);
		} else {
			a[rmi].pq = qt;
			qt = qnext(qt);
		}
	}
	rmHIR(a);
}

void LIRSplus::adjustTempreture(int t) {
	if (t == -1) {
		return;
	}
	++ temp_cnt[t];
	if ((++ cnt & 0xf) == 0) {
		int s(0);
		for (temp_thres = 0; s < (cnt >> 1) && temp_thres < 30; ++ temp_thres) {
			s += temp_cnt[temp_thres];
		}
		fprintf(stderr, "%d\n", temp_thres);
	}
}

void LIRSplus::update(LRS* a, int w) {
	if (a[w].heat & 1) { // is LIR
		moveToTop(a, w);
		rmHIR(a);
	} else if (a[w].heat & 2) { // is HIR in stack
		rmFromQueue(a, a[w].pq);
		a[w].heat |= 1;
		moveToTop(a, w);
		rmHIR(a);
		rmButton(a);
	} else { // is HIR beyond stack
		a[w].heat |= 2;
		stc[a[w].pst = tst] = a[w].pa;
		stp[tst ++] = w;
	}
}

void LIRSplus::rmHIR(LRS* a) {
	while (tst > 0) {
		int rmi(stp[0]);
		if (rmi > -1) {
			if (a[rmi].heat & 1) {
				break;
			}
			a[rmi].heat &= ~2;
		}
		for (int i = 0; i + 1 < tst; ++ i) {
			stc[i] = stc[i + 1];
			stp[i] = stp[i + 1];
			if (stp[i] > -1) {
				a[stp[i]].pst = i;
			}
		}
		-- tst;
	}
}

