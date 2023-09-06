#include "../inc/Script.hpp"
#include <iostream>

void Element::print()
{
	std::cout << type;
	
	for(std::string &arg: arg)
	{
		std::cout << " \"" << arg << "\"";
	}

	std::cout << '\n';
}

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

Script::Script(std::fstream &fs)
{
	std::vector<std::string> vector;

	splitNewline(fs, vector);

	for (std::string &line : vector)
	{
		element.emplace_back();
		Element &last = element[element.size()-1];
		int		x;

		for (x = 0; x < line.length(); x++)
		{
			if (line[x] == ' ')
			{
				x++;
				break;
			}

			last.type += line[x];
		}

		for (; x < line.length(); x++)
		{
			if (line[x] == '"')
			{
				last.arg.emplace_back("");
				x++;

				for (; x < line.length(); x++)
				{
					if (line[x] == '"')
					{
						break;
					}

					last.arg[last.arg.size() - 1] += line[x];
				}
			}
		}
	}
}

Script::~Script()
{
}

void Script::loop(std::function<int(Element &element)> func)
{
	int code = 0;

	for (Element &element : element)
	{
		code = func(element);

		if (code)
		{
			return;
		}
	}
}
