#pragma once

#include <openvr.h>

#include "osc_sender.h"
#include "configuration.h"


class ActionState {

private:
	Action action_;
	vr::VRActionHandle_t action_handle_;

	int rotate_current_;

	OSCMessage key_value_to_message(const OneShotKeyValue& kv) {
		switch (kv.type) {
		case ValueType::BOOL:  return OSCMessage(kv.key, kv.value.i32);
		case ValueType::INT:   return OSCMessage(kv.key, kv.value.i32);
		case ValueType::FLOAT: return OSCMessage(kv.key, kv.value.f32);
		}
		return OSCMessage();
	}

	std::vector<OSCMessage> handle_analog_action() {
		std::vector<OSCMessage> messages;
		vr::InputAnalogActionData_t action_data;
		vr::VRInput()->GetAnalogActionData(
			action_handle_, &action_data, sizeof(action_data), vr::k_ulInvalidInputValueHandle);
		if (!action_data.bActive) { return messages; }
		const auto& values = action_.analog_param().values;
		for (const auto& kv : values) {
			const float input = action_data.x;
			float output = 0.0f;
			if (input < kv.input_min) {
				output = kv.output_min;
			}
			else if (input > kv.input_max) {
				output = kv.output_max;
			}
			else {
				const float s = (kv.output_max - kv.output_min) / (kv.input_max - kv.input_min);
				output = (input - kv.input_min) * s + kv.output_min;
			}
			messages.emplace_back(kv.key, output);
		}
		return messages;
	}

	std::vector<OSCMessage> handle_binary_action() {
		std::vector<OSCMessage> messages;
		vr::InputDigitalActionData_t action_data;
		vr::VRInput()->GetDigitalActionData(
			action_handle_, &action_data, sizeof(action_data), vr::k_ulInvalidInputValueHandle);
		if (!action_data.bActive) { return messages; }
		if (!action_data.bChanged) { return messages; }
		const auto& param = action_.binary_param();
		const auto& values = action_data.bState ? param.press_values : param.release_values;
		for (const auto& kv : values) {
			messages.push_back(key_value_to_message(kv));
		}
		return messages;
	}

	std::vector<OSCMessage> handle_rotate_action() {
		std::vector<OSCMessage> messages;
		vr::InputDigitalActionData_t action_data;
		vr::VRInput()->GetDigitalActionData(
			action_handle_, &action_data, sizeof(action_data), vr::k_ulInvalidInputValueHandle);
		if (!action_data.bActive) { return messages; }
		if (!action_data.bChanged) { return messages; }
		if (!action_data.bState) { return messages; }
		const auto& param = action_.rotate_param().elements;
		const int n = static_cast<int>(param.size());
		if (0 <= rotate_current_ && rotate_current_ < n) {
			for (const auto& kv : param[rotate_current_].release_values) {
				messages.push_back(key_value_to_message(kv));
			}
		}
		if (++rotate_current_ >= n) { rotate_current_ = 0; }
		if (0 <= rotate_current_ && rotate_current_ < n) {
			for (const auto& kv : param[rotate_current_].press_values) {
				messages.push_back(key_value_to_message(kv));
			}
		}
		return messages;
	}

public:
	ActionState(const Action& action, const std::string& set_id)
		: action_(action)
		, action_handle_(vr::k_ulInvalidActionHandle)
		, rotate_current_(-1)
	{
		const std::string name = "/actions/" + set_id + "/in/" + action.id();
		vr::VRInput()->GetActionHandle(name.c_str(), &action_handle_);
	}

	std::vector<OSCMessage> update() {
		if (action_.is_analog()) {
			return handle_analog_action();
		}
		else if (action_.is_binary()) {
			return handle_binary_action();
		}
		else if (action_.is_rotate()) {
			return handle_rotate_action();
		}
		return {};
	}

};


class InputHandler {

private:
	std::vector<vr::VRActionSetHandle_t> action_set_handles_;
	std::vector<ActionState> action_states_;

public:
	InputHandler(const std::vector<ActionSet> action_sets)
		: action_set_handles_()
		, action_states_()
	{
		for (const auto& action_set : action_sets) {
			vr::VRActionSetHandle_t handle = vr::k_ulInvalidActionSetHandle;
			vr::VRInput()->GetActionSetHandle(("/actions/" + action_set.id()).c_str(), &handle);
			action_set_handles_.push_back(handle);
			for (const auto& action : action_set.actions()) {
				action_states_.emplace_back(action, action_set.id());
			}
		}
	}

	std::vector<OSCMessage> update() {
		std::vector<vr::VRActiveActionSet_t> action_sets;
		for (const auto& handle : action_set_handles_) {
			vr::VRActiveActionSet_t action_set = { 0 };
			action_set.ulActionSet = handle;
			action_sets.push_back(std::move(action_set));
		}
		vr::VRInput()->UpdateActionState(
			action_sets.data(), sizeof(action_sets[0]), static_cast<uint32_t>(action_sets.size()));

		std::vector<OSCMessage> messages;
		for (auto& state : action_states_) {
			auto tmp = state.update();
			for (auto& m : tmp) { messages.push_back(std::move(m)); }
		}

		return messages;
	}

};
