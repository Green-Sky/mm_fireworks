cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(mm_fireworks CXX)

add_library(particles STATIC
	./render_tasks/particles.hpp
	./render_tasks/particles.cpp
	#./services/organizer_scene.hpp
	#./services/organizer_scene.cpp
)

target_link_libraries(particles
	engine
	common_components
	opengl_renderer_s
	imgui_service
)

##########################

add_executable(fireworks1
	./fireworks1.cpp
)

target_link_libraries(fireworks1
	engine
	common_components
	organizer_scene

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

