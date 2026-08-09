#include "drivers/novium/console.h"


/* Just print hi for now */
void kernel_main(void) {
    console_clear();
    console_write("hi");
    while(1); 
}
