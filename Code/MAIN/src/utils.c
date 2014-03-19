/**
 * @file:   utils.c
 * @brief:  Utility functions
 * @author: Nathan Woltman
 * @date:   2014/03/13
 */

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
