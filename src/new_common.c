#include "new_common.h"

// returns amount of space left in buffer (0=overflow happened)
int strcat_safe(char *tg, const char *src, int tgMaxLen) {
	// keep space for 1 more char
	int curOfs = 1;

	// skip
	while(*tg != 0) {
		tg++;
		curOfs++;
		if(curOfs >= tgMaxLen - 1) {
			*tg = 0;
			return 0;
		}
	}
	// copy
	while(*src != 0) {
		*tg = *src;
		src++;
		tg++;
		curOfs++;
		if(curOfs >= tgMaxLen - 1) {
			*tg = 0;
			return 0;
		}
	}
	*tg = 0;
	return tgMaxLen-curOfs;
}


int strcpy_safe_checkForChanges(char *tg, const char *src, int tgMaxLen) {
	int changesFound = 0;
	// keep space for 1 more char
	int curOfs = 1;
	// copy
	while(*src != 0) {
		if(*tg != *src) {
			changesFound++;
			*tg = *src;
		}
		src++;
		tg++;
		curOfs++;
		if(curOfs >= tgMaxLen - 1) {
			if(*tg != 0) {
				changesFound++;
				*tg = 0;
			}
			return 0;
		}
	}
	if(*tg != 0) {
		changesFound++;
		*tg = 0;
	}
	return changesFound;
}
int strcpy_safe(char *tg, const char *src, int tgMaxLen) {
	// keep space for 1 more char
	int curOfs = 1;
	// copy
	while(*src != 0) {
		*tg = *src;
		src++;
		tg++;
		curOfs++;
		if(curOfs >= tgMaxLen - 1) {
			*tg = 0;
			return 0;
		}
	}
	*tg = 0;
	return tgMaxLen-curOfs;
}

void urldecode2_safe(char *dst, const char *srcin, int maxDstLen)
{
	int curLen = 1;
        int a = 0, b = 0;
	// avoid signing issues in conversion to int for isxdigit(int c)
	const unsigned char *src = (const unsigned char *)srcin;
        while (*src) {
		if(curLen >= (maxDstLen - 1)){
			break;
		}
                if ((*src == '%') &&
                    ((a = src[1]) && (b = src[2])) &&
                    (isxdigit(a) && isxdigit(b))) {
                        if (a >= 'a')
                                a -= 'a'-'A';
                        if (a >= 'A')
                                a -= ('A' - 10);
                        else
                                a -= '0';
                        if (b >= 'a')
                                b -= 'a'-'A';
                        if (b >= 'A')
                                b -= ('A' - 10);
                        else
                                b -= '0';
                        *dst++ = 16*a+b;
                        src+=3;
                } else if (*src == '+') {
                        *dst++ = ' ';
                        src++;
                } else {
                        *dst++ = *src++;
                }
		curLen++;
        }
        *dst++ = '\0';
}

