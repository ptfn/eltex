#include <math.h>

const char* get_name(void) {
    return "Максимум";
}

double execute(double a, double b) {
    return fmax(a, b);
}
