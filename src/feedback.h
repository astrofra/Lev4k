// LastFrameBuffer: render into one texture while sampling the other.
// Kept separate from the music framebuffer and textures.
static GLuint frameTextures[2];
static GLuint frameBuffers[2];
static unsigned int frameIndex;

__forceinline void ClearFrameHistory()
{
	glActiveTexture(GL_TEXTURE0);
	glClearColor(0, 0, 0, 0);
	for (int i = 0; i < 2; ++i) {
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[i]);
		glClear(GL_COLOR_BUFFER_BIT);
		#if USE_MIPMAPS
			glBindTexture(GL_TEXTURE_2D, frameTextures[i]);
			glGenerateMipmap(GL_TEXTURE_2D);
		#endif
	}
	frameIndex = 0;
}

__forceinline void InitFrameHistory()
{
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(2, frameTextures);
	glGenFramebuffers(2, frameBuffers);
	for (int i = 0; i < 2; ++i) {
		glBindTexture(GL_TEXTURE_2D, frameTextures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES, YRES, 0, GL_RGBA, GL_FLOAT, NULL);
		#if USE_MIPMAPS
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		#else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		#endif
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameTextures[i], 0);
		#ifdef EDITOR_CONTROLS
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				MessageBox(NULL, "Cannot create the frame-history render target.", "LastFrameBuffer", MB_OK | MB_ICONERROR);
				ExitProcess(1);
			}
		#endif
	}
	ClearFrameHistory();
}
