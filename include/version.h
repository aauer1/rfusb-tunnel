/*
 * version.h
 *
 *  Created on: Feb 3, 2015
 *      Author: andreas
 */

#ifndef VERSION_TEMPLATE_
#define VERSION_TEMPLATE_

#define MAJOR_VERSION           3
#define MINOR_VERSION           5

#define STRINGIZE(s)    STR(s)
#define STR(s)          #s

#define VERSION         STRINGIZE(MAJOR_VERSION) "." STRINGIZE(MINOR_VERSION)

extern const char *BUILD_VERSION;

#endif /* VERSION_TEMPLATE_ */
