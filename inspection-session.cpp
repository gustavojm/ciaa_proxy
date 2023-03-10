#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <csv.h>
#include "inspection-session.hpp"
#include "boost/program_options.hpp"

extern InspectionSession current_session;

InspectionSession::InspectionSession() :loaded(false) {

}

InspectionSession::InspectionSession(std::filesystem::path inspection_session_file,
		std::filesystem::path hx_directory,
		std::filesystem::path hx,
		std::filesystem::path tubesheet_csv,
		std::filesystem::path tubesheet_svg) :
		inspection_session_file(inspection_session_file), hx_directory(hx_directory), hx(hx), tubesheet_csv(
				tubesheet_csv), tubesheet_svg(tubesheet_svg) {

    try {
        namespace po = boost::program_options;
        po::options_description settings_desc("HX Settings");
        settings_desc.add_options()("leg",
                po::value<std::string>(&leg)->default_value("both"),
                "Leg (hot, cold, both)");
        settings_desc.add_options()("tube_od",
                po::value<float>(&tube_od)->default_value(1.f),
                "Tube Outside Diameter in inches");

        po::variables_map vm;

        std::filesystem::path config = hx_directory / hx / "config.ini";
        if (std::filesystem::exists(config)) {
            std::ifstream config_is = std::ifstream(config);
            po::store(po::parse_config_file(config_is, settings_desc, true), vm);
        }
        po::notify(vm);
    }catch (std::exception &e) {
        std::cout << e.what() << "\n";
    }
}

std::string InspectionSession::load_plans() {
	std::stringstream out;
	std::filesystem::path insp_plans_path = hx_directory / hx / "insp_plans";
	for (const auto &entry : std::filesystem::directory_iterator(insp_plans_path)) {
		if (!(entry.path().filename().extension() == ".csv")) {
			continue;
		}
		out << "Procesando plan: " << entry.path().filename() << "\n";

		// Parse the CSV file to extract the data for the plan
		io::CSVReader<3, io::trim_chars<' ', '\t'>, io::no_quote_escape<';'> >
		ip(	entry.path());
		ip.read_header(io::ignore_extra_column, "ROW", "COL", "TUBE");
		std::string row, col;
		std::string tube_num;
		while (ip.read_row(row, col, tube_num)) {
			std::string tube_num_stripped =tube_num.substr(5);
			insp_plans[entry.path()][tube_num_stripped] = InspectionPlanEntry{row, col, false};
		}
	}
	loaded = true;
	return out.str();
}

InspectionSession InspectionSession::load(std::filesystem::path inspection_session_file) {
	this->inspection_session_file = inspection_session_file;
	std::ifstream i(inspection_session_file);

	nlohmann::json j;
	i >> j;
	*this = j;
	this->loaded = true;

	return *this;
}

void InspectionSession::save_to_disk() {
	std::ofstream file(inspection_session_file);
	file << nlohmann::json(*this);
}

void InspectionSession::set_selected_plan(std::filesystem::path plan) {
	last_selected_plan = plan;
	changed = true;
}

std::filesystem::path InspectionSession::get_selected_plan() const {
	 return last_selected_plan;
}

void InspectionSession::set_tube_inspected(std::string tube_id, bool state) {
	insp_plans[last_selected_plan][tube_id].inspected = state;
	changed = true;
}

void InspectionSession::set_tube_inspected(std::filesystem::path insp_plan_path, std::string tube_id, bool state) {
	insp_plans[insp_plan_path][tube_id].inspected = state;
	changed = true;
}
