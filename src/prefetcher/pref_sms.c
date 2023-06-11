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
 * File         : pref_sms.c
 * Authors      : Amin Karbas and Esteban Ramos
 * Date         : 06/04/2023
 * Description  : Impl of Somogyi et al.'s "Spatial Memory Streaming", ISCA'06
 ***************************************************************************************/

#include "debug/debug_macros.h"
#include "debug/debug_print.h"
#include "globals/global_defs.h"
#include "globals/global_types.h"
#include "globals/global_vars.h"

#include "globals/assert.h"
#include "globals/utils.h"
#include "op.h"

#include "core.param.h"
#include "dcache_stage.h"
#include "debug/debug.param.h"
#include "general.param.h"
#include "libs/cache_lib.h"
#include "libs/hash_lib.h"
#include "libs/list_lib.h"
#include "memory/memory.h"
#include "memory/memory.param.h"
#include "prefetcher/pref.param.h"
#include "prefetcher/pref_common.h"
#include "prefetcher/pref_sms.h"
#include "prefetcher/pref_sms.param.h"
#include "statistics.h"

/**************************************************************************************/
/* Macros */
#define DEBUG(args...) _DEBUG(DEBUG_PREF_PHASE, ##args)
#define REGION_BASE_MASK (~N_BIT_MASK(LOG2_64(PREF_SMS_REGION_SIZE)))
#define LINES_PER_REGION (PREF_SMS_REGION_SIZE / DCACHE_LINE_SIZE)
#define REGION_OFFSET_MASK \
  (N_BIT_MASK(LOG2_64(LINES_PER_REGION) << LOG2_64(DCACHE_LINE_SIZE)))

Pref_SMS* sms_hwp;

void pref_sms_init(HWP* hwp) {
  if(!PREF_SMS_ON)
    return;

  sms_hwp                    = (Pref_SMS*)malloc(sizeof(Pref_SMS));
  sms_hwp->hwp_info          = hwp->hwp_info;
  sms_hwp->hwp_info->enabled = TRUE;
  sms_hwp->at                = (Accumulation_Table)calloc(PREF_SMS_AT_SIZE,
                                                          sizeof(Accumulation_Table_Entry));
  sms_hwp->ft                = (Filter_Table)calloc(PREF_SMS_FT_SIZE,
                                                    sizeof(Filter_Table_Entry));
  sms_hwp->pht               = (Pattern_History_Table)calloc(
    PREF_SMS_PHT_SIZE, sizeof(Pattern_History_Table_Entry));
  sms_hwp->prf.preds = (Prediction_Register_File*)calloc(
    PREF_SMS_PRF_SIZE, sizeof(Prediction_Register));
  sms_hwp->prf.live_preds = 0;
}


void pref_sms_ul0_miss(uns8 proc_id, Addr lineAddr, Addr loadPC,
                       uns32 global_hist) {
  pref_sms_ul0_train(proc_id, lineAddr, loadPC, global_hist);
}

void pref_sms_ul0_hit(uns8 proc_id, Addr lineAddr, Addr loadPC,
                      uns32 global_hist) {
  pref_sms_ul0_train(proc_id, lineAddr, loadPC, global_hist);
}

void pref_sms_ul0_prefhit(uns8 proc_id, Addr lineAddr, Addr loadPC,
                          uns32 global_hist) {
  pref_sms_ul0_train(proc_id, lineAddr, loadPC, global_hist);
}

void pref_sms_ul0_train(uns8 proc_id, Addr lineAddr, Addr loadPC,
                        uns32 global_hist) {
  Addr   region_base = lineAddr & REGION_BASE_MASK;
  Addr   offset      = lineAddr & REGION_OFFSET_MASK;
  uns64* pattern     = NULL;  // used in multiple calls

  Flag atFound = pref_sms_at_find(sms_hwp->at, proc_id, lineAddr, loadPC,
                                  &pattern);
  if(atFound) {
    SETBIT(*pattern, offset);
    pref_sms_fetch_next_preds(&sms_hwp->prf);
    return;
  }

  Addr prevOffset;  // from ft
  Flag ftEvicted;   // from ft
  Flag ftFound = pref_sms_ft_train(sms_hwp->ft, proc_id, lineAddr, loadPC,
                                   &ftEvicted, &prevOffset);

  if(!ftFound) {
    Flag phtFound = pref_sms_pht_find(sms_hwp->pht, proc_id, lineAddr, loadPC,
                                      &pattern);
    if(phtFound) {
      pref_sms_prf_insert(&sms_hwp->prf, region_base, *pattern);
    }
  } else if(ftEvicted) {
    Accumulation_Table_Entry evictedEntry;
    Flag atEvicted = pref_sms_at_insert(sms_hwp->at, proc_id, lineAddr, loadPC,
                                        &pattern, &evictedEntry);
    SETBIT(*pattern, prevOffset);
    SETBIT(*pattern, offset);
    if(atEvicted) {  // LRU
      pref_sms_pht_insert(
        sms_hwp->pht, proc_id /* todo: incorrect; do we care? */,
        evictedEntry.offset /* only the offset matters in pht */,
        evictedEntry.pc, evictedEntry.pattern);
    }
  }
  pref_sms_fetch_next_preds(&sms_hwp->prf);
}

void pref_sms_end_generation(uns8 proc_id, Addr lineAddr, Addr loadPC,
                             uns32 global_hist) {
  Addr region_base = lineAddr & REGION_BASE_MASK;
  Addr offset      = lineAddr & REGION_OFFSET_MASK;


  Flag ftFound = pref_sms_ft_discard(sms_hwp->ft, proc_id, lineAddr, loadPC);
  if(!ftFound) {
    Accumulation_Table_Entry evictedEntry;
    Flag atFound = pref_sms_at_discard(sms_hwp->at, proc_id, lineAddr, loadPC,
                                       &evictedEntry);
    if(atFound) {  // eviction or invalidation
      pref_sms_pht_insert(
        sms_hwp->pht, proc_id /* todo: incorrect; do we care? */,
        evictedEntry.offset /* only the offset matters in pht */,
        evictedEntry.pc, evictedEntry.pattern);
    }
  }
  pref_sms_fetch_next_preds(&sms_hwp->prf);
}

// todo: currently fetches until can't anymore. Keep?
void pref_sms_fetch_next_preds(Prediction_Register_File* prf) {
  Flag done = FALSE;
  for(uns ii = 0; !done & ii < prf->live_preds; ++ii) {
    if(!prf->preds[ii].pattern) {
      pref_sms_prf_discard(prf, ii);
      continue;
    }
    Addr to_fetch = prf->preds[ii].base | (LOG2_64(prf->preds[ii].pattern));
    done          = !pref_sms_fetch_region(to_fetch);
    if(!done) {
      pref_sms_prf_discard(prf, ii);
    }
  }
}

Flag pref_sms_fetch_region(Addr region_base) {
  for(uns ii = 0; ii < LINES_PER_REGION; ++ii) {
    uns8 proc_id = get_proc_id_from_cmp_addr(region_base);
    Addr line    = region_base + (ii * DCACHE_LINE_SIZE);
    if(!pref_addto_dl0req_queue(proc_id, line, 0)) {
      return FALSE;
    }
  }
  return TRUE;
}
