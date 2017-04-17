
class Valve {

	private:
		int _index;
		char _name[10];
		int _addr;
		int _wanted;
		float _real;
		int _max = 30 / 0.5;
	public:
		void index(int i) { _index = i; };
		int index() { return _index; };
		void addr(int a) { _addr = a; };
		int addr() { return _addr; };
		void wanted(float w);
		float wanted();
		void real(float r) { _real = r; };
		float real() { return _real; };
		void name(char n[10]) { strcpy(_name, n); };
		char * name() { return _name; };
		float operator+=(int i);
};

class ValvesMenu {

	private:
		int _index = 0;
		int count = 0;
		Valve items[10];
	public:
		ValvesMenu();
		~ValvesMenu();
		int index();
		void index(int i);
		int length() {return count;};
		Valve item(int i);
		void addItem(char name[10], int addr, float wanted, float real);
		void selectItem();
		void reset();
		int operator+=(int i);
		bool isCurrent(int i);
		Valve * current();
};
