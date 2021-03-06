// Own
#include "main_window.h"

// Project
#include "skinned_mesh.h"
#include "skinning_technique.h"
#include "ui_main_window.h"

// Qt
#include <QtCore\QDir>
#include <QtCore\QTimer>
#include <QtWidgets\QAction>
#include <QtWidgets\QMenu>
#include <QtWidgets\QLCDNumber>
#include <QKeyEvent>
#include <QDesktopWidget>

// Standard C/C++
#include <iomanip>

MainWindow::MainWindow(QWidget *parent)
	: 
	QMainWindow(parent)
{
	cout << "MainWindow class constructor start." << endl;
	
	ui = new Ui::MainWindow;
	ui->setupUi(this);
	m_sensor = ui->openGLWidget->ksensor();
	setupObjects();
	setupConnections();
	//resize(QDesktopWidget().availableGeometry(this).size()*0.5);
	
	cout << "MainWindow class constructor end.\n" << endl;
}
MainWindow::~MainWindow()
{
	delete ui;
}
void MainWindow::loadActiveBoneInfo()
{
	const QString &boneName = ui->comboBox_activeBone->currentText();
	if (boneName.isEmpty()) return;
	ui->checkBox_boneVisible->setChecked(ui->openGLWidget->skinnedMesh()->boneVisibility(boneName));
	loadActiveBoneRotationX();
	loadActiveBoneRotationY();
	loadActiveBoneRotationZ();
	ui->openGLWidget->setActiveBone(boneName);
}
void MainWindow::setActiveBoneVisible(bool state)
{
	const QString &boneName = ui->comboBox_activeBone->currentText();
	if (boneName.isEmpty()) return;
	uint boneId = ui->openGLWidget->skinnedMesh()->findBoneId(boneName);
	ui->openGLWidget->skinnedMesh()->setBoneVisibility(boneName, state);
	ui->openGLWidget->skinningTechnique()->enable();
	ui->openGLWidget->skinningTechnique()->setBoneVisibility(boneId, state);
	ui->openGLWidget->update();
}
void MainWindow::setActiveBoneFocused(bool state)
{
	const QString &boneName = ui->comboBox_activeBone->currentText();
	if (boneName.isEmpty()) return;
	uint boneId = ui->openGLWidget->skinnedMesh()->findBoneId(boneName);
	ui->openGLWidget->skinningTechnique()->enable();
	for (uint i = 0; i < ui->openGLWidget->skinnedMesh()->numBones(); i++) {
		if (i != boneId) {
			ui->openGLWidget->skinnedMesh()->setBoneVisibility(i, !state);
			ui->openGLWidget->skinningTechnique()->setBoneVisibility(i, !state);
		}
	}
	ui->openGLWidget->update();
}
void MainWindow::setActiveBoneRotationX(int value)
{
	const QString &boneName = ui->comboBox_activeBone->currentText();
	if (boneName.isEmpty()) return;
	ui->openGLWidget->skinnedMesh()->setBoneRotationX(boneName, value);
	ui->openGLWidget->update();
	loadActiveBoneRotationX();
	ui->openGLWidget->update();
}
void MainWindow::setActiveBoneRotationY(int value)
{
	const QString &boneName = ui->comboBox_activeBone->currentText();
	if (boneName.isEmpty()) return;
	ui->openGLWidget->skinnedMesh()->setBoneRotationY(boneName, value);
	ui->openGLWidget->update();
	loadActiveBoneRotationY();
	ui->openGLWidget->update();
}
void MainWindow::setActiveBoneRotationZ(int value)
{
	const QString &boneName = ui->comboBox_activeBone->currentText();
	if (boneName.isEmpty()) return;
	ui->openGLWidget->skinnedMesh()->setBoneRotationZ(boneName, value);
	ui->openGLWidget->update();
	loadActiveBoneRotationZ();
	ui->openGLWidget->update();
}
void MainWindow::printActiveBoneTransforms() const
{
	const QString &boneName = ui->comboBox_activeBone->currentText();
	if (boneName.isEmpty()) return;
	cout << ui->openGLWidget->skinnedMesh()->boneTransformInfo(boneName).toStdString() << endl;
}
void MainWindow::togglePlayback()
{
	ui->openGLWidget->setIsPaused(!ui->openGLWidget->isPaused());
	ui->pushButton_playStartStop->setText(ui->openGLWidget->isPaused() ? "Play" : "Pause");
}
void MainWindow::keyPressEvent(QKeyEvent *event)
{
	cout << "MainWindow saw this keyboard event" << endl;
	int key = event->key();
	switch (key) {
	case Qt::Key_Escape:
		close(); // implicitly makes the application quit by generating close event
		break;
	default:
		cout << "MainWindow: This key does not do anything." << endl;
		break;
	}	
}
void MainWindow::loadActiveBoneRotationX()
{
	const QString &boneName = ui->comboBox_activeBone->currentText();
	if (boneName.isEmpty()) return;
	float rotX = ui->openGLWidget->skinnedMesh()->boneRotationX(boneName);
	ui->horizontalSlider_xRot->blockSignals(true);
	ui->spinBox_xRot->blockSignals(true);
	ui->horizontalSlider_xRot->setValue(rotX);
	ui->spinBox_xRot->setValue(rotX);
	ui->horizontalSlider_xRot->blockSignals(false);
	ui->spinBox_xRot->blockSignals(false);
}
void MainWindow::loadActiveBoneRotationY()
{
	const QString &boneName = ui->comboBox_activeBone->currentText();
	if (boneName.isEmpty()) return;
	float rotY = ui->openGLWidget->skinnedMesh()->boneRotationY(boneName);
	ui->horizontalSlider_yRot->blockSignals(true);
	ui->spinBox_yRot->blockSignals(true);
	ui->horizontalSlider_yRot->setValue(rotY);
	ui->spinBox_yRot->setValue(rotY);
	ui->horizontalSlider_yRot->blockSignals(false);
	ui->spinBox_yRot->blockSignals(false);
}
void MainWindow::loadActiveBoneRotationZ()
{
	const QString &boneName = ui->comboBox_activeBone->currentText();
	if (boneName.isEmpty()) return;
	float rotZ = ui->openGLWidget->skinnedMesh()->boneRotationZ(boneName);
	ui->horizontalSlider_zRot->blockSignals(true);
	ui->spinBox_zRot->blockSignals(true);
	ui->horizontalSlider_zRot->setValue(rotZ);
	ui->spinBox_zRot->setValue(rotZ);
	ui->horizontalSlider_zRot->blockSignals(false);
	ui->spinBox_zRot->blockSignals(false);
}
void MainWindow::setupObjects()
{
	cout << "MainWindow: Setting up objects" << endl;

	// Main Status
	ui->radioButton_capture->setChecked(ui->openGLWidget->captureEnabled());
	ui->radioButton_playback->setChecked(!ui->openGLWidget->captureEnabled());
	ui->checkBox_athlete->setChecked(ui->openGLWidget->athleteEnabled());
	ui->checkBox_trainer->setChecked(ui->openGLWidget->trainerEnabled());
	ui->comboBox_motionType->addItems(ui->openGLWidget->motionTypeList());
	ui->comboBox_motionType->setCurrentIndex(ui->openGLWidget->activeMotionType());
	
	// Skeleton
	ui->comboBox_activeJoint->addItems(ui->openGLWidget->skeletonJointList());

	// Model
	ui->checkBox_modelSkinning->setChecked(ui->openGLWidget->modelSkinning());
	ui->comboBox_activeBone->addItems(ui->openGLWidget->modelBoneList());
	loadActiveBoneInfo();

	// Render
	ui->checkBox_axes->setChecked(ui->openGLWidget->axesDrawing());
	ui->checkBox_model->setChecked(ui->openGLWidget->skinnedMeshDrawing());
	ui->checkBox_skeleton->setChecked(ui->openGLWidget->kinectSkeletonDrawing());
	ui->checkBox_floor->setChecked(ui->openGLWidget->floorDrawing());
	ui->checkBox_barbell->setChecked(ui->openGLWidget->barbellDrawing());
	ui->checkBox_tips->setChecked(ui->openGLWidget->tipsDrawing());

	// Playback controls
	ui->pushButton_playStartStop->setText(ui->openGLWidget->isPaused() ? "Play" : "Pause");

	m_guiTimer = new QTimer(this);
	m_guiTimer->setTimerType(Qt::CoarseTimer);
	m_guiTimer->setInterval(1000);
}
void MainWindow::setupConnections()
{
	cout << "MainWindow: Setting up connections" << endl;
	
	// Main Status
	// only need to connect one of the two radio buttons
	connect(ui->radioButton_capture, SIGNAL(toggled(bool)), ui->openGLWidget, SLOT(setCaptureEnabled(bool)));
	connect(ui->checkBox_athlete, SIGNAL(toggled(bool)), ui->openGLWidget, SLOT(setAthleteEnabled(bool)));
	connect(ui->checkBox_trainer, SIGNAL(toggled(bool)), ui->openGLWidget, SLOT(setTrainerEnabled(bool)));
	connect(ui->comboBox_motionType, SIGNAL(currentIndexChanged(int)), ui->openGLWidget, SLOT(setActiveMotionType(int)));

	// Skeleton
	connect(ui->comboBox_activeJoint, SIGNAL(currentIndexChanged(int)), ui->openGLWidget, SLOT(setActiveJointId(int)));

	// Model
	connect(ui->checkBox_modelSkinning          , SIGNAL(toggled(bool))               , ui->openGLWidget, SLOT(setModelSkinning(bool)));
	connect(ui->comboBox_activeBone             , SIGNAL(currentIndexChanged(QString)), SLOT(loadActiveBoneInfo()));
	connect(ui->comboBox_activeBone             , SIGNAL(currentIndexChanged(QString)), ui->openGLWidget, SLOT(setActiveBone(QString)));
	connect(ui->checkBox_boneVisible            , SIGNAL(clicked(bool))               , SLOT(setActiveBoneVisible(bool)));
	connect(ui->checkBox_boneFocused            , SIGNAL(clicked(bool))               , SLOT(setActiveBoneFocused(bool)));
	connect(ui->pushButton_info                 , SIGNAL(clicked())                   , SLOT(printActiveBoneTransforms()));
	connect(ui->horizontalSlider_xRot           , SIGNAL(valueChanged(int))           , SLOT(setActiveBoneRotationX(int)));
	connect(ui->horizontalSlider_yRot           , SIGNAL(valueChanged(int))           , SLOT(setActiveBoneRotationY(int)));
	connect(ui->horizontalSlider_zRot           , SIGNAL(valueChanged(int))           , SLOT(setActiveBoneRotationZ(int)));
	connect(ui->spinBox_xRot                    , SIGNAL(valueChanged(int))           , SLOT(setActiveBoneRotationX(int)));
	connect(ui->spinBox_yRot                    , SIGNAL(valueChanged(int))           , SLOT(setActiveBoneRotationY(int)));
	connect(ui->spinBox_zRot                    , SIGNAL(valueChanged(int))           , SLOT(setActiveBoneRotationZ(int)));
	
	// Render
	connect(ui->checkBox_axes, SIGNAL(toggled(bool)), ui->openGLWidget, SLOT(setAxesDrawing(bool)));
	connect(ui->checkBox_model, SIGNAL(toggled(bool)), ui->openGLWidget, SLOT(setSkinnedMeshDrawing(bool)));
	connect(ui->checkBox_skeleton, SIGNAL(toggled(bool)), ui->openGLWidget, SLOT(setKinectSkeletonDrawing(bool)));
	connect(ui->checkBox_floor, SIGNAL(toggled(bool)), ui->openGLWidget, SLOT(setFloorDrawing(bool)));
	connect(ui->checkBox_barbell, SIGNAL(toggled(bool)), ui->openGLWidget, SLOT(setBarbellDrawing(bool)));
	connect(ui->checkBox_tips, SIGNAL(toggled(bool)), ui->openGLWidget, SLOT(setTipsDrawing(bool)));

	// Motion Playback
	connect(ui->pushButton_playStartStop, SIGNAL(clicked()), this, SLOT(togglePlayback()));
	connect(ui->horizontalSlider_progressPercent, SIGNAL(sliderMoved(int)), ui->openGLWidget, SLOT(setActiveMotionProgress(int)));
	connect(ui->horizontalSlider_progressPercent, SIGNAL(sliderMoved(int)), this, SLOT(updateActiveFrameInfo()));
	connect(ui->openGLWidget, SIGNAL(frameChanged(int)), ui->horizontalSlider_progressPercent, SLOT(setValue(int)));
	connect(ui->openGLWidget, SIGNAL(frameChanged(int)), this, SLOT(updateActiveFrameInfo()));
	connect(ui->horizontalSlider_offset, SIGNAL(valueChanged(int)), ui->openGLWidget, SLOT(setGeneralOffset(int)));

	// gui timer
	connect(m_guiTimer, SIGNAL(timeout()), this, SLOT(updateInfo()));

	m_guiTimer->start();
}
void MainWindow::updateInfo()
{
	QString jointVelocity;
	QTextStream qts(&jointVelocity);
	qts.setRealNumberPrecision(2);
	qts.setRealNumberNotation(QTextStream::FixedNotation);
	qts.setNumberFlags(QTextStream::ForceSign);
	QVector3D jv = ui->openGLWidget->activeJointVelocity();
	qts << "Velocity: " << jv.x() << " " << jv.y() << " " << jv.z() << flush;
	ui->label_jointVelocity->setText(jointVelocity);

	QString jointAngle("Angle: " + QString::number(ui->openGLWidget->activeJointAngle(), 'f', 1));
	ui->label_jointAngle->setText(jointAngle);

	QString barbellDisplacement;
	QTextStream qts2(&barbellDisplacement);
	qts2.setRealNumberPrecision(2);
	qts2.setRealNumberNotation(QTextStream::FixedNotation);
	qts2.setNumberFlags(QTextStream::ForceSign);
	QVector3D bd = ui->openGLWidget->activeBarbellDisplacement();
	qts2 << "Displacement: " << bd.x() << " " << bd.y() << " " << bd.z() << flush;
	ui->label_barbellDisplacement->setText(barbellDisplacement);

	QString barbellVelocity;
	QTextStream qts3(&barbellVelocity);
	qts3.setRealNumberPrecision(2);
	qts3.setRealNumberNotation(QTextStream::FixedNotation);
	qts3.setNumberFlags(QTextStream::ForceSign);
	QVector3D bv = ui->openGLWidget->activeBarbellVelocity();
	qts3 << "Velocity: " << bv.x() << " " << bv.y() << " " << bv.z() << flush;
	ui->label_barbellVelocity->setText(barbellVelocity);

	QString barbellAngle("Angle: " + QString::number(ui->openGLWidget->activeBarbellAngle(), 'f', 1));
	ui->label_barbellAngle->setText(barbellAngle);
}
void MainWindow::updateActiveFrameInfo()
{
	QString index("Index: " + QString::number((int)ui->openGLWidget->m_activeFrameIndex));;
	ui->label_index->setText(index);

	QString time("Time: " + QString::number(ui->openGLWidget->m_activeFrameTimestamp, 'f', 3));;
	ui->label_time->setText(time);
}
