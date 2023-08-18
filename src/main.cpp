#include <iostream>
#include <sstream>
#include <vector>

// [ACTION]
class Action
{
	public:
		std::string action;

		Action(std::string &line)
		{
			action = line.substr(1, line.length() - 2);
		}

		void print()
		{
			printf("[%s]\n", action.c_str());
		}
};

// NAME: CONTENT
class Dialogue
{
	public:
		std::string name;
		std::string content;

		Dialogue(std::string &line)
		{
			bool skip = false;

			for (char &c : line)
			{
				if (skip)
				{
					continue;
				}

				if (c != ':')
				{
					name += c;
					skip = true;
					continue;
				}
			}
		}

		void print()
		{
			printf("%s: %s\n", name.c_str(), content.c_str());
		}
};

enum ElementType
{
	DialogueT,
	ActionT,
	Empty,
};

struct Sequence
{
		ElementType type	= ElementType::Empty;
		void	   *element = nullptr;
};

void splitNewline(std::string &s, std::vector<std::string> &vec)
{
	std::stringstream ss(s);
	std::string		  line;

	while (ss >> line)
	{
		vec.push_back(line);
	}
}

class Script
{
	public:
		std::string			  title; // TITLE
		std::vector<Sequence> sequence;

		Script(std::string &buffer)
		{
			std::vector<std::string> vector;

			splitNewline(buffer, vector);

			sequence.reserve(vector.size() - 1);

			for (std::string &line : vector)
			{
				static int i = 0;

				if (line[0] == '[')
				{
					sequence[i].type	= ElementType::ActionT;
					sequence[i].element = new class Action(line);
				}
				else
				{
					sequence[i].type	= ElementType::DialogueT;
					sequence[i].element = new class Dialogue(line);
				}

				i++;
			}
		}

		void print()
		{
			printf("%s\n", title.c_str());

			for (Sequence &sequence : sequence)
			{
				if(sequence.type == DialogueT)
				{
					Dialogue *p = (Dialogue*)(sequence.element);
					p->print();
				}
				if(sequence.type == DialogueT)
				{
					Dialogue *p = (Dialogue*)(sequence.element);
					p->print();
				}
			}
		}
};

void cleanup(std::string &buffer)
{
}

int main()
{
	std::string buffer = "";

	cleanup(buffer);

	Script script(buffer);
	script.print();
}
