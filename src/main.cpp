#include "../inc/loadOBJ.hpp"
#include "AGL/include/math.hpp"
#include <cstdlib>
#include <fstream>
#include <functional>
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

		void loop(std::function<void(Action &action)> actionFunc, std::function<void(Dialogue &dialogue)> dialogueFunc)
		{
			printf("%s\n", title.c_str());

			for (int i = 0; i < length; i++)
			{
				if (sequence[i].type == ActionT)
				{
					Action *p = (Action *)(sequence[i].element);
					actionFunc(*p);
				}
				if (sequence[i].type == DialogueT)
				{
					Dialogue *p = (Dialogue *)(sequence[i].element);
					dialogueFunc(*p);
				}
			}
		}
};

int main()
{
	// std::fstream fs("script.txt", std::fstream::in);
	//
	// Script script(fs);
	//
	// fs.close();
	//
	// int wavIndex = 0;
	//
	// script.loop([](Action &action) { action.print(); },
	// 			[&](Dialogue &dialogue) {
	// 				dialogue.print();
	//
	// 				std::string msg = dialogue.content;
	//
	// 				for (int i = 0; i < msg.length(); i++)
	// 				{
	// 					if (msg[i] == '"')
	// 					{
	// 						msg.replace(i, 1, "\\\"");
	// 						i++;
	// 					}
	// 				}
	//
	// 				std::string cmd =
	// 					"cd tts && python3 tts.py \"" + msg + "\"" + "
	// result/"
	// + std::to_string(wavIndex) + ".wav";
	//
	// 				std::system(cmd.c_str());
	//
	// 				wavIndex++;
	// 			});

	agl::RenderWindow window;
	window.setup({1000, 1000}, "Window");
	window.setClearColor(agl::Color::Red);
	window.setFPS(0);

	agl::Event event;
	event.setWindow(window);

	agl::ShaderBuilder sv;
	sv.setDefaultVert();
	agl::ShaderBuilder sf;
	sf.setDefaultFrag();

	std::string vertSrc = sv.getSrc();
	std::string fragSrc = sf.getSrc();

	agl::Shader shader;
	shader.compileSrc(vertSrc, fragSrc);

	agl::Camera camera;
	camera.setView({10, 10, 10}, {0, 0, 0}, {0, 1, 0});
	camera.setPerspectiveProjection(90, 1, 0.1, 100);

	window.getShaderUniforms(shader);
	shader.use();

	window.updateMvp(camera);

	agl::Texture blank;
	blank.setBlank();

	agl::Shape cube([&](agl::Shape &shape) {
		std::vector<agl::Vec<float, 3>> vertex;
		std::vector<agl::Vec<float, 2>> uv;
		std::vector<agl::Vec<float, 3>> normal;

		loadOBJ("untitled.obj", vertex, uv, normal);

		float *vertexBufferData = new float[3 * vertex.size()];
		float *UVBufferData		= new float[2 * uv.size()];

		for (int i = 0; i < vertex.size(); i++)
		{
			vertexBufferData[(i * 3) + 0] = vertex[i].x;
			vertexBufferData[(i * 3) + 1] = vertex[i].y;
			vertexBufferData[(i * 3) + 2] = vertex[i].z;
			std::cout << vertex[i] << '\n';
		}

		for (int i = 0; i < uv.size(); i++)
		{
			UVBufferData[(i * 2) + 0] = uv[i].x;
			UVBufferData[(i * 2) + 1] = uv[i].y;
		}

		shape.genBuffers();
		shape.setMode(GL_TRIANGLES);
		shape.setBufferData(vertexBufferData, UVBufferData, vertex.size());

		delete[] vertexBufferData;
		delete[] UVBufferData;
	});

	cube.setTexture(&blank);
	cube.setColor(agl::Color::Blue);
	cube.setPosition({0, 0, 0});
	cube.setSize({1, 1, 1});

	while (!event.windowClose())
	{
		event.poll();

		window.clear();

		window.drawShape(cube);

		window.display();

		window.updateMvp(camera);

		static int frame;

		agl::Vec<float, 3> pos = agl::pointOnCircle(agl::degreeToRadian(frame));
		pos.z = pos.y;
		pos *= 3;
		pos.y = 5;

		camera.setView(pos, {0, 0, 0}, {0, 1, 0});

		frame++;
	}
}
