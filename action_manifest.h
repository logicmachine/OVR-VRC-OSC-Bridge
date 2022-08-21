#pragma once

#include <ostream>

#include <nlohmann/json.hpp>


template <typename Iterator>
void generate_action_manifest(std::ostream& os, Iterator first, Iterator last) {
	nlohmann::json actions = nlohmann::json::array();
	nlohmann::json action_sets = nlohmann::json::array();
	nlohmann::json labels = nlohmann::json::object();
	labels["language_tag"] = "en_us";

	for (Iterator it = first; it != last; ++it) {
		const auto& action_set = *it;
		const auto set_id = "/actions/" + action_set.id();
		action_sets.push_back(nlohmann::json({
			{"name", set_id },
			{"usage", "leftright"}
		}));
		labels[set_id] = action_set.name();
		for (const auto& action : action_set.actions()) {
			const auto action_id = set_id + "/in/" + action.id();
			if (action.is_analog()) {
				actions.push_back(nlohmann::json({
					{"name", action_id},
					{"requirement", "optional"},
					{"type", "vector1"}
				}));
			}
			else if (action.is_binary()) {
				actions.push_back(nlohmann::json({
					{"name", action_id},
					{"requirement", "optional"},
					{"type", "boolean"}
				}));
			}
			else if (action.is_rotate()) {
				actions.push_back(nlohmann::json({
					{"name", action_id},
					{"requirement", "optional"},
					{"type", "boolean"}
				}));
			}
			labels[action_id] = action.name();
		}
	}

	nlohmann::json all = nlohmann::json::object();;
	all["default_bindings"] = nlohmann::json::array();
	/*
	all["default_bindings"] = nlohmann::json::array({
		nlohmann::json({
			{"controller_type", "generic"},
			{"binding_url", "default_generic.json"}
		})
	});
	*/
	all["actions"] = std::move(actions);
	all["action_sets"] = std::move(action_sets);
	all["localization"] = nlohmann::json::array({std::move(labels)});

	os << all.dump();
}
