#include <avr/io.h> 
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////
/*							    DEFINICJE								*/
//////////////////////////////////////////////////////////////////////////

//button
#define BUTTON								(!(PIND & (1 << PD2)))

//led
//#define LED_A(x)							(x == true ? (PORTC |= (1 <<PC2) : (PORTC &= ~(1 <<PC2)))
//#define LED_B(x)							(x == true ? (PORTC |= (1 <<PC3) : (PORTC &= ~(1 <<PC3)))

//////////////////////////////////////////////////////////////////////////
/*								  ENUM									*/
//////////////////////////////////////////////////////////////////////////

enum GateState
{
	inactive,
	active,
	start
};

enum LedState
{
	led_on,
	led_off
};

//////////////////////////////////////////////////////////////////////////
/*								   KLASY								*/
//////////////////////////////////////////////////////////////////////////

class TimeCounter
{
	private:
	
		volatile uint64_t time;
	
	public:
	
		TimeCounter();
	
		void increment() volatile;
		void reset() volatile;
	
		uint64_t getTime() volatile;
};

class Time
{
	public:
		uint8_t min;
		uint8_t s;
	
		uint16_t ms;
};

class Timer
{
	private:
	
		uint64_t begin_time;
		
		uint32_t time;
	
	public:
	
		Timer();
	
		void start();
		void stop();
		
		Time getTime();
};

class Led
{
	private:
		uint8_t pin;
		uint8_t* reg;
		
		LedState state;
		
	public:
		Led();
		
		void setup(volatile uint8_t &reg_buff, uint8_t pin_buff);
		
		void on();
		void off();
		void switchState();
};

class Pin
{
	private:
		uint8_t pin;
		uint8_t* reg;
		
	public:
		Pin();
		
		void setup(volatile uint8_t &reg_buff, uint8_t pin_buff);
		
		bool readState();
};

//////////////////////////////////////////////////////////////////////////
/*								 FUNKCJE								*/
//////////////////////////////////////////////////////////////////////////

bool uartReady();

uint8_t uartReciveByte();

void uartSendByte(uint8_t text);

//////////////////////////////////////////////////////////////////////////
/*								  ZMIENNE								*/
//////////////////////////////////////////////////////////////////////////

char* text;

volatile uint8_t recived_byte = 0x00;

uint8_t byte_buff;

uint32_t buff;
const uint32_t laser_booting_time = 10;
const uint32_t check_timeout = 100;

volatile TimeCounter time_counter;

Timer timer;

Led laser_1;
Led laser_2;
Led led_A;
Led led_B;

Pin photo_transistor_1;
Pin photo_transistor_2;
Pin button;

GateState gate_state = inactive;

//////////////////////////////////////////////////////////////////////////
/*								   MAIN									*/
//////////////////////////////////////////////////////////////////////////

