#include "exe_servo_mosfet.h"
#include "include/clientTCP.h"

static int last_sent_bpm = -1;               // zuletzt gesendeter BPM (ganzzahlig)
static absolute_time_t last_send_time;       // Zeitstempel letzter Sendung


/* --- NEU: Staffelzeiten --- */
#define STAGGER_ON_DELAY_MS   800   // nach Finger-ON: Wartezeit bis MOSFET-Start
#define STAGGER_OFF_DELAY_MS  200   // nach MOSFET-OFF: Wartezeit bis Servo zentrieren
#define LAST_DELAY_MS  600  

// Zustände nur einmal anlegen (static = Dateibereich, bleiben über Aufrufe hinweg erhalten)
static uint32_t ir_buffer[MOVING_AVG_SIZE] = {0};
static uint8_t  ir_idx = 0, ir_count = 0;

static float bpm_buffer[BPM_AVG_SIZE] = {0};
static uint8_t bpm_idx = 0, bpm_count = 0;

static bool prev_above_avg = false;
static absolute_time_t last_peak_time;
static absolute_time_t last_sensor_read;

static bool actuation_enabled = false;
void servo_set_actuation_enabled(bool en) { actuation_enabled = en; }


float bpm_avg = 0;

static bool init_done = false;

/* --- NEU: Zustände für Staffelung --- */
static bool finger_on_prev          = false;   // vorheriger Fingerstatus
static bool mosfets_running         = false;   // laufen die MOSFETs bereits?
static absolute_time_t finger_on_since;        // Zeitpunkt Finger-ON (für verzögerten MOSFET-Start)

static bool awaiting_center_after_off = false; // warten wir aufs Zentrieren nach OFF?
static absolute_time_t mosfets_off_time;       // Zeitpunkt MOSFET-OFF (für verzögertes Zentrieren)

