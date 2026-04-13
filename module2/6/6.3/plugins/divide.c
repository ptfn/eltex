const char* get_name(void) {
    return "Деление";
}

double execute(double a, double b) {
    if (b == 0) {
        return 0.0 / 0.0;  // NaN
    }
    return a / b;
}
