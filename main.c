
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <GLES2/gl2.h>

// WhileTrue
#define viba for(;;)

int gl_shaderstuff() {
	const char* vertsrc = "#version 460 core\n"
	"layout (location = 0) in vec2 aPos;"
	"layout (location = 1) in vec3 aColor;"
	"out vec3 fColor;"
	"void main()"
	"{ gl_Position = vec4(aPos.x, -aPos.y, 1.0, 1.0); fColor = aColor; }";
	const char* fragsrc = "#version 460 core\n"
	"out vec4 FragColor;"
	"in vec3 fColor;"
	"void main() { FragColor = vec4(fColor, 1.0); }";
	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vert, 1, &vertsrc, NULL);
	glShaderSource(frag, 1, &fragsrc, NULL);
	glCompileShader(vert);
	glCompileShader(frag);
	int res = 0;
	glGetShaderiv(frag, GL_INFO_LOG_LENGTH, &res);
	printf("%d == 0 frag shader\n", res);
	GLuint prog = glCreateProgram();
	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	glLinkProgram(prog);
	glDeleteShader(vert);
	glDeleteShader(frag);
	return prog;
}

void clip_toeq_persp(int c, float* data) {
	for (int i = 0; i < c; i += 3) {
		data[i    ] /= data[i + 2];
		data[i + 1] /= data[i + 2];
	}
}

int next_hillbert(int clen, char* data) {
	int o = 1 + clen;
	for (int i = o; i < 4 * clen + 3; ++i) data[i] = data[i - o];
	data[clen] = 2;
	data[clen * 2 + 1] = 1;
	data[clen * 3 + 2] = 0;
	for (int i = 0; i < clen; ++i)
		data[i] ^= 3;
	for (int i = clen * 3 + 3; i < clen * 4 + 3; ++i)
		data[i] ^= 1;
	return 4 * clen + 3;
}

int fill_hillbert(char* data) {
	data[0] = 2;
	data[1] = 1;
	data[2] = 0;
	int l;
	l = next_hillbert(3, data);
	l = next_hillbert(l, data);
	l = next_hillbert(l, data);
	l = next_hillbert(l, data);
	l = next_hillbert(l, data);
	return l;
}

void fill_hillpoints(int len, char* data, float* points, float* tri) {
	points[0] = 0;
	points[1] = 0;
	float mapx[] = {0, 1, 0, -1};
	float mapy[] = {-1, 0, 1, 0};
	for (int i = 1; i < len; ++i) {
		points[i * 2    ] = points[i * 2 - 2] + mapx[data[i - 1]];
		points[i * 2 + 1] = points[i * 2 - 1] + mapy[data[i - 1]];
	}
	for (int i = 1; i < len - 1; ++i) {
		float ox, oy;
		ox = 0.3;
		oy = 0.3;
		switch (data[i - 1] << 2 | data[i]) {
		case 0b0110: oy *= -1; break; // right to down
		case 0b1001: oy *= -1; break; // down to right
		case 0b1011: break; // down to left
		case 0b1110: break; // left to down
		case 0b1100: ox *= -1; break; // left to up
		case 0b0011: ox *= -1; break; // up to left
		case 0b0100: ox *= -1; oy *= -1; break; // right to up
		case 0b0001: ox *= -1; oy *= -1; break; // up to right
		case 0b0000: ox *= -1; oy = 0; break; // down
		case 0b0101: oy *= -1; ox = 0; break; // right
		case 0b1010: oy = 0; break; // up
		case 0b1111: ox = 0; break; // left
		}
		tri[i * 4    ] = points[i * 2    ] + ox;
		tri[i * 4 + 1] = points[i * 2 + 1] + oy;
		tri[i * 4 + 2] = points[i * 2    ] - ox;
		tri[i * 4 + 3] = points[i * 2 + 1] - oy;
	}
	tri[0] = tri[1] = tri[2] = tri[3] = 0;
	tri[4 * len - 4] = tri[4 * len - 2] = 63;
	tri[4 * len - 3] = tri[4 * len - 1] = 0;
	for (int i = 0; i < 2 * len; ++i) points[i]=(points[i]*2/63-1)*0.95;
	for (int i = 0; i < 4 * len; ++i) tri[i]=(tri[i]*2/63-1)*0.95;
}

void fill_hilltricolor(int len, float* tri, float* tricolor) {
	float colors[] = {1.0, 0.3, 1.0, 0.7, 0.3, 1.0, 0.3};
	for (int i = 0; i < len * 2; ++i) {
		tricolor[i * 5    ] = tri[i * 2    ];
		tricolor[i * 5 + 1] = tri[i * 2 + 1];
		tricolor[i * 5 + 2] = 0;//(float) rand() / RAND_MAX;//colors[i * 7 / len / 2];
		tricolor[i * 5 + 3] = tricolor[i * 5 + 2];
		tricolor[i * 5 + 4] = tricolor[i * 5 + 2];
//		tricolor[i * 5 + 3] = (float) i / len / 2;
//		tricolor[i * 5 + 4] = (float) i / len / 2;
	}
}

void time_hilltricolor(int len, int min, float* tricolor,
		float r, float g, float b) {
	int i = 2 * len * min / (60 * 24 * 7);
	tricolor[i * 5 + 2] = r;
	tricolor[i * 5 + 3] = g;
	tricolor[i * 5 + 4] = b;
}

