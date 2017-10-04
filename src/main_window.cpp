// Own
#include "main_window.h"

// Project
#include "ui_main_window.h"

// Qt
#include "QtCore\QDir"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	ui = new Ui::MainWindow;
	ui->setupUi(this);

	setupObjects();
	setupConnections();
}
MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::loadActiveBoneInfo()
{
	ui->checkBox_boneVisible->setChecked(ui->openGLWidget->boneVisibility(ui->comboBox_activeBone->currentText()));
}
void MainWindow::setActiveBoneVisibility(bool state)
{
	ui->openGLWidget->setBoneVisibility(ui->comboBox_activeBone->currentText(), state);
}

void MainWindow::setupObjects()
{
	for (const auto& fi : QDir("models/", "*.dae").entryInfoList()) {
		ui->comboBox_activeModel->addItem(fi.baseName());
	}
	ui->openGLWidget->setModelName(ui->comboBox_activeModel->currentText());
	ui->checkBox_modelSkinning->setChecked(ui->openGLWidget->modelSkinning());

	ui->comboBox_activeBone->addItems(ui->openGLWidget->ModelBoneList());
	loadActiveBoneInfo();

	ui->checkBox_axes->setChecked(ui->openGLWidget->renderModel());
	ui->checkBox_model->setChecked(ui->openGLWidget->renderAxes());
}
void MainWindow::setupConnections()
{
	// active model
	connect(ui->comboBox_activeModel, SIGNAL(currentIndexChanged(QString)), ui->openGLWidget, SLOT(setModelName(QString)));
	// skinning on
	connect(ui->checkBox_modelSkinning, SIGNAL(toggled(bool)), ui->openGLWidget, SLOT(setModelSkinning(bool)));
	// flip bones visibility
	connect(ui->pushButton_flipBonesVisibility, SIGNAL(clicked()), ui->openGLWidget, SLOT(flipBonesVisibility()));
	connect(ui->pushButton_flipBonesVisibility, SIGNAL(clicked()), SLOT(loadActiveBoneInfo()));
	// active bone
	connect(ui->comboBox_activeBone, SIGNAL(currentIndexChanged(QString)), SLOT(loadActiveBoneInfo()));
	// bone visible
	connect(ui->checkBox_boneVisible, SIGNAL(toggled(bool)), SLOT(setActiveBoneVisibility(bool)));
	// sliders
	connect(ui->spinBox_xRot, SIGNAL(valueChanged(int)), ui->horizontalSlider_xRot, SLOT(setValue(int)));
	connect(ui->horizontalSlider_xRot, SIGNAL(valueChanged(int)), ui->spinBox_xRot, SLOT(setValue(int)));
	connect(ui->spinBox_yRot, SIGNAL(valueChanged(int)), ui->horizontalSlider_yRot, SLOT(setValue(int)));
	connect(ui->horizontalSlider_yRot, SIGNAL(valueChanged(int)), ui->spinBox_yRot, SLOT(setValue(int)));
	connect(ui->spinBox_zRot, SIGNAL(valueChanged(int)), ui->horizontalSlider_zRot, SLOT(setValue(int)));
	connect(ui->horizontalSlider_zRot, SIGNAL(valueChanged(int)), ui->spinBox_zRot, SLOT(setValue(int)));
	// render axes
	connect(ui->checkBox_axes, SIGNAL(toggled(bool)), ui->openGLWidget, SLOT(setRenderAxes(bool)));
	// render model
	connect(ui->checkBox_model, SIGNAL(toggled(bool)), ui->openGLWidget, SLOT(setRenderModel(bool)));
}