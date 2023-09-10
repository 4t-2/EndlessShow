#include "../inc/Script.hpp"
#include "../inc/loadOBJ.hpp"
#include "AGL/include/ShaderBuilder.hpp"
#include "AGL/include/math.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#define TEXTPADDING 8

// 31.7523d

// 5.68184 m
//-4.89812 m
// 5.29282 m

// CAM
// 7.82274 m
//-7.14782 m
// 5.32259 m
//
// 86.5994d
//-0.000018d
// 48.228d

using std::cout;

class Room {
public:
  agl::Vec<int, 2> size;

  Room(Element &element) {
    size.x = std::stoi(element.arg[0]);
    size.x = std::stoi(element.arg[1]);
  }
};

class Actor
{
	public:
		std::string		 name;
		std::string		 voice;
		agl::Color		 shirtColour;
		agl::Vec<int, 2> pos;

		Actor(Element &element)
		{
			name = element.arg[0];

			voice = element.arg[1];

			if (element.arg[2] == "RED")
			{
				shirtColour = agl::Color::Red;
			}
			else if (element.arg[2] == "GREEN")
			{
				shirtColour = agl::Color::Green;
			}
			else if (element.arg[2] == "BLUE")
			{
				shirtColour = agl::Color::Blue;
			}
			else if (element.arg[2] == "CYAN")
			{
				shirtColour = agl::Color::Cyan;
			}
			else if (element.arg[2] == "MAGENTA")
			{
				shirtColour = agl::Color::Magenta;
			}
			else if (element.arg[2] == "YELLOW")
			{
				shirtColour = agl::Color::Yellow;
			}

			pos.x = std::stoi(element.arg[3]);
			pos.y = std::stoi(element.arg[4]);
		}
};

class Dialogue
{
	public:
		Actor	   *name = nullptr;
		std::string content;

		Dialogue(Element &element, std::vector<Actor> &actor)
		{
			std::cout << "LIST" << '\n';
			for (Actor &actor : actor)
			{
				std::cout << "ACTOR " << actor.name << '\n';
				if (actor.name == element.arg[0])
				{
					name = &actor;
				}
			}

			if (name == nullptr)
			{
				std::cout << "ERROR: Dialogue name is " + element.arg[0] + " but can't be found in actor list";
				exit(1);
			}

			content = element.arg[1];
		}
};

int genAudio(Script &script)
{
	if (std::filesystem::exists("tts/result/0.wav"))
	{
		std::cout << "SKIPPED AUDIO, DETECTED FILES" << '\n';

		return 0;
	}

	int wavIndex = 0;

	std::vector<Actor> actor;

	script.loop([&](Element &element) -> int {
		element.print();

		if (element.type == "ACTOR")
		{
			actor.emplace_back(element);
		}

		if (element.type == "DIALOGUE")
		{
			Dialogue dialogue(element, actor);

			std::string msg = dialogue.content;

			for (int i = 0; i < msg.length(); i++)
			{
				if (msg[i] == '"')
				{
					msg.replace(i, 1, "\\\"");
					i++;
				}
			}

			auto safeStr = [&](std::string str) { return "\"" + str + "\""; };

			std::string voice;

			voice = dialogue.name->voice;

			std::string cmd =
				"cd tts && python3 tts.py " + safeStr(msg) + " " + voice + " " + std::to_string(wavIndex) + ".wav";

			std::cout << cmd << '\n';

			std::system(cmd.c_str());

			wavIndex++;
		}

		return 0;
	});

	return 0;
}

template <typename T> class ThreadSafe
{
	private:
		T		   t;
		std::mutex mx;

	public:
		void use(std::function<void(T *t)> func)
		{
			mx.lock();

			func(&t);

			mx.unlock();
		}
};

