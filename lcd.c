#define F_CPU 8000000UL  // 8 MHz
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/signal.h>

#define BAUD 9600 //predkosc zlacza
#define BAUDRATE ((F_CPU)/(BAUD*16UL)-1)



void delay_ms(int ms)
	{
	volatile long unsigned int i;
	for(i=0;i<ms;i++)
		_delay_ms(1);
	}

void delay_us(int us)
	{
	volatile long unsigned int i;
	for(i=0;i<us;i++)
		_delay_us(1);
	}


#define RS 0
#define RW 1
#define E  2

void LCD2x16_init(void)
{
PORTA &= ~(1<<RS);
PORTA &= ~(1<<RW);

PORTA |= (1<<E);
PORTC = 0x38;   // dwie linie, 5x7 punktow
PORTA &=~(1<<E);
_delay_us(120);

PORTA |= (1<<E);
PORTC = 0x0e;   // wlacz wyswietlacz, kursor, miganie
PORTA &=~(1<<E);
_delay_us(120);

PORTA |= (1<<E);
PORTC = 0x06;
PORTA &=~(1<<E);
_delay_us(120);
}

void LCD2x16_clear(void){
PORTA &= ~(1<<RS);
PORTA &= ~(1<<RW);

PORTA |= (1<<E);
PORTC = 0x01;
PORTA &=~(1<<E);
delay_ms(120);
}

void LCD2x16_putchar(int data)
{
PORTA |= (1<<RS);
PORTA &= ~(1<<RW);

PORTA |= (1<<E);
PORTC = data;
PORTA &=~(1<<E);
_delay_us(120);
}

void LCD2x16_pos(int wiersz, int kolumna)
{
PORTA &= ~(1<<RS);
PORTA &= ~(1<<RW);

PORTA |= (1<<E);
delay_ms(1);
PORTC = 0x80+(wiersz-1)*0x40+(kolumna-1);
delay_ms(1);
PORTA &=~(1<<E);
_delay_us(120);
}



void uart_init(void)
{
	UBRRH = (BAUDRATE >> 8);
	UBRRL =  BAUDRATE;
	UCSRB = (1<< RXCIE)|(1<<TXEN)|(1<<RXEN);//|(1<<RXC); //wlacz nadajnik i odbiornik oraz przerwania
	UCSRC = (1<<URSEL)|(1<<UCSZ0)|(1<<UCSZ1);
	sei();
}


void nadaj(char znak)
{
	while(!(UCSRA&(1<<UDRE))); //czeka na wyslanie poprzednej danej
	UDR=znak;
}

uint8_t odbierz(void)
{
	while(!(UCSRA&(1<<RXC))); //dopoki nie ma czystego buforu
	return UDR; //zwroc wartosc
}

volatile unsigned char znak='a';

//funkcja skladajaca ramke z elementow
char* zrob_ramke(char *start, char *adres, char *funkcja, char *dane, char *lrc, char *koniec)
{

	int i = strlen(dane)+12;
	int x = 0;
	char *ramka;

	ramka = (char*)calloc(i, sizeof(*ramka));

	ramka[0] = start[0];
	ramka[1] = adres[0];
	ramka[2] = adres[1];
	ramka[3] = funkcja[0];
	ramka[4] = funkcja[1];

	for (x = 0; x<strlen(dane); x++) {
		ramka[x + 5] = dane[x];
	}
	
	ramka[x + 5] = lrc[0];
	ramka[x + 6] = lrc[1];
	ramka[x + 7] = lrc[2];
	ramka[x + 8] = lrc[3];
	ramka[x + 9] = koniec[0];
	ramka[x + 10] = koniec[1];
	
	return ramka;
	free(ramka);
}
//funkcja licz¹ca LRC CRC
int zrob_lrc(char *dane){
	int i;
	char hex[2];
	int length = strlen(dane);
	int lrc = 0xFFFF;

	while (length--) {
		lrc ^= *dane++;
		for (i = 0; i<8; i++) {
			if (lrc & 0x01) { /*LSB(bit 0) = 1 */
				lrc = (lrc >> 1) ^ 0xA001;
			}
			else {
				lrc = (lrc >> 1);
			}
		}
	}

	return lrc;
}

int main(void){

DDRC = 0xff;
PORTC = 0x00;
DDRA = 0xff;
PORTA = 0x00;
DDRD=0xFF;
PORTD=0x03;

_delay_ms(200);

uart_init();
LCD2x16_init();
LCD2x16_clear();
sei(); //wlaczenie przerwan
int ms=100;

char *start = ":";
char *adres = "12";
char *funkcja = "03";
char *dane = "omg wtf lol";
char crc[2];
char *koniec = ":c";
int lrc;

lrc = zrob_lrc(dane);
sprintf(crc, "%x", lrc);
char *ramka = zrob_ramke(start, adres, funkcja, dane, crc, koniec);

int y;
//nadaj(start);
for(y=0;y<1;y++)
{
nadaj(start);
}

return 0;
}


SIGNAL(SIG_UART_RECV)  //funkcja obslugujaca przerwanie
{
znak = odbierz();
LCD2x16_putchar(znak);
}
