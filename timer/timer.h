// Класс канала таймера TimerChannel хранит время включения и выключения, содержит метод проверки и клацает релюшкой по результатам

// класс канала таймера
class TimerChannel{

	public:
	 TimerChannel(void); // конструктор
	 void begin(unsigned char relayChannel, unsigned char channelNumber); // инициализатор объекта

	 void setOnTime(unsigned char hh, unsigned char mm); // установить время включения
	 void setOffTime(unsigned char hh, unsigned char mm); // установаить время выключения

	 unsigned char getOnTimehh(void); // получить время включения и выключения
	 unsigned char getOnTimemm(void);
	 unsigned char getOffTimehh(void);
	 unsigned char getOffTimemm(void);

	 void check(unsigned char hh, unsigned char mm); // проверить время, при необходимости включить или выключить канал

	 unsigned char getNumber(void); // получить номер канала

	 bool getState (void); // получить текущее состояние

	private:
	 unsigned char hhOn; // время включения и выключения
	 unsigned char mmOn;
	 unsigned char hhOff;
	 unsigned char mmOff;

	 unsigned char lastCheckhh; // время последней проверки 
	 unsigned char lastCheckmm;

	 unsigned char relay; // номер канала для управления реле

	 bool state; //текущее состояние

	 unsigned char number; // номер канала

	 int toMinutes(unsigned char hh, unsigned char mm); // пересчитать из ЧЧ:ММ в тупо минуты, для сравнений
	 void klatz(void); // клацнуть релюшкой

};