int main(void)
{
	byte_buff = 0;
	
	//ports set up
	DDRB = 0x00;
	DDRC = (1 << PC2) | (1 << PC3);
	DDRD = (1 << PD1) | (1 << PD3) | (1 << PD4);
	
	//timer set up
	TCCR2 = (1 << WGM21) | (1 << COM21) | (1 << CS22) | (1 << CS20);					//CTC MODE | Clear timer on Compare Match | clock / 128
	OCR2 = 124;																			//
	
	//uart set up																		//
	UCSRB |= (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);									//
	UBRRL = 3;																			//baud rate 250k (error 0.0% lol f_osc = 16.0000MHz)
	
	//interrupts
	//TIMSK |= (1 << OCIE2);															//timer2 interrupt enable
	sei();																				//global interrupts enable
	
	laser_1.setup(PORTD, 4);
	laser_2.setup(PORTD, 3);
	
	led_A.setup(PORTC, 2);																//error lol
	led_B.setup(PORTC, 3);
	
	photo_transistor_1.setup(PINC, 1);
	photo_transistor_2.setup(PINC, 0);
	
	led_A.off();
	led_B.off();
	
	button.setup(PIND, 2);
	
    while (1) 
    {
//  		laser_1.on();
//  		laser_2.on();
// 		if(photo_transistor_1.readState()) led_A.on();
// 		else led_A.off();
// 		if (photo_transistor_2.readState()) led_B.on();
// 		else led_B.off();
		
		//checking stuff
		if(recived_byte == 'm') 
		{
			laser_1.switchState();
			recived_byte = 0;
		}
		if(recived_byte == 'n') 
		{
			laser_2.switchState();
			recived_byte = 0;
		}
		if(recived_byte == 'c')
		{
			laser_1.on();
			buff = 0;
			while (buff < check_timeout)
			{
				if(!photo_transistor_1.readState())
				{
					uartSendByte('1');
					break;
				}
				else buff++;
			}
			if(buff >= check_timeout) uartSendByte('0');
			laser_1.off();
			
			laser_2.on();
			buff = 0;
			while (buff < check_timeout)
			{
				if(!photo_transistor_2.readState())
				{
					uartSendByte('1');
					break;
				}
				else buff++;
			}
			if(buff >= check_timeout) uartSendByte('0');
			laser_2.off();
			
			recived_byte = 0;
		}
		if(recived_byte == 'r')
		{
			if(gate_state != inactive)
			{
				laser_1.on();
				laser_2.off();
				gate_state = active;
			}
			else
			{
				gate_state = active;
				laser_1.on();
				_delay_ms(laser_booting_time);			// give it some time to switch
			}
			recived_byte = 0;
		}
		if((gate_state == active) && photo_transistor_1.readState())
		{
			gate_state = start;
			uartSendByte('s');		// send info to computer 's'tart measurement
			laser_1.off();			// give it some time to switch
			laser_2.on();			// same here
			_delay_ms(laser_booting_time);
		}
		if((gate_state == start) && photo_transistor_2.readState())
		{
			gate_state = inactive;
			uartSendByte('f');		// send info to computer 'f'inish measurement
			laser_2.off();
			_delay_ms(laser_booting_time);
		}
    }
}

//////////////////////////////////////////////////////////////////////////
/*						      DEFINICJE KLAS							*/
//////////////////////////////////////////////////////////////////////////

TimeCounter::TimeCounter()
{
	this->time = 0;
}

void TimeCounter::increment() volatile
{
	this->time++;
}

void TimeCounter::reset() volatile
{
	this->time = 0;
}

uint64_t TimeCounter::getTime() volatile
{
	return this->time;
}

Timer::Timer()
{
	this->begin_time = 0;
	this->time = 0;
}

void Timer::start()
{
	this->begin_time = time_counter.getTime();
}

void Timer::stop()
{
	this->time = uint32_t(time_counter.getTime() - this->begin_time);
}

Time Timer::getTime()
{	
	Time buff;
	
	buff.ms = 0;
	buff.s = 0;
	buff.min = 0;
	
	buff.ms = this->time % 1000;
	buff.s = (this->time / 1000) % 60;
	buff.min = (this->time / 60000);
	
	return buff;
}

Led::Led()
{
	this->reg = 0;
	this->pin = 0;
}

void Led::setup(volatile uint8_t &reg_buff, uint8_t pin_buff)
{
	this->reg = (uint8_t*)&reg_buff;
	this->pin = pin_buff;
	state = led_off;
}

void Led::on()
{
	*reg |= (1 << pin);
	state = led_on;
}
void Led::off()
{
	*reg &= ~(1 << pin);
	state = led_off;
}

void Led::switchState()
{
	if(state == led_on) this->off();
	else if(state == led_off) this->on();
}

Pin::Pin()
{
	this->reg = 0;
	this->pin = 0;
}

void Pin::setup(volatile uint8_t &reg_buff, uint8_t pin_buff)
{
	this->reg = (uint8_t*)&reg_buff;
	this->pin = pin_buff;
}

bool Pin::readState()
{
	return (*reg & (1 << pin));
}

//////////////////////////////////////////////////////////////////////////
/*					        DEFINICJE  FUNKCJI							*/
//////////////////////////////////////////////////////////////////////////

bool uartReady()
{
	if(UCSRA & RXC) return true;
	else return false;
}

uint8_t uartReciveByte()
{
	return (uint8_t)UDR;
}

void uartSendByte(uint8_t text)
{
	while(!(UCSRA & (1 << UDRE)));
	UDR = text;
}

//////////////////////////////////////////////////////////////////////////
/*						        INTERRUPTS								*/
//////////////////////////////////////////////////////////////////////////

ISR(TIMER2_COMP_vect)
{
	time_counter.increment();
}

ISR(USART_RXC_vect)
{
	recived_byte = uartReciveByte();
}