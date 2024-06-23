#include <vcl.h>  // ������������ ���� VCL (Visual Component Library) ��� ������ � ���������� ����������
#pragma hdrstop

#include "Unit1.h"  // ������������ ����, ��������� � ������ TForm1
#include <iostream>  // ��� ������������ �����-������
#include <fstream>  // ��� ������ � ��������� ��������
#include <unordered_map>  // ��� ������������� ���-������� (�������������� ����������)
#include <filesystem>  // ��� ������ � �������� ��������
#include <chrono>  // ��� ������ � �������� � ���������
#include <ctime>  // ��� ������ � �������� � �����
#include <vector>  // ��� ������������� ������������ ��������
#include <sstream>  // ��� ������ � �������� �����
#include <iomanip>  // ��� ����������� ��������� �����-������
#include <windows.h>  // ��� ������ � Windows API

namespace fs = std::filesystem; // ��������� ��������� ��� ������������ ���� std::filesystem
using namespace std;

#pragma package(smart_init)  // ����� ������������� ������� VCL
#pragma resource "*.dfm"  // ��������� ��������� ���� ��� �����
TForm1 *Form1;  // ��������� �� ������� ����� ����������

// ��������� ������ ��� �������� ����� � ����������� ������ ����� ��������� ������
unordered_map<string, time_t> temp_new_files;  // ����� ��� ����� ������
unordered_map<string, time_t> temp_updated_files;  // ����� ��� ����������� ������

// ����������� �����, ���������������� ����� � ��������� ��������� ������
__fastcall TForm1::TForm1(TComponent* Owner)
    : TForm(Owner)
{
    // ������� ��������� ������� ��� ������� ���������
    temp_new_files.clear();
    temp_updated_files.clear();
}

// ������� ��� �������������� time_t � ������
string time_to_str(time_t time) {
    char buf[100] = { 0 };  // ����� ��� �������� ������
    struct tm timeinfo;  // ��������� ��� �������� �������
    localtime_s(&timeinfo, &time); // ����������� time_t � ��������� tm
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo); // ����������� ����� � ������
    return string(buf);  // ���������� ������
}

// ������� ��� �������������� ������ � time_t
time_t str_to_time(const string& time_str) {
    struct tm timeinfo = { 0 };  // ��������� ��� �������� �������
    istringstream ss(time_str);  // ����� ��� ������ �������
    ss >> get_time(&timeinfo, "%Y-%m-%d %H:%M:%S"); // ����������� ������ � ��������� tm
    return mktime(&timeinfo); // ����������� ��������� tm � time_t
}

// ������� ��� �������� ������� �� �����
unordered_map<string, time_t> load_registry(const string& filename) {
    unordered_map<string, time_t> registry;  // ������ ������
    ifstream infile(filename); // ��������� ���� ��� ������
    string line;
    while (getline(infile, line)) { // ������ ���� ���������
        stringstream ss(line);
        string path;
        string time_str;
        if (getline(ss, path, ',') && getline(ss, time_str)) { // ��������� ������ �� ���� � �����
            registry[path] = str_to_time(time_str); // ��������� � ������
        }
    }
    return registry;  // ���������� ������
}

// ������� ��� ���������� ������� � ����
void save_registry(const string& filename, const unordered_map<string, time_t>& registry) {
    ofstream outfile(filename); // ��������� ���� ��� ������
    for (const auto& entry : registry) {
        outfile << entry.first << "," << time_to_str(entry.second) << endl; // ���������� ���� � ����� � ����
    }
}

// ������� ��� ������������ ���������� � �������� ������� ������
unordered_map<string, time_t> scan_directory(const string& directory) {
    unordered_map<string, time_t> file_info;  // ������ ������
    for (const auto& entry : fs::recursive_directory_iterator(directory)) { // ���������� ������� ����������
        if (fs::is_regular_file(entry.path())) { // ���������, �������� �� ���� ������
            auto ftime = last_write_time(entry.path()); // �������� ����� ���������� ��������� �����
            auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
                ftime - decltype(ftime)::clock::now() + chrono::system_clock::now()
            );
            time_t cftime = chrono::system_clock::to_time_t(sctp); // ����������� ����� � time_t
            file_info[entry.path().string()] = cftime; // ��������� � ������
        }
    }
    return file_info;  // ���������� ������ ������
}

