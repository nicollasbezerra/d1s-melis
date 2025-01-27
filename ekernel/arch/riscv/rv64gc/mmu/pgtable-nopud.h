/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef _PGTABLE_NOPUD_H
#define _PGTABLE_NOPUD_H

#ifndef __ASSEMBLY__

#include "pgtable-nop4d.h"

#define __PAGETABLE_PUD_FOLDED 1

/*
 * Having the pud type consist of a p4d gets the size right, and allows
 * us to conceptually access the p4d entry that this pud is folded into
 * without casting.
 */
typedef struct
{
    p4d_t p4d;
} pud_t;

#define PUD_SHIFT   P4D_SHIFT
#define PTRS_PER_PUD    1
#define PUD_SIZE    (1UL << PUD_SHIFT)
#define PUD_MASK    (~(PUD_SIZE-1))

/*
 * The "p4d_xxx()" functions here are trivial for a folded two-level
 * setup: the pud is never bad, and a pud always exists (as it's folded
 * into the p4d entry)
 */
static inline int p4d_none(p4d_t p4d)
{
    return 0;
}
static inline int p4d_bad(p4d_t p4d)
{
    return 0;
}
static inline int p4d_present(p4d_t p4d)
{
    return 1;
}
static inline void p4d_clear(p4d_t *p4d)    { }
#define pud_ERROR(pud)              (p4d_ERROR((pud).p4d))

#define p4d_populate(mm, p4d, pud)      do { } while (0)
#define p4d_populate_safe(mm, p4d, pud)     do { } while (0)
/*
 * (puds are folded into p4ds so this doesn't get actually called,
 * but the define is needed for a generic inline function.)
 */
#define set_p4d(p4dptr, p4dval) set_pud((pud_t *)(p4dptr), (pud_t) { p4dval })

static inline pud_t *pud_offset(p4d_t *p4d, unsigned long address)
{
    return (pud_t *)p4d;
}

#define pud_val(x)              (p4d_val((x).p4d))
#define __pud(x)                ((pud_t) { __p4d(x) })

#define p4d_page(p4d)               (pud_page((pud_t){ p4d }))
#define p4d_page_vaddr(p4d)         (pud_page_vaddr((pud_t){ p4d }))

/*
 * allocating and freeing a pud is trivial: the 1-entry pud is
 * inside the p4d, so has no extra memory associated with it.
 */
#define pud_alloc_one(mm, address)      NULL
#define pud_free(mm, x)             do { } while (0)
#define __pud_free_tlb(tlb, x, a)       do { } while (0)

#undef  pud_addr_end
#define pud_addr_end(addr, end)         (end)

#endif /* __ASSEMBLY__ */
#endif /* _PGTABLE_NOPUD_H */
