/**
 * @file:   utils.h
 * @brief:  Utility functions header file
 * @author: Nathan Woltman
 * @date:   2014/03/13
 */
 
#ifndef UTILS_H_
#define UTILS_H_

/** 
 * Parses the char, interpreting its content as an integral number, which is returned as a value of type int.
 *
 * @param {char} c
 * @returns {int} The character integer as an integer type.
 */
int ctoi(char c);

/** 
 * Returns the single-character representation of an integer.
 *
 * @param {int} i
 * @returns {char} The character representation of the integer.
 * PRE: 0 <= i <= 9
 */
char itoc(int i);

/**
 * Determines if the rest of the string, starting from index n,
 * is made up of only whitespace (or non-printing) characters.
 *
 * @param {char*} s - The string to check for non-whitespace characters.
 * @param {int} n - The index of the string at which to start the search.
 * @returns {int} 1 if there is only whitespace characters starting at index n to the end of the string; else 0.
 */
int hasWhiteSpaceToEnd(char* s, int n);

/**
 * Determines the length of an integer
 * @param {int} n - The integer being passed in.
 * @returns {int} length of the integer n.
 */
int intLength(int n);

/**
 * Converts an integer to a null-terminated string and stores the result in the array given by the str parameter.
 * NOTE: Assumes the integer is base 10.
 *
 * @param {int} n - Integer to be converted to a string.
 * @param {char*} str - Array in memory where to store the resulting null-terminated string.
 * @returns {char*} A pointer to the resulting null-terminated string, same as parameter str.
 */
char* itoa (int n, char* str);

#endif /* UTILS_H_ */
