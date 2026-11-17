/*
 * sound.c
 *
 *  Created on: 2026. 10. 30.
 *      Author: kccistc
 */

/* include */
#include "sound_task.h"
#include "../motor_control_task/motor_control_task.h"
#include "../../../driver/buzzer/buzzer.h"
#include "tim.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"

/* define */
/* Frequencies in Hz; S denotes sharp (#). */
/* Octave 1 */
#define C1   33
#define CS1  35
#define D1   37
#define DS1  39
#define E1   41
#define F1   44
#define FS1  46
#define G1   49
#define GS1  52
#define A1   55
#define AS1  58
#define B1   62

/* Octave 2 */
#define C2   65
#define CS2  69
#define D2   73
#define DS2  78
#define E2   82
#define F2   87
#define FS2  92
#define G2   98
#define GS2  104
#define A2   110
#define AS2  117
#define B2   123

/* Octave 3 */
#define C3   131
#define CS3  139
#define D3   147
#define DS3  156
#define E3   165
#define F3   175
#define FS3  185
#define G3   196
#define GS3  208
#define A3   220
#define AS3  233
#define B3   247

/* Octave 4 */
#define C4   262
#define CS4  277
#define D4   294
#define DS4  311
#define E4   330
#define F4   349
#define FS4  370
#define G4   392
#define GS4  415
#define A4   440
#define AS4  466
#define B4   494

/* Octave 5 */
#define C5   523
#define CS5  554
#define D5   587
#define DS5  622
#define E5   659
#define F5   698
#define FS5  740
#define G5   784
#define GS5  831
#define A5   880
#define AS5  932
#define B5   988

/* Octave 6 */
#define C6   1047
#define CS6  1109
#define D6   1175
#define DS6  1245
#define E6   1319
#define F6   1397
#define FS6  1480
#define G6   1568
#define GS6  1661
#define A6   1760
#define AS6  1865
#define B6   1976

/* Octave 7 */
#define C7   2093
#define CS7  2217
#define D7   2349
#define DS7  2489
#define E7   2637
#define F7   2794
#define FS7  2960
#define G7   3136
#define GS7  3322
#define A7   3520
#define AS7  3729
#define B7   3951

/* Octave 8 */
#define C8   4186
#define CS8  4435
#define D8   4699
#define DS8  4978
#define E8   5274
#define F8   5588
#define FS8  5920
#define G8   6272
#define GS8  6645
#define A8   7040
#define AS8  7459
#define B8   7902

/* bpm */
#define BPM		125

/* typedef */

/* variable declarations */

/* variable definitions */
static uint32_t four;
static uint32_t two;
static uint32_t one;
static uint32_t eight;
static uint32_t sixteen;
static uint32_t handled_stop_sequence;
static uint32_t song_stop_sequence;

/* function declarations */
static void sound_get_motor_status(motor_status_t *status);
static bool sound_keep_playing(void);
static void sound_power(void);
static void sound_button(void);
static void sound_error(void);
static void sound_alarm(void);
static void sound_mario(void);

/* function definitions */
void sound_task_init(void){
	four = 60000 / BPM;
	two = four * 2;
	one = two * 2;
	eight = four / 2;
	sixteen = eight / 2;
	buzzer_init(&htim2, TIM_CHANNEL_2, &htim4, TIM_CHANNEL_2);
	sound_button();
}

void sound_task(void){
    motor_status_t status;
    sound_get_motor_status(&status);
    if (status.stop_sequence != handled_stop_sequence) {
        handled_stop_sequence = status.stop_sequence;
        buzzer1_off();
        buzzer2_off();
        sound_error();
    }
    else if (status.command.state == RUN) {
        song_stop_sequence = status.stop_sequence;
        sound_mario();
    }
}

static bool sound_keep_playing(void){
    motor_status_t status;
    sound_get_motor_status(&status);
    return status.command.state == RUN &&
           status.stop_sequence == song_stop_sequence;
}

