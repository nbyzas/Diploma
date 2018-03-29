// Own
#include "kskeleton.h"

// Qt
#include <QtCore/QFile>

// Standard C/C++
#include <iomanip>

QDataStream& operator<<(QDataStream& out, const KJoint& joint)
{
	out << joint.position << joint.trackingState;
	return out;
}
QDataStream& operator>>(QDataStream& in, KJoint& joint)
{
	in >> joint.position >> joint.trackingState;
	return in;
}
QDataStream& operator<<(QDataStream& out, const KFrame& frame)
{
	out << frame.serial << frame.timestamp;
	for (uint i = 0; i < JointType_Count; i++) {
		out << frame.joints[i];
	}
	return out;
}
QDataStream& operator>>(QDataStream& in, KFrame& frame)
{
	in >> frame.serial >> frame.timestamp;
	for (uint i = 0; i < JointType_Count; i++) {
		in >> frame.joints[i];
	}
	return in;
}

KSkeleton::KSkeleton()
{
	cout << "KSkeleton constructor start." << endl;
	m_timeStep = 0.0333345;
	initJointHierarchy();
	loadFromBinary();
	cout << "KSkeleton constructor end." << endl;
}
void KSkeleton::addFrame(const Joint* joints, const JointOrientation* orientations, const double& time)
{
	static uint addedFrames = 0;
	addedFrames++;

	for (uint i = 0; i < JointType_Count; i++) {
		const Joint& jt = joints[i];
		const JointOrientation& or = orientations[i];
		m_joints[i].position = QVector3D(jt.Position.X, jt.Position.Y, jt.Position.Z);
		m_joints[i].trackingState = jt.TrackingState;
		m_joints[i].orientation = QQuaternion(or.Orientation.w, or.Orientation.x, or.Orientation.y, or.Orientation.z);
	}

	KFrame kframe;
	kframe.timestamp = time;
	kframe.joints = m_joints;
	kframe.serial = addedFrames; // serial = size - 1

	/*if (addedFrames <= m_framesDelayed) {
		m_firstRawFrames[addedFrames - 1] = kframe;
	}*/

	if (m_recordingOn) {
		m_rawFrames.push_back(kframe);
	}
	else {
		m_firstRawFrames[m_firstFrameIndex] = kframe;
		m_firstFrameIndex = addedFrames % m_framesDelayed;
	}

	if (m_finalizingOn) {
		static uint counter = 0;
		if (counter < m_framesDelayed) {
			m_lastRawFrames[counter] = kframe;
			cout << "counter = " << counter << endl;
			counter++;
		}
		else {
			cout << "Finalizing recording." << endl;
			m_finalizingOn = false;
			interpolateRecordedFrames();
			filterRecordedFrames();
			saveToBinary();
			counter = 0;
			addedFrames = 0;
		}
	}
}
void KSkeleton::interpolateRecordedFrames()
{
	cout << "Interpolating recorded frames." << endl;
	double totalTime = m_rawFrames.back().timestamp - m_rawFrames.front().timestamp;
	cout << "Total time: " << totalTime << endl;
	double interpolationInterval = totalTime / (m_rawFrames.size()-1);
	cout << "Interpolation interval: " << interpolationInterval << endl;
	cout << "First frame index: " << m_firstFrameIndex << endl;
	for (uint i = 0; i < m_firstRawFrames.size() - 1; i++) {
		KFrame interpolatedFrame;
		int interpolatedSerial = -m_framesDelayed + i;
		double interpolatedTime = interpolationInterval * interpolatedSerial;
		double interpolationTime = m_rawFrames.front().timestamp + interpolatedTime;
		interpolatedFrame.serial = interpolatedSerial;
		interpolatedFrame.timestamp = interpolatedTime;
		uint previousIndex = (i + m_firstFrameIndex) % m_framesDelayed;
		uint nextIndex = (i + m_firstFrameIndex + 1) % m_framesDelayed;
		cout << "InterpolatedSerial=" << interpolatedSerial << " PreviousIndex: " << previousIndex << " NextIndex: " << nextIndex << endl;
		if (i == (m_firstRawFrames.size() - 1)) {
			interpolatedFrame.interpolateJoints(m_firstRawFrames[previousIndex], m_rawFrames.front(), interpolationTime);
		} else {
			interpolatedFrame.interpolateJoints(m_firstRawFrames[previousIndex], m_firstRawFrames[nextIndex], interpolationTime);
		}
		m_interpolatedFrames.push_back(interpolatedFrame);
	}
	for (uint i = 0; i < m_rawFrames.size(); i++) {
		KFrame interpolatedFrame;
		int interpolatedSerial = i;
		double interpolatedTime = interpolationInterval * interpolatedSerial;
		double interpolationTime = m_rawFrames.front().timestamp + interpolatedTime;
		interpolatedFrame.serial = interpolatedSerial;
		interpolatedFrame.timestamp = interpolatedTime;
		if (i == (m_rawFrames.size() - 1)) {
			interpolatedFrame.interpolateJoints(m_rawFrames[i], m_lastRawFrames.front(), interpolationTime);
		}
		else {
			interpolatedFrame.interpolateJoints(m_rawFrames[i], m_rawFrames[i + 1], interpolationTime);
		}
		m_interpolatedFrames.push_back(interpolatedFrame);
	}
	for (uint i = 0; i < m_lastRawFrames.size(); i++) {
		KFrame interpolatedFrame;
		int interpolatedSerial = m_rawFrames.last().serial + i +1;
		double interpolatedTime = interpolationInterval * interpolatedSerial;
		double interpolationTime = m_rawFrames.front().timestamp + interpolatedTime;
		if (i == (m_lastRawFrames.size() - 1)) {
			interpolatedFrame.interpolateJoints(m_lastRawFrames[i - 1], m_lastRawFrames[i], interpolationTime);
		}
		else {
			interpolatedFrame.interpolateJoints(m_lastRawFrames[i], m_lastRawFrames[i + 1], interpolationTime);
		}
		interpolatedFrame.serial = interpolatedSerial;
		interpolatedFrame.timestamp = interpolatedTime;
		m_interpolatedFrames.push_back(interpolatedFrame);
	}
}
void KSkeleton::filterRecordedFrames()
{
	uint np = m_sgCoefficients25.size() - 1; // number of points used
	for (uint i = m_framesDelayed; i < m_interpolatedFrames.size()- m_framesDelayed; i++) {
		KFrame filteredFrame;
		for (uint j = 0; j < JointType_Count; j++) {
			filteredFrame.joints[j].position = QVector3D(0.f, 0.f, 0.f);
			for (uint k = 0; k < 2 * m_framesDelayed + 1; k++) {
				filteredFrame.joints[j].position += m_interpolatedFrames[i + k - np / 2].joints[j].position *	m_sgCoefficients25[k] / m_sgCoefficients25.back();
				//if (j == 0) cout << (i + k - np / 2 == i ? "F:" : "") << i + k - np / 2 << ",";
			}
		}
		cout << endl;
		filteredFrame.serial = m_interpolatedFrames[i].serial;
		filteredFrame.timestamp = m_interpolatedFrames[i].timestamp;
		m_filteredFrames.push_back(filteredFrame);
	}
}
// saves filtered frame sequence to trc
bool KSkeleton::saveToTrc()
{
	QString fileName("joint_positions.trc");
	QFile qf(fileName);
	if (!qf.open(QIODevice::WriteOnly | QIODevice::Text)) {
		cout << "Could not create " << fileName.toStdString() << endl;
		return false;
	}

	QVector<KFrame>& framesToWrite = m_filteredFrames;

	QTextStream out(&qf);
	// Line 1
	out << "PathFileType\t";
	out << "4\t";
	out << "(X / Y / Z)\t";
	out << "joint_positions.trc\n";
	// Line 2
	out << "DataRate\t";
	out << "CameraRate\t";
	out << "NumFrames\t";
	out << "NumMarkers\t";
	out << "Units\t";
	out << "OrigDataRate\t";
	out << "OrigDataStartFrame\t";
	out << "OrigNumFrames\n";
	// Line 3
	out << "30\t";
	out << "30\t";
	out << framesToWrite.size() << "\t";
	out << (JointType_Count + 1) << "\t";
	out << "mm\t";
	out << "30\t";
	out << "1\t";
	out << framesToWrite.size() << "\n";
	// Line 4
	out << "Frame#\t";
	out << "Time";
	for (int i = 0; i < JointType_Count; i++) {
		out << "\t" << m_nodes[i].name << "\t\t";
	}
	out << "\t" << "HipsMid" << "\t\t";
	out << "\n";
	// Line 5
	out << "\t";
	for (int i = 0; i < JointType_Count; i++) {
		out << "\t" << "X" << (i + 1) << "\t" << "Y" << (i + 1) << "\t" << "Z" << (i + 1);
	}
	out << "\t" << "X" << (JointType_Count + 1) << "\t" << "Y" << (JointType_Count + 1) << "\t" << "Z" << (JointType_Count + 1);

	out << "\n";
	// Lines 6+
	out.setFieldWidth(12);
	for (uint i = 0; i < framesToWrite.size(); i++) {
		out << "\n" << i << "\t" << framesToWrite[i].timestamp;
		for (int j = 0; j < JointType_Count; j++) {
			out << "\t" << framesToWrite[i].joints[j].position.x()*1000.f;
			out << "\t" << framesToWrite[i].joints[j].position.y()*1000.f;
			out << "\t" << framesToWrite[i].joints[j].position.z()*1000.f;
		}
		out << "\t" << (framesToWrite[i].joints[JointType_HipLeft].position.x()*1000.f + framesToWrite[i].joints[JointType_HipRight].position.x()*1000.f) / 2.f;
		out << "\t" << (framesToWrite[i].joints[JointType_HipLeft].position.y()*1000.f + framesToWrite[i].joints[JointType_HipRight].position.y()*1000.f) / 2.f;
		out << "\t" << (framesToWrite[i].joints[JointType_HipLeft].position.z()*1000.f + framesToWrite[i].joints[JointType_HipRight].position.z()*1000.f) / 2.f;
	}
	out << flush;
	qf.close();

	cout << "Successfully created " << fileName.toStdString() << endl;
	return true;
}
void KSkeleton::initJointHierarchy()
{
	// Set parents
	// core
	m_nodes[JointType_SpineBase]     = KNode("SpineBase", INVALID_JOINT_ID);
	m_nodes[JointType_SpineMid]      = KNode("SpineMid", JointType_SpineBase);
	m_nodes[JointType_SpineShoulder] = KNode("SpineShoulder", JointType_SpineMid);
	m_nodes[JointType_Neck]          = KNode("Neck", JointType_SpineShoulder);
	m_nodes[JointType_Head]          = KNode("Head", JointType_Neck);
	// left side																				
	m_nodes[JointType_HipLeft]       = KNode("HipLeft", JointType_SpineBase);
	m_nodes[JointType_KneeLeft]      = KNode("KneeLeft", JointType_HipLeft);
	m_nodes[JointType_AnkleLeft]     = KNode("AnkleLeft", JointType_KneeLeft);
	m_nodes[JointType_FootLeft]      = KNode("FootLeft", JointType_AnkleLeft);
	m_nodes[JointType_ShoulderLeft]  = KNode("ShoulderLeft", JointType_SpineShoulder);
	m_nodes[JointType_ElbowLeft]     = KNode("ElbowLeft", JointType_ShoulderLeft);
	m_nodes[JointType_WristLeft]     = KNode("WristLeft", JointType_ElbowLeft);
	m_nodes[JointType_HandLeft]      = KNode("HandLeft", JointType_WristLeft);
	m_nodes[JointType_ThumbLeft]     = KNode("ThumbLeft", JointType_HandLeft);
	m_nodes[JointType_HandTipLeft]   = KNode("HandTipLeft", JointType_HandLeft);
	// right side																				
	m_nodes[JointType_HipRight]      = KNode("HipRight", JointType_SpineBase);
	m_nodes[JointType_KneeRight]     = KNode("KneeRight", JointType_HipRight);
	m_nodes[JointType_AnkleRight]    = KNode("AnkleRight", JointType_KneeRight);
	m_nodes[JointType_FootRight]     = KNode("FootRight", JointType_AnkleRight);
	m_nodes[JointType_ShoulderRight] = KNode("ShoulderRight", JointType_SpineShoulder);
	m_nodes[JointType_ElbowRight]    = KNode("ElbowRight", JointType_ShoulderRight);
	m_nodes[JointType_WristRight]    = KNode("WristRight", JointType_ElbowRight);
	m_nodes[JointType_HandRight]     = KNode("HandRight", JointType_WristRight);
	m_nodes[JointType_ThumbRight]    = KNode("ThumbRight", JointType_HandRight);
	m_nodes[JointType_HandTipRight]  = KNode("HandTipRight", JointType_HandRight);

	// Set children
	for (uint i = 0; i < JointType_Count; i++) {
		uint p = m_nodes[i].parentId;
		if (p != INVALID_JOINT_ID) {
			(m_nodes[p].childrenId).push_back(i);
		}
	}
}
void KSkeleton::printInfo() const
{
	cout << endl;
	cout << "Joint hierarchy:" << endl;
	printJointHierarchy();
	cout << "Joint data:" << endl;
	printJoints();
	cout << endl;
}
void KSkeleton::printJointHierarchy() const
{
	for (uint i = 0; i < JointType_Count; i++) {
		uint p = m_nodes[i].parentId;
		cout << "Joint " << setw(2) << left << i << ": " << setw(20) << left << m_nodes[i].name.toStdString();
		cout << " Parent: " << setw(20) << left << (p == INVALID_JOINT_ID ? "NONE" : m_nodes[p].name.toStdString());
		cout << " Children (" << m_nodes[i].childrenId.size() << "): ";
		for (uint j = 0; j < m_nodes[i].childrenId.size(); j++) {
			uint c = m_nodes[i].childrenId[j];
			cout << m_nodes[c].name.toStdString() << " ";
		}
		cout << endl;
	}
}
void KSkeleton::printJoints() const
{
	for (uint i = 0; i < JointType_Count; i++) {
		const KJoint &jt = m_joints[i];
		cout << setw(15) << m_nodes[i].name.toStdString() << ": ";
		cout << setw(15) << m_joints[i].position << " ";
		cout << m_joints[i].getTrackingState().toStdString();
		cout << endl;
	}
	cout << endl;
}
double KSkeleton::timeStep() const
{
	return m_timeStep;
}
void KSkeleton::setTimeStep(double timestep)
{
	m_timeStep = timestep;
}
uint KSkeleton::activeFrame() const
{
	return m_activeFrame;
}
void KSkeleton::setActiveJoints(uint frameIndex)
{
	if (m_playbackFiltered && m_playbackInterpolated) {
		m_joints = m_filteredFrames[frameIndex].joints;
	}
	else if (m_playbackInterpolated) {
		m_joints = m_interpolatedFrames[frameIndex].joints;
	}
	else if (m_playbackFiltered) { // #todo make just filtered sequence
		m_joints = m_filteredFrames[frameIndex].joints; 
	}
	else {
		m_joints = m_rawFrames[frameIndex].joints;
	}
}
array<KJoint, JointType_Count>& KSkeleton::joints()
{
	return m_joints;
}
void KSkeleton::saveToBinary() const
{
	QFile qf("sequences.txt");
	if (!qf.open(QIODevice::WriteOnly)) {
		cout << "Cannot write to sequences.txt file." << endl;
		return;
	}
	else {
		cout << "Saving to binary sequences.txt" << endl;
	}
	QDataStream out(&qf);
	//out.setVersion(QDataStream::Qt_5_9);
	out << m_rawFrames;
	out << m_interpolatedFrames;
	out << m_filteredFrames;
	qf.close();
	cout << "Size of saved sequence: " << m_rawFrames.size() << endl;
	if (m_rawFrames.size() != 0) cout << "Duration: " << m_rawFrames.back().timestamp - m_rawFrames.front().timestamp << endl;
	cout << "Size of saved interpolated sequence: " << m_interpolatedFrames.size() << endl;
	if (m_interpolatedFrames.size() != 0) cout << "Duration: " << m_interpolatedFrames.back().timestamp << endl;
	cout << "Size of saved filtered interpolated sequence: " << m_filteredFrames.size() << endl;
	if (m_filteredFrames.size() != 0) cout << "Duration: " << m_filteredFrames.back().timestamp << endl;
}
void KSkeleton::loadFromBinary()
{
	QFile qf("sequences.txt");
	if (!qf.open(QIODevice::ReadOnly)) {
		cout << "Cannot read from sequences.txt file." << endl;
		return;
	}
	else {
		cout << "Loading from binary sequences.txt" << endl;
	}
	QDataStream in(&qf);
	//in.setVersion(QDataStream::Qt_5_9);
	clearSequences();
	in >> m_rawFrames;
	in >> m_interpolatedFrames;
	in >> m_filteredFrames;
	qf.close();

	cout << "Size of loaded sequence: " << m_rawFrames.size() << endl;
	if (m_rawFrames.size() != 0) cout << "Duration: " << m_rawFrames.back().timestamp << endl;
	cout << "Size of loaded interpolated sequence: " << m_interpolatedFrames.size() << endl;
	if (m_interpolatedFrames.size() != 0) cout << "Duration: " << m_interpolatedFrames.back().timestamp << endl;
	cout << "Size of loaded filtered interpolated sequence: " << m_filteredFrames.size() << endl;
	if (m_filteredFrames.size() != 0) cout << "Duration: " << m_filteredFrames.back().timestamp << endl;
}
void KSkeleton::clearSequences()
{
	// #todo: clear may be expensive so add if for each clear
	m_rawFrames.clear();
	m_interpolatedFrames.clear();
	m_filteredFrames.clear();
	m_activeFrame = 0;
}
uint KSkeleton::sequenceSize()
{
	return m_filteredFrames.size();
}
