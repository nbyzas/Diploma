// Own
#include "main_widget.h"

// Project
#include "skinned_mesh.h"
#include "skinning_technique.h"
#include "pipeline.h"
#include "camera.h"
#include "kskeleton.h"

// Assimp
#include <assimp\Importer.hpp>      
#include <assimp\scene.h>			 
#include <assimp\postprocess.h>     

// Qt
#include <QtCore\QDebug>
#include <QtGui\QKeyEvent>
#include <QtGui\QOpenGLTexture>

// Standard C/C++
#include <cassert>
#include <iomanip>

MainWidget::MainWidget(QWidget *parent)
	: 
	QOpenGLWidget(parent),
	m_skinnedMesh(new SkinnedMesh())
{
	cout << "MainWidget class constructor start." << endl;
	
	m_VAO = 0;
	ZERO_MEM(m_Buffers);

	cout << "MainWidget class constructor end.\n" << endl;
}
MainWidget::~MainWidget()
{
	makeCurrent();
	unloadSkinnedMesh();
	doneCurrent();

	delete m_skinnedMesh;
	delete m_camera;
	delete m_technique;
	delete m_skinningTechnique;
	delete m_pipeline;
}
// This virtual function is called once before the first call to paintGL() or resizeGL().
void MainWidget::initializeGL()
{
	cout << "MainWidget initializeGL start." << endl;

	qDebug() << "Obtained format:" << format();
	initializeOpenGLFunctions();

	m_camera = new Camera();
	m_technique = new Technique();
	m_skinningTechnique = new SkinningTechnique();
	m_pipeline = new Pipeline();
	loadAxes();
	loadArrow();
	loadSkeleton();
	loadMeshJoints();
	loadCube(0.03);
	
	glEnable(GL_TEXTURE_2D);
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);               
	glClearDepth(1.0f);									
	glEnable(GL_POINT_SMOOTH);							
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	
	// glShadeModel(GL_SMOOTH); // #? deprecated
	setup();

	m_timer.setTimerType(Qt::PreciseTimer);
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(updateIndirect()));
	m_modeOfOperation = Mode::PLAYBACK;
	m_playbackInterval = m_ksensor->skeleton()->m_interpolationInterval * 1000;
	cout << "Playback interval: " << m_playbackInterval << endl;
	m_timer.setInterval(m_playbackInterval);
	m_timer.start();

	cout << "MainWidget initializeGL end." << endl;
}
void MainWidget::setup()
{
	cout << "MainWidget setup start." << endl;

	// Init SkinnedMesh
	loadSkinnedMesh("cmu_test");
	vector<QMatrix4x4> transforms;
	m_skinnedMesh->getBoneTransforms(transforms);
	m_skinnedMesh->initCorrectionVecs(m_ksensor->skeleton()->limbs());
	m_skinnedMeshOffset    = -m_skinnedMesh->getOffset();
	m_kinectSkeletonOffset = -m_ksensor->skeleton()->m_activeSequence->at(0).joints[JointType_SpineBase].position;

	// Init Pipeline
	//QVector3D spineBase = (m_ksensor->skeleton()->activeJoints())[JointType_SpineBase].position;
	//m_camera->setOffset(spineBase);
	cout << "Skinned mesh offset=" << toStringCartesian(m_skinnedMeshOffset).toStdString() << endl;
	m_camera->printInfo();
	m_pipeline->setCamera(m_camera->GetPos(), m_camera->GetTarget(), m_camera->GetUp());

	PerspectiveProjectionInfo perspectiveProjectionInfo;
	perspectiveProjectionInfo.fieldOfView = 60.f;
	perspectiveProjectionInfo.aspectRatio = m_camera->windowWidth() / m_camera->windowHeight();
	perspectiveProjectionInfo.nearPlane = 0.1f;
	perspectiveProjectionInfo.farPlane = 1000.f;
	m_pipeline->setPersProjInfo(perspectiveProjectionInfo);

	// Init Technique
	m_technique->initDefault();
	m_technique->enable();
	m_technique->setMVP(m_pipeline->getWVPtrans());
	m_technique->setSpecific(QMatrix4x4());

	// Init SkinningTechnique
	m_skinningTechnique->Init();
	m_skinningTechnique->enable();
	m_skinningTechnique->SetColorTextureUnit(0);
	DirectionalLight directionalLight;
	directionalLight.Color = QVector3D(1.f, 1.f, 1.f);
	directionalLight.AmbientIntensity = 0.7f;
	directionalLight.DiffuseIntensity = 0.9f;
	directionalLight.Direction = QVector3D(0.f, -1.f, 0.f);
	m_skinningTechnique->SetDirectionalLight(directionalLight);
	m_skinningTechnique->SetMatSpecularIntensity(0.0f);
	m_skinningTechnique->SetMatSpecularPower(0);
	m_skinningTechnique->setSkinning(true);
	m_skinningTechnique->setWVP(m_pipeline->getWVPtrans());
	for (uint i = 0; i < m_skinnedMesh->numBones(); i++) {
		m_skinningTechnique->setBoneVisibility(i, m_skinnedMesh->boneVisibility(i));
	}
	transformSkinnedMesh(true);

	cout << "MainWidget setup end." << endl;
}
// #? must enable corresponding shading technique before using each drawing function. bad design?
void MainWidget::paintGL()
{
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (m_modeOfOperation == Mode::CAPTURE) {
		m_ksensor->getBodyFrame();
	}
	else if (m_modeOfOperation == Mode::PLAYBACK){
		m_ksensor->skeleton()->setActiveJoints(m_activeFrame);
		m_skinnedMesh->setActiveCoordinates(m_activeFrame);
	}
	else {
		cout << "Unknown mode of operation." << endl;
	}
	m_time = m_skinnedMesh->timestamp(m_activeFrame);

	transformSkinnedMesh(false);

	// prepare pipeline for drawing skinned mesh related stuff
	if (m_defaultPose) m_pipeline->setWorldRotation(QQuaternion());
	else m_pipeline->setWorldRotation(m_skinnedMesh->pelvisRotation());
	if (m_defaultPose) m_pipeline->setWorldPosition(QVector3D());
	else m_pipeline->setWorldPosition(m_skinnedMesh->pelvisPosition() + m_skinnedMeshOffset);
	m_skinningTechnique->setWVP(m_pipeline->getWVPtrans());

	// enable skinning technique
	m_skinningTechnique->enable();

	// draw skinned mesh
	if (m_modeOfOperation == Mode::PLAYBACK && m_renderSkinnedMesh && m_skinnedMesh->m_successfullyLoaded) {
		drawSkinnedMesh();
	}

	// enable simple technique
	m_technique->enable();
	m_technique->setMVP(m_pipeline->getWVPtrans());

	// draw skinned mesh bone axes
	m_technique->setSpecific(m_skinnedMesh->boneGlobal(m_activeBone));
	if (m_renderAxes) drawAxes();
	
	// draw skinned mesh joints
	m_technique->setSpecific(QMatrix4x4());
	if (m_renderSkinnedMeshJoints) drawSkinnedMeshJoints();

	// draw basic axes
	m_technique->setMVP(m_pipeline->getVPtrans()); // only VP transformation! 
	m_technique->setSpecific(QMatrix4x4());
	if (m_renderAxes) drawAxes();

	// Prepare pipeline to draw kinect skeleton related stuff
	m_pipeline->setWorldRotation(QQuaternion());
	if (m_defaultPose) m_pipeline->setWorldPosition(QVector3D());
	else m_pipeline->setWorldPosition(m_kinectSkeletonOffset);
	m_technique->setMVP(m_pipeline->getWVPtrans());

	// draw kinect skeleton
	if (m_renderSkeleton) {
		drawSkeleton();
		for (uint i = 0; i < JointType_Count; i++) {
			m_technique->setSpecific(fromTranslation(m_ksensor->skeleton()->activeJoints()[i].position));
			drawCube();
		}
		QVector3D HipsMid = (m_ksensor->skeleton()->activeJoints()[JointType_HipLeft].position + m_ksensor->skeleton()->activeJoints()[JointType_HipRight].position) / 2;
		m_technique->setSpecific(fromTranslation(HipsMid));
		drawCube();
	}

	
	// draw arrow
	QVector3D leftHand = (m_ksensor->skeleton()->activeJoints())[JointType_HandLeft].position;
	QVector3D rightHand = (m_ksensor->skeleton()->activeJoints())[JointType_HandRight].position;
	QVector3D barDirection = rightHand - leftHand;
	QMatrix4x4 S = fromScaling(QVector3D(1, (rightHand - leftHand).length(), 1));
	QMatrix4x4 R = fromRotation(QQuaternion::rotationTo(QVector3D(0, 1, 0), barDirection));
	QMatrix4x4 T = fromTranslation(leftHand);
	m_technique->setSpecific(T * R * S);
	drawArrow();

	// calculate bar horizontal angle
	m_barAngle = ToDegrees(atan2(barDirection.y(), sqrt(pow(barDirection.x(), 2) + pow(barDirection.z(), 2))));
	
	// calculate bar speed
	static QVector3D previousBarPosition, currentBarPosition;
	currentBarPosition = (leftHand + rightHand) * 0.5;
	m_barSpeed = (currentBarPosition - previousBarPosition) / m_playbackInterval * 1000;
	previousBarPosition = currentBarPosition;

	// calculate knee angle
	QVector3D hipRight = (m_ksensor->skeleton()->activeJoints())[JointType_HipRight].position;
	QVector3D kneeRight = (m_ksensor->skeleton()->activeJoints())[JointType_KneeRight].position;
	QVector3D ankleRight = (m_ksensor->skeleton()->activeJoints())[JointType_AnkleRight].position;
	QVector3D kneeToHip = (hipRight - kneeRight).normalized();
	QVector3D kneeToAnkle = (ankleRight - kneeRight).normalized();
	m_kneeAngle = 180.f-ToDegrees(acos(QVector3D::dotProduct(kneeToHip, kneeToAnkle)));
	
	if (m_play && m_shouldUpdate) {
		if (++m_activeFrame > m_ksensor->skeleton()->m_activeSequence->size())  m_activeFrame = 0;
		m_shouldUpdate = false;
		calculateFPS();
	}
}
void MainWidget::calculateFPS()
{
	static clock_t ticksThisTime;
	static clock_t ticksLastTime = clock();
	static uint framesPassed = 0;

	ticksThisTime = clock();
	clock_t ticksPassed = ticksThisTime - ticksLastTime;
	double millisecondsPassed = ticksToMilliseconds(ticksPassed);

	framesPassed++;
	if (millisecondsPassed > 1000.) {
		m_fpsCount = framesPassed / (millisecondsPassed / 1000.);
		framesPassed = 0;
		ticksLastTime = ticksThisTime;
	}
}
void MainWidget::keyPressEvent(QKeyEvent *event)
{
	int key = event->key();
	switch (key) {
	case Qt::Key_Down:
	case Qt::Key_Up:
	case Qt::Key_Left:
	case Qt::Key_Right:
		m_camera->onKeyboardArrow(key, false);
		m_pipeline->setCamera(m_camera->GetPos(), m_camera->GetTarget(), m_camera->GetUp());
		m_skinningTechnique->enable();
		m_skinningTechnique->SetEyeWorldPos(m_camera->GetPos());
		m_skinningTechnique->setWVP(m_pipeline->getWVPtrans());
		m_technique->enable();
		m_technique->setMVP(m_pipeline->getVPtrans());
		break;
	case Qt::Key_0:
	case Qt::Key_1:
	case Qt::Key_2:
	case Qt::Key_3:
	case Qt::Key_4:
	case Qt::Key_5:
	case Qt::Key_6:
	case Qt::Key_7:
	case Qt::Key_8:
	case Qt::Key_9:
		m_skinnedMesh->flipParameter(key - Qt::Key_0);
		transformSkinnedMesh(true);
		break;
	case Qt::Key_Y:
		m_playbackInterval *= 1.2f; // decrease playback speed
		cout << "Playback interval: " << m_playbackInterval << endl;
		m_timer.setInterval(m_playbackInterval);
		break;
	case Qt::Key_U: 
		m_playbackInterval /= 1.2f; // increase playback speed
		cout << "Playback interval: " << m_playbackInterval << endl;
		m_timer.setInterval(m_playbackInterval);
		break;
	case Qt::Key_A:
		m_ksensor->skeleton()->processFrames();
		break;
	case Qt::Key_C:
		m_ksensor->skeleton()->calculateLimbLengths(*m_ksensor->skeleton()->m_activeSequence);
		m_ksensor->skeleton()->printLimbLengths();
		break;
	case Qt::Key_D:
		m_defaultPose = !m_defaultPose;
		cout << "Default pause " << (m_defaultPose ? "ON" : "OFF") << endl;
		break;
	case Qt::Key_G:
		if (!m_ksensor->getBodyFrame()) cout << "Could not update kinect data." << endl;
		break;
	case Qt::Key_J:
		m_ksensor->skeleton()->printActiveJoints();
		break;
	case Qt::Key_L:
		m_ksensor->skeleton()->loadFrameSequences();
		break;
	case Qt::Key_M:
		changeMode();
		break;
	case Qt::Key_N:
		m_ksensor->skeleton()->nextActiveSequence();
		break;
	case Qt::Key_P:
		if (m_play) {
			m_play = false;
			cout << "Play OFF" << endl;
			m_timer.stop();
		}
		else {
			m_play = true;
			cout << "Play ON" << endl;
			m_timer.start();
		}
		break;
	case Qt::Key_R:
		if (m_modeOfOperation==Mode::CAPTURE) m_ksensor->record();
		else cout << "Record does not work in this mode." << endl;
		break;
	case Qt::Key_S:
		m_ksensor->skeleton()->saveFrameSequences();
		break;
	case Qt::Key_Q:
		m_skinnedMesh->printInfo();
		break;
	case Qt::Key_T:
		m_ksensor->skeleton()->saveToTrc();
		break;
	case Qt::Key_Escape:
		event->ignore();	// event passed to MainWidget's parent (MainWindow)
		break;
	default:
		cout << "MainWidget: This key does not do anything." << endl;
		break;
	}
	update();
}
void MainWidget::mousePressEvent(QMouseEvent *event)
{
	m_lastMousePosition = event->pos();
}
void MainWidget::mouseMoveEvent(QMouseEvent *event)
{
	int dx = event->x() - m_lastMousePosition.x();
	int dy = event->y() - m_lastMousePosition.y();

	if (event->buttons() & Qt::LeftButton){
		if (event->modifiers() & Qt::ControlModifier) {
			m_camera->rotateRight(-dx);
		}
		else if (event->modifiers() & Qt::ShiftModifier) {
			m_camera->rotateUp(-dy);
		}
		else {
			m_camera->rotateRight(-dx);
			m_camera->rotateUp(-dy);
		}
	}
	m_lastMousePosition = event->pos();

	m_pipeline->setCamera(m_camera->GetPos(), m_camera->GetTarget(), m_camera->GetUp());
	m_skinningTechnique->enable();
	m_skinningTechnique->SetEyeWorldPos(m_camera->GetPos());
	m_skinningTechnique->setWVP(m_pipeline->getWVPtrans());
	m_technique->enable();
	m_technique->setMVP(m_pipeline->getVPtrans());
	update();
}
void MainWidget::wheelEvent(QWheelEvent *event)
{
	QPoint degrees = event->angleDelta() / 8;
	//cout << degrees.x() << " " << degrees.y() << endl;
	m_camera->onMouseWheel(degrees.y(), true);

	m_pipeline->setCamera(m_camera->GetPos(), m_camera->GetTarget(), m_camera->GetUp());
	m_skinningTechnique->enable();
	m_skinningTechnique->SetEyeWorldPos(m_camera->GetPos());
	m_skinningTechnique->setWVP(m_pipeline->getWVPtrans());
	m_technique->enable();
	m_technique->setMVP(m_pipeline->getVPtrans());
	update();
}
void MainWidget::transformSkinnedMesh(bool print)
{
	//if (!print) cout.setstate(std::ios_base::failbit);
	if (print) cout << "Transforming bones." << endl;
	vector<QMatrix4x4> transforms;
	m_skinnedMesh->getBoneTransforms(transforms); // update bone transforms from kinect
	m_skinningTechnique->enable();
	for (uint i = 0; i < transforms.size(); i++) {
		m_skinningTechnique->setBoneTransform(i, transforms[i]); // send transforms to vertex shader
	}
	//if (!print) cout.clear();
}
//*/