static void sound_power(void){
	buzzer1_on(C6);
	osDelay(250);
	buzzer1_on(E6);
	osDelay(250);
	buzzer1_on(G6);
	osDelay(250);
	buzzer1_on(C7);
	osDelay(250);
	buzzer1_off();
//	buzzer1_on(E5);
//	osDelay(sixteen);
//	buzzer1_off();
//	osDelay(10);
//
//	buzzer1_on(E5);
//	osDelay(sixteen);
//
//	buzzer1_off();
//	osDelay(sixteen);
//
//	buzzer1_on(E5);
//	osDelay(sixteen);
//
//	buzzer1_off();
//	osDelay(sixteen);
//
//	buzzer1_on(C5);
//	osDelay(sixteen);
//	buzzer1_off();
//	osDelay(10);
//
//	buzzer1_on(E5);
//	osDelay(sixteen);
//
//	buzzer1_off();
//	osDelay(sixteen);
//
//	buzzer1_on(G5);
//	osDelay(sixteen);
//
//	buzzer1_off();
}

static void sound_button(void){
	buzzer1_on(G6);
	osDelay(40);
	buzzer1_on(C7);
	osDelay(60);
	buzzer1_off();
}

static void sound_error(void){
	buzzer1_on(C5);
	osDelay(150);
	buzzer1_off();
	osDelay(60);
	buzzer1_on(C5);
	osDelay(150);
	buzzer1_off();
}

static void sound_alarm(void){
	buzzer1_on(C7);
	osDelay(50);
	buzzer1_off();
	osDelay(50);
	buzzer1_on(C7);
	osDelay(50);
	buzzer1_off();
	osDelay(50);
	buzzer1_on(C7);
	osDelay(50);
	buzzer1_off();
	osDelay(50);
	buzzer1_on(C7);
	osDelay(50);
	buzzer1_off();
	osDelay(50);
}

/* smb_over (1).mid: 105 BPM, 96 ticks/quarter; about 84 seconds.
 * Buzzer 1: original main melody (track 1).
 * Buzzer 2: alto/countermelody (track 2, "Countermelody").
 * Both voices use one millisecond timeline; bass/percussion are omitted.
 */
