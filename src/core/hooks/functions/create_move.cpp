#include "dependencies/utilities/csgo.hpp"
#include "core/features/features.hpp"
#include "core/menu/variables.hpp"
#include "core/hooks/hooks.hpp"

bool __stdcall hooks::create_move::hook(float input_sample_frametime, c_usercmd* cmd) {
	original(input_sample_frametime, cmd);

	if (!cmd || !cmd->command_number) return false;

	csgo::local_player = static_cast<player_t*>(interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player()));
	
	misc::speedgraph::update(cmd);
	misc::movement::bunny_hop(cmd);
	aim::run_aimbot(cmd);

	auto old_viewangles = cmd->viewangles;
	auto old_forwardmove = cmd->forwardmove;
	auto old_sidemove = cmd->sidemove;
	auto old_flags = csgo::local_player->flags();

	prediction::start(cmd); {
		misc::movement::edgebug(cmd, old_flags);
	} prediction::end();

	math::correct_movement(old_viewangles, cmd, old_forwardmove, old_sidemove);

	cmd->forwardmove = std::clamp(cmd->forwardmove, -450.0f, 450.0f);
	cmd->sidemove = std::clamp(cmd->sidemove, -450.0f, 450.0f);
	cmd->upmove = std::clamp(cmd->upmove, -320.0f, 320.0f);

	cmd->viewangles.normalize();
	cmd->viewangles.clamp();

	return false;
}