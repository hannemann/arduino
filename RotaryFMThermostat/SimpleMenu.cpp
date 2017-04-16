#include <Arduino.h>
#include "SimpleMenu.h"

SimpleMenu::SimpleMenu() {
};

SimpleMenu::~SimpleMenu() {
	
};

int SimpleMenu::operator+=(int i) {
	_index += i;
	if (_index < 0) _index = 0;
	return _index;
}

int SimpleMenu::index() {
	return _index;
}

void SimpleMenu::index(int i) {
	_index = i;
}

void SimpleMenu::addItem(char *name) {
	Serial.print("Add: ");
	Serial.println(name);
	struct SimpleMenuItem *item;
	item->index = sizeof(items);
	item->name = name;
	items[sizeof(items)] = item;
}

struct SimpleMenuItem *SimpleMenu::item(int i) {
	return items[i];
}

void SimpleMenu::reset() {
	delete items;
	_index = 0;
}

bool SimpleMenu::isCurrent(SimpleMenuItem *item) {
	return _index == item->index;
}
