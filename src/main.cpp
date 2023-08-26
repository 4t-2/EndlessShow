#include "../inc/loadOBJ.hpp"
#include "AGL/include/ShaderBuilder.hpp"
#include "AGL/include/math.hpp"
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <vector>

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
	// 					"cd tts && python3 tts.py \"" + msg +
	// "\""
	// + " result/"
	// + std::to_string(wavIndex) + ".wav";
	//
	// 				std::system(cmd.c_str());
	//
	// 				wavIndex++;
	// 			});

	agl::RenderWindow window;
	window.setup({1000, 1000}, "Window");
	window.setClearColor({16, 16, 32});
	window.setFPS(0);

	agl::Event event;
	event.setWindow(window);

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
					shapeColor * shaderFunc("clamp", shaderFunc("dot", normal, shaderFunc("vec3", 1., 1., 1.)), 0., 1.),
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

	agl::Shader shader;
	shader.compileSrc(vertSrc, fragSrc);

	agl::Camera camera;
	camera.setView({10, 10, 10}, {0, 0, 0}, {0, 1, 0});
	camera.setPerspectiveProjection(90, 1, 0.1, 100);

	window.getShaderUniforms(shader);
	shader.use();

	window.updateMvp(camera);

	agl::Texture blank;
	blank.loadFromFile("bigtex.png");

	agl::Shape cube([&](agl::Shape &shape) {
		std::vector<agl::Vec<float, 3>> vertex;
		std::vector<agl::Vec<float, 2>> uv;
		std::vector<agl::Vec<float, 3>> normal;

		loadOBJ("scene.obj", vertex, uv, normal);

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
	});

	cube.setTexture(&blank);
	cube.setColor(agl::Color::White);
	cube.setPosition({0, 0, 0});
	cube.setSize({1, 1, 1});

	while (!event.windowClose())
	{
		event.poll();

		window.clear();

		window.drawShape(cube);

		window.display();

		static agl::Vec<float, 3> pos;
		static agl::Vec<float, 3> rot;

		float speedPos = .1;
		float speedRot = 1;

		if(event.isKeyPressed(agl::Key::W))
		{
			pos.z+=cos(agl::degreeToRadian(rot.y)) * speedPos;
			pos.x+=sin(agl::degreeToRadian(rot.y)) * speedPos;
		}
		if(event.isKeyPressed(agl::Key::S))
		{
			pos.z-=cos(agl::degreeToRadian(rot.y)) * speedPos;
			pos.x-=sin(agl::degreeToRadian(rot.y)) * speedPos;
		}
		if(event.isKeyPressed(agl::Key::A))
		{
			pos.z-=sin(agl::degreeToRadian(rot.y)) * speedPos;
			pos.x+=cos(agl::degreeToRadian(rot.y)) * speedPos;
		}
		if(event.isKeyPressed(agl::Key::D))
		{
			pos.z+=sin(agl::degreeToRadian(rot.y)) * speedPos;
			pos.x-=cos(agl::degreeToRadian(rot.y)) * speedPos;
		}
		if(event.isKeyPressed(agl::Key::Space))
		{
			pos.y-=speedPos;
		}
		if(event.isKeyPressed(agl::Key::LeftControl))
		{
			pos.y+=speedPos;
		}

		if(event.isKeyPressed(agl::Key::Up))
		{
			rot.x-=speedRot;
		}
		if(event.isKeyPressed(agl::Key::Down))
		{
			rot.x+=speedRot;
		}
		if(event.isKeyPressed(agl::Key::Left))
		{
			rot.y+=speedRot;
		}
		if(event.isKeyPressed(agl::Key::Right))
		{
			rot.y-=speedRot;
		}

		agl::Mat4f &view = *(agl::Mat4f*)(long(&camera) + 64);

		agl::Mat4f rotate;
		agl::Mat4f translate;

		translate.translate(pos);
		rotate.rotate(rot);

		view = rotate * translate;
	
		window.updateMvp(camera);
	}
}
