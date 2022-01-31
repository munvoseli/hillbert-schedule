all:
	gcc main.c `pkg-config --cflags --libs opengl sdl2`
