#include "pcheader.h"
#include "PhysicWorld2D.h"

#include <box2d/box2d.h>

namespace OverEngine
{
	PhysicWorld2D::PhysicWorld2D(Vector2 gravity)
		: m_WorldHandle(b2Vec2(gravity.x, gravity.y))
	{
	}

	Ref<RigidBody2D> PhysicWorld2D::CreateRigidBody(const RigidBody2DProps& props)
	{
		b2BodyDef def;

		def.type = (b2BodyType)props.Type;

		def.position.Set(props.Position.x, props.Position.y);
		def.angle = props.Rotation;

		def.linearVelocity.Set(props.LinearVelocity.x, props.LinearVelocity.y);
		def.angularVelocity = props.AngularVelocity;

		def.linearDamping = props.LinearDamping;
		def.angularVelocity = props.AngularDamping;

		def.allowSleep = props.AllowSleep;
		def.awake = props.Awake;
		def.enabled = props.Enabled;

		def.fixedRotation = props.FixedRotation;
		def.gravityScale = props.GravityScale;
		def.bullet = props.Bullet;

		auto body = CreateRef<RigidBody2D>(m_WorldHandle.CreateBody(&def));
		m_Bodies.push_back(body);
		return body;
	}

	void PhysicWorld2D::DestroyRigidBody(const Ref<RigidBody2D>& rigidBody)
	{
		if (!rigidBody || !rigidBody->m_BodyHandle)
			return;

		auto it = STD_CONTAINER_FIND(m_Bodies, rigidBody);

		if (it == m_Bodies.end())
			return;

		m_WorldHandle.DestroyBody(rigidBody->m_BodyHandle);
		rigidBody->m_BodyHandle = nullptr;
		m_Bodies.erase(it);
	}

	Vector2 PhysicWorld2D::GetGravity() const
	{
		return { m_WorldHandle.GetGravity().x, m_WorldHandle.GetGravity().y };
	}

	void PhysicWorld2D::SetGravity(const Vector2& gravity)
	{
		m_WorldHandle.SetGravity({ gravity.x, gravity.y });
	}

	void PhysicWorld2D::OnUpdate(TimeStep ts, uint32_t velocityIterations, uint32_t positionIterations)
	{
		m_WorldHandle.Step(ts, velocityIterations, positionIterations);
	}
}
