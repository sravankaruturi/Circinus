#include "RigidBody.h"

using namespace DirectX;

RigidBody::RigidBody(Transform* t, const DirectX::BoundingBox* boxCollider)
	:
	velocity(0, 0, 0) 
{
	trans = t;
	IsSlerping = false;
	RigidDeltaTime = 0;
	slerpT = 0;
	timeSinceLastLoop = 0;
	slerpTime = 0;
	obb = new BoundingOrientedBox();
	DirectX::BoundingOrientedBox::CreateFromBoundingBox(*obb, *boxCollider);
}


RigidBody::~RigidBody()
{
	delete obb;
}

void RigidBody::Update(float deltatime)
{
	RigidDeltaTime = deltatime;
	trans->SetWorldPosition(trans->GetWorldPosition()->x + velocity.x * deltatime, trans->GetWorldPosition()->y + velocity.y * deltatime, trans->GetWorldPosition()->z + velocity.z * deltatime);
}

void RigidBody::SetWorldVelocity(float x, float y, float z)
{
	velocity.x = x;
	velocity.y = y;
	velocity.z = z;
}

void RigidBody::FaceTo(RigidBody* otherEntity)
{
	DirectX::XMVECTOR thisPosVec = DirectX::XMLoadFloat3(trans->GetWorldPosition());
	DirectX::XMVECTOR otherPosVec = DirectX::XMLoadFloat3(otherEntity->GetTransform()->GetWorldPosition());

	DirectX::XMVECTOR forwardVector = DirectX::XMVector3Normalize(otherPosVec - thisPosVec);

	DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0, 1, 0, 0);
	DirectX::XMVECTOR tangentVector = DirectX::XMVector3Cross(upVector, forwardVector);

	if (DirectX::XMVectorGetX(DirectX::XMVector3Length(tangentVector)) < 0.001f) //check if forward is pointed at the same dir as up
	{
		upVector = DirectX::XMVectorSet(1, 0, 0, 0);
		tangentVector = DirectX::XMVector3Cross(upVector, forwardVector);
	}

	tangentVector = DirectX::XMVector3Normalize(tangentVector);
	upVector = DirectX::XMVector3Cross(forwardVector, tangentVector);

	DirectX::XMFLOAT3 forwardF3(0, 0, 0);
	XMStoreFloat3(&forwardF3, forwardVector);
	DirectX::XMFLOAT3 upF3(0, 0, 0);
	XMStoreFloat3(&upF3, upVector);
	DirectX::XMFLOAT3 tangentF3(0, 0, 0);
	XMStoreFloat3(&tangentF3, tangentVector);

	DirectX::XMFLOAT3X3 newRotationMatrixF3X3(
		upF3.y, tangentF3.y, forwardF3.y,
		upF3.x, tangentF3.x, forwardF3.x,
		upF3.z, tangentF3.z, forwardF3.z);

	DirectX::XMMATRIX newRotationMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat3x3(&newRotationMatrixF3X3));

	DirectX::XMFLOAT3 dirFloat(0, 0, 0);
	DirectX::XMFLOAT3X3 newRotation3X3(0, 0, 0, 0, 0, 0, 0, 0, 0);
	DirectX::XMStoreFloat3x3(&newRotation3X3, newRotationMatrix);
	dirFloat = RotateToNewEuler(&newRotation3X3);

	trans->SetRotationEuler(dirFloat.x, dirFloat.y, dirFloat.z);
}

void RigidBody::ProjectileHomingAt(RigidBody* target, float speed) //Rotate this rigidbody and move it constantly towards the target. This has to be called every Tick()
{
	this->FaceTo(target);
	DirectX::XMFLOAT3 forward(0, 0, speed);
	DirectX::XMVECTOR localForward = DirectX::XMLoadFloat3(&forward);

	DirectX::XMVECTOR localRotationVec = DirectX::XMLoadFloat3(trans->GetLocalRotation());

	DirectX::XMVECTOR worldVelocityVec = XMVector3Rotate(localForward, DirectX::XMQuaternionRotationRollPitchYawFromVector(localRotationVec));
	DirectX::XMStoreFloat3(&(this->velocity), worldVelocityVec);
}

