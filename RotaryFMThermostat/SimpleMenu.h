
struct SimpleMenuItem {
	int index;
	char *name;
};

class SimpleMenu {
	private:
		int _index = 0;
		bool isCurrent(SimpleMenuItem *item);
	public:
		SimpleMenu();
		~SimpleMenu();
		struct SimpleMenuItem *items[10];
		int index();
		void index(int i);
		int length() {return sizeof(items);};
		struct SimpleMenuItem *item(int i);
		void addItem(char *name);
		void selectItem();
		void reset();
		int operator+=(int i);
};
