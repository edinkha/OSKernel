/**
 * @file:   utils.c
 * @brief:  Utility functions
 * @author: Nathan Woltman
 * @date:   2014/03/13
 */

 #include <limits.h>
 #include <math.h>
 #include <stdlib.h>

int ctoi(char c)
{
	return c - '0';
}

char itoc(int i)
{
	return '0' + i;
}

int hasWhiteSpaceToEnd(char* s, int n)
{
	s += n;
	while (*s != '\0') {
		if (*s > ' ') {
			return 0;
		}
		s++;
	}
	return 1;
}

char* itoa (int n, char* str)
{
	if (n == 0) {
		str[0] = '0';
		str[1] = '\0';
	}
	else {
		int i = 0;
		
		// Handle the case where n is negative
		if (n < 0) {
			str[0] = '-';
			i = 1;
		}
		
		// Add the number of digits in n to i (handle special case when n == INT_MIN)
		i += log10(n == INT_MIN ? INT_MAX : abs(n)) + 1;
		
		// Build the string
		str[i] = '\0';
		while (n) {
			str[--i] = itoc(abs(n % 10));
			n /= 10;
		}
	}
	
	return str;
}
