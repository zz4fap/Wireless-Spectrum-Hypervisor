/*
 * utils.h
 *
 *  Created on: 01 Jun 2017
 *      Author: rubenmennes
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>
#include <logging/LogLevel.h>

uint64_t clock_get_time_ns();
void setup_signal_catching(void (*handler)(int));
char* strtolower(char *str);
loglevel parse_log_level(char* levelstring);

#endif /* UTILS_H_ */
