// Élison Natanael M. de Araujo
#define F_CPU 16000000UL
#define  BAUD 9600
#define MYUBRR  F_CPU/16/BAUD-1
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
//#include "nokia5110.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/eeprom.h>
#include "SSD1306.h"
#include "Font5x8.h"



unsigned char saida[16];
char matriz[2];
uint32_t botao = 50, Rpm = 0, distancia = 0, velM = 0, tempo= 0,aux=0,t_eeprom=0, distancia_auxiliar =0,ler_ADC=0,sinal_pwm =0,bordaS, tmpD, sinal_pwm_bat=0,bat=0,sinal_pwm_Tp =0,Tp=0,Pino=0,sirene=0;



ISR(INT0_vect){ //interrupção externa INT0, quando o botão é pressionado o diamentro aumenta

	botao++;

	if ( botao> 65){
		
		botao =65;
	}
	eeprom_write_byte(0,botao);
	

}

ISR(INT1_vect){ //interrupção externa INT1, quando o botão é pressionado o diamentro diminui



	botao-- ;
	eeprom_write_byte(0,botao);
	
}

ISR(TIMER0_COMPA_vect)
{
	static float aux_novo;
	
	tempo++;
	
	if((tempo % 1000) == 0)
	{
		aux_novo = (aux/2);
		
		Rpm = (aux_novo*60);
		
		aux = 0;
		
		distancia_auxiliar++;
	}
}
ISR(PCINT2_vect)
{
	aux++;
	
	//if(PIND&0b01000000==0){ //PD6
	//MARCHA = 'D';
	//}
	//else{
	//MARCHA='R';
	//}
	//if(PIND&0b10000000==0){//PD7
	//	MARCHA='P';
	//	}
}
ISR(ADC_vect)
{
	
	
	if (Pino==0){
		ler_ADC = ADC;
		
		if (ADC==1023)
		{
			sinal_pwm = 255;
		}
		else
		{
			sinal_pwm = (ADC/4);
		}
		
		if(tmpD <= 299 && velM > 19){
			
			
			sinal_pwm=100;
			
		}
	}
	if(Pino==1){ // Pino da bateria
		
		if (ADC==1023)
		{
			sinal_pwm_bat = 255;
			bat=100; //100% da bateria
		}
		else
		{
			sinal_pwm_bat= (ADC/4);
			
			bat=(sinal_pwm_bat*0.39);
		}
	    if (bat<30)
	    {
			PORTC |= 0b01000000;
	    } 
	    else
	    {
			PORTC &= 0b10111111;
	    }
		
	}



	if(Pino==2){ //Pio da Temperatura
		
		if (ADC==1023)
		{
			sinal_pwm_Tp = 255;
			
			Tp=200; // temperutura máxima
		}
		else
		{
			sinal_pwm_Tp= (ADC/4);
			
			Tp= ((sinal_pwm_Tp/0.34)-(105/0.34));
		}
		
	}
	
	
}
ISR(TIMER1_CAPT_vect){
	
	if(TCCR1B &(1<<ICES1))
	
	bordaS=ICR1;
	
	
	
	else
	
	tmpD=((ICR1-bordaS)*16)/58;
	
	TCCR1B^=(1<<ICES1);
	
}
void USART_Init(unsigned int ubrr){
	
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L=(unsigned char)ubrr;
	UCSR0B =  (1<<RXCIE0) | ( 1<<RXEN0) | (1<<TXEN0);
	UCSR0C =(1<<USBS0)|(3<<UCSZ00);

}
void USART_Transmit(unsigned char data){
	
	while(!( UCSR0A & (1<< UDRE0)));
	UDR0 = data;
}
unsigned char USART_Receive (void){
	
	while(!(UCSR0A & (1<<RXC0)));
	return UDR0;
	
	
}
ISR(USART_RX_vect){
	
	char entregue;
	entregue = UDR0;
	int d=0;
	
	sprintf(matriz,"%u",t_eeprom);
	if(entregue=='l'){
		eeprom_write_byte(3,0);
	}
	
	if(entregue=='d'){
		
		if (t_eeprom <= 9)
		{
			d=1;
		}
		else if (t_eeprom <= 99 ){
			d=2;
		}
		else {
			d=3;
		}
		
		for (int i=0;i<d;i++)
		{
			USART_Transmit(matriz[i]);
		}
		
		
	}
	if(entregue=='s'){
		
		sirene = 1;
		
	}
	if(entregue=='x'){
		
		sirene = 0;
		
	}
	if(entregue=='b'){
		
		sirene = 2;
		
	}
	
}

