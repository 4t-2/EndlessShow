#pragma once

#include <functional>
#include <string>
#include <vector>
#include <fstream>

// [ACTION]
class Action
{
	public:
		std::string action;

		Action(std::string &line);

		void print();
};

// NAME: CONTENT
class Dialogue
{
	public:
		std::string name;
		std::string content;

		Dialogue(std::string &line);

		void print();
};

enum ElementType
{
	DialogueT,
	ActionT,
	Empty,
};

class Sequence
{
	public:
		ElementType type	= ElementType::Empty;
		void	   *element = nullptr;

		~Sequence();
};

class Script
{
	public:
		std::string title	 = ""; // TITLE
		Sequence   *sequence = nullptr;
		int			length	 = 0;

		Script(std::fstream &fs);

		~Script();

		void loop(std::function<int(Action &action)> actionFunc, std::function<int(Dialogue &dialogue)> dialogueFunc);
};
