#pragma once

#include <string>
#include <vector>
#include <optional>

#include <nlohmann/json.hpp>


enum class ValueType {
	BOOL,
	INT,
	FLOAT,
};

struct OneShotKeyValue {
	std::string key;
	ValueType type;
	union {
		int32_t i32;
		float f32;
	} value;

	static OneShotKeyValue from_json(const nlohmann::json& obj) {
		OneShotKeyValue ret;
		ret.key = obj["key"];
		const auto& value = obj["value"];
		if (value.is_boolean()) {
			ret.type = ValueType::BOOL;
			ret.value.i32 = static_cast<bool>(value);
		}
		else if (value.is_number_integer()) {
			ret.type = ValueType::INT;
			ret.value.i32 = static_cast<int32_t>(value);
		}
		else if (value.is_number_float()) {
			ret.type = ValueType::FLOAT;
			ret.value.f32 = static_cast<float>(value);
		}
		else {
			throw std::runtime_error("a value with unsupported type is specified.");
		}
		return ret;
	}
};

struct RangeKeyValue {
	std::string key;
	float input_min;
	float input_max;
	float output_min;
	float output_max;

	static RangeKeyValue from_json(const nlohmann::json& obj) {
		RangeKeyValue ret;
		ret.key = obj["key"];
		ret.input_min = obj["input_min"];
		ret.input_max = obj["input_max"];
		ret.output_min = obj["output_min"];
		ret.output_max = obj["output_max"];
		return ret;
	}
};


struct AnalogActionParameter {
	std::vector<RangeKeyValue> values;

	static AnalogActionParameter from_json(const nlohmann::json& obj) {
		AnalogActionParameter ret;
		for (const auto& elem : obj) {
			ret.values.push_back(RangeKeyValue::from_json(elem));
		}
		return ret;
	}
};

struct BinaryActionParameter {
	std::vector<OneShotKeyValue> press_values;
	std::vector<OneShotKeyValue> release_values;

	static BinaryActionParameter from_json(const nlohmann::json& obj) {
		BinaryActionParameter ret;
		if (obj.contains("press")) {
			for (const auto& elem : obj["press"]) {
				ret.press_values.push_back(OneShotKeyValue::from_json(elem));
			}
		}
		if (obj.contains("release")) {
			for (const auto& elem : obj["release"]) {
				ret.release_values.push_back(OneShotKeyValue::from_json(elem));
			}
		}
		return ret;
	}
};

struct RotateActionParameter {
	std::vector<BinaryActionParameter> elements;

	static RotateActionParameter from_json(const nlohmann::json& obj) {
		RotateActionParameter ret;
		for (const auto& rotate_elem : obj) {
			BinaryActionParameter bp;
			if (rotate_elem.contains("enter")) {
				for (const auto& elem : rotate_elem["enter"]) {
					bp.press_values.push_back(OneShotKeyValue::from_json(elem));
				}
			}
			if (rotate_elem.contains("exit")) {
				for (const auto& elem : rotate_elem["exit"]) {
					bp.release_values.push_back(OneShotKeyValue::from_json(elem));
				}
			}
			ret.elements.push_back(std::move(bp));
		}
		return ret;
	}
};


class Action {

private:
	std::string id_;
	std::string name_;

	std::optional<AnalogActionParameter> analog_;
	std::optional<BinaryActionParameter> binary_;
	std::optional<RotateActionParameter> rotate_;

public:
	static Action from_json(const nlohmann::json& obj) {
		Action action;
		action.id_ = obj["id"];
		action.name_ = obj["name"];
		if (obj.contains("analog")) {
			action.analog_ = AnalogActionParameter::from_json(obj["analog"]);
		}
		if (obj.contains("binary")) {
			action.binary_ = BinaryActionParameter::from_json(obj["binary"]);
		}
		if (obj.contains("rotate")) {
			action.rotate_ = RotateActionParameter::from_json(obj["rotate"]);
		}
		return action;
	}

	const std::string& id() const { return id_; }
	const std::string& name() const { return name_; }

	bool is_analog() const { return analog_.has_value(); }
	bool is_binary() const { return binary_.has_value(); }
	bool is_rotate() const { return rotate_.has_value(); }

	const AnalogActionParameter& analog_param() const { return *analog_; }
	const BinaryActionParameter& binary_param() const { return *binary_; }
	const RotateActionParameter& rotate_param() const { return *rotate_; }

};


class ActionSet {

private:
	std::string id_;
	std::string name_;
	std::vector<Action> actions_;

public:
	static ActionSet from_json(const nlohmann::json& obj) {
		ActionSet action_set;
		action_set.id_ = obj["id"];
		action_set.name_ = obj["name"];
		for (const auto& elem : obj["actions"]) {
			action_set.actions_.push_back(Action::from_json(elem));
		}
		return action_set;
	}

	const std::string& id() const { return id_; }
	const std::string& name() const { return name_; }
	const std::vector<Action>& actions() const { return actions_; }

};

