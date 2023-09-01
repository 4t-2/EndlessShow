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

int genAudio(Script &script)
{
	if (std::filesystem::exists("tts/result/0.wav"))
	{
		std::cout << "SKIPPED AUDIO, DETECTED FILES" << '\n';

		return 0;
	}

	int wavIndex = 0;

	script.loop(
		[](Action &action) -> int {
			action.print();
			return 0;
		},
		[&](Dialogue &dialogue) -> int {
			dialogue.print();

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

			if (dialogue.name == "Joey")
			{
				voice = "schlatt";
			}
			else if (dialogue.name == "Michael")
			{
				voice = "narrator";
			}
			else if (dialogue.name == "Andrew")
			{
				voice = "joe";
			}
			else
			{
				return 1;
			}

			std::string cmd =
				"cd tts && python3 tts.py " + safeStr(msg) + " " + voice + " " + std::to_string(wavIndex) + ".wav";

			std::cout << cmd << '\n';

			std::system(cmd.c_str());

			wavIndex++;

			return 0;
		});

	return 1;
}

class Tweet
{
	public:
		std::string author;
		std::string content;

		Tweet(std::string &line)
		{
			std::string *p = &author;

			for (int i = 0; i < line.length(); i++)
			{
				char &c = line[i];

				if (c != ':')
				{
					*p += c;
				}
				else
				{
					p = &content;
					i++;
				}
			}
		}
};

class Article
{
	public:
		Article(std::string &line)
		{
			std::string *p = &title;

			for (char &c : line)
			{
				if (c != ':')
				{
					*p += c;
				}
				else
				{
					p = &subtext;
				}
			}
		}
		std::string title;
		std::string subtext;
};

