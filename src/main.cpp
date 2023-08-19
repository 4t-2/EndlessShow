#include <fstream>
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
			std::string *p = &name;

			for (char &c : line)
			{
				if (c != ':')
				{
					*p += c;
				}
				else
				{
					p = &content;
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

class Sequence
{
	public:
		ElementType type	= ElementType::Empty;
		void	   *element = nullptr;

		~Sequence()
		{
			if (type == DialogueT)
			{
				delete (Dialogue *)element;
			}
			if (type == ActionT)
			{
				delete (Action *)element;
			}
		}
};

void splitNewline(std::fstream &fs, std::vector<std::string> &vec)
{
	for (std::string line; !fs.eof(); std::getline(fs, line))
	{
		if (line == "")
		{
			continue;
		}
		vec.push_back(line);
	}
}

class Script
{
	public:
		std::string title	 = ""; // TITLE
		Sequence   *sequence = nullptr;
		int			length	 = 0;

		Script(std::fstream &fs)
		{
			std::vector<std::string> vector;

			splitNewline(fs, vector);

			title = vector[0].substr(1, vector[0].length() - 2);

			length	 = vector.size() - 1;
			sequence = new Sequence[length];

			for (int i = 0; i < vector.size() - 1; i++)
			{
				std::string &line = vector[i + 1];

				if (line[0] == '[')
				{
					sequence[i].type	= ElementType::ActionT;
					sequence[i].element = new Action(line);
				}
				else
				{
					sequence[i].type	= ElementType::DialogueT;
					sequence[i].element = new Dialogue(line);
				}

				i++;
			}
		}

		~Script()
		{
			delete[] sequence;
		}

		void print()
		{
			printf("%s\n", title.c_str());

			for (int i = 0; i < length; i++)
			{
				if (sequence[i].type == ActionT)
				{
					Action *p = (Action *)(sequence[i].element);
					p->print();
				}
				if (sequence[i].type == DialogueT)
				{
					Dialogue *p = (Dialogue *)(sequence[i].element);
					p->print();
				}
			}
		}
};

int main()
{
	std::fstream fs("script.txt", std::fstream::in);

	Script script(fs);

	fs.close();

	script.print();
}
