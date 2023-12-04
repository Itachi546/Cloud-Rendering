#pragma once 

#include <assert.h>
#include <string>
#include <vector>
#include <stdint.h>
#include <glad/glad.h>

/*****************************************************************************************************************************************/
// Helper Structs

struct DrawElementsIndirectCommand {
	GLuint count_;
	GLuint instanceCount_;
	GLuint firstIndex_;
	GLuint baseVertex_;
	GLuint baseInstance_;
};

extern float gOGLVersion;

/*************************************************************************************************************************************************/
// Shader

class GLShader
{
public:

	explicit GLShader(const char* filename);

	GLShader(GLenum type, const char* shaderCode);

	inline GLenum getType() { return type_; }

	inline GLuint getHandle() { return handle_; }

	~GLShader() { glDeleteShader(handle_); }

private:
	GLenum        type_;
	GLuint        handle_;
};

/*************************************************************************************************************************************************/

class GLProgram
{
public:

	GLProgram() : handle_(0) {}

	void init(GLShader a, GLShader b);

	void init(GLShader a, GLShader b, GLShader c);

	void destroy() { glDeleteProgram(handle_); }

	void use() const { glUseProgram(handle_); }

	void setTexture(std::string name, int binding, unsigned int textureId);

	void setInt(std::string name, int val);

	void setFloat(std::string name, float val);

	void setVec2(std::string name, float x, float y);

	void setVec3(std::string name, float* val);

	void setVec4(std::string name, float* val);

	void setMat4(std::string name, float* data);

	GLint getAttribLocation(std::string name) {
		return glGetAttribLocation(handle_, name.c_str());
	}

protected:

	GLuint        handle_;
};

/*************************************************************************************************************************************************/

class GLComputeProgram
{
public:

	GLComputeProgram() : handle_(0) {}

	void init(GLShader shader);

	void setTexture(int binding, uint32_t textureId, GLenum access, GLenum format);

	void setInt(std::string name, int val);

	void setFloat(std::string name, float val);

	void setVec3(std::string name, float* val);

	void dispatch(uint32_t workGroupX, uint32_t workGroupY, uint32_t workGroupZ) const;

	void use() const { glUseProgram(handle_); }

	void destroy() const { glDeleteProgram(handle_); }
private:
	GLuint       handle_;
};

/*************************************************************************************************************************************************/

struct GLBuffer
{
	GLBuffer() : handle(0) {}

	void init(void* data, uint32_t size, GLbitfield flags);

	void destroy() {
		glDeleteBuffers(1, &handle);
	}

	GLuint handle;

}; 

/*************************************************************************************************************************************************/
struct TextureCreateInfo {
	uint32_t width = 256;
	uint32_t height = 256;
	uint32_t depth = 1;
	GLuint format = GL_RGBA;
	GLuint internalFormat = GL_RGBA8;
	GLuint target = GL_TEXTURE_2D;
	GLuint dataType = GL_UNSIGNED_BYTE;
	bool generateMipmap = false;

	GLuint wrapType = GL_CLAMP_TO_EDGE;
	GLuint minFilterType = GL_LINEAR;
	GLuint magFilterType = GL_LINEAR;
};

inline void InitializeDepthTexture(TextureCreateInfo* createInfo,
	uint32_t width,
	uint32_t height,
	uint32_t format) {
	createInfo->width = width;
	createInfo->height = height;
	createInfo->format = format;
	createInfo->internalFormat = GL_DEPTH_COMPONENT;
	createInfo->target = GL_TEXTURE_2D;
	createInfo->minFilterType = createInfo->magFilterType = GL_NEAREST;
}

struct GLTexture {
	void init(TextureCreateInfo* createInfo, void* data = nullptr);

	GLuint handle;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
};

/*************************************************************************************************************************************************/
struct Attachment {
	uint32_t index;
	TextureCreateInfo* attachmentInfo;
};

struct GLFramebuffer {
	void init(const std::vector<Attachment>& attachments, TextureCreateInfo* depthAttachment);

	void bind() {
		glBindFramebuffer(GL_FRAMEBUFFER, handle);
	}

	void unbind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void setViewport(uint32_t width, uint32_t height) {
		glViewport(0, 0, width, height);
	}

	void clear(bool clearDepth = false) {
		GLenum clearFlag = GL_COLOR_BUFFER_BIT;
		if (clearDepth) clearFlag |= GL_DEPTH_BUFFER_BIT;
		glClear(clearFlag);
	}

	void setClearColor(float r, float g, float b, float a) {
		glClearColor(r, g, b, a);
	}

	void destroy();

	GLuint handle;
	std::vector<GLuint> attachments;
	GLuint depthAttachment;
	
};