#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <math.h>
#include <stdlib.h>

unsigned const char pwm[16] = {
	0, 1, 1, 2, 3, 4, 5, 8, 12, 16, 22, 32, 45, 64, 91, 127
};

unsigned char ee_red EEMEM;
unsigned char ee_green EEMEM;
unsigned char ee_yellow EEMEM;
unsigned char ee_opmode EEMEM;

enum {
	OM_STATIC = 0, OM_QFADE, OM_SFADE, OM_RED, OM_INVAL
} opmode;

unsigned char red, green, yellow;

static unsigned char load_brightness(unsigned char *ptr)
{
	unsigned char tmp = eeprom_read_byte(ptr);
	if (tmp >= 16)
		return 15;
	return tmp;
}

static void setup_mode(void)
{
	switch (opmode) {
		case OM_STATIC:
			red = load_brightness(&ee_red);
			yellow = load_brightness(&ee_yellow);
			green = load_brightness(&ee_green);
			break;
		case OM_RED:
			green = yellow = 0;
			break;
	}
}

static inline void handle_btn(void)
{
	static unsigned char n_red = 0, n_green = 0, n_yellow = 0;
	static unsigned char btn = 0, skip = 0;
	static unsigned char fade = 0;

	if (((opmode == OM_QFADE) && !(btn % 12))
			|| ((opmode == OM_SFADE) && !(fade % 10) && !btn)) {
		if (red < n_red)
			red++;
		else if (red > n_red)
			red--;
		if (green < n_green)
			green++;
		else if (green > n_green)
			green--;
		if (yellow < n_yellow)
			yellow++;
		else if (yellow > n_yellow)
			yellow--;
	}
	else if ((opmode == OM_RED) && !(btn % 12)) {
		if (n_red && (red < 15))
			red++;
		else if (!n_red && (red > 0))
			red--;
		else
			n_red = !n_red;
	}

	if (++btn == 200) {
		asm("wdr");
		btn = 0;

		if (opmode == OM_QFADE) {
			n_red = rand() % 16;
			n_green = rand() % 16;
			n_yellow = rand() % 16;
		}
		else if ((opmode == OM_SFADE) && (++fade == 160)) {
			n_red = rand() % 8;
			n_green = rand() % 16;
			n_yellow = rand() % 16;
			fade = 0;
		}

		if (skip)
			skip--;
		else {
			if ((~PINB & _BV(PB0)) && (~PINB & _BV(PB2))) {
				srand(red + green + yellow);

				opmode = (opmode + 1) % OM_INVAL;
				eeprom_write_byte(&ee_opmode, opmode);

				skip = 10;

				setup_mode();
			}
			else if (~PINB & _BV(PB0)) {
				green = (green + 1) % 16;
				if (green == 15)
					skip = 10;
				eeprom_write_byte(&ee_green, green);
			}
			else if (~PINB & _BV(PB1)) {
				yellow = (yellow + 1) % 16;
				if (yellow == 15)
					skip = 10;
				eeprom_write_byte(&ee_yellow, yellow);
			}
			else if (~PINB & _BV(PB2)) {
				red = (red + 1) % 16;
				if (red == 15)
					skip = 10;
				eeprom_write_byte(&ee_red, red);
			}
		}
	}
}

int main (void)
{
	unsigned char cur = 0;
	/* run at 8 MHz */
	CLKPR = _BV(CLKPCE);
	CLKPR = 0;

	/* watchdog reset after ~8 seconds */
	MCUSR = 0;
	WDTCSR = _BV(WDE) | _BV(WDP3) | _BV(WDP0);

	/* disable analog comparator to reduce power consumption */
	ACSR |= _BV(ACD);

	/* in / out */
	DDRD = _BV(DDD2) | _BV(DDD3) | _BV(DDD4) | _BV(DDD5);
	DDRB = 0;

	/* enable internal pull-ups */
	PORTB = _BV(PB0) | _BV(PB1) | _BV(PB2);

	/* PWM */
	OCR0A = 0xff;
	TCCR0A = _BV(COM0B0) | _BV(WGM01); /* OC0B, count from 0 to OCR0A */
	/* TCCR0B = _BV(CS01) | _BV(CS00); */ /* /64 prescaler */

	opmode = eeprom_read_byte(&ee_opmode);
	if (opmode >= OM_INVAL)
		opmode = OM_STATIC;

	setup_mode();

	while (1) {
		if (++cur == 0) {
			PORTD |= _BV(PD2) | _BV(PD3) | _BV(PD4);
			handle_btn();
		}
		if (cur == pwm[green])
			PORTD &= ~_BV(PD4);
		if (cur == pwm[yellow])
			PORTD &= ~_BV(PD3);
		if (cur == pwm[red])
			PORTD &= ~_BV(PD2);
	}

	return 0;
}
