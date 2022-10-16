#include "hooks.h"
#include "settings.h"
#include "perilous.h"
#include "dodge.h"

#include "include/Utils.h"
namespace hooks
{
	void install()
	{
		on_attack_action::install();
	}

	/// <summary>
	/// Handle perilous attacking.
	/// </summary>
	/// <param name="a_actionData"></param>
	/// <returns>Whether the attack action is performed.</returns>
	bool on_attack_action::perform_atk_action(RE::TESActionData* a_actionData)
	{
		if (!a_actionData) {
			return _perform_atk_action(a_actionData);
		}
		auto ref = a_actionData->source.get();
		if (!ref) {
			return _perform_atk_action(a_actionData);
		}

		RE::Actor* actor = ref->As<RE::Actor>();
		if (!actor) {
			return _perform_atk_action(a_actionData);
		}
		if (settings::bPerilous_enable) {
			//perilous::GetSingleton()->attempt_start_perilous_attack(actor);
		}

		return _perform_atk_action(a_actionData);
	}

	void on_animation_event::ProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink, RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource)
	{
		if (!a_event->holder) {
			return;
		}
		std::string_view eventTag = a_event->tag.data();
		switch (hash(eventTag.data(), eventTag.size())) {
		case "preHitFrame"_h:
			dodge::GetSingleton()->react_to_attack(const_cast<RE::TESObjectREFR*>(a_event->holder)->As<RE::Actor>());
			perilous::GetSingleton()->attempt_start_perilous_attack(const_cast<RE::TESObjectREFR*>(a_event->holder)->As<RE::Actor>());
			break;
		case "attackStop"_h:
			perilous::GetSingleton()->attempt_end_perilous_attack(const_cast<RE::TESObjectREFR*>(a_event->holder)->As<RE::Actor>());
			break;
		}
	}

	EventResult on_animation_event::ProcessEvent_NPC(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink, RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource)
	{
		ProcessEvent(a_sink, a_event, a_eventSource);
		return _ProcessEvent_NPC(a_sink, a_event, a_eventSource);
	}

	EventResult on_animation_event::ProcessEvent_PC(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink, RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource)
	{
		ProcessEvent(a_sink, a_event, a_eventSource);
		return _ProcessEvent_PC(a_sink, a_event, a_eventSource);
	}

	ptr_CombatPath on_combatBehavior_backoff_createPath::create_path(RE::Actor* a_actor, RE::NiPoint3* a_newPos, float a3, int speed_ind)
	{
		switch (settings::iDodgeFramework) {
		case 0:
			dodge::GetSingleton()->attempt_dodge(a_actor, dodge::dodge_directions_tk_back);
			break;

		}
		
		return _create_path(a_actor, a_newPos, a3, speed_ind);
	}

	ptr_CombatPath on_combatBehavior_circle_createPath::create_path(RE::Actor* a_actor, RE::NiPoint3* a_newPos, float a3, int speed_ind)
	{
		switch (settings::iDodgeFramework) {
		case 0:
			dodge::GetSingleton()->attempt_dodge(a_actor, dodge::dodge_directions_tk_horizontal);
			break;
		}
		

		return _create_path(a_actor, a_newPos, a3, speed_ind);
	}

	ptr_CombatPath on_combatBehavior_fallback_createPath::create_path(RE::Actor* a_actor, RE::NiPoint3* a_newPos, float a3, int speed_ind)
	{
		switch (settings::iDodgeFramework) {
		case 0:
			dodge::GetSingleton()->attempt_dodge(a_actor, dodge::dodge::dodge_directions_tk_back);
			break;
		}
		
		return _create_path(a_actor, a_newPos, a3, speed_ind);
	}

	ptr_CombatPath on_combatBehavior_dodgethreat_createPath::create_path(RE::Actor* a_actor, RE::NiPoint3* a_newPos, float a3, int speed_ind)
	{
		switch (settings::iDodgeFramework) {
		case 0:
			dodge::GetSingleton()->attempt_dodge(a_actor, dodge::dodge_directions_tk_all, true);
			break;
		}
		

		return _create_path(a_actor, a_newPos, a3, speed_ind);

	}

	void on_melee_hit::processHit(RE::Actor* victim, RE::HitData& hitData)
	{
		RE::Actor* attacker = hitData.aggressor.get().get();
		if (attacker) {
			RE::ActorHandle handle;
			if (perilous::GetSingleton()->is_perilous_attacking(attacker, handle)) {
				auto headingAngle = victim->GetHeadingAngle(attacker->GetPosition(), false);
				auto direction = (headingAngle >= 0.0f) ? headingAngle / 360.0f : (360.0f + headingAngle) / 360.0f;
				victim->SetGraphVariableFloat("staggerDirection", direction);
				victim->SetGraphVariableFloat("StaggerMagnitude", 0.7);
				victim->NotifyAnimationGraph("staggerStart");
			}
		}

		_ProcessHit(victim, hitData);
	}


	inline void offsetRotation(RE::Actor* a_actor, float& a_angle) 
	{
		float angleDelta = Utils::math::NormalRelativeAngle(a_angle - a_actor->data.angle.z);
		switch (a_actor->AsActorState()->GetAttackState()) {
		case RE::ATTACK_STATE_ENUM::kDraw:
			angleDelta *= settings::fNPCCommitment_AttackStartMult;
			break;
		case RE::ATTACK_STATE_ENUM::kSwing:
		case RE::ATTACK_STATE_ENUM::kHit:
			angleDelta *= settings::fNPCCommitment_AttackMidMult;
			break;
		case RE::ATTACK_STATE_ENUM::kNextAttack:
		case RE::ATTACK_STATE_ENUM::kFollowThrough:
			angleDelta *= settings::fNPCCommitment_AttackEndMult;
			break;
		}
		a_angle = a_actor->data.angle.z + angleDelta;
	}

	
	void on_set_rotation::Actor_SetRotationX(RE::Actor* a_this, float a_angle)
	{
		if (!a_this->IsPlayerRef()) {
			offsetRotation(a_this, a_angle);
		}
		_Actor_SetRotationX(a_this, a_angle);
	}

	void on_set_rotation::Actor_SetRotationZ(RE::Actor* a_this, float a_angle)
	{
		if (!a_this->IsPlayerRef()) {
			offsetRotation(a_this, a_angle);
		}
		_Actor_SetRotationZ(a_this, a_angle);
	}

};
