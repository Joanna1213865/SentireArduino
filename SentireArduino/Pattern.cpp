#include "Pattern.h"

Pattern::Pattern()
{
}

Pattern::Pattern(const char* id, const char* name, const char* emotion, const char* type, const char* value, int score) {
	_id = id;
	_name = name;
	_emotion = emotion;
	_type = type;
	_value = value;
	_score = score;
}

Pattern::~Pattern()
{
}

void Pattern::setId(const char* id) {
	_id = id;
}

const char* Pattern::getId() {
	return _id;
}

void Pattern::setName(const char* name) {
	_name = name;
}

const char* Pattern::getName() {
	return _name;
}

void Pattern::setEmotion(const char* emotion) {
	_emotion = emotion;
}

const char* Pattern::getEmotion() {
	return _emotion;
}

void Pattern::setType(const char* type) {
	_type = type;
}

const char* Pattern::getType() {
	return _type;
}

void Pattern::setValue(const char* value) {
	_value = value;
}

const char* Pattern::getValue() {
	return _value;
}

void Pattern::setScore(int score) {
	_score = score;
}

int Pattern::getScore() {
	return _score;
}