template <typename T> class ThreadSafe
{
	private:
		T		   t;
		std::mutex mx;

	public:
		void use(std::function<void(T &t)> func)
		{
			mx.lock();

			func(t);

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
					shaderFunc("vec3", .1, .1, .1) +
						shapeColor * 1.5 *
							shaderFunc("clamp",
									   shaderFunc("dot", normal, shaderFunc("vec3", 0.57735, 0.57735, 0.57735)), 0.,
									   1.),
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

	objPath = "assets/j1.obj";
	agl::Shape jaw1(objToShape);
	jaw1.setTexture(&sceneTexture);
	jaw1.setColor(agl::Color::White);
	jaw1.setPosition({-3.99502, 4.69708, 0.000001});
	jaw1.setSize({1, 1, 1});

	objPath = "assets/j2.obj";
	agl::Shape jaw2(objToShape);
	jaw2.setTexture(&sceneTexture);
	jaw2.setColor(agl::Color::White);
	jaw2.setPosition({4.3209, 5.25919, 0});
	jaw2.setSize({1, 1, 1});

	objPath = "assets/j3.obj";
	agl::Shape jaw3(objToShape);
	jaw3.setTexture(&sceneTexture);
	jaw3.setColor(agl::Color::White);
	jaw3.setPosition({0, 4.59805, -4.0351});
	jaw3.setSize({1, 1, 1});

	agl::Rectangle rect;
	rect.setTexture(&blank);
	rect.setColor(agl::Color::White);

	agl::Vec<float, 3> pos;
	agl::Vec<float, 3> rot;
	int				   frame	   = 0;
	float			   jawRotation = std::sin(agl::degreeToRadian(frame * 12)) / 2 + .5;
	jawRotation *= 40;
	bool		closeMouth = false;
	std::string subContent = "";
	Tweet	   *tweet	   = nullptr;
	Article	   *article	   = nullptr;

	while (!event.isKeyPressed(agl::Key::Return))
	{
		event.poll();
	}

	auto threadFunc = [&]() {
		int wavIndex = 0;

		script.loop(
			[&](Action &action) -> int {
				pos		   = {0, -4, -4};
				rot		   = {0, 0, 0};
				closeMouth = true;

				int			i = 0;
				std::string actype;

				for (i = 0; i < action.action.length(); i++)
				{
					if (action.action[i] == ' ')
					{
						i++;
						break;
					}
					actype += action.action[i];
				}

				std::string str = action.action.substr(i, action.action.length() - i);

				if (actype == "TWEET")
				{
					tweet	   = new Tweet(str);
				}
				else if (actype == "ARTICLE")
				{
					article = new Article(str);
				}

				return 0;
			},
			[&](Dialogue &dialogue) -> int {
				subContent = dialogue.name + ": " + dialogue.content;

				if (dialogue.name == "Joey")
				{
					pos = {1.74179, -3.9, -0.727935};
					rot = {-5, 68, 0};
				}
				if (dialogue.name == "Michael")
				{
					pos = {-2.93833, -6, -1.66266};
					rot = {36, -43, 0};
				}
				if (dialogue.name == "Andrew")
				{
					pos = {0, -4.8, 3.34516};
					rot = {0, 0, 0};
				}

				closeMouth = false;

				std::system(("cd tts/result && aplay " + std::to_string(wavIndex) + ".wav").c_str());

				wavIndex++;

				if (tweet != nullptr)
				{
					delete tweet;
					tweet = nullptr;
				}
				if (article != nullptr)
				{
					delete article;
					article = nullptr;
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
		camera.setPerspectiveProjection(90, (float)state.size.x / state.size.y, 0.1, 100);
		uiCamera.setOrthographicProjection(0, state.size.x, state.size.y, 0, 0.1, 100);

		window.clear();

		if (tweet != nullptr) // Tweet render
		{
			window.getShaderUniforms(shaderUI);
			shaderUI.use();
			window.updateMvp(uiCamera);

			agl::Vec<float, 3> tweetSize	= {1000, 500};
			agl::Vec<float, 3> tweetPos		= state.size / 2 - tweetSize / 2;
			agl::Vec<float, 3> tweetPadding = {50, 50};
			agl::Vec<float, 3> iconSize		= {100, 100};
			float			   namePadding	= 5;

			rect.setPosition(tweetPos + agl::Vec<float, 3>{0, 0, -1});
			rect.setSize(tweetSize);
			rect.setTexture(&blank);
			window.drawShape(rect);

			rect.setPosition(tweetPos + tweetPadding);
			rect.setSize(agl::Vec<float, 2>{100, 100});
			rect.setTexture(&iconDef);
			window.drawShape(rect);

			text.setPosition(
				tweetPos + tweetPadding +
				agl::Vec<float, 2>{iconSize.x + namePadding, iconSize.y / 2 - text.getHeight() - int(namePadding / 2)});
			text.setText(tweet->author);
			text.setColor(agl::Color::Black);
			window.drawText(text);

			text.setPosition(tweetPos + tweetPadding +
							 agl::Vec<float, 2>{iconSize.x + namePadding, iconSize.y / 2 + int(namePadding / 2)});
			text.setText("@" + tweet->author);
			text.setColor(agl::Color::Gray);
			window.drawText(text);

			text.setFont(&fontLarge);
			text.setPosition(tweetPos + tweetPadding + agl::Vec<float, 2>{0, iconSize.y + 50});
			text.setText(tweet->content);
			text.setColor(agl::Color::Black);
			window.drawText(text, tweetSize.x - (tweetPadding.x * 2));
			text.setFont(&font);
		}
		else if (article != nullptr) // Article render
		{
		}
		else // Scene render
		{
			window.getShaderUniforms(shaderScene);
			shaderScene.use();
			window.updateMvp(camera);

			window.drawShape(scene);
			window.drawShape(jaw1);
			window.drawShape(jaw2);
			window.drawShape(jaw3);
		}

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

		if (!closeMouth)
		{
			jaw1.setRotation({0, 0, jawRotation});
			jaw2.setRotation({0, 0, -jawRotation});
			jaw3.setRotation({jawRotation, 0, 0});
		}
		else
		{
			jaw1.setRotation({0, 0, 0});
			jaw2.setRotation({0, 0, 0});
			jaw3.setRotation({0, 0, 0});
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