static void sound_mario(void){
	typedef struct {
		uint16_t frequency;
		uint16_t duration_ms;
		uint16_t rest_ms;
	} mario_note_t;
	static const mario_note_t melody[] = {
		{ E5, 95, 48 }, { E5, 95, 191 }, { E5, 95, 190 }, { C5, 96, 47 },
		{ E5, 95, 191 }, { G5, 95, 1048 }, { C5, 95, 333 }, { G4, 96, 333 },
		{ E4, 95, 333 }, { A4, 96, 190 }, { B4, 95, 191 }, { AS4, 95, 48 },
		{ A4, 95, 190 }, { G4, 96, 95 }, { E5, 95, 95 }, { G5, 96, 95 },
		{ A5, 95, 191 }, { F5, 95, 47 }, { G5, 96, 190 }, { E5, 95, 191 },
		{ C5, 95, 48 }, { D5, 95, 48 }, { B4, 95, 333 }, { C5, 95, 334 },
		{ G4, 95, 333 }, { E4, 96, 333 }, { A4, 95, 191 }, { B4, 95, 190 },
		{ AS4, 96, 47 }, { A4, 95, 191 }, { G4, 95, 95 }, { E5, 96, 95 },
		{ G5, 95, 95 }, { A5, 96, 190 }, { F5, 95, 48 }, { G5, 95, 191 },
		{ E5, 95, 190 }, { C5, 96, 47 }, { D5, 95, 48 }, { B4, 95, 619 },
		{ G5, 96, 47 }, { FS5, 95, 48 }, { F5, 95, 48 }, { DS5, 95, 191 },
		{ E5, 95, 190 }, { GS4, 96, 47 }, { A4, 95, 48 }, { C5, 95, 191 },
		{ A4, 95, 48 }, { C5, 95, 47 }, { D5, 96, 333 }, { G5, 95, 48 },
		{ FS5, 95, 48 }, { F5, 95, 48 }, { DS5, 95, 190 }, { E5, 96, 190 },
		{ C6, 95, 191 }, { C6, 95, 48 }, { C6, 95, 762 }, { G5, 95, 48 },
		{ FS5, 95, 47 }, { F5, 96, 47 }, { DS5, 96, 190 }, { E5, 95, 191 },
		{ GS4, 95, 48 }, { A4, 95, 47 }, { C5, 96, 190 }, { A4, 95, 48 },
		{ C5, 95, 48 }, { D5, 95, 333 }, { DS5, 96, 333 }, { D5, 95, 334 },
		{ C5, 95, 1333 }, { G5, 95, 48 }, { FS5, 95, 48 }, { F5, 95, 48 },
		{ DS5, 95, 190 }, { E5, 96, 190 }, { GS4, 95, 48 }, { A4, 95, 48 },
		{ C5, 95, 191 }, { A4, 95, 47 }, { C5, 96, 47 }, { D5, 96, 333 },
		{ G5, 95, 48 }, { FS5, 95, 48 }, { F5, 95, 47 }, { DS5, 96, 190 },
		{ E5, 95, 191 }, { C6, 95, 191 }, { C6, 95, 47 }, { C6, 96, 762 },
		{ G5, 95, 47 }, { FS5, 96, 47 }, { F5, 95, 48 }, { DS5, 95, 191 },
		{ E5, 95, 191 }, { GS4, 95, 47 }, { A4, 96, 47 }, { C5, 95, 191 },
		{ A4, 95, 48 }, { C5, 95, 48 }, { D5, 95, 333 }, { DS5, 95, 334 },
		{ D5, 95, 333 }, { C5, 96, 1047 }, { C5, 95, 48 }, { C5, 95, 191 },
		{ C5, 95, 191 }, { C5, 95, 47 }, { D5, 96, 190 }, { E5, 95, 48 },
		{ C5, 95, 191 }, { A4, 95, 48 }, { G4, 95, 476 }, { C5, 95, 48 },
		{ C5, 95, 191 }, { C5, 95, 190 }, { C5, 95, 48 }, { D5, 95, 48 },
		{ E5, 95, 1191 }, { C5, 95, 48 }, { C5, 95, 190 }, { C5, 95, 191 },
		{ C5, 95, 48 }, { D5, 95, 191 }, { E5, 95, 47 }, { C5, 96, 190 },
		{ A4, 95, 48 }, { G4, 95, 476 }, { E5, 96, 47 }, { E5, 95, 191 },
		{ E5, 95, 191 }, { C5, 95, 48 }, { E5, 95, 190 }, { G5, 95, 1048 },
		{ C5, 95, 334 }, { G4, 95, 333 }, { E4, 95, 334 }, { A4, 95, 191 },
		{ B4, 95, 190 }, { AS4, 95, 48 }, { A4, 95, 191 }, { G4, 95, 95 },
		{ E5, 96, 95 }, { G5, 95, 95 }, { A5, 95, 191 }, { F5, 95, 48 },
		{ G5, 95, 191 }, { E5, 95, 190 }, { C5, 95, 48 }, { D5, 95, 48 },
		{ B4, 95, 334 }, { C5, 95, 333 }, { G4, 95, 334 }, { E4, 95, 333 },
		{ A4, 95, 191 }, { B4, 95, 191 }, { AS4, 95, 48 }, { A4, 95, 190 },
		{ G4, 95, 96 }, { E5, 95, 95 }, { G5, 95, 96 }, { A5, 95, 190 },
		{ F5, 96, 47 }, { G5, 95, 191 }, { E5, 95, 191 }, { C5, 95, 48 },
		{ D5, 95, 47 }, { B4, 96, 333 }, { E5, 95, 48 }, { C5, 95, 191 },
		{ G4, 95, 333 }, { GS4, 95, 191 }, { A4, 95, 48 }, { F5, 95, 190 },
		{ F5, 96, 47 }, { A4, 95, 477 }, { B4, 95, 95 }, { A5, 95, 96 },
		{ A5, 95, 95 }, { A5, 95, 96 }, { G5, 95, 95 }, { F5, 95, 96 },
		{ E5, 95, 47 }, { C5, 96, 190 }, { A4, 95, 48 }, { G4, 95, 476 },
		{ E5, 96, 47 }, { C5, 95, 191 }, { G4, 95, 334 }, { GS4, 95, 190 },
		{ A4, 95, 48 }, { F5, 95, 191 }, { F5, 95, 48 }, { A4, 95, 476 },
		{ B4, 95, 48 }, { F5, 95, 191 }, { F5, 95, 48 }, { F5, 95, 95 },
		{ E5, 95, 95 }, { D5, 96, 95 }, { C5, 95, 1048 }, { E5, 95, 48 },
		{ C5, 95, 190 }, { G4, 96, 333 }, { GS4, 95, 191 }, { A4, 95, 48 },
		{ F5, 95, 190 }, { F5, 95, 48 }, { A4, 95, 477 }, { B4, 95, 95 },
		{ A5, 95, 95 }, { A5, 96, 95 }, { A5, 95, 95 }, { G5, 96, 95 },
		{ F5, 95, 95 }, { E5, 96, 47 }, { C5, 95, 191 }, { A4, 95, 48 },
		{ G4, 95, 476 }, { E5, 95, 48 }, { C5, 95, 191 }, { G4, 95, 333 },
		{ GS4, 96, 190 }, { A4, 95, 48 }, { F5, 95, 191 }, { F5, 95, 48 },
		{ A4, 95, 476 }, { B4, 95, 48 }, { F5, 95, 191 }, { F5, 95, 47 },
		{ F5, 96, 95 }, { E5, 95, 95 }, { D5, 96, 95 }, { C5, 95, 1048 },
		{ C5, 95, 48 }, { C5, 95, 190 }, { C5, 95, 191 }, { C5, 95, 48 },
		{ D5, 95, 191 }, { E5, 95, 47 }, { C5, 96, 190 }, { A4, 95, 48 },
		{ G4, 95, 476 }, { C5, 96, 47 }, { C5, 95, 191 }, { C5, 95, 191 },
		{ C5, 95, 48 }, { D5, 95, 47 }, { E5, 96, 1190 }, { C5, 95, 48 },
		{ C5, 95, 191 }, { C5, 95, 190 }, { C5, 96, 47 }, { D5, 95, 191 },
		{ E5, 95, 48 }, { C5, 95, 190 }, { A4, 96, 47 }, { G4, 96, 476 },
		{ E5, 95, 48 }, { E5, 95, 190 }, { E5, 96, 190 }, { C5, 95, 48 },
		{ E5, 95, 191 }, { G5, 95, 1047 }, { E5, 96, 47 }, { C5, 96, 190 },
		{ G4, 95, 334 }, { GS4, 95, 190 }, { A4, 96, 47 }, { F5, 95, 191 },
		{ F5, 95, 48 }, { A4, 95, 476 }, { B4, 95, 96 }, { A5, 95, 95 },
		{ A5, 95, 96 }, { A5, 95, 95 }, { G5, 95, 96 }, { F5, 95, 95 },
		{ E5, 95, 48 }, { C5, 95, 191 }, { A4, 95, 47 }, { G4, 96, 476 },
		{ E5, 95, 48 }, { C5, 95, 190 }, { G4, 96, 333 }, { GS4, 95, 191 },
		{ A4, 95, 48 }, { F5, 95, 190 }, { F5, 96, 47 }, { A4, 95, 477 },
		{ B4, 95, 47 }, { F5, 96, 190 }, { F5, 95, 48 }, { F5, 95, 96 },
		{ E5, 95, 95 }, { D5, 95, 95 }, { C5, 96, 0 },
	};

	static const mario_note_t harmony[] = {
		{ FS4, 95, 48 }, { FS4, 95, 191 }, { FS4, 95, 190 }, { FS4, 96, 47 },
		{ FS4, 95, 191 }, { B4, 95, 476 }, { G4, 96, 476 }, { E4, 95, 333 },
		{ C4, 96, 333 }, { G3, 95, 333 }, { C4, 96, 190 }, { D4, 95, 191 },
		{ CS4, 95, 48 }, { C4, 95, 190 }, { C4, 96, 95 }, { G4, 95, 95 },
		{ B4, 96, 95 }, { C5, 95, 191 }, { A4, 95, 47 }, { B4, 96, 190 },
		{ A4, 95, 191 }, { E4, 95, 48 }, { F4, 95, 48 }, { D4, 95, 333 },
		{ E4, 95, 334 }, { C4, 95, 333 }, { G3, 96, 333 }, { C4, 95, 191 },
		{ D4, 95, 190 }, { CS4, 96, 47 }, { C4, 95, 191 }, { C4, 95, 95 },
		{ G4, 96, 95 }, { B4, 95, 95 }, { C5, 96, 190 }, { A4, 95, 48 },
		{ B4, 95, 191 }, { A4, 95, 190 }, { E4, 96, 47 }, { F4, 95, 48 },
		{ D4, 95, 619 }, { E5, 96, 47 }, { DS5, 95, 48 }, { D5, 95, 48 },
		{ B4, 95, 191 }, { C5, 95, 190 }, { E4, 96, 47 }, { F4, 95, 48 },
		{ G4, 95, 191 }, { C4, 95, 48 }, { E4, 95, 47 }, { F4, 96, 333 },
		{ E5, 95, 48 }, { DS5, 95, 48 }, { D5, 95, 48 }, { B4, 95, 190 },
		{ C5, 96, 190 }, { F5, 95, 191 }, { F5, 95, 48 }, { F5, 95, 762 },
		{ E5, 95, 48 }, { DS5, 95, 47 }, { D5, 96, 47 }, { B4, 96, 190 },
		{ C5, 95, 191 }, { E4, 95, 48 }, { F4, 95, 47 }, { G4, 96, 190 },
		{ C4, 95, 48 }, { E4, 95, 48 }, { F4, 95, 333 }, { GS4, 96, 333 },
		{ F4, 95, 334 }, { E4, 95, 1333 }, { E5, 95, 48 }, { DS5, 95, 48 },
		{ D5, 95, 48 }, { B4, 95, 190 }, { C5, 96, 190 }, { E4, 95, 48 },
		{ F4, 95, 48 }, { G4, 95, 191 }, { C4, 95, 47 }, { E4, 96, 47 },
		{ F4, 96, 333 }, { E5, 95, 48 }, { DS5, 95, 48 }, { D5, 95, 47 },
		{ B4, 96, 190 }, { C5, 95, 191 }, { F5, 95, 191 }, { F5, 95, 47 },
		{ F5, 96, 762 }, { E5, 95, 47 }, { DS5, 96, 47 }, { D5, 95, 48 },
		{ B4, 95, 191 }, { C5, 95, 191 }, { E4, 95, 47 }, { F4, 96, 47 },
		{ G4, 95, 191 }, { C4, 95, 48 }, { E4, 95, 48 }, { F4, 95, 333 },
		{ GS4, 95, 334 }, { F4, 95, 333 }, { E4, 96, 1047 }, { GS4, 95, 48 },
		{ GS4, 95, 191 }, { GS4, 95, 191 }, { GS4, 95, 47 }, { AS4, 96, 190 },
		{ G4, 95, 48 }, { E4, 95, 191 }, { E4, 95, 48 }, { C4, 95, 476 },
		{ GS4, 95, 48 }, { GS4, 95, 191 }, { GS4, 95, 190 }, { GS4, 95, 48 },
		{ AS4, 95, 48 }, { G4, 95, 1191 }, { GS4, 95, 48 }, { GS4, 95, 190 },
		{ GS4, 95, 191 }, { GS4, 95, 48 }, { AS4, 95, 191 }, { G4, 95, 47 },
		{ E4, 96, 190 }, { E4, 95, 48 }, { C4, 95, 476 }, { FS4, 96, 47 },
		{ FS4, 95, 191 }, { FS4, 95, 191 }, { FS4, 95, 48 }, { FS4, 95, 190 },
		{ B4, 95, 477 }, { G4, 95, 476 }, { E4, 95, 334 }, { C4, 95, 333 },
		{ G3, 95, 334 }, { C4, 95, 191 }, { D4, 95, 190 }, { CS4, 95, 48 },
		{ C4, 95, 191 }, { C4, 95, 95 }, { G4, 96, 95 }, { B4, 95, 95 },
		{ C5, 95, 191 }, { A4, 95, 48 }, { B4, 95, 191 }, { A4, 95, 190 },
		{ E4, 95, 48 }, { F4, 95, 48 }, { D4, 95, 334 }, { E4, 95, 333 },
		{ C4, 95, 334 }, { G3, 95, 333 }, { C4, 95, 191 }, { D4, 95, 191 },
		{ CS4, 95, 48 }, { C4, 95, 190 }, { C4, 95, 96 }, { G4, 95, 95 },
		{ B4, 95, 96 }, { C5, 95, 190 }, { A4, 96, 47 }, { B4, 95, 191 },
		{ A4, 95, 191 }, { E4, 95, 48 }, { F4, 95, 47 }, { D4, 96, 333 },
		{ C5, 95, 48 }, { A4, 95, 191 }, { E4, 95, 333 }, { E4, 95, 191 },
		{ F4, 95, 48 }, { C5, 95, 190 }, { C5, 96, 47 }, { F4, 95, 477 },
		{ G4, 95, 95 }, { F5, 95, 96 }, { F5, 95, 95 }, { F5, 95, 96 },
		{ E5, 95, 95 }, { D5, 95, 96 }, { C5, 95, 47 }, { A4, 96, 190 },
		{ F4, 95, 48 }, { E4, 95, 476 }, { C5, 96, 47 }, { A4, 95, 191 },
		{ E4, 95, 334 }, { E4, 95, 190 }, { F4, 95, 48 }, { C5, 95, 191 },
		{ C5, 95, 48 }, { F4, 95, 476 }, { G4, 95, 48 }, { D5, 95, 191 },
		{ D5, 95, 48 }, { D5, 95, 95 }, { C5, 95, 95 }, { B4, 96, 95 },
		{ G4, 95, 48 }, { E4, 95, 191 }, { E4, 95, 47 }, { C4, 96, 476 },
		{ C5, 95, 48 }, { A4, 95, 190 }, { E4, 96, 333 }, { E4, 95, 191 },
		{ F4, 95, 48 }, { C5, 95, 190 }, { C5, 95, 48 }, { F4, 95, 477 },
		{ G4, 95, 95 }, { F5, 95, 95 }, { F5, 96, 95 }, { F5, 95, 95 },
		{ E5, 96, 95 }, { D5, 95, 95 }, { C5, 96, 47 }, { A4, 95, 191 },
		{ F4, 95, 48 }, { E4, 95, 476 }, { C5, 95, 48 }, { A4, 95, 191 },
		{ E4, 95, 333 }, { E4, 96, 190 }, { F4, 95, 48 }, { C5, 95, 191 },
		{ C5, 95, 48 }, { F4, 95, 476 }, { G4, 95, 48 }, { D5, 95, 191 },
		{ D5, 95, 47 }, { D5, 96, 95 }, { C5, 95, 95 }, { B4, 96, 95 },
		{ G4, 95, 48 }, { E4, 95, 190 }, { E4, 96, 47 }, { C4, 95, 477 },
		{ GS4, 95, 48 }, { GS4, 95, 190 }, { GS4, 95, 191 }, { GS4, 95, 48 },
		{ AS4, 95, 191 }, { G4, 95, 47 }, { E4, 96, 190 }, { E4, 95, 48 },
		{ C4, 95, 476 }, { GS4, 96, 47 }, { GS4, 95, 191 }, { GS4, 95, 191 },
		{ GS4, 95, 48 }, { AS4, 95, 47 }, { G4, 96, 1190 }, { GS4, 95, 48 },
		{ GS4, 95, 191 }, { GS4, 95, 190 }, { GS4, 96, 47 }, { AS4, 95, 191 },
		{ G4, 95, 48 }, { E4, 95, 190 }, { E4, 96, 47 }, { C4, 96, 476 },
		{ FS4, 95, 48 }, { FS4, 95, 190 }, { FS4, 96, 190 }, { FS4, 95, 48 },
		{ FS4, 95, 191 }, { B4, 95, 476 }, { G4, 95, 476 }, { C5, 96, 47 },
		{ A4, 96, 190 }, { E4, 95, 334 }, { E4, 95, 190 }, { F4, 96, 47 },
		{ C5, 95, 191 }, { C5, 95, 48 }, { F4, 95, 476 }, { G4, 95, 96 },
		{ F5, 95, 95 }, { F5, 95, 96 }, { F5, 95, 95 }, { E5, 95, 96 },
		{ D5, 95, 95 }, { C5, 95, 48 }, { A4, 95, 191 }, { F4, 95, 47 },
		{ E4, 96, 476 }, { C5, 95, 48 }, { A4, 95, 190 }, { E4, 96, 333 },
		{ E4, 95, 191 }, { F4, 95, 48 }, { C5, 95, 190 }, { C5, 96, 47 },
		{ F4, 95, 477 }, { G4, 95, 47 }, { D5, 96, 190 }, { D5, 95, 48 },
		{ D5, 95, 96 }, { C5, 95, 95 }, { B4, 95, 95 }, { G4, 96, 47 },
		{ E4, 96, 190 }, { E4, 95, 48 }, { C4, 95, 0 },
	};

	const mario_note_t *voices[2] = { melody, harmony };
	const uint32_t counts[2] = {
		sizeof(melody) / sizeof(melody[0]),
		sizeof(harmony) / sizeof(harmony[0])
	};
	uint32_t indices[2] = { 0, 0 };
	uint32_t next_ms[2] = { 0, 0 };
	bool note_on[2] = { false, false };
	uint32_t elapsed_ms = 0;
	TickType_t start_tick = xTaskGetTickCount();

	buzzer1_off();
	buzzer2_off();
	for(;;){
		if (!sound_keep_playing()) break;
		for(uint32_t voice = 0; voice < 2; voice++){
			/* Process both edges, including a zero-length rest at this time. */
			while(indices[voice] < counts[voice] && next_ms[voice] == elapsed_ms){
				const mario_note_t *note = &voices[voice][indices[voice]];
				if(note_on[voice]){
					if(voice == 0) buzzer1_off();
					else buzzer2_off();
					note_on[voice] = false;
					next_ms[voice] += note->rest_ms;
					indices[voice]++;
				}
				else{
					if(voice == 0) buzzer1_on(note->frequency);
					else buzzer2_on(note->frequency);
					note_on[voice] = true;
					next_ms[voice] += note->duration_ms;
				}
			}
		}

		uint32_t next_event_ms = UINT32_MAX;
		for(uint32_t voice = 0; voice < 2; voice++){
			if(indices[voice] < counts[voice] && next_ms[voice] < next_event_ms){
				next_event_ms = next_ms[voice];
			}
		}
		if(next_event_ms == UINT32_MAX) break;

		/* Wait relative to the song start so processing time does not accumulate. */
		TickType_t target_ticks = pdMS_TO_TICKS(next_event_ms);
		for (;;) {
			if (!sound_keep_playing()) {
				buzzer1_off();
				buzzer2_off();
				return;
			}
			TickType_t elapsed_ticks = xTaskGetTickCount() - start_tick;
			if (elapsed_ticks >= target_ticks) break;
			TickType_t wait = target_ticks - elapsed_ticks;
			TickType_t slice = pdMS_TO_TICKS(10);
			vTaskDelay(wait > slice ? slice : wait);
		}
		elapsed_ms = next_event_ms;
	}
	buzzer1_off();
	buzzer2_off();
}

static void sound_get_motor_status(motor_status_t *status) {
	taskENTER_CRITICAL();
	*status = applied;
	taskEXIT_CRITICAL();
}