int main(void)
{
	
	DDRB = 0b11111110; //Habilita os pinos PB0...7 como saídas
	PORTB = 0b00000001; //// Habilita o resistor de pull - up do pino PB0
	DDRC = 0b11111000; // Habilita os pino como saída
	PORTC = 0b10110000;
	DDRD  = 0b10100001;; // Habilita os pinos PD2...PD3 como entradas
	PORTD = 0b01101101; // Habilita o resistor de pull - up do pino PD1... PD7
	
	/*
	ICR1=39999;
	TCCR1A=0b10100010;
	TCCR1B=0b00011010;
	
	OCR1A=2000;
	OCR1B=4000;
	*/
	
	
	TCCR0A = 0b00000010; // Habilita modo CTC do TC0
	TCCR0B = 0b00000011; //Liga TC0 com prescaler = 64
	OCR0A = 249; // Ajusta o comparador para o TC0 contar ate 249
	TIMSK0 = 0b00000010; // Habilita a interrupção na igualdade de comparação OCR0A. A interrupção ocorre a cada 1ms = (64*(249+1))/16MHz
	TCCR1B= (1<<ICES1)|(1<<CS12);
	TIMSK1 = 1<<ICIE1;
	
	//configurações das interrupções
	
	EICRA = 0b00001010; //interrupção externa INT0 e INT1 na borda de descida
	EIMSK = 0b00000011; //habilita a interrupção externa INT0 e INT1
	
	
	PCICR= 0b00000100; //Habilita a interrupção PCINT2
	PCMSK2= 0b00010000; // Habilitação do pino D7 na interrupção PCINT2
	
	ADMUX = 0b11000000;
	
	ADCSRA = 0b11101111;
	
	ADCSRB = 0x00;
	
	DIDR0 = 0b00111000;
	
	TCCR0A = 0b10100011;
	
	TCCR0B = 0b00000011;
	
	USART_Init(MYUBRR);
	sei(); //Habilita interrupções globais, ativando o bit I do SREG
	GLCD_Setup();
	GLCD_SetFont(Font5x8,5,8, GLCD_Overwrite);
	GLCD_InvertScreen();
	
	
	while (1)
	
	{
		
		velM = 2*3.14*(botao/2)/100000*Rpm*60;
		
		distancia = distancia_auxiliar*(2*3.14*(botao/2)/100000*Rpm*60)/3600;
		OCR0A = sinal_pwm;
		botao=eeprom_read_byte(0);
		
		
		
		if (sirene==1)
		{
			PORTC ^= 0b00001000;
			
			_delay_ms(10);
			
			PORTD ^= 0b10000000;
			
			
		}
		if (sirene==0)
		{
			PORTC &= 0b11110111;
			
			_delay_ms(10);
			
			PORTD &= 0b01111111;
		}
		if (sirene==2)
		{
			PORTC ^= 0b00001000;
			
			_delay_ms(10);
			
			PORTD ^= 0b10000000;
			
			_delay_ms(10);
			
			PORTC ^= 0b01000000;
		}
		
		
		
	/*
		for (uint16_t i=2000; i<4000; i+=100)
		{
			OCR1A =i;
			_delay_ms(100);
		}
		for (uint16_t i=4000; i>2000; i+=100)
		{
			OCR1A=i;
			_delay_ms(100);
		}
		*/
		if((tempo % 9) == 0)
		{
			
			GLCD_Clear();
			
			ADMUX = 0b11000000;
			Pino=0;
			_delay_ms(80);
			
			
			ADMUX = 0b11000001;
			Pino=1;
			_delay_ms(80);
			
			GLCD_GotoXY(120,15);
			GLCD_PrintString("%");
			GLCD_GotoXY(100,15);
			GLCD_PrintInteger(bat);
			
			
			
			ADMUX = 0b11000010;
			Pino=2;
			_delay_ms(80);
			
			GLCD_GotoXY(120,30);
			GLCD_PrintString("C");
			GLCD_GotoXY(100,30);
			GLCD_PrintInteger(Tp);
			
			t_eeprom= eeprom_read_byte(3);
			
			if (Tp>t_eeprom)
			{
				eeprom_write_byte(3,Tp);
			}
			
			
			GLCD_GotoXY(0,0);
			GLCD_PrintInteger(tmpD);
			
			GLCD_GotoXY(0,15);
			GLCD_PrintString("diamentro:");
			GLCD_GotoXY(60,15);
			GLCD_PrintInteger(botao);
			
			GLCD_GotoXY(0,30);
			GLCD_PrintString("rpm:");
			GLCD_GotoXY(30,30);
			GLCD_PrintInteger(Rpm);
			
			
			GLCD_GotoXY(70,45);
			GLCD_PrintInteger(distancia);
			
			GLCD_Render();
			
			
			
			
		}
		
		
		
		
		PORTB &= 0b00000001;
		PORTB |= 0b11000000;
		PORTB |=((((velM/1)%10)*2) & 0b00011110);
		
		_delay_ms(2);
		
		PORTB &= 0b00000001;
		PORTB |= 0b10100000;
		PORTB |= ((((velM/10)%10)*2) & 0b00011110);
		
		_delay_ms(2);
		
		PORTB &= 0b00000001;
		PORTB |= 0b01100000;
		PORTB |= ((((velM/100)%10)*2) & 0b00011110);
		
		_delay_ms(2);
	}
	
}






