#include "pcheader.h"
#include "RigidBody2D.h"

#include "PhysicWorld2D.h"

#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_shape.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>

namespace OverEngine
{
	RigidBody2D::RigidBody2D(b2Body* bodyHandle)
		: m_BodyHandle(bodyHandle)
	{
	}

	RigidBody2DType RigidBody2D::GetType()
	{
		return (RigidBody2DType)m_BodyHandle->GetType();
	}

	void RigidBody2D::SetType(const RigidBody2DType& type)
	{
		m_BodyHandle->SetType((b2BodyType)type);
	}

	Vector2 RigidBody2D::GetPosition()
	{
		const auto& pos = m_BodyHandle->GetPosition();
		return { pos.x, pos.y };
	}

	void RigidBody2D::SetPosition(const Vector2& position)
	{
		m_BodyHandle->SetTransform(
			{ position.x, position.y },
			m_BodyHandle->GetAngle()
		);
	}

	float RigidBody2D::GetRotation()
	{
		return m_BodyHandle->GetAngle();
	}
	
	void RigidBody2D::SetRotation(float rotation)
	{
		m_BodyHandle->SetTransform(
			m_BodyHandle->GetPosition(),
			rotation
		);
	}

	Vector2 RigidBody2D::GetLinearVelocity()
	{
		return { m_BodyHandle->GetLinearVelocity().x, m_BodyHandle->GetLinearVelocity().y };
	}

	void RigidBody2D::SetLinearVelocity(Vector2 velocity)
	{
		m_BodyHandle->SetLinearVelocity({ velocity.x, velocity.y });
	}

	float RigidBody2D::GetAngularVelocity()
	{
		return m_BodyHandle->GetAngularVelocity();
	}

	void RigidBody2D::SetAngularVelocity(float velocity)
	{
		m_BodyHandle->SetAngularVelocity(velocity);
	}

	bool RigidBody2D::IsEnabled() const
	{
		return m_BodyHandle->IsEnabled();
	}

	void RigidBody2D::SetEnabled(bool enabled)
	{
		m_BodyHandle->SetEnabled(enabled);
	}

	Ref<Collider2D> RigidBody2D::CreateCollider(const Collider2DProps& props)
	{
		struct
		{
			b2PolygonShape polygonShape;
			b2CircleShape circleShape;
		} shapes;

		b2FixtureDef def;
		Vector2 sizeHint(0.0f);

		if (props.Shape.Type == Collider2DType::Box)
		{
			shapes.polygonShape = b2PolygonShape();
			shapes.polygonShape.SetAsBox(
				props.Shape.BoxSize.x / 2.0f, props.Shape.BoxSize.y / 2.0f,
				{ 0.0f, 0.0f }, 0.0f
			);

			def.shape = &shapes.polygonShape;
			sizeHint = props.Shape.BoxSize;
		}
		else if (props.Shape.Type == Collider2DType::Circle)
		{
			shapes.circleShape = b2CircleShape();
			shapes.circleShape.m_radius = props.Shape.CircleRadius;

			def.shape = &shapes.circleShape;
			sizeHint.x = props.Shape.CircleRadius;
		}

		def.isSensor = props.IsTrigger;
		def.friction = props.Friction;
		def.density = props.Density;
		def.restitution = props.Bounciness;
		def.restitutionThreshold = props.BouncinessThreshold;

		b2Fixture* fixture = m_BodyHandle->CreateFixture(&def);

		auto collider = CreateRef<Collider2D>(this, fixture, props.Shape.Type, props.Offset, props.Rotation, sizeHint);
		m_Colliders.push_back(collider);
		return collider;
	}

	void RigidBody2D::DestroyCollider(const Ref<Collider2D>& collider)
	{
		m_BodyHandle->DestroyFixture(collider->m_FixtureHandle);
		collider->m_FixtureHandle = nullptr;
	}
}
