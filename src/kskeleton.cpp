// Own
#include "kskeleton.h"

// Qt
#include <QtCore/QFile>

// Standard C/C++
#include <iomanip>

QDataStream& operator<<(QDataStream& out, const KJoint& joint)
{
	out << joint.position << joint.orientation << joint.trackingState;
	return out;
}
QDataStream& operator>>(QDataStream& in, KJoint& joint)
{
	in >> joint.position >> joint.orientation >> joint.trackingState;
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
	
	m_sequenceLog.setFileName("sequences_log.txt");
	if (!m_sequenceLog.open(QIODevice::WriteOnly | QIODevice::Text)) {
		cout << "Could not open sequences log file." << endl;
		return;
	}
	m_sequenceLogData.setFieldAlignment(QTextStream::AlignLeft);
	m_sequenceLogData.setRealNumberPrecision(6);
	m_sequenceLogData.setDevice(&m_sequenceLog);

	initJoints();
	printJointHierarchy();
	initLimbs();

	loadMotion();
	
	cout << "KSkeleton constructor end.\n" << endl;
}
KSkeleton::~KSkeleton()
{
	m_sequenceLog.close();
}
void KSkeleton::initJoints()
{
	// Set parents
	// core
	m_nodes[JointType_SpineBase] = KNode("SpineBase", INVALID_JOINT_ID);
	m_nodes[JointType_SpineMid] = KNode("SpineMid", JointType_SpineBase);
	m_nodes[JointType_SpineShoulder] = KNode("SpineShoulder", JointType_SpineMid);
	m_nodes[JointType_Neck] = KNode("Neck", JointType_SpineShoulder);
	m_nodes[JointType_Head] = KNode("Head", JointType_Neck);
	// left side																				
	m_nodes[JointType_HipLeft] = KNode("HipLeft", JointType_SpineBase);
	m_nodes[JointType_KneeLeft] = KNode("KneeLeft", JointType_HipLeft);
	m_nodes[JointType_AnkleLeft] = KNode("AnkleLeft", JointType_KneeLeft);
	m_nodes[JointType_FootLeft] = KNode("FootLeft", JointType_AnkleLeft);
	m_nodes[JointType_ShoulderLeft] = KNode("ShoulderLeft", JointType_SpineShoulder);
	m_nodes[JointType_ElbowLeft] = KNode("ElbowLeft", JointType_ShoulderLeft);
	m_nodes[JointType_WristLeft] = KNode("WristLeft", JointType_ElbowLeft);
	m_nodes[JointType_HandLeft] = KNode("HandLeft", JointType_WristLeft);
	m_nodes[JointType_ThumbLeft] = KNode("ThumbLeft", JointType_HandLeft);
	m_nodes[JointType_HandTipLeft] = KNode("HandTipLeft", JointType_HandLeft);
	// right side																				
	m_nodes[JointType_HipRight] = KNode("HipRight", JointType_SpineBase);
	m_nodes[JointType_KneeRight] = KNode("KneeRight", JointType_HipRight);
	m_nodes[JointType_AnkleRight] = KNode("AnkleRight", JointType_KneeRight);
	m_nodes[JointType_FootRight] = KNode("FootRight", JointType_AnkleRight);
	m_nodes[JointType_ShoulderRight] = KNode("ShoulderRight", JointType_SpineShoulder);
	m_nodes[JointType_ElbowRight] = KNode("ElbowRight", JointType_ShoulderRight);
	m_nodes[JointType_WristRight] = KNode("WristRight", JointType_ElbowRight);
	m_nodes[JointType_HandRight] = KNode("HandRight", JointType_WristRight);
	m_nodes[JointType_ThumbRight] = KNode("ThumbRight", JointType_HandRight);
	m_nodes[JointType_HandTipRight] = KNode("HandTipRight", JointType_HandRight);

	m_nodes[JointType_SpineMid].helperId = JointType_HipRight;
	m_nodes[JointType_SpineShoulder].helperId = JointType_Neck;
	m_nodes[JointType_Neck].helperId = JointType_Head;
	m_nodes[JointType_Head].helperId = JointType_SpineShoulder;
	m_nodes[JointType_ShoulderLeft].helperId = JointType_ElbowLeft;
	m_nodes[JointType_ShoulderRight].helperId = JointType_ElbowRight;
	m_nodes[JointType_ElbowLeft].helperId = JointType_WristLeft;
	m_nodes[JointType_ElbowRight].helperId = JointType_WristRight;
	m_nodes[JointType_WristLeft].helperId = JointType_ThumbLeft;  
	m_nodes[JointType_WristRight].helperId = JointType_ThumbRight;
	m_nodes[JointType_HandLeft].helperId = JointType_ThumbLeft;
	m_nodes[JointType_HandRight].helperId = JointType_ThumbRight;
	m_nodes[JointType_KneeLeft].helperId = JointType_AnkleLeft;
	m_nodes[JointType_KneeRight].helperId = JointType_AnkleRight;
	m_nodes[JointType_AnkleLeft].helperId = JointType_FootLeft;
	m_nodes[JointType_AnkleRight].helperId = JointType_FootRight;
	m_nodes[JointType_FootLeft].helperId = JointType_KneeLeft;
	m_nodes[JointType_FootRight].helperId = JointType_KneeRight;

	// Set children
	for (uint i = 0; i < JointType_Count; i++) {
		uint p = m_nodes[i].parentId;
		if (p != INVALID_JOINT_ID) {
			(m_nodes[p].childrenId).push_back(i);
		}
	}
}
void KSkeleton::initLimbs()
{
	// Core
	m_limbs[0] = KLimb(JointType_SpineBase, JointType_SpineShoulder);
	// Legs 
	m_limbs[1] = KLimb(JointType_SpineBase, JointType_HipLeft, 2);
	m_limbs[2] = KLimb(JointType_SpineBase, JointType_HipRight, 1);
	m_limbs[3] = KLimb(JointType_HipLeft, JointType_KneeLeft, 4);
	m_limbs[4] = KLimb(JointType_HipRight, JointType_KneeRight, 3);
	m_limbs[5] = KLimb(JointType_KneeLeft, JointType_AnkleLeft, 6);
	m_limbs[6] = KLimb(JointType_KneeRight, JointType_AnkleRight, 5);
	m_limbs[7] = KLimb(JointType_AnkleLeft, JointType_FootLeft, 8);
	m_limbs[8] = KLimb(JointType_AnkleRight, JointType_FootRight, 7);
	// Arms																 			  
	m_limbs[9] = KLimb(JointType_SpineBase, JointType_ShoulderLeft); // helper limb, must be before limb 11
	m_limbs[10] = KLimb(JointType_SpineBase, JointType_ShoulderRight); // helper limb, must be before limb 12
	m_limbs[11] = KLimb(JointType_SpineShoulder, JointType_ShoulderLeft, 12);
	m_limbs[12] = KLimb(JointType_SpineShoulder, JointType_ShoulderRight, 11);
	m_limbs[13] = KLimb(JointType_ShoulderLeft, JointType_ElbowLeft, 14);
	m_limbs[14] = KLimb(JointType_ShoulderRight, JointType_ElbowRight, 13);
	m_limbs[15] = KLimb(JointType_ElbowLeft, JointType_WristLeft, 16);
	m_limbs[16] = KLimb(JointType_ElbowRight, JointType_WristRight, 15);
	m_limbs[17] = KLimb(JointType_WristLeft, JointType_HandLeft, 18);
	m_limbs[18] = KLimb(JointType_WristRight, JointType_HandRight, 17);
	m_limbs[19] = KLimb(JointType_WristLeft, JointType_HandTipLeft, 20);
	m_limbs[20] = KLimb(JointType_WristRight, JointType_HandTipRight, 19);
	m_limbs[21] = KLimb(JointType_HandLeft, JointType_ThumbLeft, 22);
	m_limbs[22] = KLimb(JointType_HandRight, JointType_ThumbRight, 21);

	for (uint l = 0; l < m_limbs.size(); l++) {
		m_limbs[l].name = m_nodes[m_limbs[l].start].name + "->" + m_nodes[m_limbs[l].end].name;
	}
}
void KSkeleton::record(bool trainerRecording)
{
	if (!m_isRecording) {
		cout << "Recording started." << endl;
		m_recordedMotion.clear();
		m_isRecording = true;
	}
	else {
		cout << "Recording finished." << endl;
		m_isRecording = false;
		m_isFinalizing = true;
	}
}
KFrame KSkeleton::addFrame(const Joint* joints, const JointOrientation* jointOrientations, double time)
{
	static uint addedFrames = 0;
	addedFrames++;

	KFrame kframe;

	for (uint i = 0; i < JointType_Count; i++) {
		const Joint& jt = joints[i];
		const JointOrientation& or = jointOrientations[i];
		kframe.joints[i].position = QVector3D(jt.Position.X, jt.Position.Y, jt.Position.Z);
		kframe.joints[i].orientation = QQuaternion(or.Orientation.w, or.Orientation.x, or.Orientation.y, or.Orientation.z);
		kframe.joints[i].trackingState = jt.TrackingState;
	}
	kframe.timestamp = time;
	kframe.serial = addedFrames; 

	if (m_isRecording) {
		m_recordedMotion.push_back(kframe);
	} 
	else if (m_isFinalizing) {
		static uint counter = 0;
		if (counter < m_framesDelayed) {
			counter++;
			uint index = (m_firstFrameIndex - counter + m_framesDelayed) % m_framesDelayed;
			m_recordedMotion.push_front(m_firstRawFrames[index]);
			m_recordedMotion.push_back(kframe);
		}
		else {
			cout << "Recording finished." << endl;
			double timeOffset = m_recordedMotion[m_framesDelayed].timestamp;
			int serialOffset = m_recordedMotion[m_framesDelayed].serial;
			for (uint i = 0; i < m_recordedMotion.size(); i++) {
				m_recordedMotion[i].timestamp -= timeOffset;
				m_recordedMotion[i].serial -= serialOffset;
			}
			processRecording();
			counter = 0;
			addedFrames = 0;
			m_firstFrameIndex = 0;
			m_isFinalizing = false;
		}
	}
	else {
		if (addedFrames <= m_framesDelayed) {
			m_firstRawFrames[addedFrames - 1] = kframe;
		}
		else {
			m_firstRawFrames[m_firstFrameIndex] = kframe;
			m_firstFrameIndex = addedFrames % m_framesDelayed;
		}
	}

	return kframe;
}
void KSkeleton::processRecording()
{
	if (m_athleteRecording) {
		cout << "Processing athlete motion" << endl;
		m_athleteRawMotion = m_recordedMotion;
		m_athleteInterpolatedMotion = interpolateMotion(m_athleteRawMotion);
		m_athleteFilteredMotion = filterMotion(m_athleteInterpolatedMotion);
		m_athleteAdjustedMotion = adjustMotion(m_athleteFilteredMotion);
		m_athleteResizedMotion = resizeMotion(m_athleteRawMotion);
		calculateJointOrientations(m_athleteInterpolatedMotion);
		calculateJointOrientations(m_athleteFilteredMotion);
		calculateJointOrientations(m_athleteAdjustedMotion);
		calculateJointOrientations(m_athleteResizedMotion);
		cropMotion(m_athleteRawMotion);
		cropMotion(m_athleteInterpolatedMotion);
	}
	if (m_trainerRecording) {
		cout << "Processing trainer motion" << endl;
		m_trainerRawMotion = m_recordedMotion;
		m_trainerInterpolatedMotion = interpolateMotion(m_trainerRawMotion);
		m_trainerFilteredMotion = filterMotion(m_trainerInterpolatedMotion);
		m_trainerAdjustedMotion = adjustMotion(m_trainerFilteredMotion);
		m_trainerResizedMotion = resizeMotion(m_trainerRawMotion);
		calculateJointOrientations(m_trainerInterpolatedMotion);
		calculateJointOrientations(m_trainerFilteredMotion);
		calculateJointOrientations(m_trainerAdjustedMotion);
		calculateJointOrientations(m_trainerResizedMotion);
		cropMotion(m_trainerRawMotion);
		cropMotion(m_trainerInterpolatedMotion);
		if (m_trainerRawMotion.size() > m_bigMotionSize) m_bigMotionSize = m_trainerRawMotion.size();
	}
	calculateOffsets();
	printMotionsToLog();
}
QVector<KFrame> KSkeleton::resizeMotion(const QVector<KFrame>& motion)
{
	QVector<KFrame> resizedMotion;
	return resizedMotion;
}
QVector<KFrame> KSkeleton::interpolateMotion(const QVector<KFrame>& motion)
{
	QVector<KFrame> interpolatedMotion;

	if (motion.empty()) {
		cout << "interpolateMotion: empty input motion!" << endl;
		return interpolatedMotion;
	}

	cout << "Interpolating recorded frames." << endl;
	cout << "Interpolation interval: " << m_interpolationInterval << endl;
	cout << "First frame index: " << m_firstFrameIndex << endl;
	static int index = 1;
	static int counter = 0;
	double interpolationTime = 0;
	while (counter < motion.size()) {
		KFrame interpolatedFrame;
		interpolationTime = m_interpolationInterval * (counter - m_framesDelayed);
		while (motion[index].timestamp <= interpolationTime && index < motion.size()) {
			index++;
		}
		if (index > motion.size() - 2) index = motion.size() - 2;
		interpolatedFrame.serial = counter - m_framesDelayed;
		interpolatedFrame.timestamp = interpolationTime;
		interpolatedFrame.interpolateJoints(motion[--index], motion[index + 1], interpolationTime);
		interpolatedMotion.push_back(interpolatedFrame);
		counter++;
	}
	index = 1; // was index = 0
	counter = 0;

	return interpolatedMotion;
}
QVector<KFrame> KSkeleton::filterMotion(const QVector<KFrame>& motion)
{
	QVector<KFrame> filteredMotion;

	if (motion.empty()) {
		cout << "filterMotion: empty input motion!" << endl;
		return filteredMotion;
	}

	cout << "Filtering motion" << endl;
	uint np = m_sgCoefficients.size() - 1; // number of points used
	for (uint i = m_framesDelayed; i < motion.size() - m_framesDelayed; i++) {
		KFrame filteredFrame;
		for (uint j = 0; j < JointType_Count; j++) {
			filteredFrame.joints[j].position = QVector3D(0.f, 0.f, 0.f);
			for (uint k = 0; k < 2 * m_framesDelayed + 1; k++) {
				filteredFrame.joints[j].position += 
					motion[i + k - np / 2].joints[j].position *	
					m_sgCoefficients[k] * 
					m_sgCoefficients.back();
			}
		}
		filteredFrame.serial = motion[i].serial;
		filteredFrame.timestamp = motion[i].timestamp;
		filteredMotion.push_back(filteredFrame);
	}

	return filteredMotion;
}
QVector<KFrame> KSkeleton::adjustMotion(const QVector<KFrame>& motion)
{
	QVector<KFrame> adjustedMotion;

	if (motion.empty()) {
		cout << "adjustMotion: empty input motion" << endl;
		return adjustedMotion;
	}

	cout << "Adjusting motion" << endl;
	cout << "Before adjustments:" << endl;
	calculateLimbLengths(motion);
	printLimbLengths();

	for (uint i = 0; i < motion.size(); i++) {
		m_leftFootOffset = QVector3D();
		m_rightFootOffset = QVector3D();
		KFrame adjustedFrame = motion[i];
		for (uint l = 0; l < m_limbs.size(); l++) {
			KLimb& limb = m_limbs[l];
			const QVector3D& startPosition = motion[i].joints[limb.start].position;
			const QVector3D& endPosition = motion[i].joints[limb.end].position;
			QVector3D direction = endPosition - startPosition;
			float limbCurrentLength = startPosition.distanceToPoint(endPosition);
			limb.desiredLength =
				m_limbs[l].sibling == INVALID_JOINT_ID ?
				m_limbs[l].averageLength :
				(m_limbs[l].averageLength + m_limbs[m_limbs[l].sibling].averageLength) / 2.f;;
			float adjustmentFactor = limb.desiredLength / limbCurrentLength;
			if (i == 0) {
				cout << "Limb=" << limb.name.toStdString();
				cout << " DesiredLength=" << limb.desiredLength;
				cout << " CurrentLength=" << limbCurrentLength;
				cout << " CurrentFactor=" << adjustmentFactor << endl;
			}
			adjustLimbLength(adjustedFrame, limb.end, direction, adjustmentFactor);
		}

		// Point thumbs towards opposite hand
		QVector3D handRightToLeft =
			adjustedFrame.joints[JointType_HandLeft].position -
			adjustedFrame.joints[JointType_HandRight].position;
		adjustedFrame.joints[JointType_ThumbLeft].position =
			adjustedFrame.joints[JointType_HandLeft].position -
			handRightToLeft.normalized()*m_limbs[21].desiredLength;
		adjustedFrame.joints[JointType_ThumbRight].position =
			adjustedFrame.joints[JointType_HandRight].position +
			handRightToLeft.normalized()*m_limbs[22].desiredLength;

		// Move all joints towards the ground
		for (uint j = 0; j < JointType_Count; j++) {
			adjustedFrame.joints[j].position.setY(
				adjustedFrame.joints[j].position.y() -
				(m_leftFootOffset.y() + m_rightFootOffset.y()) / 2.f
			);
		}

		adjustedMotion.push_back(adjustedFrame);
	}

	calculateLimbLengths(adjustedMotion);
	cout << "After adjustments: " << endl;
	printLimbLengths();

	return adjustedMotion;
}
void KSkeleton::adjustLimbLength(KFrame& kframe, uint jointId, const QVector3D& direction, float factor)
{
	QVector3D& end = kframe.joints[jointId].position;
	QVector3D deltaEnd = -(1 - factor) * direction;
	end = end + deltaEnd;
	if (jointId == JointType_FootLeft) m_leftFootOffset += deltaEnd;
	if (jointId == JointType_FootRight) m_rightFootOffset += deltaEnd;
	if (kframe.serial == 0) cout << m_nodes[jointId].name.toStdString() << " ";
	for (uint i = 0; i < m_nodes[jointId].childrenId.size(); i++) {
		uint childId = m_nodes[jointId].childrenId[i];
		adjustLimbLength(kframe, childId, direction, factor);
	}
}
void KSkeleton::calculateOffsets()
{
	if (m_athleteRawMotion.size() > 0) {
		m_athletePelvisOffset = m_athleteAdjustedMotion.front().joints[JointType_SpineBase].position;
		cout << "Athlete pelvis offset: " << toStringCartesian(m_athletePelvisOffset).toStdString() << endl;
		m_athleteFeetOffset =
			m_athleteAdjustedMotion.front().joints[JointType_AnkleLeft].position.y() / 2.f +
			m_athleteAdjustedMotion.front().joints[JointType_AnkleRight].position.y() / 2.f -
			m_athletePelvisOffset.y();
		cout << "Athlete feet offset: " << m_athleteFeetOffset << endl;
	}
	if (m_trainerRawMotion.size() > 0) {
		m_trainerPelvisOffset = m_trainerAdjustedMotion.front().joints[JointType_SpineBase].position;
		cout << "Trainer pelvis offset: " << toStringCartesian(m_trainerPelvisOffset).toStdString() << endl;
		m_trainerFeetOffset =
			m_trainerAdjustedMotion.front().joints[JointType_AnkleLeft].position.y() / 2.f +
			m_trainerAdjustedMotion.front().joints[JointType_AnkleRight].position.y() / 2.f -
			m_trainerPelvisOffset.y();
		cout << "Athlete feet offset: " << m_trainerFeetOffset << endl;
	}
}
void KSkeleton::calculateJointOrientations(QVector<KFrame>& motion)
{
	for (uint i = 0; i < motion.size(); i++) {
		for (uint j = 0; j < JointType_Count; j++) {
			uint child = j;
			uint parent = m_nodes[j].parentId;
			uint helper = m_nodes[j].helperId;
			if (parent == INVALID_JOINT_ID || helper == INVALID_JOINT_ID) {
				if (i == 0) cout << "Invalid joint id " << parent << " " << helper << endl;
				continue;
			}
			if (i == 0) cout << "Child=" << m_nodes[child].name.toStdString();
			if (i == 0) cout << " Parent=" << m_nodes[parent].name.toStdString();
			if (i == 0) cout << " Helper=" << m_nodes[helper].name.toStdString();
			if (i == 0) cout << endl;

			QVector3D childToOrigin =
				motion[i].joints[parent].position -
				motion[i].joints[child].position;
			QVector3D childToHelper =
				motion[i].joints[helper].position -
				motion[i].joints[child].position;

			QVector3D upDirection = -childToOrigin;
			QVector3D leftDirection = QVector3D::crossProduct(childToOrigin, childToHelper);
			QVector3D frontDirection = QVector3D::crossProduct(childToOrigin, leftDirection);
			if (child == JointType_SpineMid) {
				QVector3D hipsDirection =
					motion[i].joints[JointType_HipLeft].position -
					motion[i].joints[JointType_HipRight].position;
				frontDirection = QVector3D::crossProduct(hipsDirection, upDirection);
			}
			else if (child == JointType_SpineShoulder) {
				QVector3D shouldersDirection =
					motion[i].joints[JointType_ShoulderLeft].position -
					motion[i].joints[JointType_ShoulderRight].position;
				frontDirection = QVector3D::crossProduct(shouldersDirection, upDirection);
			}
			else if (child == JointType_Neck) {
				QVector3D shouldersDirection =
					motion[i].joints[JointType_ShoulderLeft].position -
					motion[i].joints[JointType_ShoulderRight].position;
				frontDirection = QVector3D::crossProduct(shouldersDirection, upDirection);
			}
			else if (child == JointType_Head) {
				QVector3D shouldersDirection =
					motion[i].joints[JointType_ShoulderLeft].position -
					motion[i].joints[JointType_ShoulderRight].position;
				frontDirection = QVector3D::crossProduct(shouldersDirection, upDirection);
			}
			else if (child == JointType_ShoulderLeft) frontDirection = leftDirection;
			else if (child == JointType_ShoulderRight) frontDirection = -leftDirection;
			else if (child == JointType_ElbowLeft) frontDirection = leftDirection;
			else if (child == JointType_ElbowRight) frontDirection = -leftDirection;
			else if (child == JointType_WristLeft) frontDirection = -leftDirection;
			else if (child == JointType_WristRight) frontDirection = -leftDirection;
			else if (child == JointType_HandLeft) frontDirection = -frontDirection;
			else if (child == JointType_HandRight) frontDirection = -frontDirection;
			else if (child == JointType_KneeLeft) frontDirection = frontDirection;
			else if (child == JointType_KneeRight) frontDirection = leftDirection;
			else if (child == JointType_AnkleLeft) frontDirection = leftDirection;
			else if (child == JointType_AnkleRight) frontDirection = -leftDirection;
			else if (child == JointType_FootLeft) frontDirection = leftDirection;
			else if (child == JointType_FootRight) frontDirection = -leftDirection;

			childToOrigin.normalize();
			childToHelper.normalize();
			QQuaternion q;
			float angle = ToDegrees(acos(QVector3D::dotProduct(childToOrigin, childToHelper)));
			if (angle < 0.001) {
				if (i == 0) cout << "AngleBetweenDirections=" << angle << endl;
			}
			q = QQuaternion::fromDirection(frontDirection, upDirection);
			if (i == 0) cout << "Hand-made quat: " << toString(q).toStdString() << toStringEulerAngles(q).toStdString() << toStringAxisAngle(q).toStdString() << endl;

			QQuaternion absQ = motion[i].joints[child].orientation;
			if (i == 0) cout << "Original  quat: " << toString(absQ).toStdString() << toStringEulerAngles(absQ).toStdString() << toStringAxisAngle(absQ).toStdString() << endl;
			
			motion[i].joints[child].orientation = q;
		}
	}
}
void KSkeleton::cropMotion(QVector<KFrame>& motion)
{
	cout << "Sequence size before crop:" << motion.size() << endl;
	for (uint i = 0; i < m_framesDelayed; i++) {
		motion.removeFirst();
		motion.removeLast();
	}
	cout << "Sequence size after crop:" << motion.size() << endl;
}
float KLimb::gapAverage = 0.f;
void KSkeleton::calculateLimbLengths(const QVector<KFrame>& sequence)
{
	if (m_limbs.empty()) {
		cout << "Limbs array is empty! Returning." << endl;
		return;
	}
	if (sequence.empty()) {
		cout << "Frame sequence is empty! Returning." << endl;
		return;
	}

	KLimb::gapAverage = 0.f;
	for (uint l = 0; l < m_limbs.size(); l++) {
		if (m_limbs[l].end == INVALID_JOINT_ID) continue;
		m_limbs[l].maxLength = FLT_MIN;
		m_limbs[l].minLength = FLT_MAX;
		m_limbs[l].averageLength = 0;
		for (uint i = 0; i < sequence.size(); i++) {
			const QVector3D& startPosition = sequence[i].joints[m_limbs[l].start].position;
			const QVector3D& endPosition = sequence[i].joints[m_limbs[l].end].position;
			float length = startPosition.distanceToPoint(endPosition);
			if (length > m_limbs[l].maxLength) {
				m_limbs[l].maxLength = length;
				m_limbs[l].serialMax = sequence[i].serial;
			}
			if (length < m_limbs[l].minLength) {
				m_limbs[l].minLength = length;
				m_limbs[l].serialMin = sequence[i].serial;
			}
			m_limbs[l].averageLength += length;
		}
		m_limbs[l].averageLength /= sequence.size();
		KLimb::gapAverage += m_limbs[l].maxLength - m_limbs[l].minLength;
	}
	KLimb::gapAverage /= (m_limbs.size() - 1);

	for (uint l = 0; l < m_limbs.size(); l++) {
		m_limbs[l].desiredLength = (
			m_limbs[l].sibling == INVALID_JOINT_ID ?
			m_limbs[l].averageLength :
			(m_limbs[l].averageLength + m_limbs[m_limbs[l].sibling].averageLength) / 2.f
			);
	}
}
const array<KNode, JointType_Count>& KSkeleton::nodes() const
{
	return m_nodes;
}
void KSkeleton::printLimbLengths() const
{
	cout << "Limb lengths: " << endl;
	for (uint l = 0; l < m_limbs.size(); l++) {
		if (m_limbs[l].end == INVALID_JOINT_ID) continue;
		cout << setw(40) << left << m_limbs[l].name.toStdString() << " ";
		cout << "Min=" << setw(10) << m_limbs[l].minLength << " (" << setw(5) << m_limbs[l].serialMin;
		cout << ") Max=" << setw(10) << m_limbs[l].maxLength << " (" << setw(5) << m_limbs[l].serialMax;
		cout << ") Avg=" << setw(10) << m_limbs[l].averageLength << " ";
		cout << " Des=" << setw(10) << m_limbs[l].desiredLength << " ";
		cout << "Gap=" << setw(10) << m_limbs[l].maxLength - m_limbs[l].minLength << endl;
	}
	cout << "GapAverage=" << KLimb::gapAverage << endl;
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
// saves filtered frame sequence to trc
bool KSkeleton::exportToTRC()
{
	QString fileName("joint_positions.trc");
	QFile qf(fileName);
	if (!qf.open(QIODevice::WriteOnly | QIODevice::Text)) {
		cout << "Could not create " << fileName.toStdString() << endl;
		return false;
	}
	QTextStream out(&qf);

	QVector<KFrame>& exportedMotion = (m_trainerRecording) ? m_trainerAdjustedMotion : m_athleteAdjustedMotion;

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
	out << exportedMotion.size() << "\t";
	out << (JointType_Count + 1) << "\t";
	out << "mm\t";
	out << "30\t";
	out << "1\t";
	out << exportedMotion.size() << "\n";
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
	for (uint i = 0; i < exportedMotion.size(); i++) {
		out << "\n" << i << "\t" << exportedMotion[i].timestamp;
		for (int j = 0; j < JointType_Count; j++) {
			out << "\t" << exportedMotion[i].joints[j].position.x()*1000.f;
			out << "\t" << exportedMotion[i].joints[j].position.y()*1000.f;
			out << "\t" << exportedMotion[i].joints[j].position.z()*1000.f;
		}
		out << "\t" << (exportedMotion[i].joints[JointType_HipLeft].position.x()*1000.f + exportedMotion[i].joints[JointType_HipRight].position.x()*1000.f) / 2.f;
		out << "\t" << (exportedMotion[i].joints[JointType_HipLeft].position.y()*1000.f + exportedMotion[i].joints[JointType_HipRight].position.y()*1000.f) / 2.f;
		out << "\t" << (exportedMotion[i].joints[JointType_HipLeft].position.z()*1000.f + exportedMotion[i].joints[JointType_HipRight].position.z()*1000.f) / 2.f;
	}
	out << flush;
	qf.close();

	cout << "Successfully created " << fileName.toStdString() << endl;
	return true;
}
void KSkeleton::saveFrameSequences()
{
	QFile qf("sequences.txt");
	if (!qf.open(QIODevice::WriteOnly)) {
		cout << "Cannot write to sequences.txt binary file." << endl;
		return;
	}
	else {
		cout << "Saving to sequences.txt binary file" << endl;
	}

	QDataStream out(&qf);
	//out.setVersion(QDataStream::Qt_5_9);
	out << m_athleteRawMotion;
	out << m_athleteInterpolatedMotion;
	out << m_athleteFilteredMotion;
	out << m_athleteAdjustedMotion;
	out << m_athleteResizedMotion;
	out << m_trainerRawMotion;
	out << m_trainerInterpolatedMotion;
	out << m_trainerFilteredMotion;
	out << m_trainerAdjustedMotion;
	out << m_trainerResizedMotion;
	qf.close();
}
void KSkeleton::loadMotion()
{
	QFile qf("sequences.txt");
	if (!qf.open(QIODevice::ReadOnly)) {
		cout << "Cannot read from sequences.txt binary file." << endl;
		return;
	}
	else {
		cout << "Loading from sequences.txt binary file." << endl;
	}

	m_athleteRawMotion.clear();
	m_athleteInterpolatedMotion.clear();
	m_athleteFilteredMotion.clear();
	m_athleteAdjustedMotion.clear();
	m_athleteResizedMotion.clear();
	m_trainerRawMotion.clear();
	m_trainerInterpolatedMotion.clear();
	m_trainerFilteredMotion.clear();
	m_trainerAdjustedMotion.clear();
	m_trainerResizedMotion.clear();

	QDataStream in(&qf);
	//in.setVersion(QDataStream::Qt_5_9);
	in >> m_athleteRawMotion;
	in >> m_athleteInterpolatedMotion;
	in >> m_athleteFilteredMotion;
	in >> m_athleteAdjustedMotion;
	in >> m_athleteResizedMotion;
	in >> m_trainerRawMotion;
	in >> m_trainerInterpolatedMotion;
	in >> m_trainerFilteredMotion;
	in >> m_trainerAdjustedMotion;
	in >> m_trainerResizedMotion;
	qf.close();

	if (m_athleteRawMotion.size() > m_trainerRawMotion.size())  m_bigMotionSize = m_athleteRawMotion.size();
	else m_bigMotionSize = m_trainerRawMotion.size();
	cout << "Big motion size: " << m_bigMotionSize << endl;

	calculateOffsets();

	printMotionsToLog();
}
void KSkeleton::printMotionsToLog()
{
	m_sequenceLog.resize(0);

	// headers
	m_sequenceLogData << qSetFieldWidth(10) << "Motion";
	m_sequenceLogData << qSetFieldWidth(20) << "Athlete";
	m_sequenceLogData << qSetFieldWidth(20) << "Trainer";
	m_sequenceLogData << endl;

	// motion type
	m_sequenceLogData << qSetFieldWidth(10) << "Raw" << endl;
	// motion size
	m_sequenceLogData << qSetFieldWidth(10) << "Size:";
	m_sequenceLogData << qSetFieldWidth(20) << m_athleteRawMotion.size();
	m_sequenceLogData << qSetFieldWidth(20) << m_trainerRawMotion.size();
	m_sequenceLogData << endl;
	// motion duration
	m_sequenceLogData << qSetFieldWidth(10) << "Duration:";
	m_sequenceLogData << qSetFieldWidth(20) << (m_athleteRawMotion.size() > 0 ? m_athleteRawMotion.back().timestamp : 0);
	m_sequenceLogData << qSetFieldWidth(20) << (m_trainerRawMotion.size() > 0 ? m_trainerRawMotion.back().timestamp : 0);
	m_sequenceLogData << endl;
	// motion serials, timestamps
	for (uint i = 0; i < m_bigMotionSize; i++) {
		m_sequenceLogData << qSetFieldWidth(10) << i;
		m_sequenceLogData << qSetFieldWidth(5) << (i < m_athleteRawMotion.size() ? m_athleteRawMotion[i].serial : 0);
		m_sequenceLogData << qSetFieldWidth(15) << (i < m_athleteRawMotion.size() ? m_athleteRawMotion[i].timestamp : 0);
		m_sequenceLogData << qSetFieldWidth(5) << (i < m_trainerRawMotion.size() ? m_trainerRawMotion[i].serial : 0);
		m_sequenceLogData << qSetFieldWidth(15) << (i < m_trainerRawMotion.size() ? m_trainerRawMotion[i].timestamp : 0);
		m_sequenceLogData << endl;
	}
	
	// motion type
	m_sequenceLogData << qSetFieldWidth(10) << "Interpolated" << endl;
	// motion size
	m_sequenceLogData << qSetFieldWidth(10) << "Size:";
	m_sequenceLogData << qSetFieldWidth(20) << m_athleteInterpolatedMotion.size();
	m_sequenceLogData << qSetFieldWidth(20) << m_trainerInterpolatedMotion.size();
	m_sequenceLogData << endl;
	// motion duration
	m_sequenceLogData << qSetFieldWidth(10) << "Duration:";
	m_sequenceLogData << qSetFieldWidth(20) << (m_athleteInterpolatedMotion.size() > 0 ? m_athleteInterpolatedMotion.back().timestamp : 0);
	m_sequenceLogData << qSetFieldWidth(20) << (m_trainerInterpolatedMotion.size() > 0 ? m_trainerInterpolatedMotion.back().timestamp : 0);
	m_sequenceLogData << endl;
	// motion serials, timestamps
	for (uint i = 0; i < m_bigMotionSize; i++) {
		m_sequenceLogData << qSetFieldWidth(10) << i;
		m_sequenceLogData << qSetFieldWidth(5) << (i < m_athleteInterpolatedMotion.size() ? m_athleteInterpolatedMotion[i].serial : 0);
		m_sequenceLogData << qSetFieldWidth(15) << (i < m_athleteInterpolatedMotion.size() ? m_athleteInterpolatedMotion[i].timestamp : 0);
		m_sequenceLogData << qSetFieldWidth(5) << (i < m_trainerInterpolatedMotion.size() ? m_trainerInterpolatedMotion[i].serial : 0);
		m_sequenceLogData << qSetFieldWidth(15) << (i < m_trainerInterpolatedMotion.size() ? m_trainerInterpolatedMotion[i].timestamp : 0);
		m_sequenceLogData << endl;
	}

	// motion type
	m_sequenceLogData << qSetFieldWidth(10) << "Filtered" << endl;
	// motion size
	m_sequenceLogData << qSetFieldWidth(10) << "Size:";
	m_sequenceLogData << qSetFieldWidth(20) << m_athleteFilteredMotion.size();
	m_sequenceLogData << qSetFieldWidth(20) << m_trainerFilteredMotion.size();
	m_sequenceLogData << endl;
	// motion duration
	m_sequenceLogData << qSetFieldWidth(10) << "Duration:";
	m_sequenceLogData << qSetFieldWidth(20) << (m_athleteFilteredMotion.size() > 0 ? m_athleteFilteredMotion.back().timestamp : 0);
	m_sequenceLogData << qSetFieldWidth(20) << (m_trainerFilteredMotion.size() > 0 ? m_trainerFilteredMotion.back().timestamp : 0);
	m_sequenceLogData << endl;
	// motion serials, timestamps
	for (uint i = 0; i < m_bigMotionSize; i++) {
		m_sequenceLogData << qSetFieldWidth(10) << i;
		m_sequenceLogData << qSetFieldWidth(5) << (i < m_athleteFilteredMotion.size() ? m_athleteFilteredMotion[i].serial : 0);
		m_sequenceLogData << qSetFieldWidth(15) << (i < m_athleteFilteredMotion.size() ? m_athleteFilteredMotion[i].timestamp : 0);
		m_sequenceLogData << qSetFieldWidth(5) << (i < m_trainerFilteredMotion.size() ? m_trainerFilteredMotion[i].serial : 0);
		m_sequenceLogData << qSetFieldWidth(15) << (i < m_trainerFilteredMotion.size() ? m_trainerFilteredMotion[i].timestamp : 0);
		m_sequenceLogData << endl;
	}

	// motion type
	m_sequenceLogData << qSetFieldWidth(10) << "Adjusted" << endl;
	// motion size
	m_sequenceLogData << qSetFieldWidth(10) << "Size:";
	m_sequenceLogData << qSetFieldWidth(20) << m_athleteAdjustedMotion.size();
	m_sequenceLogData << qSetFieldWidth(20) << m_trainerAdjustedMotion.size();
	m_sequenceLogData << endl;
	// motion duration
	m_sequenceLogData << qSetFieldWidth(10) << "Duration:";
	m_sequenceLogData << qSetFieldWidth(20) << (m_athleteAdjustedMotion.size() > 0 ? m_athleteAdjustedMotion.back().timestamp : 0);
	m_sequenceLogData << qSetFieldWidth(20) << (m_trainerAdjustedMotion.size() > 0 ? m_trainerAdjustedMotion.back().timestamp : 0);
	m_sequenceLogData << endl;
	// motion serials, timestamps
	for (uint i = 0; i < m_bigMotionSize; i++) {
		m_sequenceLogData << qSetFieldWidth(10) << i;
		m_sequenceLogData << qSetFieldWidth(5) << (i < m_athleteAdjustedMotion.size() ? m_athleteAdjustedMotion[i].serial : 0);
		m_sequenceLogData << qSetFieldWidth(15) << (i < m_athleteAdjustedMotion.size() ? m_athleteAdjustedMotion[i].timestamp : 0);
		m_sequenceLogData << qSetFieldWidth(5) << (i < m_trainerAdjustedMotion.size() ? m_trainerAdjustedMotion[i].serial : 0);
		m_sequenceLogData << qSetFieldWidth(15) << (i < m_trainerAdjustedMotion.size() ? m_trainerAdjustedMotion[i].timestamp : 0);
		m_sequenceLogData << endl;
	}

	// motion type
	m_sequenceLogData << qSetFieldWidth(10) << "Resized" << endl;
	// motion size
	m_sequenceLogData << qSetFieldWidth(10) << "Size:";
	m_sequenceLogData << qSetFieldWidth(20) << m_athleteResizedMotion.size();
	m_sequenceLogData << qSetFieldWidth(20) << m_trainerResizedMotion.size();
	m_sequenceLogData << endl;
	// motion duration
	m_sequenceLogData << qSetFieldWidth(10) << "Duration:";
	m_sequenceLogData << qSetFieldWidth(20) << (m_athleteResizedMotion.size() > 0 ? m_athleteResizedMotion.back().timestamp : 0);
	m_sequenceLogData << qSetFieldWidth(20) << (m_trainerResizedMotion.size() > 0 ? m_trainerResizedMotion.back().timestamp : 0);
	m_sequenceLogData << endl;
	// motion serials, timestamps
	for (uint i = 0; i < m_bigMotionSize; i++) {
		m_sequenceLogData << qSetFieldWidth(10) << i;
		m_sequenceLogData << qSetFieldWidth(5) << (i < m_athleteResizedMotion.size() ? m_athleteResizedMotion[i].serial : 0);
		m_sequenceLogData << qSetFieldWidth(15) << (i < m_athleteResizedMotion.size() ? m_athleteResizedMotion[i].timestamp : 0);
		m_sequenceLogData << qSetFieldWidth(5) << (i < m_trainerResizedMotion.size() ? m_trainerResizedMotion[i].serial : 0);
		m_sequenceLogData << qSetFieldWidth(15) << (i < m_trainerResizedMotion.size() ? m_trainerResizedMotion[i].timestamp : 0);
		m_sequenceLogData << endl;
	}
}
const array<KLimb, NUM_LIMBS>& KSkeleton::limbs() const
{
	return m_limbs;
}
