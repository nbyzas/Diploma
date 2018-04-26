#ifndef MAIN_WIDGET_H
#define MAIN_WIDGET_H

// Project
class KSensor;
class Camera;
class SkinnedMesh;
class Technique;
class SkinningTechnique;
class Pipeline;
#include "opensim_model.h"
#include "util.h"

// Kinect
#include <Kinect.h>

// Qt
#include <QtCore\QTimer>
#include <QtWidgets\QOpenGLWidget>
#include <QtGui\QOpenGLShaderProgram>
QT_FORWARD_DECLARE_CLASS(QOpenGLTexture);

// Standard C/C++
//#include <vector>

class MainWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
	Q_OBJECT
public:
	MainWidget(QWidget *parent = Q_NULLPTR);
	~MainWidget();
	
	// get functions
	SkinnedMesh* skinnedMesh();
	SkinningTechnique* skinningTechnique();
	Technique* technique();
	bool renderAxes() const;
	bool renderSkinnedMesh() const;
	bool renderKinectSkeleton() const;
	bool modelSkinning() const;
	QStringList modelBoneList() const;

	void transformSkinnedMesh(bool print);

	// #todo #? make private and create get/set functions
	const uint m_captureInterval = 10; // in milliseconds
	uint m_playbackInterval; // in milliseconds
	uint m_fpsCount = 0; // for counting fps
	
	float m_kneeAngle;
	float m_barAngle;
	QVector3D m_barSpeed;
	double m_time;

public slots:
	void setRenderAxes(bool state);
	void setRenderModel(bool state);
	void setRenderSkeleton(bool state);
	void setModelName(const QString& modelName);
	void setModelSkinning(bool state);
	void setBoneAxes(const QString& boneName);
	void setKSensor(KSensor& ksensor);
	void changeMode();
	void updateIndirect(); // #todo replace it with something else
	void setActiveFrame(uint index);
	void setActiveBone(const QString& modelName);

protected:
	void initializeGL();
	void paintGL();
	void calculateFPS();
	void keyPressEvent(QKeyEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;

private:
	enum class Mode
	{
		CAPTURE,
		PLAYBACK
	};

	Mode m_modeOfOperation;

	KSensor* m_ksensor;

	SkinnedMesh* m_skinnedMesh;
	Camera* m_camera;
	Technique* m_technique;
	SkinningTechnique* m_skinningTechnique;
	Pipeline* m_pipeline;
	OpenSimModel m_osm;
	
	QPoint m_lastMousePosition;

	bool m_renderAxes = true;
	bool m_renderSkeleton = true;
	bool m_renderSkinnedMesh = true;
	bool m_renderSkinnedMeshJoints = true;

	bool m_play = true;	

	
	void setup();

	// Skinned mesh
	bool m_modelSkinning = true;
	QString m_modelName;
	bool loadSkinnedMesh(const string& basename);
	void unloadSkinnedMesh();
	enum VB_TYPES {
		INDEX_BUFFER,
		POS_VB,
		NORMAL_VB,
		TEXCOORD_VB,
		BONE_VB,
		NUM_VBs
	};
	GLuint m_VAO;
	GLuint m_Buffers[NUM_VBs];
	vector<QOpenGLTexture*> m_textures;
	void drawSkinnedMesh();
	bool m_defaultPose = false;

	// Skinned mesh joint dots
#define NUM_BONES 10
	GLuint m_meshJointsVAO;
	GLuint m_meshJointsVBO;
	void loadMeshJoints();
	void updateSkinnedMeshJoints();
	float m_skinnedMeshJoints[2 * 3 * NUM_BONES];
	void drawSkinnedMeshJoints();


	// Kinect skeleton
	GLuint m_skeletonVAO;
	GLuint m_skeletonVBO;
	void loadSkeleton();
	void updateSkeletonData();
	float m_skeletonData[2 * 3 * JointType_Count]; // 2 attributes x 3 components x JointType_Count joints
	void drawSkeleton();

	GLuint m_arrowVAO;
	void loadArrow();
	void drawArrow();

	GLuint m_axesVAO;
	void loadAxes();
	void drawAxes();

	GLuint m_skeletonCubesVAO;
	GLuint m_skeletonCubesVBO;
	float m_skeletonCubesColorData[3 * 8];
	void loadCube(float r);
	void drawCube();

	uint m_activeBone = 0;
	uint m_activeFrame = 0;

	QTimer m_timer;
	bool m_shouldUpdate = false;
	QVector3D m_skinnedMeshOffset;
	QVector3D m_kinectSkeletonOffset;
};

#endif /* MAIN_WIDGET_H */
