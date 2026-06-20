#ifndef TTM_H
#define TTM_H

#include "mytypes.h"
#include "ttm_types.h"

extern int ttmDx;
extern int ttmDy;

uint32 ttmFindTag(struct TTtmSlot *ttmSlot, uint16 reqdTag);
void   ttmLoadTtm(struct TTtmSlot *ttmSlot, char *ttmName);
void   ttmInitSlot(struct TTtmSlot *ttmSlot);
void   ttmResetSlot(struct TTtmSlot *ttmSlot);
void   ttmPlay(struct TTtmThread *ttmThread);

#endif /* TTM_H */
