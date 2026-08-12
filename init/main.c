#include <drivers/novium/console.h>


// just print hi for now 
void kernel_main(void) {
    console_clear();
    console_write("Hello World");
    while(1); 
}
