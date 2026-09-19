// Run from the repository root. Uses a hidden window, real OpenGL, and no audio playback.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

// Report expected shader failures without opening a modal dialog during the probe.
static int probeMessageBox(HWND, LPCSTR text, LPCSTR, UINT) {
	std::printf("Shader diagnostic: %s\n", text);
	return IDOK;
}
#undef MessageBox
#define MessageBox probeMessageBox
#define EDITOR_CONTROLS 1
#define AUDIO_SHAUDIO 2
#define AUDIO_TYPE AUDIO_SHAUDIO
#define XRES 1920
#define YRES 1080
#ifndef USE_MIPMAPS
#define USE_MIPMAPS 0
#endif
#include "../src/debug.h"
#include "../src/feedback.h"
#ifndef LEV4K_SHADER_HEADER
#define LEV4K_SHADER_HEADER "../src/shaders/fragment.inl"
#endif
#include LEV4K_SHADER_HEADER

int pidMain, pidPost, pidMusic;
bool AudioNeedReset;

static void require(bool condition, const char* message) {
	if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

static bool reload(std::string source) {
	return replaceShaderPrograms(&source[0]);
}

static void readHistory(unsigned int index, float* pixel) {
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[index]);
	glReadPixels(XRES / 2, YRES / 2, 1, 1, GL_RGBA, GL_FLOAT, pixel);
}

static void drawFrame(int time) {
	glViewport(0, 0, XRES, YRES);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frameTextures[frameIndex ^ 1]);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[frameIndex]);
	glUseProgram(pidMain);
	glUniform1i(glGetUniformLocation(pidMain, "sb1"), 0);
	glUniform1i(glGetUniformLocation(pidMain, "m"), time);
	glRects(-1, -1, 1, 1);
	#if USE_MIPMAPS
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, frameTextures[frameIndex]);
		glGenerateMipmap(GL_TEXTURE_2D);
	#endif
	frameIndex ^= 1;
}

