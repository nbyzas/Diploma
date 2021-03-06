#ifndef UTIL_H
#define	UTIL_H

// Assimp
#include <assimp\matrix4x4.h>

// Qt
#include <QtGui\QOpenGLFunctions_3_3_Core>
#include <QtGui\QOpenGLFunctions_3_3_Compatibility>
#include <QtGui\QMatrix4x4>
#include <QtGui\QVector3D>
#include <QtGui\QGenericMatrix>

// Standard C/C++
using namespace std;

#define OPENGL_FUNCTIONS QOpenGLFunctions_3_3_Compatibility
#define ZERO_MEM(a) memset(a, 0, sizeof(a))
#define ZERO_MEM_VAR(var) memset(&var, 0, sizeof(var))
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define INVALID_UNIFORM_LOCATION 0xffffffff
#define INVALID_OGL_VALUE 0xffffffff
#define SAFE_DELETE(p) if (p) { delete p; p = NULL; }
#define GLNoError() (glGetError() == GL_NO_ERROR)
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
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#define SNPRINTF _snprintf_s
#define VSNPRINTF vsnprintf_s
#define RANDOM rand
#define SRANDOM srand((unsigned)time(NULL))
#define PI 3.141592653589
#define ToRadians(x) (float)(((x) * PI / 180.f))
#define ToDegrees(x) (float)(((x) * 180.f / PI))

bool readFile(const char* fileName, std::string& outFile);
inline bool getBit(int number, int position)
{
	return (number >> position) & 0;
}
float wrapAngle(float angle, float limit);

QString toStringCartesian(const QVector3D& v);
QString toStringSpherical(const QVector3D& v);
QString toString(const QQuaternion& q);
QString toStringEulerAngles(const QQuaternion& q);
QString toStringAxisAngle(const QQuaternion& q);
QString toString(const QMatrix4x4& m);

double ticksToMilliseconds(clock_t ticks);
std::ostream& operator<<(std::ostream& out, const QVector3D& v);
QTextStream& operator<<(QTextStream& out, const QVector3D& v);
QQuaternion extractQuaternion(const QMatrix4x4& m);
QMatrix4x4 toQMatrix(const aiMatrix4x4& aiMat);
QMatrix4x4 fromScaling(const QVector3D& scaling);
QMatrix4x4 fromScaling(float xScaling, float yScaling, float zScaling);
QMatrix4x4 fromRotation(const QQuaternion& q);
QMatrix4x4 fromRotation(float xRotation, float yRotation, float zRotation); // Euler angles in degrees
QMatrix4x4 fromTranslation(const QVector3D& v);
QMatrix4x4 fromTranslation(float xTranslation, float yTranslation, float zTranslation);
QMatrix4x4 getScalingPart(const QMatrix4x4& m);
QMatrix4x4 getRotationPart(const QMatrix4x4& m);
QMatrix4x4 getTranslationPart(const QMatrix4x4& m);
#endif	/* UTIL_H */
