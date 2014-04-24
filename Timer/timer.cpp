#include "Arduino.h" // надо для AnalogWrite
#include "timer.h"

//конструктор
TimerChannel::TimerChannel(void)
{
}

//инициализация. Это, по идее, должно быть в конструкторе, но ЧОТО ОПА НИХУЯ
void TimerChannel::begin(unsigned char relayChannel, unsigned char channelNumber)
{
	//обнуление времени
	hhOn = 0;
	mmOn = 0;
	hhOff = 0;
	mmOff = 0;

	// канал выключен
	state = false;

	// задание канала реле
	relay = relayChannel;

	// задание номера канала
	number = channelNumber;
}

//установка времени включения и выключения
void TimerChannel::setOnTime(unsigned char hh, unsigned char mm){

	hhOn = hh;
	mmOn = mm;
}

void TimerChannel::setOffTime(unsigned char hh, unsigned char mm){

	hhOff = hh;
	mmOff = mm;
}

// пересчитывание ЧЧ:ММ в минуты, для упрощения сравнения
int TimerChannel::toMinutes(unsigned char hh, unsigned char mm){

	return hh * 60 + mm;
}

// клацнуть релюшкой в соответствие с переменной state
void TimerChannel::klatz(void){

	if (state) {digitalWrite(relay, HIGH);} else {digitalWrite(relay, LOW);}
}


// проверка времени
void TimerChannel::check(unsigned char hh, unsigned char mm){

	// если текущее время в диапазоне работы, проверить текущее состояние, если выключено - включить
	// если текущее время не в диапазоне работы, проверить текущее состояние, если включено - выключить

	if ( (toMinutes(hhOn, mmOn) <= toMinutes(hh, mm)) &&
		 (toMinutes(hh, mm) < toMinutes(hhOff, mmOff) ))

	{
		if ( !state ) { state = true; klatz(); }

	} else

	{
		if ( state ) { state = false; klatz(); }
	}

}

unsigned char TimerChannel::getOnTimehh(void) { return hhOn;} // выдача времени вкл и выкл
unsigned char TimerChannel::getOnTimemm(void) { return mmOn;}
unsigned char TimerChannel::getOffTimehh(void) { return hhOff;}
unsigned char TimerChannel::getOffTimemm(void) { return mmOff;}

unsigned char TimerChannel::getNumber(void) { return number;} // выдача времени канала, надо для отображения

bool TimerChannel::getState(void) {return state;} // выдача текущего состояния, для дебага надо