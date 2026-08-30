#ifndef NOVIUM_INPUT_H
#define NOVIUM_INPUT_H

#include <novium/types.h>

/* keyboard - raw scancode interface. */
void keyboard_init(void);      
int  keyboard_pop(void); 
int keyboard_getchar(void);      

#endif
