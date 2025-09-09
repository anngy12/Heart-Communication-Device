int main(){
    stdio_init_all();

    void max30102_init(void);

    void servo_init_and_default();

    Mosfet mosfets[MOSFET_COUNT];
    mosfet_init_all(mosfets);

    while (true) {
        servo_mosfet(servos, mosfets);
        sleep_ms(5);
    }
}