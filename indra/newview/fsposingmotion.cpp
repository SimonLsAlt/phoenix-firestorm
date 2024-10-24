/**
 * @file fsposingmotion.cpp
 * @brief Model for posing your (and other) avatar(s).
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Phoenix Firestorm Viewer Source Code
 * Copyright (c) 2024 Angeldark Raymaker @ Second Life
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "fsposingmotion.h"
#include "llcharacter.h"

FSPosingMotion::FSPosingMotion(const LLUUID &id) : LLMotion(id)
{
    mName = "fs_poser_pose";
    _motionID = id;
}

LLMotion::LLMotionInitStatus FSPosingMotion::onInitialize(LLCharacter *character)
{
    if (!character)
        return STATUS_FAILURE;

    _jointPoses.clear();

    LLJoint* targetJoint;
    for (S32 i = 0; (targetJoint = character->getCharacterJoint(i)); ++i)
    {
        if (!targetJoint)
            continue;

        FSJointPose jointPose = FSJointPose(targetJoint);
        _jointPoses.push_back(jointPose);

        addJointState(jointPose.getJointState());
    }

    for (S32 i = 0; (targetJoint = character->findCollisionVolume(i)); ++i)
    {
        if (!targetJoint)
            continue;

        FSJointPose jointPose = FSJointPose(targetJoint, true);
        _jointPoses.push_back(jointPose);

        addJointState(jointPose.getJointState());
    }

    return STATUS_SUCCESS;
}

bool FSPosingMotion::onActivate() { return true; }

bool FSPosingMotion::onUpdate(F32 time, U8* joint_mask)
{
    LLQuaternion targetRotation;
    LLQuaternion currentRotation;
    LLVector3 currentPosition;
    LLVector3 targetPosition;
    LLVector3 currentScale;
    LLVector3 targetScale;

    for (FSJointPose jointPose : _jointPoses)
    {
        LLJoint* joint = jointPose.getJointState()->getJoint();
        if (!joint)
            continue;

        currentRotation = joint->getRotation();
        currentPosition = joint->getPosition();
        currentScale = joint->getScale();
        targetRotation = jointPose.getTargetRotation();
        targetPosition = jointPose.getTargetPosition();
        targetScale = jointPose.getTargetScale();

        if (currentPosition != targetPosition)
        {
            currentPosition = lerp(currentPosition, targetPosition, _interpolationTime);
            jointPose.getJointState()->setPosition(currentPosition);
        }

        if (currentRotation != targetRotation)
        {
            currentRotation = slerp(_interpolationTime, currentRotation, targetRotation);
            jointPose.getJointState()->setRotation(currentRotation);
        }

        if (currentScale != targetScale)
        {
            currentScale = lerp(currentScale, targetScale, _interpolationTime);
            jointPose.getJointState()->setScale(currentScale);
        }
    }

    return true;
}

void FSPosingMotion::onDeactivate() { revertChangesToPositionsScalesAndCollisionVolumes(); }

void FSPosingMotion::revertChangesToPositionsScalesAndCollisionVolumes()
{
    for (FSJointPose jointPose : _jointPoses)
    {
        jointPose.revertJointScale();
        jointPose.revertJointPosition();

        if (jointPose.isCollisionVolume())
            jointPose.revertCollisionVolume();

        LLJoint* joint = jointPose.getJointState()->getJoint();
        if (!joint)
            continue;

        addJointToState(joint);
    }
}

bool FSPosingMotion::currentlyPosingJoint(FSJointPose* joint)
{
    if (!joint)
        return false;

    LLJoint* avJoint = joint->getJointState()->getJoint();
    if (!avJoint)
        return false;

    return currentlyPosingJoint(avJoint);
}

void FSPosingMotion::addJointToState(FSJointPose* joint)
{
    if (!joint)
        return;

    LLJoint* avJoint = joint->getJointState()->getJoint();
    if (!avJoint)
        return;

    setJointState(avJoint, POSER_JOINT_STATE);
}

void FSPosingMotion::removeJointFromState(FSJointPose* joint)
{
    if (!joint)
        return;

    LLJoint* avJoint = joint->getJointState()->getJoint();
    if (!avJoint)
        return;

    joint->revertJointScale();
    joint->revertJointPosition();

    if (joint->isCollisionVolume())
        joint->revertCollisionVolume();

    setJointState(avJoint, 0);
}

void FSPosingMotion::addJointToState(LLJoint* joint) { setJointState(joint, POSER_JOINT_STATE); }

void FSPosingMotion::removeJointFromState(LLJoint *joint) { setJointState(joint, 0); }

void FSPosingMotion::setJointState(LLJoint *joint, U32 state)
{
    if (_jointPoses.size() < 1)
        return;
    if (!joint)
        return;

    LLPose* pose = this->getPose();
    if (!pose)
        return;

    LLPointer<LLJointState> jointState = pose->findJointState(joint);
    if (jointState.isNull())
        return;

    pose->removeJointState(jointState);
    FSJointPose *jointPose = getJointPoseByJointName(joint->getName());
    if (!jointPose)
        return;

    jointPose->getJointState()->setUsage(state);
    addJointState(jointPose->getJointState());
}

FSPosingMotion::FSJointPose* FSPosingMotion::getJointPoseByJointName(std::string name)
{
    if (_jointPoses.size() < 1)
        return nullptr;

    std::vector<FSPosingMotion::FSJointPose>::iterator poserJoint_iter;
    for (poserJoint_iter = _jointPoses.begin(); poserJoint_iter != _jointPoses.end(); ++poserJoint_iter)
    {
        if (!boost::iequals(poserJoint_iter->jointName(), name))
            continue;

        return &*poserJoint_iter;
    }

    return nullptr;
}

bool FSPosingMotion::currentlyPosingJoint(LLJoint* joint)
{
    if (_jointPoses.size() < 1)
        return false;

    if (!joint)
        return false;

    LLPose* pose = this->getPose();
    if (!pose)
        return false;

    LLPointer<LLJointState> jointState = pose->findJointState(joint);
    if (jointState.isNull())
        return false;

    U32 state = jointState->getUsage();
    return (state & POSER_JOINT_STATE);
}