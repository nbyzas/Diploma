// Own
#include "technique.h"

// Project
#include "util.h"

// Standard C/C++
#include <stdio.h>
#include <string.h>
#include <iostream>

Technique::Technique()
{
    m_shaderProg = 0;	
}
Technique::~Technique()
{
    // Delete the intermediate shader objects that have been added to the program
    // The list will only contain something if shaders were compiled but the object itself
    // was destroyed prior to linking.
    for (ShaderObjList::iterator it = m_shaderObjList.begin() ; it != m_shaderObjList.end() ; it++)
    {
        glDeleteShader(*it);
    }

    if (m_shaderProg != 0)
    {
        glDeleteProgram(m_shaderProg);
        m_shaderProg = 0;
    }
}
bool Technique::Init()
{
	initializeOpenGLFunctions();
    m_shaderProg = glCreateProgram();

    if (m_shaderProg == 0) {
        printf("Error creating shader program\n");
        return false;
	}
	else {
		printf("Created shader program %d\n", m_shaderProg);
		return true;
	}
}
// Use this method to add shaders to the program. When finished - call finalize()
bool Technique::AddShader(GLenum ShaderType, const char* pFilename)
{
    std::string s;
    
    if (!readFile(pFilename, s)) {
        return false;
    }
    
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        printf("Error creating shader type %d\n", ShaderType);
        return false;
    }

    // Save the shader object - will be deleted in the destructor
    m_shaderObjList.push_back(ShaderObj);

    const GLchar* p[1];
    p[0] = s.c_str();
    GLint Lengths[1] = { (GLint)s.size() };

    glShaderSource(ShaderObj, 1, p, Lengths);

    glCompileShader(ShaderObj);

    GLint success;
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);

    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		printf("Error compiling '%s': '%s'\n", pFilename, InfoLog);
        //fprintf(stderr, "Error compiling '%s': '%s'\n", pFilename, InfoLog);
        return false;
    }

    glAttachShader(m_shaderProg, ShaderObj);
	printf("Added %s to shader program %d\n", pFilename, m_shaderProg);
    return true;
}
// After all the shaders have been added to the program call this function
// to link and validate the program.
bool Technique::Finalize()
{
    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };

    glLinkProgram(m_shaderProg);

    glGetProgramiv(m_shaderProg, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(m_shaderProg, sizeof(ErrorLog), NULL, ErrorLog);
		printf("Error linking shader program: '%s'\n", ErrorLog);
		//fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        return false;
	}

    glValidateProgram(m_shaderProg);
    glGetProgramiv(m_shaderProg, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(m_shaderProg, sizeof(ErrorLog), NULL, ErrorLog);
		printf("Invalid shader program: '%s'\n", ErrorLog);
        //fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
     //   return false;
    }
	
    // Delete the intermediate shader objects that have been added to the program
    for (ShaderObjList::iterator it = m_shaderObjList.begin() ; it != m_shaderObjList.end() ; it++) {
        glDeleteShader(*it);
    }

    m_shaderObjList.clear();
	
    return GLNoError();
}
void Technique::enable()
{
    glUseProgram(m_shaderProg);
}
bool Technique::initDefault()
{
	if (!Technique::Init()) {
		printf("Cannot initialize default technique\n");
		return false;
	}

	if (!AddShader(GL_VERTEX_SHADER, "shaders/simple.vert")) {
		printf("Cannot add simple vertex shader\n");
		return false;
	}

	if (!AddShader(GL_FRAGMENT_SHADER, "shaders/simple.frag")) {
		printf("Cannot add simple fragment shader\n");
		return false;
	}
	
	if (!Finalize()) {
		printf("Cannot finalize default shaders\n");
		return false;
	}

	m_locationMVP = GetUniformLocation("gMVP");
	cout << "Technique: gMVP location = " << m_locationMVP << endl;
	m_locationSpecific = GetUniformLocation("gSpecific");
	cout << "Technique: gSpecific location = " << m_locationSpecific << endl;

	return true;
}
void Technique::setMVP(const QMatrix4x4& MVP)
{
	glUniformMatrix4fv(m_locationMVP, 1, GL_TRUE, MVP.transposed().data());
}
GLint Technique::GetUniformLocation(const char* pUniformName)
{
    GLuint location = glGetUniformLocation(m_shaderProg, pUniformName);

    if (location == INVALID_UNIFORM_LOCATION) {
		printf("Warning! Unable to get the location of uniform '%s' (Shader program: %d)\n", pUniformName, m_shaderProg);
        //fprintf(stderr, "Warning! Unable to get the location of uniform '%s'\n", pUniformName);
    }

    return location;
}
GLint Technique::GetProgramParam(GLint param)
{
    GLint ret;
    glGetProgramiv(m_shaderProg, param, &ret);
    return ret;
}
void Technique::setSpecific(const QMatrix4x4& MVP)
{
	glUniformMatrix4fv(m_locationSpecific, 1, GL_TRUE, MVP.transposed().data());
}