void MainWidget::setRenderAxes(bool state)
{
	if (m_renderAxes != state) {
		m_renderAxes = state;		
		update();
	}
}
void MainWidget::setRenderModel(bool state)
{
	if (m_renderSkinnedMesh != state) {
		m_renderSkinnedMesh = state;
		update();
	}
}
void MainWidget::setRenderSkeleton(bool state)
{
	if (m_renderSkeleton != state) {
		m_renderSkeleton = state;
		update();
	}
}
bool MainWidget::renderAxes() const
{
	return m_renderAxes;
}
bool MainWidget::renderSkinnedMesh() const
{
	return m_renderSkinnedMesh;
}
bool MainWidget::renderKinectSkeleton() const
{
	return m_renderSkeleton;
}
bool MainWidget::modelSkinning() const
{
	return m_modelSkinning;
}
QStringList MainWidget::modelBoneList() const
{
	QStringList qsl;
	for (const auto& it : m_skinnedMesh->boneMap()) qsl << QString::fromLocal8Bit(it.first.c_str());
	return qsl;
}
void MainWidget::setModelName(const QString &modelName)
{
	m_modelName = modelName;
}
void MainWidget::setModelSkinning(bool state)
{
	m_skinningTechnique->enable();
	m_skinningTechnique->setSkinning(state);
	update();
}
SkinnedMesh* MainWidget::skinnedMesh()
{
	return m_skinnedMesh;
}
SkinningTechnique* MainWidget::skinningTechnique()
{
	return m_skinningTechnique;
}
Technique* MainWidget::technique()
{
	return m_technique;
}
void MainWidget::unloadSkinnedMesh()
{
	for (uint i = 0; i < m_textures.size(); i++) {
		SAFE_DELETE(m_textures[i]);
	}

	if (m_Buffers[0] != 0) {
		glDeleteBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);
	}

	if (m_VAO != 0) {
		glDeleteVertexArrays(1, &m_VAO);
		m_VAO = 0;
	}
}
void MainWidget::updateIndirect()
{
	m_shouldUpdate = true;
	update(); // #? use QWidget->update or QWidget->repaint
}
void MainWidget::setActiveFrame(uint index)
{
	m_activeFrame = (uint)(index / 100.f * m_ksensor->skeleton()->m_activeSequence->size());
}
void MainWidget::changeMode()
{
		if (m_modeOfOperation == Mode::CAPTURE) {
			m_modeOfOperation = Mode::PLAYBACK;
			m_timer.setInterval(m_playbackInterval);
			cout << "Mode: PLAYBACK" << endl;
		}
		else {
			m_modeOfOperation = Mode::CAPTURE;
			m_timer.setInterval(m_captureInterval);
			cout << "Mode: CAPTURE" << endl;
		}
		m_timer.start();
}
void MainWidget::setKSensor(KSensor& ksensor)
{
	m_ksensor = &ksensor;
}
void MainWidget::setBoneAxes(const QString &boneName)
{
	uint boneId = m_skinnedMesh->findBoneId(boneName);
	assert(boneId < m_boneInfo.size());
	m_technique->enable();
	m_technique->setSpecific(m_skinnedMesh->boneGlobal(boneId));
}
void MainWidget::setActiveBone(const QString& boneName)
{
	m_activeBone = m_skinnedMesh->findBoneId(boneName);
}
bool MainWidget::loadSkinnedMesh(const string& basename)
{
	// Release the previously loaded mesh (if it exists)
	unloadSkinnedMesh();

	bool Ret = false;
	for (int i = 0; i < m_skinnedMesh->images().size(); i++) m_textures.push_back(new QOpenGLTexture(m_skinnedMesh->images()[i]));

	// Create the VAO
	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	// Create the buffers for the vertices attributes
	glGenBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);

