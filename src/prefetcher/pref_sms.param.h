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
 * File         : pref_sms.param.h
 * Authors      : Amin Karbas and Esteban Ramos
 * Date         : 06/04/2023
 * Description  : Impl of Somogyi et al.'s "Spatial Memory Streaming", ISCA'06
 ***************************************************************************************/

#ifndef __PREF_SMS_PARAM_H__
#define __PREF_SMS_PARAM_H__

#include "globals/global_types.h"

#define DEF_PARAM(name, variable, type, func, def, const) \
  extern const type variable;
#include "pref_sms.param.def"
#undef DEF_PARAM

#endif /* __PREF_SMS_PARAM_H__ */