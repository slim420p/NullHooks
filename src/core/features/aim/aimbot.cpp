#include "dependencies/utilities/csgo.hpp"
#include "core/features/features.hpp"
#include "core/menu/variables.hpp"

bool aim::can_fire(weapon_t* active_weapon) {
	if (!active_weapon->clip1_count()) return false;			// No ammo so don't aimbot

	if (csgo::local_player->next_attack() > interfaces::globals->cur_time)
		return false;

	if (active_weapon->next_primary_attack() > interfaces::globals->cur_time)
		return false;

	return true;
}

// Checks if the current weapon can shoot and all that
bool aim::aimbot_weapon_check() {
	if (csgo::local_player->is_defusing()) return false;

	weapon_t* active_weapon = csgo::local_player->active_weapon();
	if (!active_weapon) return false;

	const auto weapon_data = active_weapon->get_weapon_data();
	if (!weapon_data) return false;

	switch (weapon_data->weapon_type) {									// Only aimbot on weapons that shoot
		case WEAPONTYPE_MACHINEGUN:
		case WEAPONTYPE_RIFLE:
		case WEAPONTYPE_SUBMACHINEGUN:
		case WEAPONTYPE_SHOTGUN:
		case WEAPONTYPE_SNIPER_RIFLE:
		case WEAPONTYPE_PISTOL: {
			if (!can_fire(active_weapon)) return false;					// Check if we can fire

			if (weapon_data->weapon_type == WEAPONTYPE_SNIPER_RIFLE
				&& !csgo::local_player->is_scoped()
				&& !variables::aim::aimbot_noscope) return false;		// We are not scoped and have the noscope option disabled

			break;
		}
		default: return false;
	}

	// (We reached here without return so we are good to use aimbot)
	return true;
}

vec3_t get_best_target(c_usercmd* cmd, weapon_t* active_weapon) {
	vec3_t best_target(0,0,0);								// Position of best bone
	float best_fov{ 180.f * variables::aim::aimbot_fov };	// This variable will store the fov of the closest player to crosshair, we start it as the fov setting

	const auto weapon_data = active_weapon->get_weapon_data();
	if (!weapon_data) return best_target;

	// Store selected hitboxes
	std::vector<int> selected_bones;
	if (variables::aim::hitboxes.vector[0].state) {		// Head
		selected_bones.emplace_back(8);
		selected_bones.emplace_back(7);
	}
	if (variables::aim::hitboxes.vector[1].state) {		// Chest
		selected_bones.emplace_back(6);
		selected_bones.emplace_back(5);
		selected_bones.emplace_back(4);
		selected_bones.emplace_back(3);
	}
	if (variables::aim::hitboxes.vector[2].state) {		// Arms	TODO: Broken
		selected_bones.emplace_back(12);
		selected_bones.emplace_back(40);
	}
	if (variables::aim::hitboxes.vector[3].state) {		// Legs TODO: Broken
		selected_bones.emplace_back(65);
		selected_bones.emplace_back(66);
		selected_bones.emplace_back(72);
		selected_bones.emplace_back(73);
	}

	// Check each player
	for (int n = 1; n <= 64; n++) {
		auto cur_player = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(n));
		if (!cur_player
			|| cur_player == csgo::local_player
			|| !cur_player->is_alive()
			|| cur_player->dormant()
			|| cur_player->has_gun_game_immunity()) continue;
		if (cur_player->team() == csgo::local_player->team() && !variables::aim::target_friends) continue;

		auto local_eye_pos = csgo::local_player->get_eye_pos();		// Get eye pos from origin player_t

		for (const auto bone : selected_bones) {
			auto bone_pos = cur_player->get_bone_position(bone);

			// Ignore everything if we have "ignore walls" setting (2)
			if (variables::aim::autowall.idx != 2) {
				if ((!csgo::local_player->can_see_player_pos(cur_player, bone_pos) && variables::aim::autowall.idx == 0)
					|| !aim::autowall::is_able_to_scan(csgo::local_player, cur_player, bone_pos, weapon_data, (int)variables::aim::min_damage)) continue;
			}

			vec3_t aim_angle = math::calculate_angle(local_eye_pos, bone_pos);
			aim_angle.clamp();

			// First time checks the fov setting, then will overwrite if it finds a player that is closer to crosshair
			const float fov = cmd->viewangles.distance_to(aim_angle);
			if (fov < best_fov) {
				best_fov = fov;
				best_target = bone_pos;
			}
		}
	}

	return best_target;		// vec3_t position of the best bone
}

void aim::run_aimbot(c_usercmd* cmd) {
	if (!(variables::aim::autofire && input::gobal_input.IsHeld(variables::aim::aimbot_key.key))	// Not holding aimbot key
		&& !(!variables::aim::autofire && (cmd->buttons & cmd_buttons::in_attack))) return;			// or not attacking
	if (!variables::aim::aimbot) return;
	if (!interfaces::engine->is_connected() || !interfaces::engine->is_in_game()) return;
	if (!csgo::local_player) return;
	if (!aimbot_weapon_check()) return;

	// We need to get weapon_type here too for aim_punch and for autowall
	weapon_t* active_weapon = csgo::local_player->active_weapon();
	if (!active_weapon) return;
	const auto weapon_data = active_weapon->get_weapon_data();
	if (!weapon_data) return;

	auto local_eye_pos = csgo::local_player->get_eye_pos();		// Get eye pos from origin player_t
	vec3_t target = get_best_target(cmd, active_weapon);
	if (target.is_zero()) return;

	vec3_t aim_angle = math::calculate_angle(local_eye_pos, target);
	aim_angle.clamp();

	vec3_t local_aim_punch{};	// Initialize at 0 because we only want aim punch with rifles
	if (variables::aim::non_rifle_aimpunch) {
		local_aim_punch = csgo::local_player->aim_punch_angle();
	} else {
		switch (weapon_data->weapon_type) {
			case WEAPONTYPE_RIFLE:
			case WEAPONTYPE_SUBMACHINEGUN:
			case WEAPONTYPE_MACHINEGUN:
				local_aim_punch = csgo::local_player->aim_punch_angle();
		}
	}

	static auto recoil_scale = interfaces::console->get_convar("weapon_recoil_scale")->get_float();
	vec3_t enemy_angle = (aim_angle - (local_aim_punch * recoil_scale)) - cmd->viewangles;
	enemy_angle.clamp();

	vec3_t final_angle = enemy_angle;	
	if (!variables::aim::silent) final_angle *= (1.f - variables::aim::aimbot_smoothing);	// Scale acording to smoothing if not silent
	
	cmd->viewangles += final_angle;
	if (variables::aim::autofire && input::gobal_input.IsHeld(variables::aim::aimbot_key.key))
		cmd->buttons |= in_attack;
}

// https://www.unknowncheats.me/forum/counterstrike-global-offensive/129068-draw-aimbot-fov.html
// TODO: Needs fix
void aim::draw_fov() {
	if (false /*REPLACE VAR*/) return;
	if (!interfaces::engine->is_connected() || !interfaces::engine->is_in_game()) return;
	if (!csgo::local_player) return;
	if (!aimbot_weapon_check()) return;
	
	// Screen width and height
	int sw, sh;
	interfaces::engine->get_screen_size(sw, sh);
	const int x_mid = sw / 2, y_mid = sh / 2;

	float rad = tanf((DEG2RAD(variables::aim::aimbot_fov)) / 6) / tanf(97)*x_mid;
	
	render::draw_circle(x_mid, y_mid, rad, 255, color::white());
}