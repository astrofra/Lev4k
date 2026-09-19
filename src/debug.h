// This header contains some useful functions for debugging OpenGL.
// Remember to disable them when building your final releases.

#include <windows.h>
#include <GL/gl.h>
#include "glext.h"
#include "gldefs.h"
#include "timeapi.h"

static GLchar* getErrorString(GLenum errorCode)
{
	if (errorCode == GL_NO_ERROR) {
		return (GLchar*) "No error";
	}
	else if (errorCode == GL_INVALID_VALUE) {
		return (GLchar*) "Invalid value";
	}
	else if (errorCode == GL_INVALID_ENUM) {
		return (GLchar*) "Invalid enum";
	}
	else if (errorCode == GL_INVALID_OPERATION) {
		return (GLchar*) "Invalid operation";
	}
	else if (errorCode == GL_STACK_OVERFLOW) {
		return (GLchar*) "Stack overflow";
	}
	else if (errorCode == GL_STACK_UNDERFLOW) {
		return (GLchar*) "Stack underflow";
	}
	else if (errorCode == GL_OUT_OF_MEMORY) {
		return (GLchar*) "Out of memory";
	}
	return (GLchar*) "Unknown";
}

static void assertGlError(const char* error_message)
{
	const GLenum ErrorValue = glGetError();
	if (ErrorValue == GL_NO_ERROR) return;

	const char* APPEND_DETAIL_STRING = ": %s\n";
	const size_t APPEND_LENGTH = strlen(APPEND_DETAIL_STRING) + 1;
	const size_t message_length = strlen(error_message);
	MessageBox(NULL, error_message, getErrorString(ErrorValue), 0x00000000L);
	ExitProcess(0);
}

static bool shaderDebug(const char* shader, GLenum type, bool kill_on_failure = true, const char* filename = "")
{
	if (!shader) return false;

	// try and compile the shader 
	int result;
	const int debugid = glCreateShader(type);
	glShaderSource(debugid, 1, &shader, 0);
	glCompileShader(debugid);
	
	// get compile result
	glGetShaderiv(debugid, GL_COMPILE_STATUS, &result);
	if(result == GL_FALSE)
	{	
		// display compile log on failure
		char info[2048];
		glGetShaderInfoLog(debugid, 2047, NULL, (char*)info);
		MessageBox(NULL, info, filename, 0x00000000L);
		if(kill_on_failure)
		{
			ExitProcess(0);
		}
		else
		{
			return false;
		}
	}
	else
	{
		glDeleteShader(debugid);
		return true;
	}
}

#define STRINGIFY2(x) #x // Thanks sooda!
#define STRINGIFY(x) STRINGIFY2(x)
#define CHECK_ERRORS() assertGlError(STRINGIFY(__LINE__))

#ifdef EDITOR_CONTROLS
	#include <stdlib.h>
	#include <stdio.h>

	static DWORD lastLoad;

	char* updateShader(const char* filename)
	{
		FILE* file = fopen(filename, "rb");
		if (!file) return NULL;
		fseek(file, 0, SEEK_END);
		long inputSize = ftell(file);
		rewind(file);
		if (inputSize < 0) { fclose(file); return NULL; }
		char* shaderString = static_cast<char*>(calloc(inputSize + 1, sizeof(char)));
		if (shaderString && fread(shaderString, 1, inputSize, file) != static_cast<size_t>(inputSize)) {
			free(shaderString);
			shaderString = NULL;
		}
		fclose(file);
		return shaderString;
	}

	// Compile all passes before replacing any live program. LF and CRLF are both valid.
	bool replaceShaderPrograms(char* source)
	{
		char* selector = strstr(source, "#define m1 main");
		if (!selector) {
			MessageBox(NULL, "Missing #define m1 main shader selector.", "Shader reload", MB_OK | MB_ICONERROR);
			return false;
		}
		selector += strlen("#define m");
		extern int pidMain;
		extern int pidPost;
		#if AUDIO_TYPE == AUDIO_SHAUDIO
			extern int pidMusic;
			#if SHAUDIO_REVERB
				extern int pidMusicReverb;
			#endif
		#endif
		int* targets[] = { &pidMain, &pidPost,
			#if AUDIO_TYPE == AUDIO_SHAUDIO
				&pidMusic,
				#if SHAUDIO_REVERB
					&pidMusicReverb,
				#endif
			#endif
		};
		const int count = sizeof(targets) / sizeof(targets[0]);
		GLuint programs[count] = {};
		for (int i = 0; i < count; ++i) {
			*selector = '1' + i;
			const char* passSource = source;
			programs[i] = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &passSource);
			GLint linked = GL_FALSE;
			if (programs[i]) glGetProgramiv(programs[i], GL_LINK_STATUS, &linked);
			if (!linked) {
				char info[2048] = "Cannot create shader program.";
				if (programs[i]) glGetProgramInfoLog(programs[i], sizeof(info), NULL, info);
				printf("Shader pass m%d: %s\n", i + 1, info);
				MessageBox(NULL, info, "Shader reload failed; keeping previous programs", MB_OK | MB_ICONERROR);
				for (int j = 0; j <= i; ++j) if (programs[j]) glDeleteProgram(programs[j]);
				*selector = '1';
				return false;
			}
		}
		*selector = '1';
		for (int i = 0; i < count; ++i) {
			if (*targets[i]) glDeleteProgram(*targets[i]);
			*targets[i] = programs[i];
		}
		#if AUDIO_TYPE == AUDIO_SHAUDIO
			extern bool AudioNeedReset;
			AudioNeedReset = true;
		#endif
		return true;
	}

	bool refreshShaders(bool force)
	{
		DWORD now = timeGetTime();
		if (!force && (!(GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState('S')) || now - lastLoad <= 200)) return false;
		// Allow the editor to finish writing the file before loading it.
		if (!force) Sleep(100);
		lastLoad = timeGetTime();
#if EDITOR_RELEASE
		char* newSource = updateShader("./fragment.frag");
#else
		#if DEBUG_USE_MINIFIEDSHADER
			extern char* fragment_frag;
			char* newSource = _strdup(fragment_frag);
		#else
			char* newSource = updateShader("./src/shaders/fragment.frag");
		#endif
#endif
		if (!newSource) {
			MessageBox(NULL, "Cannot read the shader source. Check the working directory.", "Shader reload", MB_OK | MB_ICONERROR);
			return false;
		}
		bool replaced = replaceShaderPrograms(newSource);
		free(newSource);
		return replaced;
	}
#endif
