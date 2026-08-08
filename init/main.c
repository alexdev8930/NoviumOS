#include "drivers/novium/console.h"

void kernel_main(void) {
    console_clear();
    console_write("hi");
    while(1); 
}
