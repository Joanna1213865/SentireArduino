#pragma once
class Pattern
{
public:
	Pattern();
	Pattern(const char* id, const char* name, const char* emotion, const char* type, const char* value, int score);
	~Pattern();

	void setId(const char* id);
	const char* getId(void);
	void setName(const char* name);
	const char* getName(void);
	void setEmotion(const char* emotion);
	const char* getEmotion(void);
	void setType(const char* type);
	const char* getType(void);
	void setValue(const char* value);
	const char* getValue(void);
	void setScore(int score);
	int getScore(void);


private:
	const char* _id;
	const char* _name;
	const char* _emotion;
	const char* _type;
	const char* _value;
	int _score;
};

