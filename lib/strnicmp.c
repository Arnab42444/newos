#include <string.h>
#include <ctype.h>

int strnicmp(const char *s1, const char *s2, size_t len)
{
	unsigned char c1 = '\0';
	unsigned char c2 = '\0';

	if(len > 0) {
		do {
			c1 = *s1; c2 = *s2;
			s1++; s2++;
			if(!c1)
				break;
			if(!c2)
				break;
			if(c1 == c2)
				continue;
			c1 = tolower(c1);
			c2 = tolower(c2);
			if (c1 != c2)
				break;
		} while(--len);
	}
	return (int)c1 - (int)c2;
}