#define POSITION_LOCATION    0
#define TEX_COORD_LOCATION   1
#define NORMAL_LOCATION      2
#define BONE_ID_LOCATION     3
#define BONE_WEIGHT_LOCATION 4

	const auto& Positions = m_skinnedMesh->positions();
	const auto& TexCoords = m_skinnedMesh->texCoords();
	const auto& Normals = m_skinnedMesh->normals();
	const auto& Bones = m_skinnedMesh->vertexBoneData();
	const auto& Indices = m_skinnedMesh->indices();

	// Generate and populate the buffers with vertex attributes and the indices
	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Positions[0]) * Positions.size(), &Positions[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(POSITION_LOCATION);
	glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(TexCoords[0]) * TexCoords.size(), TexCoords.constData(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(TEX_COORD_LOCATION);
	glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Normals[0]) * Normals.size(), &Normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(NORMAL_LOCATION);
	glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[BONE_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Bones[0]) * Bones.size(), &Bones[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(BONE_ID_LOCATION);
	glVertexAttribIPointer(BONE_ID_LOCATION, 4, GL_INT, sizeof(VertexBoneData), (const GLvoid*)0);
	glEnableVertexAttribArray(BONE_WEIGHT_LOCATION);
	glVertexAttribPointer(BONE_WEIGHT_LOCATION, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (const GLvoid*)16);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices[0]) * Indices.size(), &Indices[0], GL_STATIC_DRAW);

	// Make sure the VAO is not changed from the outside
	glBindVertexArray(0);

	Ret = GLCheckError();
	if (Ret) cout << "Successfully loaded SkinnedMesh to GPU (MainWidget)" << endl;
	return Ret;
}
void MainWidget::drawSkinnedMesh()
{
	glBindVertexArray(m_VAO);
	auto Entries = m_skinnedMesh->entries();
	for (uint i = 0; i < Entries.size(); i++) {
		const uint MaterialIndex = Entries[i].MaterialIndex;

		assert(MaterialIndex < m_textures.size());

		if (m_textures[MaterialIndex]) {
			m_textures[MaterialIndex]->bind();
		}
		glDrawElementsBaseVertex(GL_TRIANGLES,
			Entries[i].NumIndices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(uint) * Entries[i].BaseIndex),
			Entries[i].BaseVertex);
	}

	// Make sure the VAO is not changed from the outside    
	glBindVertexArray(0);
}
void MainWidget::loadArrow()
{
	const float head = 0.1f;
	const float colorRed = 255.f;
	const float colorBlue = 255.f;
	const float colorGreen = 255.f;

	const GLfloat vertices[] =
	{
		0         ,   0, 0, // start
		colorRed  , colorGreen, colorBlue,
		0         ,   1, 0, // end
		colorRed  , colorGreen, colorBlue,
		head      , 1 - head, 0, // +x
		colorRed  , colorGreen, colorBlue,
		-head     , 1 - head, 0, // -x
		colorRed  , colorGreen, colorBlue,
		0         , 1 - head, head, // +z
		colorRed  , colorGreen, colorBlue,
		0         , 1 - head, -head, // -z
		colorRed  , colorGreen, colorBlue,
	};

	const GLushort indices[] =
	{
		0, 1,
		1, 2,
		1, 3,
		1, 4,
		1, 5,
	};
	glGenVertexArrays(1, &m_arrowVAO);
	glBindVertexArray(m_arrowVAO);

	GLuint arrowIBO;
	glGenBuffers(1, &arrowIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, arrowIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices[0], GL_STATIC_DRAW);

	GLuint arrowVBO;
	glGenBuffers(1, &arrowVBO);
	glBindBuffer(GL_ARRAY_BUFFER, arrowVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, BUFFER_OFFSET(sizeof(GLfloat) * 3));

	glBindVertexArray(0); // break the existing vertex array object binding
}
void MainWidget::drawArrow()
{
	glBindVertexArray(m_arrowVAO);
	glDrawElements(GL_LINES, 10, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}
void MainWidget::loadAxes()
{
	GLfloat vertices[] =
	{
		0.f  , 0.f, 0.f,  // origin position
		0.f  , 0.f, 0.f, // origin color
		1.f  , 0.f, 0.f, // x axis position
		255.f, 0.f  , 0.f, // x axis color
		0.f  , 1.f, 0.f, // y axis position
		0.f  , 255.f, 0.f, // y axis color
		0.f  , 0.f, 1.f, // z axis position
		0.f  , 0.f  , 255.f // z axis color
	};

	GLushort indices[] =
	{
		0, 1,
		0, 2,
		0, 3
	};

	glGenVertexArrays(1, &m_axesVAO);
	glBindVertexArray(m_axesVAO);

	GLuint axesIBO;
	glGenBuffers(1, &axesIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axesIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices[0], GL_STATIC_DRAW);

	GLuint axesVBO;
	glGenBuffers(1, &axesVBO);
	glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, BUFFER_OFFSET(sizeof(GLfloat) * 3));

	glBindVertexArray(0);
}
void MainWidget::drawAxes()
{
	glBindVertexArray(m_axesVAO);
	glDrawElements(GL_LINES, 6, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}
void MainWidget::loadSkeleton()
{
	GLushort indices[] =
	{
		// core (4 parts)
		JointType_SpineBase    , JointType_SpineMid,
		JointType_SpineMid     , JointType_SpineShoulder,
		JointType_SpineShoulder, JointType_Neck,
		JointType_Neck         , JointType_Head,
		// left side (10 parts)	   
		JointType_SpineShoulder, JointType_ShoulderLeft,
		JointType_ShoulderLeft , JointType_ElbowLeft,
		JointType_ElbowLeft    , JointType_WristLeft,
		JointType_WristLeft    , JointType_HandLeft,
		JointType_HandLeft     , JointType_ThumbLeft,
		JointType_HandLeft     , JointType_HandTipLeft,
		JointType_SpineBase    , JointType_HipLeft,
		JointType_HipLeft      , JointType_KneeLeft,
		JointType_KneeLeft     , JointType_AnkleLeft,
		JointType_AnkleLeft    , JointType_FootLeft,
		// Right side (10 parts) 
		JointType_SpineShoulder, JointType_ShoulderRight,
		JointType_ShoulderRight, JointType_ElbowRight,
		JointType_ElbowRight   , JointType_WristRight,
		JointType_WristRight   , JointType_HandRight,
		JointType_HandRight    , JointType_ThumbRight,
		JointType_HandRight    , JointType_HandTipRight,
		JointType_SpineBase    , JointType_HipRight,
		JointType_HipRight     , JointType_KneeRight,
		JointType_KneeRight    , JointType_AnkleRight,
		JointType_AnkleRight   , JointType_FootRight
	};

	glGenVertexArrays(1, &m_skeletonVAO);
	glBindVertexArray(m_skeletonVAO);

	GLuint skeletonIBO;
	glGenBuffers(1, &skeletonIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skeletonIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices[0], GL_STATIC_DRAW);

	GLuint skeletonVBO;
	glGenBuffers(1, &m_skeletonVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_skeletonVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 2 * 3 * JointType_Count, NULL, GL_STREAM_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, BUFFER_OFFSET(sizeof(GLfloat) * 3));

	glBindVertexArray(0);
}
void MainWidget::updateSkeletonData()
{
	glBindBuffer(GL_ARRAY_BUFFER, m_skeletonVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(m_skeletonData), m_skeletonData);
}
void MainWidget::drawSkeleton()
{
	for (uint i = 0; i < JointType_Count; i++) {
		m_skeletonData[6 * i    ] = m_ksensor->skeleton()->activeJoints()[i].position.x();
		m_skeletonData[6 * i + 1] = m_ksensor->skeleton()->activeJoints()[i].position.y();
		m_skeletonData[6 * i + 2] = m_ksensor->skeleton()->activeJoints()[i].position.z();
		m_skeletonData[6 * i + 3] = (m_ksensor->skeleton()->activeJoints()[i].trackingState == TrackingState_NotTracked ? 255.f : 0.f);
		m_skeletonData[6 * i + 4] = (m_ksensor->skeleton()->activeJoints()[i].trackingState == TrackingState_Tracked ? 255.f : 0.f);
		m_skeletonData[6 * i + 5] = (m_ksensor->skeleton()->activeJoints()[i].trackingState == TrackingState_Inferred ? 255.f : 0.f);
	}
	updateSkeletonData();

	glBindVertexArray(m_skeletonVAO);
	glDrawElements(GL_LINES, 48, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}
void MainWidget::loadMeshJoints()
{
	glGenVertexArrays(1, &m_meshJointsVAO);
	glBindVertexArray(m_meshJointsVAO);

	GLuint m_meshJointsVBO;
	glGenBuffers(1, &m_meshJointsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_meshJointsVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 2 * 3 * NUM_BONES, NULL, GL_STREAM_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, BUFFER_OFFSET(sizeof(GLfloat) * 3));

	glBindVertexArray(0);
}
void MainWidget::updateSkinnedMeshJoints()
{
	glBindBuffer(GL_ARRAY_BUFFER, m_meshJointsVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(m_skinnedMeshJoints), m_skinnedMeshJoints);
}
void MainWidget::drawSkinnedMeshJoints()
{
	for (uint i = 0; i < NUM_BONES; i++) {
		m_skinnedMeshJoints[6 * i    ] = m_skinnedMesh->boneEndPosition(i).x();
		m_skinnedMeshJoints[6 * i + 1] = m_skinnedMesh->boneEndPosition(i).y();
		m_skinnedMeshJoints[6 * i + 2] = m_skinnedMesh->boneEndPosition(i).z();
		m_skinnedMeshJoints[6 * i + 3] = 0.f;
		m_skinnedMeshJoints[6 * i + 4] = 255.f;
		m_skinnedMeshJoints[6 * i + 5] = 0.f;	 
	}
	updateSkinnedMeshJoints();

	glBindVertexArray(m_meshJointsVAO);
	glDrawArrays(GL_POINTS, 0, NUM_BONES*60);
	glBindVertexArray(0);
}
void MainWidget::loadCube(float r)
{
	GLfloat vertices[] =
	{
		+r / 2, -r / 2, +r / 2, // 0
		+r / 2, +r / 2, +r / 2, // 1
		-r / 2, +r / 2, +r / 2, // 2
		-r / 2, -r / 2, +r / 2, // 3
		+r / 2, -r / 2, -r / 2, // 4
		+r / 2, +r / 2, -r / 2, // 5
		-r / 2, +r / 2, -r / 2, // 6
		-r / 2, -r / 2, -r / 2, // 7
		255.f, 255.f, 255.f,
		255.f, 255.f, 255.f,
		255.f, 255.f, 255.f,
		255.f, 255.f, 255.f,
		255.f, 255.f, 255.f,
		255.f, 255.f, 255.f,
		255.f, 255.f, 255.f,
		255.f, 255.f, 255.f
	};

	GLushort indices[] =
	{
		0, 1, 2, 3,
		0, 4, 5, 1,
		4, 7, 6, 5,
		7, 3, 2, 6,
		2, 1, 5, 6,
		0, 4, 7, 3
	};

	glGenVertexArrays(1, &m_skeletonCubesVAO);
	glBindVertexArray(m_skeletonCubesVAO);

	GLuint skeletonCubesIBO;
	glGenBuffers(1, &skeletonCubesIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skeletonCubesIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &m_skeletonCubesVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_skeletonCubesVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, BUFFER_OFFSET(sizeof(vertices) / 2));

	glBindVertexArray(0);
}
void MainWidget::drawCube()
{
	for (uint i = 0; i < JointType_Count; i++) {
		QVector3D color(0.f, 0.f, 0.f);
		if (m_ksensor->skeleton()->activeJoints()[i].trackingState == TrackingState_NotTracked) color.setX(255.f);
		else if (m_ksensor->skeleton()->activeJoints()[i].trackingState == TrackingState_Tracked) color.setY(255.f);
		else color.setZ(255.f);
		for (uint j = 0; j < 8; j++) {
			m_skeletonCubesColorData[3 * j + 0] = color.x();
			m_skeletonCubesColorData[3 * j + 1] = color.y();
			m_skeletonCubesColorData[3 * j + 2] = color.z();
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_skeletonCubesVBO);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(m_skeletonCubesColorData), sizeof(m_skeletonCubesColorData), m_skeletonCubesColorData);

	glBindVertexArray(m_skeletonCubesVAO);
	glDrawElements(GL_TRIANGLE_STRIP, 24, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}