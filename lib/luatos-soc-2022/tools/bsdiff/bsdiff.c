/*-
* Copyright 2003-2005 Colin Percival
* Copyright 2012 Matthew Endsley
* Copyright 2025 freshev
* All rights reserved
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted providing that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#include "bsdiff.h"
#include "bzlib.h"

#include <limits.h>
#include <string.h>
#include <sys/types.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

#define MIN(x,y) (((x)<(y)) ? (x) : (y))

static void split(int64_t *I, int64_t *V, int64_t start, int64_t len, int64_t h)
{
    int64_t i, j, k, x, tmp, jj, kk;

    if (len<16) {
	for (k = start; k<start + len; k += j) {
	    j = 1; x = V[I[k] + h];
	    for (i = 1; k + i<start + len; i++) {
		if (V[I[k + i] + h]<x) {
		    x = V[I[k + i] + h];
		    j = 0;
		};
		if (V[I[k + i] + h] == x) {
		    tmp = I[k + j]; I[k + j] = I[k + i]; I[k + i] = tmp;
		    j++;
		};
	    };
	    for (i = 0; i<j; i++) V[I[k + i]] = k + j - 1;
	    if (j == 1) I[k] = -1;
	};
	return;
    };

    x = V[I[start + len / 2] + h];
    jj = 0; kk = 0;
    for (i = start; i<start + len; i++) {
	if (V[I[i] + h]<x) jj++;
	if (V[I[i] + h] == x) kk++;
    };
    jj += start; kk += jj;

    i = start; j = 0; k = 0;
    while (i<jj) {
	if (V[I[i] + h]<x) {
	    i++;
	}
	else if (V[I[i] + h] == x) {
	    tmp = I[i]; I[i] = I[jj + j]; I[jj + j] = tmp;
	    j++;
	}
	else {
	    tmp = I[i]; I[i] = I[kk + k]; I[kk + k] = tmp;
	    k++;
	};
    };

    while (jj + j<kk) {
	if (V[I[jj + j] + h] == x) {
	    j++;
	}
	else {
	    tmp = I[jj + j]; I[jj + j] = I[kk + k]; I[kk + k] = tmp;
	    k++;
	};
    };

    if (jj>start) split(I, V, start, jj - start, h);

    for (i = 0; i<kk - jj; i++) V[I[jj + i]] = kk - 1;
    if (jj == kk - 1) I[jj] = -1;

    if (start + len>kk) split(I, V, kk, start + len - kk, h);
}

static void qsufsort(int64_t *I, int64_t *V, const uint8_t *old, int64_t oldsize)
{
    int64_t buckets[256];
    int64_t i, h, len;

    for (i = 0; i<256; i++) buckets[i] = 0;
    for (i = 0; i<oldsize; i++) buckets[old[i]]++;
    for (i = 1; i<256; i++) buckets[i] += buckets[i - 1];
    for (i = 255; i>0; i--) buckets[i] = buckets[i - 1];
    buckets[0] = 0;

    for (i = 0; i<oldsize; i++) I[++buckets[old[i]]] = i;
    I[0] = oldsize;
    for (i = 0; i<oldsize; i++) V[i] = buckets[old[i]];
    V[oldsize] = 0;
    for (i = 1; i<256; i++) if (buckets[i] == buckets[i - 1] + 1) I[buckets[i]] = -1;
    I[0] = -1;

    for (h = 1; I[0] != -(oldsize + 1); h += h) {
	len = 0;
	for (i = 0; i<oldsize + 1;) {
	    if (I[i]<0) {
		len -= I[i];
		i -= I[i];
	    }
	    else {
		if (len) I[i - len] = -len;
		len = V[I[i]] + 1 - i;
		split(I, V, i, len, h);
		i += len;
		len = 0;
	    };
	};
	if (len) I[i - len] = -len;
    };

    for (i = 0; i<oldsize + 1; i++) I[V[i]] = i;
}

static int64_t matchlen(const uint8_t *old, int64_t oldsize, const uint8_t *nnew, int64_t newsize)
{
    int64_t i;

    for (i = 0; (i<oldsize) && (i<newsize); i++)
	if (old[i] != nnew[i]) break;

    return i;
}

static int64_t search(const int64_t *I, const uint8_t *old, int64_t oldsize,
    const uint8_t *nnew, int64_t newsize, int64_t st, int64_t en, int64_t *pos)
{
    int64_t x, y;

    if (en - st<2) {
	x = matchlen(old + I[st], oldsize - I[st], nnew, newsize);
	y = matchlen(old + I[en], oldsize - I[en], nnew, newsize);

	if (x>y) {
	    *pos = I[st];
	    return x;
	}
	else {
	    *pos = I[en];
	    return y;
	}
    };

    x = st + (en - st) / 2;
    if (memcmp(old + I[x], nnew, MIN(oldsize - I[x], newsize))<0) {
	return search(I, old, oldsize, nnew, newsize, x, en, pos);
    }
    else {
	return search(I, old, oldsize, nnew, newsize, st, x, pos);
    };
}

static int offtout(int64_t x, uint8_t *buf)
{
    int64_t y;

    if (x<0) y = -x; else y = x;

    buf[0] = y % 256; y -= buf[0];
    y = y / 256; buf[1] = y % 256; y -= buf[1];
    y = y / 256; buf[2] = y % 256; y -= buf[2];
    y = y / 256; buf[3] = y % 256;
    if (x<0) buf[3] |= 0x80;
    return 1;
}

struct bsdiff_request
{
    int32_t partno;
    int32_t flag;
    const uint8_t* old;
    int64_t oldsize;
    const uint8_t* nnew;
    int64_t newsize;
    //struct bsdiff_stream* stream;
    char* norTmpFileName;
    int64_t *I;
    uint32_t buffersize;
    uint8_t *buffer;
};

static int bsdiff_internal(struct bsdiff_request * req)
{
    //struct bsdiff_request &req = *pReq;
    int64_t *I, *V;
    int64_t scan, pos, len;
    int64_t lastscan, lastpos, lastoffset;
    int64_t oldscore, scsc;
    int64_t s, Sf, lenf, Sb, lenb;
    int64_t overlap, Ss, lens;
    int64_t i;
    uint8_t *buffer;
    uint8_t buf[4 * 3];

    int bz2err;
    BZFILE* bz2;
    memset(&bz2, 0, sizeof(bz2));

    if ((req->I = (int64_t*)malloc((req->oldsize + 1)*sizeof(int64_t))) == NULL)
	return -1;	

    if ((V = (int64_t*)malloc((req->oldsize + 1)*sizeof(int64_t))) == NULL) {		
	free(req->I);
	return -1;
    }
    if ((req->buffer = (uint8_t*)malloc(req->newsize + 1)) == NULL)
    {
	free(req->I);
	free(V);
	return -1;
    }

    I = req->I;

    qsufsort(I, V, req->old, req->oldsize);
    free(V);

    buffer = req->buffer;

    FILE* norTmpFile = fopen(req->norTmpFileName, "wb");
    if (!norTmpFile) {
	printf("Open failed %s", req->norTmpFileName);
	return -1;
    }

    offtout(req->newsize, buf);
    if(fwrite("BSDIFF40\0\0\0\0\0\0\0\0", 16, 1, norTmpFile) != 1 ||
	fwrite(buf, 4, 1, norTmpFile) != 1) {
	printf("Failed to write NOR header");
	return 1;
    }	

    if (NULL == (bz2 = BZ2_bzWriteOpen(&bz2err, norTmpFile, 1, 0, 0))) {
	printf("BZ2_bzWriteOpen, bz2err=%d", bz2err);
	return 1;
    }

    if (req->newsize) {

	/* Compute the differences, writing ctrl as we go */
	scan = 0; len = 0; pos = 0;
	lastscan = 0; lastpos = 0; lastoffset = 0;
	while (scan < req->newsize) {
	    oldscore = 0;

	    for (scsc = scan += len; scan < req->newsize; scan++) {
		len = search(I, req->old, req->oldsize, req->nnew + scan, req->newsize - scan,
		    0, req->oldsize, &pos);

		for (; scsc < scan + len; scsc++)
		    if ((scsc + lastoffset<req->oldsize) &&
			(req->old[scsc + lastoffset] == req->nnew[scsc]))
			oldscore++;

		if (((len == oldscore) && (len != 0)) ||
		    (len>oldscore + 8)) break;

		if ((scan + lastoffset < req->oldsize) &&
		    (req->old[scan + lastoffset] == req->nnew[scan]))
		    oldscore--;
	    };

	    if ((len != oldscore) || (scan == req->newsize)) {
		s = 0; Sf = 0; lenf = 0;
		for (i = 0; (lastscan + i < scan) && (lastpos + i < req->oldsize);) {
		    if (req->old[lastpos + i] == req->nnew[lastscan + i]) s++;
		    i++;
		    if (s * 2 - i > Sf * 2 - lenf) { Sf = s; lenf = i; };
		};

		lenb = 0;
		if (scan < req->newsize) {
		    s = 0; Sb = 0;
		    for (i = 1; (scan >= lastscan + i) && (pos >= i); i++) {
			if (req->old[pos - i] == req->nnew[scan - i]) s++;
			if (s * 2 - i>Sb * 2 - lenb) { Sb = s; lenb = i; };
		    };
		};

		if (lastscan + lenf > scan - lenb) {
		    overlap = (lastscan + lenf) - (scan - lenb);
		    s = 0; Ss = 0; lens = 0;
		    for (i = 0; i < overlap; i++) {
			if (req->nnew[lastscan + lenf - overlap + i] ==
			    req->old[lastpos + lenf - overlap + i]) s++;
			if (req->nnew[scan - lenb + i] ==
			    req->old[pos - lenb + i]) s--;
			if (s > Ss) { Ss = s; lens = i + 1; };
		    };

		    lenf += lens - overlap;
		    lenb -= lens;
		};

		offtout(lenf, buf);
		offtout((scan - lenb) - (lastscan + lenf), buf + 4);
		offtout((pos - lenb) - (lastpos + lenf), buf + 8);

		/* Write control data */
		//if (writedata(bz2, buf, sizeof(buf))) return -1;
		BZ2_bzWrite(&bz2err, bz2, buf, sizeof(buf));
		if (bz2err != BZ_STREAM_END && bz2err != BZ_OK) return -1;

		/* Write diff data */
		for (i = 0; i < lenf; i++) buffer[i] = req->nnew[lastscan + i] - req->old[lastpos + i];
		//if (writedata(bz2, buffer, lenf)) return -1;
		BZ2_bzWrite(&bz2err, bz2, buffer, lenf);
		if (bz2err != BZ_STREAM_END && bz2err != BZ_OK) return -1;

		/* Write extra data */
		for (i = 0; i < (scan - lenb) - (lastscan + lenf); i++) buffer[i] = req->nnew[lastscan + lenf + i];
		//if (writedata(bz2, buffer, (scan - lenb) - (lastscan + lenf))) return -1;
		BZ2_bzWrite(&bz2err, bz2, buffer, (scan - lenb) - (lastscan + lenf));
		if (bz2err != BZ_STREAM_END && bz2err != BZ_OK) return -1;

		lastscan = scan - lenb;
		lastpos = pos - lenb;
		lastoffset = pos - scan;
	    }
	}
    }

    BZ2_bzWriteClose(&bz2err, bz2, 0, NULL, NULL);
    if (bz2err != BZ_OK) {
	printf("BZ2_bzWriteClose, bz2err=%d", bz2err);
	return 1;
    }

    if (fclose(norTmpFile)) {
	printf("fclose");
	return 1;
    }

    if (req->flag == 1) offtout(0x43424571, buf);
    else offtout(0, buf);

    uint64_t fd, newsize;
    if (((fd = open(req->norTmpFileName, O_RDWR, 0)) < 0) ||
	((newsize = lseek(fd, 0, SEEK_END)) == -1) ||		
	(offtout(newsize, buf + 4)) != 1  ||
	(lseek(fd, 8, SEEK_SET) != 8) ||
	(write(fd, buf, 8) != 8) ||
	(close(fd) == -1)) {
	printf("Write header failed");
	return 1;
    }	

    free(req->buffer);
    free(req->I);

    req->buffersize = newsize;
    req->buffer = (uint8_t *)malloc(req->buffersize);
    if (((fd = open(req->norTmpFileName, O_RDONLY, 0)) < 0) ||
	(read(fd, req->buffer, req->buffersize) == 0) ||
	(close(fd) == -1)) {
	printf("Read failed %s", req->norTmpFileName); 
	return 1;
    }
    remove(req->norTmpFileName);

    return 0;
}


