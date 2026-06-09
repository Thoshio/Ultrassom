#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <ultrassonic.h>               // Biblioteca personalizada


int main(void)
{   
    ultrassonic_init();

    for(;;) {
        print_distance();
    }
    
    return 0;
}