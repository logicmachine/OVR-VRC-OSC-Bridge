#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <openvr.h>

#include "osc_sender.h"
#include "configuration.h"
#include "action_manifest.h"
#include "input_handler.h"


std::filesystem::path get_executable_directory() {
	char buffer[2048];
	GetModuleFileNameA(NULL, buffer, sizeof(buffer));
	std::filesystem::path executable_path(buffer);
	return executable_path.parent_path();
}


int main(int argc, char* argv[]) {
	// Get path for configuration file
	std::filesystem::path config_dir;
	if (argc < 2) {
		config_dir = get_executable_directory() / "default.json";
	}
	else {
		config_dir = argv[1];
	}
	const std::string config_name = config_dir.filename().string();
	config_dir.remove_filename();

	// Load configuration file
	std::ifstream config_ifs((config_dir / config_name).c_str());
	if (!config_ifs.is_open()) {
		std::cerr << "Failed to open configuration file." << std::endl;
		return -1;
	}
	const auto config = nlohmann::json::parse(config_ifs);
	config_ifs.close();

	// Load action sets
	const auto action_sets_dir = config_dir / config["action_sets"];
	std::vector<ActionSet> action_sets;
	for (const auto& entry : std::filesystem::directory_iterator(action_sets_dir)) {
		std::ifstream ifs(entry.path().c_str());
		action_sets.push_back(ActionSet::from_json(nlohmann::json::parse(ifs)));
	}

	// Load other configurations
	const double frame_interval = 1.0 / config["poll_rate"];
	const std::string destination_host = config["destination_host"];
	const int destination_port = config["destination_port"];

	// Generate an action manifest
	const auto manifest_path = config_dir / (config_name + ".manifest.json");
	{
		std::ofstream manifest_ofs(manifest_path.c_str());
		generate_action_manifest(manifest_ofs, action_sets.begin(), action_sets.end());
	}

	// Initialize OpenVR
	vr::HmdError hmd_error;
	vr::VR_Init(&hmd_error, vr::EVRApplicationType::VRApplication_Overlay);
	if (hmd_error != vr::HmdError::VRInitError_None) {
		std::cerr << "Failed to initialize OpenVR" << std::endl;
		return -1;
	}

	// Initialize input system
	const auto manifest_path_str = std::filesystem::absolute(manifest_path).generic_string();
	vr::VRInput()->SetActionManifestPath(manifest_path_str.c_str());

	InputHandler input_handler(action_sets);

	// Initialize networking
	boost::asio::io_service io_service;
	OSCSender sender(io_service, destination_host, destination_port);

	// Main loop
	while (true) {
		const auto messages = input_handler.update();
		if (!messages.empty()) {
			OSCBundle bundle;
			for (const auto& m : messages) { bundle.add(m); }
			sender.send(bundle);
		}
		std::this_thread::sleep_for(
			std::chrono::milliseconds(static_cast<int>(1000.0 * frame_interval)));
	}

	vr::VR_Shutdown();
	return 0;
}