void RigidBody::ProjectileSlerpAngle(DirectX::XMVECTOR* angle, DirectX::XMVECTOR* targetAngle, float t)
{
	/*for (float i = 0; i < 1; i += RigidDeltaTime / time)
	{
		DirectX::XMMATRIX newRotationMatrix = DirectX::XMMatrixRotationQuaternion(DirectX::XMQuaternionSlerp(*angle, *targetAngle, i));

		DirectX::XMFLOAT3 dirFloat(0, 0, 0);
		DirectX::XMFLOAT3X3 newRotation3X3(0, 0, 0, 0, 0, 0, 0, 0, 0);
		DirectX::XMStoreFloat3x3(&newRotation3X3, newRotationMatrix);
		dirFloat = RotateToNewEuler(&newRotation3X3);

		trans->SetRotationEuler(dirFloat.x, dirFloat.y, dirFloat.z);
	}*/


	DirectX::XMMATRIX newRotationMatrix = DirectX::XMMatrixRotationQuaternion(DirectX::XMQuaternionSlerp(*angle, *targetAngle, t));

	DirectX::XMFLOAT3 dirFloat(0, 0, 0);
	DirectX::XMFLOAT3X3 newRotation3X3(0, 0, 0, 0, 0, 0, 0, 0, 0);
	DirectX::XMStoreFloat3x3(&newRotation3X3, newRotationMatrix); 

	//DirectX::XMFLOAT3X3 newRotationMatrixF3X3(0, 0, 0, 0, 0, 0, 0, 0, 0);
	//newRotationMatrixF3X3._11 = newRotation3X3._12;
	//newRotationMatrixF3X3._12 = newRotation3X3._32;
	//newRotationMatrixF3X3._13 = newRotation3X3._22;
	//newRotationMatrixF3X3._31 = newRotation3X3._21;
	//newRotationMatrixF3X3._32 = newRotation3X3._11;
	//newRotationMatrixF3X3._33 = newRotation3X3._31;
	//newRotationMatrixF3X3._21 = newRotation3X3._33;
	//newRotationMatrixF3X3._22 = newRotation3X3._23;
	//newRotationMatrixF3X3._23 = newRotation3X3._13;
	//newRotationMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat3x3(&newRotationMatrixF3X3));
	//DirectX::XMStoreFloat3x3(&newRotationMatrixF3X3, newRotationMatrix);
	//dirFloat = RotateToNewEuler(&newRotationMatrixF3X3);

	newRotationMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat3x3(&newRotation3X3));
	DirectX::XMStoreFloat3x3(&newRotation3X3, newRotationMatrix);
	dirFloat = RotateToNewEuler(&newRotation3X3);

	trans->SetRotationEuler(dirFloat.x, dirFloat.y, dirFloat.z);
}

void RigidBody::ProjectileSwarmingAt(RigidBody* target, float speed, int minTurnTime, int maxTurnTime, int maxOffAngle)
{
	if (!IsSlerping)
	{
		slerpT = 0;
		timeSinceLastLoop = 0;
		srand((time(0) * this->GetEntityID()));
		slerpTime = minTurnTime + (rand() % (maxTurnTime - minTurnTime));

		//currentQuat = DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3(trans->GetLocalRotation()));
		DirectX::XMFLOAT3 oldRot = *trans->GetLocalRotation();
		FaceTo(target);
		newQuat = DirectX::XMQuaternionRotationRollPitchYaw(
			trans->GetLocalRotation()->x + ((rand() % (2 * maxOffAngle) - maxOffAngle) / 360.0f), 
			trans->GetLocalRotation()->y + ((rand() % (2 * maxOffAngle) - maxOffAngle) / 360.0f), 
			trans->GetLocalRotation()->z + ((rand() % (2 * maxOffAngle) - maxOffAngle) / 360.0f));
		//newQuat = DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3(trans->GetLocalRotation()));
		/*newQuat = DirectX::XMQuaternionRotationRollPitchYaw(
			trans->GetLocalRotation()->x + (maxOffAngle / 360.0f),
			trans->GetLocalRotation()->y + (maxOffAngle / 360.0f),
			trans->GetLocalRotation()->z + (maxOffAngle / 360.0f));*/
		trans->SetRotationEuler(oldRot.x, oldRot.y, oldRot.z);

		IsSlerping = true;
	}

	else if (IsSlerping)
	{
		if (slerpT > 1)
		{
			currentQuat = newQuat;
			IsSlerping = false;
		}

		slerpT += RigidDeltaTime / (slerpTime / 1000.0f);
		//slerpT += RigidDeltaTime / slerpTime;
		ProjectileSlerpAngle(&currentQuat, &newQuat, slerpT);

		DirectX::XMVECTOR wrongQuat = DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3(trans->GetLocalRotation()));
		DirectX::XMMATRIX newRotationMatrix = DirectX::XMMatrixRotationQuaternion(wrongQuat);
		DirectX::XMFLOAT3 dirFloat(0, 0, 0);
		DirectX::XMFLOAT3X3 newRotation3X3(0, 0, 0, 0, 0, 0, 0, 0, 0);
		DirectX::XMStoreFloat3x3(&newRotation3X3, newRotationMatrix);
		newRotationMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat3x3(&newRotation3X3));
		DirectX::XMStoreFloat3x3(&newRotation3X3, newRotationMatrix);
		dirFloat = RotateToNewEuler(&newRotation3X3);
		trans->SetRotationEuler(dirFloat.x, dirFloat.y, dirFloat.z);

		timeSinceLastLoop += RigidDeltaTime;
	}

	//FaceTo(target);
	DirectX::XMFLOAT3 forward(0, 0, speed);
	DirectX::XMVECTOR localForward = DirectX::XMLoadFloat3(&forward);

	DirectX::XMVECTOR localRotationVec = DirectX::XMLoadFloat3(trans->GetLocalRotation());

	DirectX::XMVECTOR worldVelocityVec = XMVector3Rotate(localForward, DirectX::XMQuaternionRotationRollPitchYawFromVector(localRotationVec));
	DirectX::XMStoreFloat3(&(this->velocity), worldVelocityVec);
}

