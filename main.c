#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <math.h>
#include <stdlib.h>

#define BRIGHTNESS_MAX 40

volatile unsigned int int_en = 0;

inline void set_led(char red, char green, char blue)
{
	PORTD = (red << PD4) | (blue << PD5) | (green << PD6);
}

int main (void)
{
	/* in / out */
	DDRD = _BV(DDD2) | _BV(DDD3) | _BV(DDD4) | _BV(DDD5);
	DDRB = 0;

	/* enable internal pull-ups */
	PORTB = _BV(PB0) | _BV(PB1) | _BV(PB2);

	cli();

	/* enable pin change interrupts */
	PCMSK = _BV(PCINT0) | _BV(PCINT1) | _BV(PCINT2);
	GIMSK = _BV(PCIE);

	/* PWM */
	OCR0A = 0xff;
	TCCR0A = _BV(COM0B0) | _BV(WGM01);
	TCCR0B = _BV(CS01) | _BV(CS00); /* /64 */

	PORTD |= _BV(PD4);

	sei();
	while (1) {
		if (++int_en == 0) {
			sei();
			PIND = _BV(PD2) | _BV(PD3) | _BV(PD4);
			OCR0A++;
		}
	}

	return 0;
}

ISR(PCINT_vect)
{
	cli();
	if (PIND & _BV(PD4))
		PORTD &= ~_BV(PD4);
	else
		PORTD |= _BV(PD4);
	int_en = 0;
}