void render(Script &script)
{
	agl::RenderWindow window;
	window.setup({1000, 1000}, "Window");
	window.setClearColor({16, 16, 32});
	window.setFPS(0);

	agl::Event event;
	event.setWindow(window);

	agl::Shader shaderScene;

	{
		agl::ShaderBuilder sv;
		{
			using namespace agl;

			ADD_LAYOUT(0, agl::vec3, position, (&sv));
			ADD_LAYOUT(1, agl::vec2, vertexUV, (&sv));
			ADD_LAYOUT(2, agl::vec3, normal, (&sv));

			ADD_UNIFORM(agl::mat4, transform, (&sv));
			ADD_UNIFORM(agl::mat4, mvp, (&sv));
			ADD_UNIFORM(agl::vec3, shapeColor, (&sv));
			ADD_UNIFORM(agl::mat4, textureTransform, (&sv));

			ADD_OUT(agl::vec2, UVcoord, (&sv));
			ADD_OUT(agl::vec4, fragColor, (&sv));

			sv.setMain({
				UVcoord =
					shaderFunc("vec2", "(" + (textureTransform * shaderFunc("vec4", vertexUV, 1, 1)).code + ").xy"), //
																													 //
				fragColor = shaderFunc(
					"vec4",
					shaderFunc("vec3", .15, .15, .15) +
						shapeColor * 0.8 *
							shaderFunc("clamp",
									   shaderFunc("dot", shaderFunc("vec3", transform * shaderFunc("vec4", normal, 0)),
												  shaderFunc("vec3", 0.4685212856658182, 0.6246950475544243,
															 0.6246950475544243)),
									   0., 1.),
					1),																	  //
																						  //
				val(val::gl_Position) = mvp * transform * shaderFunc("vec4", position, 1) //
			});
		}

		agl::ShaderBuilder sf;
		{
			using namespace agl;

			ADD_IN(agl::vec2, UVcoord, (&sf));
			ADD_IN(agl::vec4, fragColor, (&sf));

			ADD_OUT(agl::vec4, color, (&sf));

			ADD_UNIFORM(agl::sampler2D, myTextureSampler, (&sf));

			sf.setMain({
				color = fragColor * shaderFunc("texture", myTextureSampler, UVcoord), //
			});
		}

		std::string vertSrc = sv.getSrc();
		std::string fragSrc = sf.getSrc();

		shaderScene.compileSrc(vertSrc, fragSrc);
	}

	agl::Shader shaderUI;
	{
		agl::ShaderBuilder sv;
		sv.setDefaultVert();
		agl::ShaderBuilder sf;
		sf.setDefaultFrag();

		std::string vertSrc = sv.getSrc();
		std::string fragSrc = sf.getSrc();

		shaderUI.compileSrc(vertSrc, fragSrc);
	}

	agl::Camera camera;
	camera.setView({10, 10, 10}, {0, 0, 0}, {0, 1, 0});

	agl::WindowState state = window.getState();
	agl::Camera		 uiCamera;
	uiCamera.setView({0, 0, 50}, {0, 0, 0}, {0, 1, 0});

	window.getShaderUniforms(shaderScene);
	shaderScene.use();

	window.updateMvp(camera);

	agl::Texture sceneTexture;
	sceneTexture.loadFromFile("assets/bigtex.png");

	agl::Texture blank;
	blank.setBlank();

	agl::Texture iconDef;
	iconDef.loadFromFile("./assets/twitterdefault.png");

	agl::Rectangle subBack;
	subBack.setPosition({0, 0});
	subBack.setColor({0, 0, 0});
	subBack.setSize({0, 30});
	subBack.setTexture(&blank);

	agl::Font font;
	font.setup("/usr/share/fonts/TTF/Arial.TTF", 24);

	agl::Font fontLarge;
	fontLarge.setup("/usr/share/fonts/TTF/Arial.TTF", 36);

	agl::Text text;
	text.setFont(&font);
	text.setPosition({0, 0});
	text.setScale(1);
	text.setColor(agl::Color::White);
	text.setText("");

	std::string objPath = "assets/scene.obj";

	auto objToShape = [&](agl::Shape &shape) {
		std::vector<agl::Vec<float, 3>> vertex;
		std::vector<agl::Vec<float, 2>> uv;
		std::vector<agl::Vec<float, 3>> normal;

		loadOBJ(objPath.c_str(), vertex, uv, normal);

		float *vertexBufferData = new float[3 * vertex.size()];
		float *normalBufferData = new float[3 * normal.size()];
		float *UVBufferData		= new float[2 * uv.size()];

		for (int i = 0; i < vertex.size(); i++)
		{
			vertexBufferData[(i * 3) + 0] = vertex[i].x;
			vertexBufferData[(i * 3) + 1] = vertex[i].y;
			vertexBufferData[(i * 3) + 2] = vertex[i].z;
		}

		for (int i = 0; i < normal.size(); i++)
		{
			normalBufferData[(i * 3) + 0] = normal[i].x;
			normalBufferData[(i * 3) + 1] = normal[i].y;
			normalBufferData[(i * 3) + 2] = normal[i].z;
		}

		for (int i = 0; i < uv.size(); i++)
		{
			UVBufferData[(i * 2) + 0] = uv[i].x;
			UVBufferData[(i * 2) + 1] = uv[i].y;
		}

		agl::GLPrimative &shapeData = *(agl::GLPrimative *)(long(&shape) + 456);

		shapeData.genBuffers(3);
		shape.setMode(GL_TRIANGLES);
		shape.setBufferData(vertexBufferData, UVBufferData, vertex.size());
		shapeData.setBufferData(2, normalBufferData, 3);

		delete[] vertexBufferData;
		delete[] UVBufferData;
	};

	agl::Shape scene(objToShape);
	scene.setTexture(&sceneTexture);
	scene.setColor(agl::Color::White);
	scene.setPosition({0, 0, 0});
	scene.setSize({1, 1, 1});

	objPath = "assets/jaw.obj";
	agl::Shape jaw(objToShape);
	jaw.setTexture(&sceneTexture);
	jaw.setColor(agl::Color::White);
	jaw.setPosition({5.68184, 5.29282, 4.89812});
	jaw.setRotation({0, 31.7523 - 64, 0});
	jaw.setSize({1, 1, 1});

	agl::Rectangle rect;
	rect.setTexture(&blank);
	rect.setColor(agl::Color::White);

	agl::Vec<float, 3> pos		   = {-7.82274, -5.32259, -7.14782};
	agl::Vec<float, 3> rot		   = {0, 48.228, -0.00001};
	int				   frame	   = 0;
	float			   jawRotation = std::sin(agl::degreeToRadian(frame * 12)) / 2 + .5;
	jawRotation *= 40;
	bool						   closeMouth = false;
	std::string					   subContent = "";
	ThreadSafe<std::vector<Actor>> actor;

	while (!event.isKeyPressed(agl::Key::Return))
	{
		event.poll();
	}

	auto threadFunc = [&]() {
		int wavIndex = 0;

		script.loop([&](Element &element) -> int {
			closeMouth = true;
			if (element.type == "ACTOR")
			{
				actor.use([&](std::vector<Actor>*actor) {
					actor->push_back(element);
                                        cout << (*actor)[0].name << '\n';
				});

				actor.use([&](std::vector<Actor>*actor) {
                                        cout << (*actor)[0].name << '\n';
				});
			}
			if (element.type == "DIALOGUE")
			{
				actor.use([&](auto actor) {
					Dialogue dialogue(element, *actor);

					subContent = dialogue.name->name + ": " + dialogue.content;
				});

				closeMouth = false;

				std::system(("cd tts/result && aplay " + std::to_string(wavIndex) + ".wav").c_str());

				wavIndex++;
			}

			return 0;
		});
	};

	std::thread scriptThread(threadFunc);

	while (!event.windowClose())
	{
		frame += 1;
		jawRotation = std::sin(agl::degreeToRadian(frame * 12)) / 2 + .5;
		jawRotation *= 40;

		event.poll();

		state = window.getState();

		window.setViewport(0, 0, state.size.x, state.size.y);
		camera.setPerspectiveProjection(49, (float)state.size.x / state.size.y, 0.1, 100);
		uiCamera.setOrthographicProjection(0, state.size.x, state.size.y, 0, 0.1, 100);

		window.clear();

		// Scene render
		window.getShaderUniforms(shaderScene);
		shaderScene.use();
		window.updateMvp(camera);

		window.drawShape(jaw);
		window.drawShape(scene);

		// Subtitle render

		window.getShaderUniforms(shaderUI);
		shaderUI.use();
		window.updateMvp(uiCamera);

		glClear(GL_DEPTH_BUFFER_BIT);

		text.setPosition(state.size);
		text.setText(subContent);
		agl::Vec textEnd = window.drawText(text, state.size.x - TEXTPADDING);

		subBack.setSize(state.size);

		text.setColor(agl::Color::White);
		text.setPosition({TEXTPADDING, (float)(state.size.y - font.getHeight() - TEXTPADDING) - textEnd.y, 1});
		subBack.setPosition({0, (float)(state.size.y - font.getHeight() - (TEXTPADDING * 2)) - textEnd.y, 0});

		glClear(GL_DEPTH_BUFFER_BIT);

		window.drawShape(subBack);
		window.drawText(text, state.size.x - TEXTPADDING);

		window.display();

		if (closeMouth)
		{
			jaw.setRotation({0, 31.7523 - 64, 0});
		}
		else
		{
			long		pshape = (long)&jaw;
			agl::Mat4f *rotMat = (agl::Mat4f *)(pshape + 388);
			agl::Mat4f	mat1;
			mat1.rotate({0, 31.7523 - 64, 0});
			agl::Mat4f mat2;
			mat2.rotate({jawRotation, 0, 0});
			*rotMat = mat1 * mat2;
		}

		float speedPos = .1;
		float speedRot = 1;

		if (event.isKeyPressed(agl::Key::W))
		{
			pos.z += cos(agl::degreeToRadian(rot.y)) * speedPos;
			pos.x += sin(agl::degreeToRadian(rot.y)) * speedPos;
		}
		if (event.isKeyPressed(agl::Key::S))
		{
			pos.z -= cos(agl::degreeToRadian(rot.y)) * speedPos;
			pos.x -= sin(agl::degreeToRadian(rot.y)) * speedPos;
		}
		if (event.isKeyPressed(agl::Key::A))
		{
			pos.z -= sin(agl::degreeToRadian(rot.y)) * speedPos;
			pos.x += cos(agl::degreeToRadian(rot.y)) * speedPos;
		}
		if (event.isKeyPressed(agl::Key::D))
		{
			pos.z += sin(agl::degreeToRadian(rot.y)) * speedPos;
			pos.x -= cos(agl::degreeToRadian(rot.y)) * speedPos;
		}
		if (event.isKeyPressed(agl::Key::Space))
		{
			pos.y -= speedPos;
		}
		if (event.isKeyPressed(agl::Key::LeftControl))
		{
			pos.y += speedPos;
		}

		if (event.isKeyPressed(agl::Key::Up))
		{
			rot.x -= speedRot;
		}
		if (event.isKeyPressed(agl::Key::Down))
		{
			rot.x += speedRot;
		}
		if (event.isKeyPressed(agl::Key::Left))
		{
			rot.y += speedRot;
		}
		if (event.isKeyPressed(agl::Key::Right))
		{
			rot.y -= speedRot;
		}

		agl::Mat4f &view = *(agl::Mat4f *)(long(&camera) + 64);

		agl::Mat4f rotate;
		agl::Mat4f translate;

		translate.translate(pos);
		rotate.rotate(rot);

		view = rotate * translate;
	}
}

int main()
{
	std::fstream fs("script.txt", std::fstream::in);

	Script script(fs);

	fs.close();

	if (genAudio(script))
	{
		return 0;
	}
	render(script);

	exit(0);
}
