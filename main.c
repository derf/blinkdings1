#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <math.h>
#include <stdlib.h>

#define BRIGHTNESS_MAX 40

volatile unsigned char btn = 0;
volatile unsigned char red, green, yellow;

unsigned char pwm[16] = {
	0, 1, 1, 2, 3, 4, 5, 8, 12, 16, 22, 32, 45, 64, 91, 127
};

inline void set_led(char red, char green, char blue)
{
	PORTD = (red << PD4) | (blue << PD5) | (green << PD6);
}

int main (void)
{

	unsigned char cur = 0;

	red = 1;
	yellow = 15;
	green = 15;

	/* in / out */
	DDRD = _BV(DDD2) | _BV(DDD3) | _BV(DDD4) | _BV(DDD5);
	DDRB = 0;

	/* enable internal pull-ups */
	PORTB = _BV(PB0) | _BV(PB1) | _BV(PB2);

	/* PWM */
	OCR0A = 0xff;
	TCCR0A = _BV(COM0B0) | _BV(WGM01); /* OC0B, count from 0 to OCR0A */
	/* TCCR0B = _BV(CS01) | _BV(CS00); */ /* /64 prescaler */

	PORTD |= _BV(PD4);

	while (1) {
		cur++;

		if (cur == 0) {
			PORTD |= _BV(PD2) | _BV(PD3) | _BV(PD4);
			if (++btn == 16) {
				btn = 0;
				if (~PINB & _BV(PB0)) {
					green = (green + 1) % 16;
					if (green == 15)
						btn = 128;
				}
				if (~PINB & _BV(PB1)) {
					yellow = (yellow + 1) % 16;
					if (yellow == 15)
						btn = 128;
				}
				if (~PINB & _BV(PB2)) {
					red = (red + 1) % 16;
					if (red == 15)
						btn = 128;
				}
			}
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
