cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(mm_fireworks CXX)

add_library(particles STATIC
	./render_tasks/particles.hpp
	./render_tasks/particles.cpp

	./components/particles.hpp
	./components/firework_particles.hpp

	./systems/particles.hpp
	./systems/particles.cpp
	./systems/fireworks.hpp
	./systems/fireworks.cpp
)

target_link_libraries(particles
	engine
	opengl_renderer_s

	common_components
	random
)

##########################

add_executable(fireworks1
	./fireworks1.cpp
)

target_link_libraries(fireworks1
	engine
	organizer_scene

	random

	opengl_renderer_s
	imgui_service
	imgui_render_task
	imgui_tools

	particles
)

if(EMSCRIPTEN)
	set_target_properties(fireworks1 PROPERTIES COMPILE_FLAGS "-s USE_SDL=2")
	set_target_properties(fireworks1 PROPERTIES LINK_FLAGS "-s USE_SDL=2 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 -s FULL_ES2=1 -s FULL_ES3=1 -s ALLOW_MEMORY_GROWTH=1")
endif()

##########################

add_executable(fireworks2
	./fireworks2.cpp
)

target_link_libraries(fireworks2
	engine
	organizer_scene

	random

	opengl_renderer_s
	imgui_service
	imgui_render_task
	imgui_tools

	clear_render_task
	bloom
	composition_render_task

	particles
)

if(EMSCRIPTEN)
	set_target_properties(fireworks2 PROPERTIES COMPILE_FLAGS "-s USE_SDL=2")
	set_target_properties(fireworks2 PROPERTIES LINK_FLAGS "-s USE_SDL=2 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 -s FULL_ES2=1 -s FULL_ES3=1 -s ALLOW_MEMORY_GROWTH=1")
endif()

