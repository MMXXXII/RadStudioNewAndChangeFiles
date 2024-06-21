#include <vcl.h>
#pragma hdrstop

#include "Unit1.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <vector>
#include <sstream>
#include <iomanip>
#include <windows.h>

namespace fs = std::filesystem;
using namespace std;

#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;

__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner)
{
}

// Function to convert time_t to string
string time_to_str(time_t time) {
	char buf[100] = { 0 };
	struct tm timeinfo;
	localtime_s(&timeinfo, &time);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return string(buf);
}

// Function to convert string to time_t
time_t str_to_time(const string& time_str) {
    struct tm timeinfo = { 0 };
    istringstream ss(time_str);
    ss >> get_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
	return mktime(&timeinfo);
}

// Function to load registry from file
unordered_map<string, time_t> load_registry(const string& filename) {
	unordered_map<string, time_t> registry;
	ifstream infile(filename);
	string line;
	while (getline(infile, line)) {
		stringstream ss(line);
		string path;
		string time_str;
		if (getline(ss, path, ',') && getline(ss, time_str)) {
			registry[path] = str_to_time(time_str);
		}
	}
	return registry;
}

// Function to save registry to file
void save_registry(const string& filename, const unordered_map<string, time_t>& registry) {
	ofstream outfile(filename);
	for (const auto& entry : registry) {
		outfile << entry.first << "," << time_to_str(entry.second) << endl;
	}
}

// Function to save new files registry to file
void save_new_files_registry(const string& filename, const unordered_map<string, time_t>& registry) {
	ofstream outfile(filename);
	for (const auto& entry : registry) {
		outfile << entry.first << "," << time_to_str(entry.second) << endl;
	}
}

// Function to save updated files registry to file
void save_updated_files_registry(const string& filename, const unordered_map<string, time_t>& registry) {
	ofstream outfile(filename);
	for (const auto& entry : registry) {
		outfile << entry.first << "," << time_to_str(entry.second) << endl;
	}
}

// Function to scan directory and create file registry
unordered_map<string, time_t> scan_directory(const string& directory) {
	unordered_map<string, time_t> file_info;
	for (const auto& entry : fs::recursive_directory_iterator(directory)) {
		if (fs::is_regular_file(entry.path())) {
			auto ftime = last_write_time(entry.path());
			auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
				ftime - decltype(ftime)::clock::now() + chrono::system_clock::now()
			);
			time_t cftime = chrono::system_clock::to_time_t(sctp);
			file_info[entry.path().string()] = cftime;
		}
	}
	return file_info;
}

// Function to check if a path is a directory
bool is_directory(const string& path) {
	return fs::is_directory(path);
}

// Function to handle new files
void handle_new_files(const string& directory_to_scan, const string& new_files_registry_filename) {
    if (!is_directory(directory_to_scan)) {
        ShowMessage("Введенная строка не является папкой. Пожалуйста, введите корректный путь.");
        return;
    }

    unordered_map<string, time_t> previous_registry;

    if (fs::exists(new_files_registry_filename)) {
        previous_registry = load_registry(new_files_registry_filename);
    }

    unordered_map<string, time_t> current_registry = scan_directory(directory_to_scan);
    vector<string> new_files;

    for (const auto& entry : current_registry) {
        auto it = previous_registry.find(entry.first);
        if (it == previous_registry.end()) {
            new_files.push_back(entry.first);
        }
    }

    string message = "";
    if (!new_files.empty()) {
        message += "Новые файлы:\n";
        for (const auto& file : new_files) {
            message += file + " (обнаружен в " + time_to_str(current_registry[file]) + ")\n";
        }
    } else {
        message = "Новых файлов не обнаружено.";
    }

    ShowMessage(message.c_str());

    // Сохранение текущего реестра новых файлов в файл
    save_new_files_registry(new_files_registry_filename, current_registry);
}


// Function to handle updated files
void handle_updated_files(const string& directory_to_scan, const string& updated_files_registry_filename) {
	if (!is_directory(directory_to_scan)) {
		ShowMessage("Введенная строка не является папкой. Пожалуйста, введите корректный путь.");
		return;
	}

	unordered_map<string, time_t> previous_registry;

	if (fs::exists(updated_files_registry_filename)) {
		previous_registry = load_registry(updated_files_registry_filename);
	}

	unordered_map<string, time_t> current_registry = scan_directory(directory_to_scan);
	vector<string> updated_files;

	for (const auto& entry : current_registry) {
		auto it = previous_registry.find(entry.first);
		if (it != previous_registry.end() && it->second != entry.second) {
			updated_files.push_back(entry.first);
		}
	}

	string message = "";
	if (!updated_files.empty()) {
		message += "Обновленные файлы:\n";
		for (const auto& file : updated_files) {
			message += file + " (обновлен в " + time_to_str(current_registry[file]) + ")\n";
		}
	} else {
		message = "Обновленных файлов не обнаружено.";
	}

	ShowMessage(message.c_str());
	save_updated_files_registry(updated_files_registry_filename, current_registry);
}

// Handler for "New Files" button click
void __fastcall TForm1::Button1Click(TObject *Sender) {
	string directory_to_scan = AnsiString(Edit1->Text).c_str();
	const string new_files_registry_filename = "new_files_registry.txt";
	handle_new_files(directory_to_scan, new_files_registry_filename);
}

// Handler for "Updated Files" button click
void __fastcall TForm1::Button2Click(TObject *Sender) {
	string directory_to_scan = AnsiString(Edit1->Text).c_str();
	const string updated_files_registry_filename = "updated_files_registry.txt";
	handle_updated_files(directory_to_scan, updated_files_registry_filename);
}

// Handler for "Other Files" button click
void __fastcall TForm1::Button3Click(TObject *Sender) {
	string directory_to_scan = AnsiString(Edit1->Text).c_str();
	const string new_files_registry_filename = "new_files_registry.txt";
	const string updated_files_registry_filename = "updated_files_registry.txt";
	if (!is_directory(directory_to_scan)) {
		ShowMessage("Введенная строка не является папкой. Пожалуйста, введите корректный путь.");
		return;
	}

	unordered_map<string, time_t> previous_new_registry;
	unordered_map<string, time_t> previous_updated_registry;

	if (fs::exists(new_files_registry_filename)) {
		previous_new_registry = load_registry(new_files_registry_filename);
	}
	if (fs::exists(updated_files_registry_filename)) {
		previous_updated_registry = load_registry(updated_files_registry_filename);
	}

	unordered_map<string, time_t> current_registry = scan_directory(directory_to_scan);
	vector<string> new_files;
	vector<string> updated_files;

	for (const auto& entry : current_registry) {
		auto it_new = previous_new_registry.find(entry.first);
		auto it_updated = previous_updated_registry.find(entry.first);
		if (it_new == previous_new_registry.end()) {
			new_files.push_back(entry.first);
		} else if (it_updated != previous_updated_registry.end() && it_updated->second != entry.second) {
			updated_files.push_back(entry.first);
		}
	}

	string message = "Остальные файлы:\n";
	for (const auto& entry : current_registry) {
		if (find(new_files.begin(), new_files.end(), entry.first) == new_files.end() &&
			find(updated_files.begin(), updated_files.end(), entry.first) == updated_files.end()) {
			message += entry.first + " (последнее изменение: " + time_to_str(entry.second) + ")\n";
		}
	}

	ShowMessage(message.c_str());

	save_new_files_registry(new_files_registry_filename, current_registry);
	save_updated_files_registry(updated_files_registry_filename, current_registry);
}


