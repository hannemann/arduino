#include <Arduino.h>
#include "SimpleMenu.h"

SimpleMenu::~SimpleMenu() {
	
};

void SimpleMenu::operator+=(int i) {
	_index += i;
	if (_index < 0) _index = 0;
	return _index;
}

int SimpleMenu::index() {
	return _index;
}

void SimpleMenu::index(int index) {
	_index = index;
}

void SimpleMenu::addIndex(int index) {
	index += index;
}

void SimpleMenu::addItem(char *name) {
	Serial.print("Add: ");
	Serial.println(name);
	struct SimpleMenuItem *item;
	item->index = sizeof(this->items);
	item->name = name;
	this->items[sizeof(this->items)] = item;
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
