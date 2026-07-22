#ifndef MEMCARD_H
#define MEMCARD_H

/* MEMCARD.C's one-card-block size, recovered from PSX.SYM. */
enum { BLOCKSIZE = 8192 };

/* MEMCARD.C-private originally; extern because that source is split here. */
extern unsigned char *TENCHU_ID;
/* INFOVIEW.C-private originally; extern because that source is split here. */
extern unsigned char *CID;

#endif