void servo_mosfet(Servo servos[], Mosfet mosfets[])
{
    if (!init_done) {
        last_peak_time    = get_absolute_time();
        last_sensor_read  = get_absolute_time();
        finger_on_since   = last_peak_time;
        mosfets_off_time  = last_peak_time;
        init_done = true;
        last_send_time = get_absolute_time();
    }

    // === Sensor regelmäßig lesen (alle ~20ms) ===
    if (absolute_time_diff_us(last_sensor_read, get_absolute_time()) / 1000 > 20) {
        last_sensor_read = get_absolute_time();

        if (max30102_fifo_available() > 0) {
            uint32_t ir = max30102_read_ir_sample();
            if (ir > 0) {
                // Moving Average über IR
                ir_buffer[ir_idx] = ir;
                ir_idx = (ir_idx + 1) % MOVING_AVG_SIZE;
                if (ir_count < MOVING_AVG_SIZE) ir_count++;

                uint64_t sum = 0;
                for (uint8_t i = 0; i < ir_count; i++) sum += ir_buffer[i];
                uint32_t ir_avg = (ir_count ? (sum / ir_count) : 0);

                bool finger_on = (ir_avg >= FINGER_ON_THRESHOLD);

                if (!finger_on) {
                    servo_set_actuation_enabled(false);
                    /* ------------------ FINGER OFF ------------------
                       1) MOSFETs sofort aus
                       2) kurze Wartezeit
                       3) dann Servos zentrieren
                    */
                    if (finger_on_prev) {
                        // nur beim Wechsel von ON -> OFF auslösen
                        for (int i = 0; i < MOSFET_COUNT; i++) mosfet_stop(mosfets, i);
                        mosfets_running = false;

                        mosfets_off_time = get_absolute_time();
                        awaiting_center_after_off = true;

                        // Peak-Detektion zurücksetzen
                        prev_above_avg = false;
                    }
                    /** 
                    absolute_time_t now = get_absolute_time();
                    int32_t ms = absolute_time_diff_us(last_send_time, now) / 1000;
                    if (ms >= 500 || last_sent_bpm != 0) {   // sofort 1x + dann alle 500ms
                        tcp_client_send_bpm(&client, 0);
                        last_sent_bpm  = 0;
                        last_send_time = now;
                    }*/

                    finger_on_prev = false;
                } else {
                    /* ------------------ FINGER ON -------------------
                       Servos zuerst (laufen sowieso via servo_tick/servo_set_bpm),
                       MOSFETs erst nach STAGGER_ON_DELAY_MS starten.
                    */
                    if (!finger_on_prev) {
                        // Rising edge: neuen Zyklus vorbereiten
                        finger_on_since = get_absolute_time();
                        mosfets_running = false;
                        awaiting_center_after_off = false; // evtl. off-Zentrierung abbrechen
                    }

                    if (actuation_enabled) {
                    // MOSFETs nach Verzögerung genau einmal starten
                    if (!mosfets_running) {
                        int32_t ms_on = absolute_time_diff_us(finger_on_since, get_absolute_time()) / 1000;
                        if (ms_on >= STAGGER_ON_DELAY_MS) {
                            // Erst die ersten zwei gleichzeitig
                            mosfet_start_oscillate_delayed(mosfets, 0, 5000, 500, 0);
                            mosfet_start_oscillate_delayed(mosfets, 1, 5000, 500, 0);

                            // Dann den dritten nach Delay
                            mosfet_start_oscillate_delayed(mosfets, 2, 5000, 500, LAST_DELAY_MS);
                            mosfets_running = true;
                        }
                    }
                }

                    // Peak-Detektion / BPM
                    bool curr_above_avg = ir > ir_avg;
                    if (curr_above_avg && !prev_above_avg) {
                        absolute_time_t now = get_absolute_time();
                        int32_t diff_ms = absolute_time_diff_us(last_peak_time, now) / 1000;
                        if (diff_ms > MIN_PEAK_INTERVAL_MS) {
                            float bpm_instant = 60000.0f / diff_ms;
                            if (bpm_instant >= MIN_VALID_BPM && bpm_instant <= MAX_VALID_BPM) {
                                bpm_buffer[bpm_idx] = bpm_instant;
                                bpm_idx = (bpm_idx + 1) % BPM_AVG_SIZE;
                                if (bpm_count < BPM_AVG_SIZE) bpm_count++;

                                float sum_bpm = 0;
                                for (uint8_t i = 0; i < bpm_count; i++) sum_bpm += bpm_buffer[i];
                                bpm_avg = sum_bpm / bpm_count;

                                printf("Herzfrequenz: %.1f BPM\n", bpm_avg);
                                // Setzt nur das Zieltempo; Schalten macht servo_tick()
                               // servo_set_bpm(servos, bpm_avg);
                               // nach servo_set_bpm(servos, bpm_avg);
                                int bpm_to_send = (int)(bpm_avg + 0.5f);
                                int32_t ms_since = absolute_time_diff_us(last_send_time, now) / 1000;
                                if (bpm_to_send != last_sent_bpm || ms_since >= 1000) {
                                    tcp_client_send_bpm(&client, bpm_to_send);
                                    last_sent_bpm  = bpm_to_send;
                                    last_send_time = now;
                                }

                            }
                            last_peak_time = now;
                        }
                    }
                    prev_above_avg = curr_above_avg;
                    finger_on_prev = true;
                }
            }
        }
    }

    /* --- NEU: verzögertes Zentrieren nach OFF (außerhalb des Sensor-Ifs,
       damit es zuverlässig passiert, auch wenn gerade keine Samples kommen) --- */
    if (awaiting_center_after_off) {
        int32_t ms_since_off = absolute_time_diff_us(mosfets_off_time, get_absolute_time()) / 1000;
        if (ms_since_off >= STAGGER_OFF_DELAY_MS) {
            servo_center_all(servos);
            awaiting_center_after_off = false;
        }
    }

    // MOSFET-State-Maschine stets updaten
    mosfet_update_all(mosfets);

    // Servos immer ticken lassen
    if(actuation_enabled)
    servo_tick(servos);
}
