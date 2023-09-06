#pragma once

#include <functional>
#include <string>
#include <vector>
#include <fstream>

class Element
{
	public:
		std::string type;
		std::vector<std::string> arg;

		void print();
};

class Script
{
	public:
		std::vector<Element> element;

		Script(std::fstream &fs);

		~Script();

		void loop(std::function<int(Element &element)> func);
};
