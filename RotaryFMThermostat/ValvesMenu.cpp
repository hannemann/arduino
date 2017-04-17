#include <Arduino.h>
#include "ValvesMenu.h"

ValvesMenu::ValvesMenu() {
};

ValvesMenu::~ValvesMenu() {
};

int ValvesMenu::operator+=(int i) {
	_index += i;
	if (_index < 0) _index = 0;
	if (_index >= count-1) _index = count-1;
	return _index;
}

int ValvesMenu::index() {
	return _index;
}

void ValvesMenu::index(int i) {
	_index = i;
}

void ValvesMenu::addItem(char name[10], int addr, float wanted, float real) {
	Valve item;
	item.index(count);
	item.name(name);
	item.addr(addr);
	item.wanted(wanted);
	item.real(real);
	items[count] = item;
	count++;
}

Valve ValvesMenu::item(int i) {
	return items[i];
}

void ValvesMenu::reset() {
	delete items;
	_index = 0;
}

bool ValvesMenu::isCurrent(int i) {
	return _index == i;
}