int main() {
	HWND window = CreateWindowA("static", "Lev4K feedback probe", WS_POPUP, 0, 0, 64, 64, NULL, NULL, NULL, NULL);
	HDC dc = GetDC(window);
	PIXELFORMATDESCRIPTOR format = {};
	format.nSize = sizeof(format); format.nVersion = 1;
	format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	format.iPixelType = PFD_TYPE_RGBA; format.cColorBits = 32;
	require(window && dc && SetPixelFormat(dc, ChoosePixelFormat(dc, &format), &format), "window/pixel format");
	HGLRC context = wglCreateContext(dc);
	require(context && wglMakeCurrent(dc, context), "OpenGL context");
	std::printf("GPU: %s\nOpenGL: %s\nUSE_MIPMAPS=%d\n", glGetString(GL_RENDERER), glGetString(GL_VERSION), USE_MIPMAPS);

	// Texture and framebuffer names deliberately differ, as they may with audio enabled.
	GLuint otherTextures[3], otherFbo;
	glGenTextures(3, otherTextures);
	glGenFramebuffers(1, &otherFbo);
	InitFrameHistory();
	float pixel[4];
	for (int i = 0; i < 2; ++i) {
		readHistory(i, pixel);
		for (int c = 0; c < 4; ++c) require(pixel[c] == 0, "initial history must be zero");
	}

	std::ifstream input("src/shaders/fragment.frag", std::ios::binary);
	require(input.good(), "run from the repository root");
	std::string raw((std::istreambuf_iterator<char>(input)), {});
	std::string lf, crlf;
	for (char c : raw) if (c != '\r') lf += c;
	for (char c : lf) { if (c == '\n') crlf += '\r'; crlf += c; }
	require(reload(lf), "raw LF visual/post/music passes");
	require(reload(crlf), "raw CRLF visual/post/music passes");
	require(reload(fragment_frag), "minified visual/post/music passes");
	require(AudioNeedReset, "successful reload requests audio regeneration");

	// The production selector still uses offset 22 in the generated source.
	std::string compact = fragment_frag;
	require(compact[22] == '1', "generated selector offset");
	for (int pass = 1; pass <= 3; ++pass) {
		compact[22] = '0' + pass;
		const char* source = compact.c_str();
		GLuint program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &source);
		GLint linked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &linked);
		require(linked == GL_TRUE, "production shader linking");
		glDeleteProgram(program);
	}

	// Exercise the real sample: both frames must be finite, visible, and reproducible after reset.
	require(reload(lf), "reload sample");
	ClearFrameHistory();
	drawFrame(22050);
	std::vector<float> first(XRES * YRES * 4), repeated(first.size());
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[frameIndex ^ 1]);
	glReadPixels(0, 0, XRES, YRES, GL_RGBA, GL_FLOAT, first.data());
	float brightest = 0;
	for (size_t i = 0; i < first.size(); i += 4) {
		require(std::isfinite(first[i]), "finite sample output");
		if (first[i] > brightest) brightest = first[i];
	}
	require(brightest > 0.001f, "sample produces visible pixels");
	drawFrame(44100);
	ClearFrameHistory();
	drawFrame(22050);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[frameIndex ^ 1]);
	glReadPixels(0, 0, XRES, YRES, GL_RGBA, GL_FLOAT, repeated.data());
	for (size_t i = 0; i < first.size(); ++i) require(std::fabs(first[i] - repeated[i]) < 0.00001f, "reset reproduces the first frame");

	const std::string fixture =
		"#version 330\n#define m1 main\nuniform sampler2D sb1;out vec4 o1;"
		"void m1(){o1=mix(vec4(1,.5,.25,1),texture(sb1,gl_FragCoord.xy/vec2(1920,1080)),.95);}"
		"void m2(){o1=texture(sb1,gl_FragCoord.xy/vec2(1920,1080))*.5;}"
		"void m3(){o1=vec4(0);}";
	require(reload(fixture), "load deterministic feedback fixture");
	ClearFrameHistory();
	for (int frame = 1; frame <= 4; ++frame) {
		drawFrame(0);
		readHistory(frameIndex ^ 1, pixel);
		float expected = 1.0f - std::pow(0.95f, static_cast<float>(frame));
		require(std::fabs(pixel[0] - expected) < 0.00001f, "feedback recurrence");
		require(std::fabs(pixel[1] - expected * 0.5f) < 0.00001f, "feedback channels");
	}
	// Present into a separate target; post-processing must not overwrite the history.
	glBindFramebuffer(GL_FRAMEBUFFER, otherFbo);
	glBindTexture(GL_TEXTURE_2D, otherTextures[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES, YRES, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, otherTextures[0], 0);
	require(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "presentation target");
	glBindTexture(GL_TEXTURE_2D, frameTextures[frameIndex ^ 1]);
	glUseProgram(pidPost);
	glUniform1i(glGetUniformLocation(pidPost, "sb1"), 0);
	glRects(-1, -1, 1, 1);
	float presented[4];
	glReadPixels(XRES / 2, YRES / 2, 1, 1, GL_RGBA, GL_FLOAT, presented);
	for (int c = 0; c < 4; ++c) require(std::fabs(presented[c] - pixel[c] * 0.5f) < 0.00001f, "post-processing reads the latest frame");

	// Missing a later entry point must preserve ALL previously linked programs and history.
	glUseProgram(0);
	int oldMain = pidMain, oldPost = pidPost, oldMusic = pidMusic;
	AudioNeedReset = false;
	for (const char* entry : { "void m1()", "void m2()", "void m3()" }) {
		std::string broken = fixture;
		broken.replace(broken.find(entry), strlen(entry), "void missing()");
		require(!reload(broken), "invalid pass rejected");
		require(pidMain == oldMain && pidPost == oldPost && pidMusic == oldMusic, "failed reload is atomic");
		require(!AudioNeedReset, "failed reload leaves audio intact");
	}
	require(!reload("#version 330\nvoid main(){}"), "missing selector rejected");
	float afterFailure[4];
	readHistory(frameIndex ^ 1, afterFailure);
	for (int c = 0; c < 4; ++c) require(pixel[c] == afterFailure[c], "failed reload preserves history");
	require(reload(fixture), "recover with a valid reload");
	auto isProgram = reinterpret_cast<PFNGLISPROGRAMPROC>(wglGetProcAddress("glIsProgram"));
	require(!isProgram(oldMain) && !isProgram(oldPost) && !isProgram(oldMusic), "successful reload releases old programs");

	ClearFrameHistory();
	for (int i = 0; i < 2; ++i) {
		readHistory(i, pixel);
		for (int c = 0; c < 4; ++c) require(pixel[c] == 0, "reset clears both textures");
	}
	require(glGetError() == GL_NO_ERROR, "no OpenGL errors");
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(context);
	ReleaseDC(window, dc);
	DestroyWindow(window);
	std::puts("PASS: initialization, shaders, rendering, feedback, post-processing, reset, and reload recovery.");
}
