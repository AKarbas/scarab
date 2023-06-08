/* Copyright 2023 Esteban Ramos and Mohammadamin Karbasforushan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/***************************************************************************************
 * File         : pref_sms.h
 * Authors      : Amin Karbas and Esteban Ramos
 * Date         : 06/04/2023
 * Description  : Impl of Somogyi et al.'s "Spatial Memory Streaming", ISCA'06
 ***************************************************************************************/

#ifndef __PREF_SMS_H__
#define __PREF_SMS_H__

#include "globals/global_types.h"

/**************************************************************************************/
/* Forward Declarations */

struct Mem_Req_struct;
struct Pref_Mem_Req_struct;

/**************************************************************************************/
/* Types */

typedef struct Filter_Table_Entry_struct {
  Addr    tag;
  Addr    pc;
  Addr    offset;
  Counter last_access_time; /* for replacement policy */
} Filter_Table_Entry;

typedef Filter_Table_Entry* Filter_Table;

typedef struct Accumulation_Table_Entry_struct {
  Addr    tag;
  Addr    pc;
  Addr    offset;
  uns64   pattern;
  Counter last_access_time; /* for replacement policy */
} Accumulation_Table_Entry;

typedef Accumulation_Table_Entry* Accumulation_Table;

typedef struct Pattern_History_Table_Entry_struct {
  Addr    pc;
  Addr    offset;
  uns64   pattern;
  Counter last_access_time; /* for replacement policy */
} Pattern_History_Table_Entry;

typedef Pattern_History_Table_Entry* Pattern_History_Table;

typedef struct Prediction_Register_struct {
  Addr  base;
  uns64 pattern;
} Prediction_Register;

typedef struct Prediction_Register_File_struct {
  Prediction_Register* preds;
  uns                  live_preds = 0;
} Prediction_Register_File;

typedef struct Pref_SMS_struct {
  HWP_Info*                hwp_info;
  Accumulation_Table       at;
  Filter_Table             ft;
  Pattern_History_Table    pht;
  Prediction_Register_File prf;
} Pref_SMS;

void pref_sms_init(HWP* hwp);
void pref_sms_ul0_miss(uns8 proc_id, Addr lineAddr, Addr loadPC,
                       uns32 global_hist);
void pref_sms_ul0_hit(uns8 proc_id, Addr lineAddr, Addr loadPC,
                      uns32 global_hist);
void pref_sms_ul0_prefhit(uns8 proc_id, Addr lineAddr, Addr loadPC,
                          uns32 global_hist);

void pref_sms_ul0_train(uns8 proc_id, Addr lineAddr, Addr loadPC,
                        uns32 global_hist);

// returns whether the entry was found with a different offset.
// if found, sets evicted and prevOffset.
Flag pref_sms_ft_train(Filter_Table ft, uns8 proc_id, Addr lineAddr,
                       Addr loadPC, Flag* evicted, Addr* prevOffset);

// populates pattern, to be set by caller.
// also updates lru, if found.
Flag pref_sms_at_find(Accumulation_Table at, uns8 proc_id, Addr lineAddr,
                      Addr loadPC, uns64** pattern);

// returns whether something got evicted; if so, populates evicted.
// populates pattern, to be set by caller.
Flag pref_sms_at_insert(Accumulation_Table at, uns8 proc_id, Addr lineAddr,
                        Addr loadPC, uns64** pattern,
                        Accumulation_Table_Entry* evicted);

// populates pattern, to be used by caller.
Flag pref_sms_pht_find(Pattern_History_Table pht, uns8 proc_id, Addr lineAddr,
                       Addr loadPC, uns64** pattern);

// inserts new entry to pht; possibly replacing lru
void pref_sms_pht_insert(Pattern_History_Table pht, uns8 proc_id, Addr lineAddr,
                         Addr loadPC, uns64 pattern);

// adds a prediction to the to-fetch queue. if full, replaces an entry
// todo: which entry? counter? lru? rand?
void pref_sms_prf_insert(Prediction_Register_File* prf, Addr base,
                         uns64 pattern);

// fetches from to-fetch queue; round-robin between live entries
// todo: fetch one or as many as the memory system takes?
void pref_sms_fetch_next_preds(Prediction_Register_File* prf);

#endif /* __PREF_SMS_H__ */