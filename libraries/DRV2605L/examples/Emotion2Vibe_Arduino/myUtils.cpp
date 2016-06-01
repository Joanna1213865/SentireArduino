#include <iostream>
#include <vector>
#include <string>
#include <sstream>

std::vector<std::string> splitString(std::string s, std::string delimiter) {
	std::vector<std::string> splitted_strings;
	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos) {
		token = s.substr(0, pos);
		splitted_strings.push_back(token);
		s.erase(0, pos + delimiter.length());
	}

	splitted_strings.push_back(s);
	return splitted_strings;
}


std::vector<int> extractEffects(std::string s) {
	std::vector<int> effects;
	std::stringstream ss(s);
	int i;

	while (ss >> i)
	{
		effects.push_back(i);

		if (ss.peek() == ',')
			ss.ignore();
	}

	return effects;
}