void event_hilltricolor(int len, int startmin, int endmin, float* tricolor,
		float r, float g, float b) {
	int startp = 2 * len * startmin / (60 * 24 * 7);
	int endp = 2 * len * endmin / (60 * 24 * 7);
	for (int i = startp; i < endp; ++i) {
		tricolor[i * 5 + 2] = r;
		tricolor[i * 5 + 3] = g;
		tricolor[i * 5 + 4] = b;
	}
}

int str_to_dmin(char* str) { // pa00
	int m = str[0] == 'p' ? 12 : 0;
	if (str[1] >= 'a')
		m += (str[1] + 10 - 'a');
	else
		m += (str[1] - '0');
	m *= 60;
	m += (str[2] - '0') * 10;
	m += (str[3] - '0');
	return m;
}

int day_to_min(char day) {
	switch (day) {
	case 'U': return 0;
	case 'M': return 60 * 24;
	case 'T': return 60 * 24 * 2;
	case 'W': return 60 * 24 * 3;
	case 'R': return 60 * 24 * 4;
	case 'F': return 60 * 24 * 5;
	case 'A': return 60 * 24 * 6;
	}
	return 0;
}

float parse_colorboi(char* colorboi) {
	float flot;
	if (colorboi[0] < 'a')
		flot = colorboi[0] - '0';
	else
		flot = colorboi[0] - 'a' + 10;
	flot *= 16;
	if (colorboi[1] < 'a')
		flot += colorboi[1] - '0';
	else
		flot += colorboi[1] - 'a' + 10;
	return flot / 255;
}

void put_event(int len, char* event, float* tricolor) {
	float r = parse_colorboi(event);
	float g = parse_colorboi(event + 2);
	float b = parse_colorboi(event + 4);
	int wi = 7; // wi = start of weekdays

	int ai = wi + 1;
	while (event[ai] != ' ') ++ai;
	++ai; // ai = start of time a

	int time_a = str_to_dmin(event + ai);
	if (event[ai + 4] == 0) { // one point
		for (int d = wi; d < ai - 1; ++d)
			time_hilltricolor(len, time_a + day_to_min(event[d]), tricolor,
				r,g,b);
	}
	else {
		int bi = ai + 5;
		int bd = str_to_dmin(event + bi);
		for (int d = wi; d < ai - 1; ++d) {
			int dm = day_to_min(event[d]);
			event_hilltricolor(len, time_a + dm, bd + dm, tricolor, r,g,b);
		}
		printf("%d %d\n", time_a, bd);
	}
}

int main() {
	char hilldata[4095];
	int hillen = fill_hillbert(hilldata);
	printf("%d\n", hillen);
	float hillpoints[4096 * 2];
	float hilltri[4096 * 4];
	float hilltricolor[4096 * 10]; // x y r g b, two points for each point
	fill_hillpoints(4096, hilldata, hillpoints, hilltri);
//	for (int i = 0; i < 15; ++i)
//		printf("%f %f %d\n",hillpoints[i*2], hillpoints[i*2 + 1], hilldata[i]);
	fill_hilltricolor(4096, hilltri, hilltricolor);
	put_event(4096, "ff0000 MWF aa05 ab00", hilltricolor); // 231
	put_event(4096, "ff7000 TR aa05 ab25", hilltricolor); // 222
	put_event(4096, "ffff00 MW p115 p235", hilltricolor); // 111
	put_event(4096, "ffffff TF p010 p200", hilltricolor); // 271
	put_event(4096, "0000ff TR p250 p410", hilltricolor); // 261
	put_event(4096, "777777 UMTWRFA a000 a630", hilltricolor); // sleep
	put_event(4096, "777777 UMTWRFA p900 pc00", hilltricolor); // sleep
	time_t t = time(NULL);
	struct tm local_time = *localtime(&t);
	time_hilltricolor(4096, local_time.tm_wday * 24 * 60
			+ local_time.tm_hour * 60
			+ local_time.tm_min, hilltricolor, 1.0f, 1.0f, 1.0f);
//	printf("...\n%f %f\n", hillpoints[8190], hillpoints[8191]);
//	printf("%d >= 0, SDL_Init\n", SDL_Init(SDL_INIT_VIDEO));
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_Window* winp = SDL_CreateWindow(
		"Ekriv", 0, 0, 600, 600,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_GLContext* conp = SDL_GL_CreateContext(winp);
	printf("OpenGL version %s\n"
	       "GLSL version %s\n"
	       "GL vendor %s\n"
	       "GL renderer %s\n",
	       glGetString(GL_VERSION),
	       glGetString(GL_SHADING_LANGUAGE_VERSION),
	       glGetString(GL_VENDOR),
	       glGetString(GL_RENDERER));
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	printf("OpenGL version %d.%d\n", major, minor);
	glEnable(GL_DEBUG_OUTPUT);
	GLuint prog = gl_shaderstuff();
	glUseProgram(prog);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(hilltricolor),
		hilltricolor, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
		5 * sizeof(float), NULL);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
		5 * sizeof(float), (void*) (2 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	viba {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_q) {
				goto end;
			}
		}
		glClearColor(0.3, 0.3, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
//		glDrawArrays(GL_LINES, 0, 3);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4096 * 2);
		SDL_GL_SwapWindow(winp);
		SDL_Delay(100);
	}

	end:
	glDeleteBuffers(1, &vbo);
	SDL_GL_DeleteContext(conp);
	SDL_Quit();
	return 0;
}
