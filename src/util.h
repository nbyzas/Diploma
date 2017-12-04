#ifndef UTIL_H
#define	UTIL_H

// Qt
#include <QtGui\QOpenGLFunctions_3_3_Compatibility>

// Standard C/C++
using namespace std;

#define OPENGL_FUNCTIONS QOpenGLFunctions_3_3_Compatibility
#define ZERO_MEM(a) memset(a, 0, sizeof(a))
#define ZERO_MEM_VAR(var) memset(&var, 0, sizeof(var))
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define INVALID_UNIFORM_LOCATION 0xffffffff
#define INVALID_OGL_VALUE 0xffffffff
#define SAFE_DELETE(p) if (p) { delete p; p = NULL; }
#define GLCheckError() (glGetError() == GL_NO_ERROR)
#define GLPrintError()																					\
{																										\
	int error=glGetError();																				\
	switch (error) {																					\
		case GL_INVALID_OPERATION:				  printf("INVALID_OPERATION\n");			    break;	\
		case GL_INVALID_ENUM:					  printf("INVALID_ENUM\n");					    break;	\
		case GL_INVALID_VALUE:					  printf("INVALID_VALUE\n");				    break;	\
		case GL_OUT_OF_MEMORY:				      printf("OUT_OF_MEMORY\n");				    break;	\
		case GL_NO_ERROR:						  printf("No error\n");							break;	\
		}																								\
}
#define GLExitIfError                                                           \
{                                                                               \
    GLenum Error = glGetError();                                                \
                                                                                \
    if (Error != GL_NO_ERROR) {                                                 \
        printf("OpenGL error in %s:%d: 0x%x\n", __FILE__, __LINE__, Error);     \
        exit(0);                                                                \
    }                                                                           \
}

#define SNPRINTF _snprintf_s
#define VSNPRINTF vsnprintf_s
#define RANDOM rand
#define SRANDOM srand((unsigned)time(NULL))

bool ReadFile(const char* fileName, std::string& outFile);
uint Mod(uint start, uint edge, int step);
inline bool getBit(int number, int position)
{
	return (number >> position) & 0;
}
float wrapAngle(float angle, float limit);
QString toString(const QQuaternion &q);
QString toStringEulerAngles(const QQuaternion &q);
QString toStringAxisAngle(const QQuaternion &q);
QString toStringCartesian(const QVector3D &v);
QString toStringSpherical(const QVector3D &v);
double ticksToMilliseconds(clock_t ticks);

#endif	/* UTIL_H */
