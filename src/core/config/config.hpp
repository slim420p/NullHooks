#pragma once
#include "dependencies/utilities/csgo.hpp"

namespace config {
	void init();

	inline std::string nullhooks_config_folder;
	void get_nullhooks_folder();

	void load_config(std::string filename);
	void test_config();
	
	inline rapidjson::Document::AllocatorType& allocator;
	void save_config(std::string filename);

	namespace load {
		void parse_bool(rapidjson::Document& doc, bool& target, std::string json_name);
		void parse_float(rapidjson::Document& doc, float& target, std::string json_name);
		void parse_combobox(rapidjson::Document& doc, combobox_toggle_t& target, std::string json_name);
		void parse_multicombo(rapidjson::Document& doc, multicombobox_toggle_t& target, std::string json_name);
		void parse_color(rapidjson::Document& doc, colorpicker_col_t& target, std::string json_name);
		void parse_hotkey(rapidjson::Document& doc, hotkey_t& target, std::string json_name);
	}

	namespace save {
		void parse_bool(rapidjson::Document& doc, bool& target, std::string json_name);
		void parse_float(rapidjson::Document& doc, float& target, std::string json_name);
		void parse_combobox(rapidjson::Document& doc, combobox_toggle_t& target, std::string json_name);
		void parse_multicombo(rapidjson::Document& doc, multicombobox_toggle_t& target, std::string json_name);
		void parse_color(rapidjson::Document& doc, colorpicker_col_t& target, std::string json_name);
		void parse_hotkey(rapidjson::Document& doc, hotkey_t& target, std::string json_name);
	}
}