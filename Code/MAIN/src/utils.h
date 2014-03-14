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

#endif /* UTILS_H_ */
