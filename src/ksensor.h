#ifndef KSENSOR_H
#define KSENSOR_H

// Project
#include "math_3d.h"
#include "util.h"
#include "kskeleton.h"

// Kinect
#include <Kinect.h>

// Qt
#include <QtCore/QFile>

// Standard C/C++
#include <string>
#include <vector>

class KSensor
{
public:
	KSensor();
	~KSensor();
	bool init();
	bool connect();
	KSkeleton *skeleton();
	bool getBodyData();
	void processBodyFrameData(IBody** bodies);
	void calculateFPS();

	// Start/Stop frame recording in KSkeleton class
	void record();

	// Playback
	void setSkeletonActiveFrame(uint progressPercent);

private:
	KJoint m_Joints[JointType_Count];
	IKinectSensor *m_sensor = nullptr;
	IBodyFrameSource *m_source = nullptr;
	IBodyFrameReader *m_reader = nullptr;

	QFile m_captureLog;
	QTextStream m_forLog;

	// Frame counter and timestamps
	bool m_lastAttemptFailed = false;
	uint m_consecutiveFails = 0;

	uint m_acquiredFrames = 0;
	INT64 m_firstRelativeTime;
	INT64 m_lastRelativeTime;

	uint m_acceptedFrames = 0;
	double m_totalTime = 0;
	clock_t m_ticksNow;
	clock_t m_ticksBefore;

	// FPS counter
	double m_fps = 0.;

	KSkeleton m_skeleton;
	KJoint m_leftFootStance;
	KJoint m_rightFootStance;
};
#endif /* SENSOR_H */