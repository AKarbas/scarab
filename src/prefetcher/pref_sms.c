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

Pref_SMS* sms_hwp;

uns64 region_mask = N_BIT_MASK(LOG2_64(PREF_SMS_REGION_SIZE));


void pref_sms_init(HWP* hwp) {
  int ii;

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


bool pref_sms_ft_train(Filter_Table ft, uns8 proc_id, Addr lineAddr,
                       Addr loadPC, Addr* offset) {
  Addr region_tag = lineAddr & ~region_mask;
  Addr offset     = lineAddr & region_mask;

  // Check the accummulation table for Tag - PC with different Offset
  // if match set pattern
  // else
  // check Filter Table for Tag - PC
  // if no match TriggerAccess!
  // else
  // compareOffset
  // if different
  // move to AccumulationTable and remove from FilterTable
  // else
  // Update LRU
  // if AccumulationTable Full
  // Evict LRU to PHT
  // Call Evict Handler Method
  // if FilterTable Full
  // Evict LRU and forget about it
}