// ������� ��� ��������, �������� �� ���� �����������
bool is_directory(const string& path) {
    return fs::is_directory(path);  // ���������� ��������� ��������
}

// ������� ��� ��������� ����� ������
void handle_new_files(const string& directory_to_scan, const string& new_files_registry_filename) {
    if (!is_directory(directory_to_scan)) { // ���������, �������� �� ���� �����������
        ShowMessage("��������� ������ �� �������� ������. ����������, ������� ���������� ����.");  // ������� ��������� �� ������
        return;
    }

    unordered_map<string, time_t> previous_registry;

    if (fs::exists(new_files_registry_filename)) {
        previous_registry = load_registry(new_files_registry_filename); // ��������� ���������� ������, ���� ���� ����������
    }

    unordered_map<string, time_t> current_registry = scan_directory(directory_to_scan); // ��������� ������� ����������
    vector<string> new_files;

    for (const auto& entry : current_registry) {
        auto it = previous_registry.find(entry.first);
        if (it == previous_registry.end() && temp_new_files.find(entry.first) == temp_new_files.end()) {
            new_files.push_back(entry.first); // ��������� ����� �����
            temp_new_files[entry.first] = entry.second; // ��������� � ��������� �����
        }
    }

    // ��������� ����� ������������ ����� ����� � ��������� � ���������
    for (const auto& entry : temp_new_files) {
        if (current_registry.find(entry.first) != current_registry.end() && find(new_files.begin(), new_files.end(), entry.first) == new_files.end()) {
            new_files.push_back(entry.first);
        }
    }

    string message = "";
    if (!new_files.empty()) {
        message += "����� �����:\n";
        for (size_t i = 0; i < new_files.size(); ++i) {
            message += to_string(i + 1) + ") " + new_files[i] + " (��������� � " + time_to_str(temp_new_files[new_files[i]]) + ")\n";
        }
    } else {
        message = "����� ������ �� ����������.";
    }

    ShowMessage(message.c_str());

    // ���������� �������� ������� ����� ������ � ����
    save_registry(new_files_registry_filename, current_registry);
}

// ������� ��� ��������� ����������� ������
void handle_updated_files(const string& directory_to_scan, const string& updated_files_registry_filename) {
    if (!is_directory(directory_to_scan)) { // ���������, �������� �� ���� �����������
        ShowMessage("��������� ������ �� �������� ������. ����������, ������� ���������� ����.");  // ������� ��������� �� ������
        return;
    }

    unordered_map<string, time_t> previous_registry;

    if (fs::exists(updated_files_registry_filename)) {
        previous_registry = load_registry(updated_files_registry_filename); // ��������� ���������� ������, ���� ���� ����������
    }

    unordered_map<string, time_t> current_registry = scan_directory(directory_to_scan); // ��������� ������� ����������
    vector<string> updated_files;

    for (const auto& entry : current_registry) {
        auto it = previous_registry.find(entry.first);
        if (it != previous_registry.end() && it->second != entry.second && temp_updated_files.find(entry.first) == temp_updated_files.end()) {
            updated_files.push_back(entry.first); // ��������� ����������� �����
            temp_updated_files[entry.first] = entry.second; // ��������� � ��������� �����
        }
    }

    // ��������� ����� ������������ ����������� ����� � ��������� � ���������
    for (const auto& entry : temp_updated_files) {
        if (current_registry.find(entry.first) != current_registry.end() && find(updated_files.begin(), updated_files.end(), entry.first) == updated_files.end()) {
            updated_files.push_back(entry.first);
        }
    }

    string message = "";
    if (!updated_files.empty()) {
        message += "����������� �����:\n";
        for (size_t i = 0; i < updated_files.size(); ++i) {
            message += to_string(i + 1) + ") " + updated_files[i] + " (�������� � " + time_to_str(temp_updated_files[updated_files[i]]) + ")\n";
        }
    } else {
        message = "����������� ������ �� ����������.";
    }

    ShowMessage(message.c_str());
    save_registry(updated_files_registry_filename, current_registry); // ��������� ������� ������ ����������� ������ � ����
}

