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
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#include <err.h>
#endif

#define MIN(x,y) (((x)<(y)) ? (x) : (y))

static void split(int32_t *I, int32_t *V, int32_t start, int32_t len, int32_t h)
{
	int32_t i, j, k, x, tmp, jj, kk;

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

static void qsufsort(int32_t *I, int32_t *V, const uint8_t *old, int32_t oldsize)
{
	int32_t buckets[256];
	int32_t i, h, len;

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

	for (i = 0; i < oldsize + 1; i++) {
		I[V[i]] = i;
	}
}

static int32_t matchlen(const uint8_t *old, int32_t oldsize, const uint8_t *nnew, int32_t newsize)
{
	int32_t i;

	for (i = 0; (i<oldsize) && (i<newsize); i++)
		if (old[i] != nnew[i]) break;

	return i;
}

static int32_t search(const int32_t *I, const uint8_t *old, int32_t oldsize,
	const uint8_t *nnew, int32_t newsize, int32_t st, int32_t en, int32_t *pos)
{
	int32_t x, y;
	if (en - st < 2) {
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

static int offtout(int32_t x, uint8_t *buf)
{
	int32_t y;

	if (x<0) y = -x; else y = x;

	buf[0] = y % 256; y -= buf[0];
	y = y / 256; buf[1] = y % 256; y -= buf[1];
	y = y / 256; buf[2] = y % 256; y -= buf[2];
	y = y / 256; buf[3] = y % 256;
	if (x<0) buf[3] |= 0x80;
	return 1;
}

struct bsdiff_request {
	int32_t partno;
	int32_t flag;
	const uint8_t* old;
	int32_t oldsize;
	const uint8_t* nnew;
	int32_t newsize;
	char* temp_file_name;
	int32_t *I;
	uint32_t buffersize;
	uint8_t *buffer;
};

static int bsdiff_internal(struct bsdiff_request * req)
{
	int32_t *I, *V;
	int32_t scan, pos, len;
	int32_t lastscan, lastpos, lastoffset;
	int32_t oldscore, scsc;
	int32_t s, Sf, lenf, Sb, lenb;
	int32_t overlap, Ss, lens;
	int32_t i;
	uint8_t *buffer;
	uint8_t buf[4 * 3];

	int bz2err;
	BZFILE* bz2;
	memset(&bz2, 0, sizeof(bz2));

	if ((req->I = (int32_t*)malloc((req->oldsize + 1)*sizeof(int32_t))) == NULL)
		return -1;

	if ((V = (int32_t*)malloc((req->oldsize + 1)*sizeof(int32_t))) == NULL) {
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

	FILE* temp_file = fopen(req->temp_file_name, "wb");
	if (!temp_file) {
		printf("Open failed %s", req->temp_file_name);
		return -1;
	}

	offtout(req->newsize, buf);
	if (fwrite("BSDIFF40\0\0\0\0\0\0\0\0", 16, 1, temp_file) != 1 ||
		fwrite(buf, 4, 1, temp_file) != 1) {
		printf("Failed to write NOR header");
		return 1;
	}

	if (NULL == (bz2 = BZ2_bzWriteOpen(&bz2err, temp_file, 1, 0, 0))) {
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
					if ((scsc + lastoffset < req->oldsize) &&
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

	if (fclose(temp_file)) {
		printf("Close file failed");
		return 1;
	}

	if (req->flag == 1) offtout(0x43424571, buf);
	else offtout(0, buf);

	uint32_t fd, newsize;
	if (((fd = open(req->temp_file_name, O_RDWR, 0)) < 0) ||
		((newsize = lseek(fd, 0, SEEK_END)) == -1) ||
		(offtout(newsize, buf + 4)) != 1 ||
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
	if (((fd = open(req->temp_file_name, O_RDONLY, 0)) < 0) ||
		(read(fd, req->buffer, req->buffersize) == 0) ||
		(close(fd) == -1)) {
		printf("Read failed %s", req->temp_file_name);
		return 1;
	}
	remove(req->temp_file_name);

	return 0;
}

int main_normal(char ** argv, uint8_t *old, off_t oldsize, uint8_t *nnew, off_t newsize) {
	struct bsdiff_request req = { 0 };
	char partno_string[11];
	int32_t total_parts = (newsize + 0x7FFF) / 0x8000;
	uint32_t * header_buffer = (uint32_t*)malloc(total_parts * sizeof(uint32_t));
	int32_t tmp_file_name_len = 0;
	FILE * pf;
	uint8_t buf[4];

	if (header_buffer == NULL) {
		printf("malloc failed");
		return 1;
	}
	memset(header_buffer, 0, total_parts * sizeof(uint32_t));

	/* Create the patch file */
	if ((pf = fopen(argv[3], "wb")) == NULL) {
		printf("%s", argv[3]);
		return 1;
	}

	/* Write header (signature+newsize)*/
	offtout(total_parts, buf);
	if (fwrite("BPHD", 4, 1, pf) != 1 ||
		fwrite(buf, sizeof(buf), 1, pf) != 1 ||
		offtout(oldsize, buf) != 1 ||
		fwrite(buf, sizeof(buf), 1, pf) != 1 ||
		offtout(0x4F726F4E, buf) != 1 ||
		fwrite(buf, sizeof(buf), 1, pf) != 1 ||
		fwrite(header_buffer, total_parts * sizeof(uint32_t), 1, pf) != 1) {
		printf("Failed to write header");
		return 1;
	}

	char * tmp_file_name = (char*)malloc(strlen(argv[3]) + 20);
	if (tmp_file_name == NULL) {
		printf("Create tmp file failed");
		return 1;
	}

	strcpy(tmp_file_name, argv[3]);
	strcat(tmp_file_name, ".nor.tmp");
	tmp_file_name_len = strlen(tmp_file_name);
	req.temp_file_name = tmp_file_name;

	if (newsize > 0) {
		int32_t new_part_counter = 0;
		int32_t new_position = 0;
		int32_t partno = 0;
		int32_t old_position = oldsize;
		int32_t temp = 0;

		do {
			if (new_part_counter >= total_parts) {
				printf("Partno error");
				return 1;
			}
			req.partno = partno;
			sprintf(partno_string, "%d", (int)partno);
			strcpy(tmp_file_name + tmp_file_name_len, partno_string);
			req.flag = 0;

			if (new_position + 0x8000 >= newsize) {
				req.old = (uint8_t *)&old[new_position];
				if (old_position <= 0)
					req.oldsize = 0;
				else
					req.oldsize = oldsize - new_position;

				req.nnew = (uint8_t *)&nnew[new_position];
				req.newsize = newsize - new_position;
			}
			else {
				req.old = (uint8_t *)&old[new_position];
				if (old_position <= 0)
					temp = 0;
				else
					temp = oldsize - new_position;
				req.oldsize = temp;
				req.nnew = (uint8_t *)&nnew[new_position];
				req.newsize = 0x8000;
				if (temp >= 0x8000 && !memcmp(&nnew[new_position], &old[new_position], 0x8000)) {
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
			new_position += 0x8000;
			old_position -= 0x8000;
			header_buffer[new_part_counter++] = req.buffersize;

		} while (new_position < (int)newsize);		

		if (partno != total_parts) {
			printf("partno err");
			return 1;
		}

		int fd;
		if (((fd = open(argv[3], O_RDWR, 0)) < 0) ||
			(lseek(fd, 16, SEEK_SET) != 16) ||
			(write(fd, header_buffer, total_parts * sizeof(uint32_t))) != total_parts * sizeof(uint32_t) ||
			(close(fd) == -1)) {
			printf("Failed to write header");
			return 1;
		}
	}
	free(header_buffer);
	free(tmp_file_name);	
	fflush(pf);
	fclose(pf);
	return 0;
}

int main_reverse(char ** argv, uint8_t *old, off_t oldsize, uint8_t *nnew, off_t newsize) {
	struct bsdiff_request req = { 0 };
	char partno_string[11];
	int32_t total_parts = (newsize + 0x7FFF) / 0x8000;
	uint32_t * header_buffer = (uint32_t*)malloc(total_parts * sizeof(uint32_t));
	int32_t tmp_file_name_len = 0;
	FILE * pf;
	uint8_t buf[4];

	if (header_buffer == NULL) {
		printf("malloc failed");
		return 1;
	}
	memset(header_buffer, 0, total_parts * sizeof(uint32_t));

	/* Create the patch file */
	if ((pf = fopen(argv[3], "wb")) == NULL) {
		printf("%s", argv[3]);
		return 1;
	}

	/* Write header (signature+newsize)*/
	offtout(total_parts, buf);
	if (fwrite("BPHD", 4, 1, pf) != 1 ||
		fwrite(buf, sizeof(buf), 1, pf) != 1 ||
		offtout(oldsize, buf) != 1 ||
		fwrite(buf, sizeof(buf), 1, pf) != 1 ||
		offtout(0x4F737652, buf) != 1 ||
		fwrite(buf, sizeof(buf), 1, pf) != 1 ||
		fwrite(header_buffer, total_parts * sizeof(uint32_t), 1, pf) != 1) {
		printf("Failed to write header");
		return 1;
	}

	char * tmp_file_name = (char*)malloc(strlen(argv[3]) + 20);
	if (tmp_file_name == NULL) {
		printf("Create tmp file failed");
		return 1;
	}

	strcpy(tmp_file_name, argv[3]);
	strcat(tmp_file_name, ".rvs.tmp");
	tmp_file_name_len = strlen(tmp_file_name);
	req.temp_file_name = tmp_file_name;

	int32_t partno = total_parts - 1;
	int32_t old_position = oldsize;
	int32_t old_position2;

	if (partno >= 0) {
		int32_t newPartCounter = partno;
		old_position2 = old_position - (partno << 15);
		int32_t new_position = (partno << 15) + 0x8000;
		int32_t temp = 0;

		do {
			if (newPartCounter >= total_parts) {
				printf("Partno error");
				return 1;
			}
			req.partno = partno;
			sprintf(partno_string, "%d", (int)partno);
			
			strcpy(tmp_file_name + tmp_file_name_len, partno_string);
			req.flag = 0;

			if (new_position >= newsize) {
				req.old = (uint8_t *)&old[new_position];
				temp = 0;
				if (old_position > new_position)
					req.oldsize = new_position;
				else
					req.oldsize = 0;

				req.nnew = (uint8_t *)&nnew[new_position - 0x8000];
				req.newsize = newsize - new_position + 0x8000;
			}
			else {
				const uint8_t * temp_old = req.old;
				int32_t temp_new_position = new_position - 0x8000;
				temp = old_position;
				if (old_position2 > 0) temp_old = old;
				req.old = temp_old;
				if (old_position > new_position) temp = new_position;
				req.oldsize = temp;

				req.nnew = (uint8_t *)&nnew[temp_new_position];
				req.newsize = 0x8000;
				if (temp >= 0x8000 && !memcmp(&nnew[temp_new_position], temp_old, 0x8000)) {
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

			//++partno;
			new_position -= 0x8000;
			old_position2 += 0x8000;
			header_buffer[newPartCounter--] = req.buffersize;
			partno--;
		} while (partno >= 0);

		int fd;
		if (((fd = open(argv[3], O_RDWR, 0)) < 0) ||
			(lseek(fd, 16, SEEK_SET) != 16) ||
			(write(fd, header_buffer, total_parts * sizeof(uint32_t))) != total_parts * sizeof(uint32_t) ||
			(close(fd) == -1)) {
			printf("Failed to write header");
			return 1;
		}
	}
	free(header_buffer);
	free(tmp_file_name);
	fflush(pf);
	fclose(pf);
	return 0;
}


int main(int argc, char *argv[]) {
	int fd;
	uint8_t *old, *nnew;
	off_t oldsize, newsize;	

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
		(close(fd) == -1)) {
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
		if (old) free(old);
		if (nnew) free(nnew);
		printf("%s", argv[2]);
		return 1;
	}

	if (!access(argv[3], 0) && remove(argv[3])) {
		printf("Remove dest file failed");
		free(old);
		free(nnew);
		return 1;
	}

	if (main_normal(argv, old, oldsize, nnew, newsize)) {
		printf("main_normal err");
		free(old);
		free(nnew);
	}

	char * tmp_file_name_normal = (char*)malloc(strlen(argv[3]) + 20);	
	if (tmp_file_name_normal == NULL) {
		printf("Memory allocation failed");
		free(old);
		free(nnew);
		return 1;
	}
	char * tmp_file_name_reverse = (char*)malloc(strlen(argv[3]) + 20);
	if (tmp_file_name_reverse == NULL) {
		printf("Memory allocation failed");
		free(tmp_file_name_normal);
		free(old);
		free(nnew);
		return 1;
	}

	strcpy(tmp_file_name_normal, argv[3]);
	strcat(tmp_file_name_normal, ".nor.tmp");
	if (!access(tmp_file_name_normal, 0) && remove(tmp_file_name_normal)) {
		printf("Remove old normal file failed");
		free(tmp_file_name_normal);
		free(tmp_file_name_reverse);
		free(old);
		free(nnew);
		return 1;
	}
	if (rename(argv[3], tmp_file_name_normal) != 0) {
		printf("Rename normal file failed");
		free(tmp_file_name_normal);
		free(tmp_file_name_reverse);
		free(old);
		free(nnew);
		return 1;
	}


	if(main_reverse(argv, old, oldsize, nnew, newsize)) {
		printf("main_reverse err");
		free(old);
		free(nnew);
	}
	strcpy(tmp_file_name_reverse, argv[3]);
	strcat(tmp_file_name_reverse, ".rvs.tmp");
	if (!access(tmp_file_name_reverse, 0) && remove(tmp_file_name_reverse)) {
		printf("Remove old reverse file failed");
		free(tmp_file_name_normal);
		free(tmp_file_name_reverse);
		free(old);
		free(nnew);
		return 1;
	}
	if (rename(argv[3], tmp_file_name_reverse) != 0) {
		printf("Rename reverse file failed");
		free(tmp_file_name_normal);
		free(tmp_file_name_reverse);
		free(old);
		free(nnew);
		return 1;
	}
	free(old);
	free(nnew);

	if (((fd = open(tmp_file_name_normal , O_RDONLY, 0)) < 0) ||
		((oldsize = lseek(fd, 0, SEEK_END)) == -1) ||
		(close(fd) == -1)) {
		printf("Get normal file size failed"); 
		free(tmp_file_name_normal);
		free(tmp_file_name_reverse);
		return 1;
	}

	if (((fd = open(tmp_file_name_reverse, O_RDONLY, 0)) < 0) ||
		((newsize = lseek(fd, 0, SEEK_END)) == -1) ||
		(close(fd) == -1)) {
		printf("Get reverse file size failed");
		free(tmp_file_name_normal);
		free(tmp_file_name_reverse);
		return 1;
	}
	if (oldsize < newsize) {
		if (rename(tmp_file_name_normal, argv[3]) != 0) {
			printf("Rename normal file failed");
			free(tmp_file_name_normal);
			free(tmp_file_name_reverse);
			return 1;
		}
		if (!access(tmp_file_name_reverse, 0) && remove(tmp_file_name_reverse)) {
			printf("Remove reverse file failed");
			free(tmp_file_name_normal);
			free(tmp_file_name_reverse);
			return 1;
		}
	}
	else {
		if (rename(tmp_file_name_reverse, argv[3]) != 0) {
			printf("Rename reverse file failed");
			free(tmp_file_name_normal);
			free(tmp_file_name_reverse);
			return 1;
		}
		if (!access(tmp_file_name_normal, 0) && remove(tmp_file_name_normal)) {
			printf("Remove normal file failed");
			free(tmp_file_name_normal);
			free(tmp_file_name_reverse);
			return 1;
		}
	}
	return 0;
}

