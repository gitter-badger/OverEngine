#include "pcheader.h"
#include "TransformComponent.h"

namespace OverEngine
{
	#define ENTITY_FROM_HANDLE(handle) Entity{ handle, AttachedEntity.GetScene() }
	#define ENTITY_HANDLE_TRANSFORM(handle) ENTITY_FROM_HANDLE(handle).GetComponent<TransformComponent>()

	Vector3 TransformComponent::GetPosition() const
	{
		return m_LocalToWorld[3];
	}

	void TransformComponent::SetPosition(const Vector3& position)
	{
		if (m_Parent != entt::null)
		{
			auto mvm = ENTITY_HANDLE_TRANSFORM(m_Parent).GetLocalToWorld();

			for (uint8_t i = 0; i < 3; i++)
			{
				if (mvm[i][i] == 0.0f)
					mvm[i][i] = 1.0;
			}

			mvm = glm::inverse(mvm);
			SetLocalPosition(Vector3(mvm * Vector4(position, 1.0f)));
		}
		else
			SetLocalPosition(position);
	}

	Vector3 TransformComponent::GetLocalPosition() const
	{
		return m_LocalToParent[3];
	}

	void TransformComponent::SetLocalPosition(const Vector3& position)
	{
		m_LocalToParent[3].x = position.x;
		m_LocalToParent[3].y = position.y;
		m_LocalToParent[3].z = position.z;

		m_ChangedFlags |= ChangedFlags_LocalToWorld_RN;
		Invalidate();
		Change();
	}

	Vector3 TransformComponent::GetEulerAngles()
	{
		if (m_Parent == entt::null)
			return GetLocalEulerAngles();

		return QuaternionToEulerAngles(GetRotation());
	}

	void TransformComponent::SetEulerAngles(const Vector3& rotation)
	{
		if (m_Parent != entt::null)
			SetLocalRotation(glm::quat_cast(glm::inverse(ENTITY_HANDLE_TRANSFORM(m_Parent).GetLocalToWorld())
				* glm::mat4_cast(EulerAnglesToQuaternion(rotation))));
		else
			SetLocalEulerAngles(rotation);
	}

	void TransformComponent::SetLocalEulerAngles(const Vector3& rotation)
	{
		m_LocalEulerAngles = rotation;
		m_LocalRotation = EulerAnglesToQuaternion(m_LocalEulerAngles);

		m_ChangedFlags |= ChangedFlags_LocalToParent_RN;
		Invalidate();
		Change();
	}

	Quaternion TransformComponent::GetRotation()
	{
		if (m_Parent == entt::null)
			return GetLocalRotation();

		Mat3x3 rotationMat = {
			m_LocalToWorld[0].x, m_LocalToWorld[0].y, m_LocalToWorld[0].z,
			m_LocalToWorld[1].x, m_LocalToWorld[1].y, m_LocalToWorld[1].z,
			m_LocalToWorld[2].x, m_LocalToWorld[2].y, m_LocalToWorld[2].z,
		};

		Vector3 lossyScale = GetLossyScale();
		rotationMat[0] /= lossyScale.x;
		rotationMat[1] /= lossyScale.y;
		rotationMat[2] /= lossyScale.z;

		return glm::quat_cast(rotationMat);
	}

	void TransformComponent::SetRotation(const Quaternion& rotation)
	{
		if (m_Parent != entt::null)
			SetLocalRotation(glm::quat_cast(glm::inverse(ENTITY_HANDLE_TRANSFORM(m_Parent).GetLocalToWorld())
				* glm::mat4_cast(rotation)));
		else
			SetLocalRotation(rotation);
	}

	void TransformComponent::SetLocalRotation(const Quaternion& rotation)
	{
		m_LocalEulerAngles = QuaternionToEulerAngles(rotation);
		m_LocalRotation = rotation;

		m_ChangedFlags |= ChangedFlags_LocalToParent_RN;
		Invalidate();
		Change();
	}

	void TransformComponent::SetLocalScale(const Vector3& scale)
	{
		m_LocalScale = scale;

		m_ChangedFlags |= ChangedFlags_LocalToParent_RN;
		Invalidate();
		Change();
	}

	Vector3 TransformComponent::GetLossyScale() const
	{
		return {
			glm::fastLength(Vector3(m_LocalToWorld[0])),
			glm::fastLength(Vector3(m_LocalToWorld[1])),
			glm::fastLength(Vector3(m_LocalToWorld[2]))
		};
	}

	void TransformComponent::DetachFromParent()
	{
		if (m_Parent != entt::null)
		{
			auto& parentChildren = ENTITY_HANDLE_TRANSFORM(m_Parent).m_Children;
			auto it = STD_CONTAINER_FIND(parentChildren, AttachedEntity.GetRuntimeID());
			if (it != parentChildren.end())
				parentChildren.erase(it);

			m_Parent = entt::null;

			AttachedEntity.GetScene()->GetRootHandles().push_back(AttachedEntity.GetRuntimeID());

			m_ChangedFlags |= ChangedFlags_LocalToWorld_RN;
			Invalidate();
			Change();
		}
	}