int main(int argc, char *argv[])
{
    int fd;
    uint8_t *old, *nnew;
    off_t oldsize, newsize;
    uint8_t buf[4];
    FILE * pf;	


    if (argc != 4) {
	printf("usage: %s oldfile newfile patchfile\n", argv[0]);
	return 1;
    }

    /* Allocate oldsize+1 bytes instead of oldsize bytes to ensure
    that we never try to malloc(0) and get a NULL pointer */
    if (((fd = open(argv[1], O_RDONLY, 0)) < 0) ||
	((oldsize = lseek(fd, 0, SEEK_END)) == -1) ||
	((old = (uint8_t*)malloc(oldsize + 1)) == NULL) ||
	(lseek(fd, 0, SEEK_SET) != 0) ||
	(read(fd, old, oldsize) == 0) ||
	(close(fd) == -1))
    {
	printf("%s", argv[1]); return 1;
    }

    /* Allocate newsize+1 bytes instead of newsize bytes to ensure
    that we never try to malloc(0) and get a NULL pointer */
    if (((fd = open(argv[2], O_RDONLY, 0)) < 0) ||
	((newsize = lseek(fd, 0, SEEK_END)) == -1) ||
	((nnew = (uint8_t*)malloc(newsize + 1)) == NULL) ||
	(lseek(fd, 0, SEEK_SET) != 0) ||
	(read(fd, nnew, newsize) == 0) ||
	(close(fd) == -1)) {
	printf("%s", argv[2]);
	return 1;
    }

    struct bsdiff_request req = { 0 };
    char partnostring[8];
    int64_t totalParts = (newsize + 0x7FFF) / 0x8000;
    uint32_t * headerBuffer = (uint32_t*)malloc(totalParts * sizeof(uint32_t));
    int64_t norTmpFileNameLen = 0;

    if (headerBuffer == NULL) {
	printf("malloc failed");
	return 1;
    }
    memset(headerBuffer, 0, totalParts * sizeof(uint32_t));

    /* Create the patch file */
    if ((pf = fopen(argv[3], "wb")) == NULL) {
	printf("%s", argv[3]);
	return 1;
    }

    /* Write header (signature+newsize)*/
    offtout(totalParts, buf);
    if (fwrite("BPHD", 4, 1, pf) != 1 ||
	fwrite(buf, sizeof(buf), 1, pf) != 1 ||
	offtout(oldsize, buf) != 1 ||
	fwrite(buf, sizeof(buf), 1, pf) != 1 ||
	offtout(0x4F726F4E, buf) != 1 ||
	fwrite(buf, sizeof(buf), 1, pf) != 1 ||
	fwrite(headerBuffer, totalParts * sizeof(uint32_t), 1, pf) != 1) {
	printf("Failed to write header");
	return 1;
    }	

    char * norTmpFileName = (char*)malloc(strlen(argv[3]) + 20);
    if (norTmpFileName == NULL) {
	printf("Create tmp file failed");
	return 1;
    }	

    strcpy(norTmpFileName, argv[3]);
    strcat(norTmpFileName, ".nor.tmp");
    norTmpFileNameLen = strlen(norTmpFileName);
    req.norTmpFileName = norTmpFileName;
    
    if (newsize > 0) {
	int64_t newPartCounter = 0;
	int64_t newPosition = 0;
	int64_t partno = 0;
	int64_t oldPosition = oldsize;
	int64_t tempSize = 0;

	do {
	    if (newPartCounter >= totalParts) {
		printf("Partno error");
		return 1;
	    }
	    req.partno = partno;
	    //itoa((int)partno, partnostring, 10);
	    sprintf(partnostring,"%d",(int)partno);
	    strcpy(norTmpFileName + norTmpFileNameLen, partnostring);
	    req.flag = 0;

	    if (newPosition + 0x8000 >= newsize) {
		req.old = (uint8_t *)&old[newPosition];
		if (oldPosition <= 0)
		    req.oldsize = 0;
		else
		    req.oldsize = oldsize - newPosition;

		req.nnew = (uint8_t *)&nnew[newPosition];
		req.newsize = newsize - newPosition;
	    }
	    else {
		req.old = (uint8_t *)&old[newPosition];
		if (oldPosition <= 0)
		    tempSize = 0;
		else
		    tempSize = oldsize - newPosition;
		req.oldsize = tempSize;
		req.nnew = (uint8_t *)&nnew[newPosition];
		req.newsize = 0x8000;
		if (tempSize >= 0x8000 && !memcmp(&nnew[newPosition], &old[newPosition], 0x8000)) {
		    req.flag = 1;
		}
	    }

	    int res = bsdiff_internal(&req);

	    if (res) {
		printf("main_partition");
		return 1;
	    }	
	    
	    if (fwrite(req.buffer, req.buffersize, 1, pf) != 1) {
		printf("fwrite(%s)", argv[3]);
		return 1;
	    }
	    fflush(pf);
	    free(req.buffer);

	    ++partno;
	    newPosition += 0x8000;
	    oldPosition = oldPosition - 0x8000;
	    headerBuffer[newPartCounter++] = req.buffersize;

	} while (newPosition < (int)newsize);

	if (partno != totalParts) {
	    printf("partno err");
	    return 1;
	}

	if (((fd = open(argv[3], O_RDWR, 0)) < 0) ||			
	    (lseek(fd, 16, SEEK_SET) != 16) ||			
	    (write(fd, headerBuffer, totalParts * sizeof(uint32_t))) != totalParts * sizeof(uint32_t) ||
	    (close(fd) == -1)) {
	    printf("Failed to write header");
	    return 1;
	}
    }

    /* Free the memory we used */
    free(old);
    free(nnew);
    free(headerBuffer);
    free(norTmpFileName);

    return 0;
}