void RigidBody::ProjectileShootAt(RigidBody* target, float speed) //Rotate this rigidbody and shoot it towards the target. It won't change direction after shot.
{
	static int i = 0;

	if (i == 0)
	{
		this->FaceTo(target);
		i++;
	}

	DirectX::XMFLOAT3 forward(0, 0, speed);
	DirectX::XMVECTOR localForward = DirectX::XMLoadFloat3(&forward);

	DirectX::XMVECTOR localRotationVec = DirectX::XMLoadFloat3(trans->GetLocalRotation());

	DirectX::XMVECTOR worldVelocityVec = XMVector3Rotate(localForward, DirectX::XMQuaternionRotationRollPitchYawFromVector(localRotationVec));
	DirectX::XMStoreFloat3(&(this->velocity), worldVelocityVec);
}

DirectX::XMFLOAT3 RigidBody::RotateToNewEuler(DirectX::XMFLOAT3X3* m)
{
	XMFLOAT3 newEulerAngles(0, 0, 0);
	// Assuming the angles are in radians.
	if (m->_21 > 0.998) // singularity at north pole
	{ 
		newEulerAngles.x = std::atan2(m->_13, m->_33);
		newEulerAngles.y = DirectX::XM_PI / 2;
		newEulerAngles.z = 0;
		return newEulerAngles;
	}

	if (m->_21 < -0.998) // singularity at south pole
	{ 
		newEulerAngles.x = std::atan2(m->_13, m->_33);
		newEulerAngles.y = -DirectX::XM_PI / 2;
		newEulerAngles.z = 0;
		return newEulerAngles;
	}

	newEulerAngles.x = std::atan2(-m->_31, m->_11);
	newEulerAngles.y = std::atan2(-m->_23, m->_22);
	newEulerAngles.z = std::asin(m->_21);

	return newEulerAngles;
}

void RigidBody::SetEntity(int id)
{
	this->Component::SetEntity(id);
	// TODO: Update with transform position
}

bool RigidBody::SphereCollisionCheck(RigidBody *otherRbody)
{
	const BoundingOrientedBox &box1 = *(obb);
	BoundingSphere sphere1;
	BoundingSphere::CreateFromBoundingBox(sphere1, box1);
	XMFLOAT3 rBody1Position = *(trans->GetLocalPosition());
	sphere1.Center.x += rBody1Position.x;
	sphere1.Center.y += rBody1Position.y;
	sphere1.Center.z += rBody1Position.z;

	const BoundingOrientedBox &box2 = *(otherRbody->obb);
	BoundingSphere sphere2;
	BoundingSphere::CreateFromBoundingBox(sphere2, box2);
	XMFLOAT3 rBody2Position = *(otherRbody->GetTransform()->GetLocalPosition());
	sphere2.Center.x += rBody2Position.x;
	sphere2.Center.y += rBody2Position.y;
	sphere2.Center.z += rBody2Position.z;

	return sphere1.Intersects(sphere2);
}

bool RigidBody::BoxCollisionCheck(RigidBody * otherRbody)
{
	XMFLOAT3* pos1 = trans->GetLocalPosition();
	obb->Center.x = pos1->x;
	obb->Center.x = pos1->y;
	obb->Center.x = pos1->z;

	XMFLOAT3* pos2 = otherRbody->trans->GetLocalPosition();
	otherRbody->obb->Center.x = pos2->x;
	otherRbody->obb->Center.y = pos2->y;
	otherRbody->obb->Center.z = pos2->z;

	return obb->Intersects(*(otherRbody->obb));
}