	void TransformComponent::DetachChildren()
	{
		for (const auto& child : m_Children)
			ENTITY_HANDLE_TRANSFORM(child).DetachFromParent();
	}

	Entity TransformComponent::GetParent() const
	{
		return ENTITY_FROM_HANDLE(m_Parent);
	}

	void TransformComponent::SetParent(Entity parent)
	{
		if (parent)
		{
			// Add to parent children list
			ENTITY_HANDLE_TRANSFORM(parent).m_Children.push_back(AttachedEntity.GetRuntimeID());
			
			if (m_Parent == entt::null)
			{
				auto& sceneRootHandles = AttachedEntity.GetScene()->GetRootHandles();
				auto it = STD_CONTAINER_FIND(sceneRootHandles, AttachedEntity.GetRuntimeID());
				OE_CORE_ASSERT(it != sceneRootHandles.end(), "Cannot find Entity in scene root list!");
				sceneRootHandles.erase(it);
			}
			else
			{
				auto& parentChildren = ENTITY_HANDLE_TRANSFORM(m_Parent).m_Children;
				auto it = STD_CONTAINER_FIND(parentChildren, AttachedEntity.GetRuntimeID());
				OE_CORE_ASSERT(it != parentChildren.end(), "Cannot find Entity in parent's children list!");
				parentChildren.erase(it);
			}

			m_Parent = parent.GetRuntimeID();

			m_ChangedFlags |= ChangedFlags_LocalToWorld_RN;
			Invalidate();
			Change();
		}
		else
		{
			DetachFromParent();
		}
	}

	uint32_t TransformComponent::GetSiblingIndex()
	{
		if (m_Parent == entt::null)
		{
			const auto& rootEntities = AttachedEntity.GetScene()->GetRootHandles();
			auto it = STD_CONTAINER_FIND(rootEntities, AttachedEntity.GetRuntimeID());
			return (uint32_t)(it - rootEntities.begin());
		}

		const auto& parentChildren = ENTITY_HANDLE_TRANSFORM(m_Parent).GetChildrenHandles();
		auto it = STD_CONTAINER_FIND(parentChildren, AttachedEntity.GetRuntimeID());
		return (uint32_t)(it - parentChildren.begin());
	}

	void TransformComponent::SetSiblingIndex(uint32_t index)
	{
		if (m_Parent == entt::null)
		{
			auto& rootEntities = AttachedEntity.GetScene()->GetRootHandles();
			auto it = STD_CONTAINER_FIND(rootEntities, AttachedEntity.GetRuntimeID());
			Move(rootEntities, it - rootEntities.begin(), index);

			return;
		}

		auto& parentChildren = ENTITY_HANDLE_TRANSFORM(m_Parent).GetChildrenHandles();
		auto it = STD_CONTAINER_FIND(parentChildren, AttachedEntity.GetRuntimeID());
		Move(parentChildren, it - parentChildren.begin(), index);
	}

	void TransformComponent::Invalidate()
	{
		if (m_ChangedFlags & ChangedFlags_LocalToParent_RN)
		{
			Vector4 lastCol = m_LocalToParent[3];
			m_LocalToParent = glm::mat4_cast(m_LocalRotation) * SCALE_MAT4X4(m_LocalScale);
			m_LocalToParent[3] = lastCol;
		}

		if (m_ChangedFlags & ChangedFlags_LocalToWorld_RN || m_ChangedFlags & ChangedFlags_LocalToParent_RN)
		{
			if (m_Parent != entt::null)
				m_LocalToWorld = ENTITY_HANDLE_TRANSFORM(m_Parent).GetLocalToWorld() * m_LocalToParent;
			else
				m_LocalToWorld = m_LocalToParent;

			if (m_ChangedFlags & ChangedFlags_LocalToWorld_RN)
				m_ChangedFlags ^= ChangedFlags_LocalToWorld_RN;
		}

		if (m_ChangedFlags & ChangedFlags_LocalToParent_RN)
			m_ChangedFlags ^= ChangedFlags_LocalToParent_RN;
	}

	// Add changed flag and force all children to update
	void TransformComponent::Change()
	{
		m_ChangedFlags |= ChangedFlags_Changed | ChangedFlags_ChangedForPhysics;

		for (const auto& child : m_Children)
		{
			auto& childTransform = ENTITY_HANDLE_TRANSFORM(child);
			childTransform.m_ChangedFlags |= ChangedFlags_LocalToWorld_RN;
			childTransform.Invalidate();
			childTransform.Change();
		}
	}
}