// ������� ��� ��������� ���������� ������
void handle_remaining_files(const string& directory_to_scan) {
    const string new_files_registry_filename = "new_files_registry.txt";  // ��� ����� ��� ������� ����� ������
    const string updated_files_registry_filename = "updated_files_registry.txt";  // ��� ����� ��� ������� ����������� ������

    unordered_map<string, time_t> previous_new_registry;
    unordered_map<string, time_t> previous_updated_registry;

    if (fs::exists(new_files_registry_filename)) {
        previous_new_registry = load_registry(new_files_registry_filename); // ��������� ������ ����� ������, ���� ���� ����������
    }
    if (fs::exists(updated_files_registry_filename)) {
        previous_updated_registry = load_registry(updated_files_registry_filename); // ��������� ������ ����������� ������, ���� ���� ����������
    }

    unordered_map<string, time_t> current_registry = scan_directory(directory_to_scan); // ��������� ������� ����������
    vector<string> new_files;
    vector<string> updated_files;

    for (const auto& entry : current_registry) {
        auto it_new = previous_new_registry.find(entry.first);
        auto it_updated = previous_updated_registry.find(entry.first);
        if (it_new == previous_new_registry.end() && it_updated == previous_updated_registry.end()) {
            new_files.push_back(entry.first); // ��������� ����� �����
        } else if (it_updated != previous_updated_registry.end() && it_updated->second != entry.second) {
            updated_files.push_back(entry.first); // ��������� ����������� �����
        }
    }

    string message = "��������� �����:\n";
    size_t index = 1;
    for (const auto& entry : current_registry) {
        if (find(new_files.begin(), new_files.end(), entry.first) == new_files.end() &&
            find(updated_files.begin(), updated_files.end(), entry.first) == updated_files.end() &&
            temp_new_files.find(entry.first) == temp_new_files.end() &&
            temp_updated_files.find(entry.first) == temp_updated_files.end()) {
            message += to_string(index++) + ") " + entry.first + " (��������� ���������: " + time_to_str(entry.second) + ")\n"; // ��������� ���������� ����� � ���������
        }
    }

    ShowMessage(message.c_str());

    // ���������� ������� ����� � ����������� ������
    unordered_map<string, time_t> combined_registry = previous_new_registry;
    combined_registry.insert(previous_updated_registry.begin(), previous_updated_registry.end());
    save_registry(new_files_registry_filename, combined_registry); // ��������� ������������ ������ ����� ������
    save_registry(updated_files_registry_filename, combined_registry); // ��������� ������������ ������ ����������� ������
}

// ����������� ������� ������, ����������� ��� ����� ���������� �������� ����� � ����������� ������
void __fastcall TForm1::Button1Click(TObject *Sender) {
    string directory_to_scan = AnsiString(Edit1->Text).c_str();  // �������� ���� �� ���������� ����
    handle_new_files(directory_to_scan, "new_files_registry.txt");  // ������������ ����� �����
}

void __fastcall TForm1::Button2Click(TObject *Sender) {
    string directory_to_scan = AnsiString(Edit1->Text).c_str();  // �������� ���� �� ���������� ����
    handle_updated_files(directory_to_scan, "updated_files_registry.txt");  // ������������ ����������� �����
}

void __fastcall TForm1::Button3Click(TObject *Sender) {
    string directory_to_scan = AnsiString(Edit1->Text).c_str();  // �������� ���� �� ���������� ����
    handle_remaining_files(directory_to_scan);  // ������������ ���������� �����
}

// ���������� ������� ������ ��� ������� ��������� �������
void __fastcall TForm1::Button4Click(TObject *Sender)
{
    // ������� ��������� ������� ����� � ����������� ������
    temp_new_files.clear();
    temp_updated_files.clear();

    // �������� ������ �������� ����� � ����������� ������
    const string new_files_registry_filename = "new_files_registry.txt";
    const string updated_files_registry_filename = "updated_files_registry.txt";

    ShowMessage("��������� ������ � ����� �������� �������.");
}

