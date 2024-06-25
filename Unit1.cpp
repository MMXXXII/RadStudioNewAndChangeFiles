#include <vcl.h>
#pragma hdrstop

#include "Unit1.h"
#include <iostream>
#include <fstream>
#include <unordered_map>  // Для использования хэш-таблицы (ассоциативного контейнера)
#include <filesystem>  // Для работы с файловой системой
#include <chrono>  // Для работы с временем и таймерами
#include <ctime>  // Для работы с временем и датой
#include <vector>
#include <sstream>  // Для работы с потоками строк
#include <iomanip>  // Для манипуляции форматами ввода-вывода
#include <windows.h>

namespace fs = std::filesystem; // Объявляем псевдоним для пространства имен std::filesystem
using namespace std;

#pragma package(smart_init)  // Умная инициализация пакетов VCL
#pragma resource "*.dfm"  // Указываем ресурсный файл для формы
TForm1 *Form1;  // Указатель на главную форму приложения

// Временные наборы для хранения новых и обновленных файлов между нажатиями кнопок
unordered_map<string, time_t> temp_new_files;  // Набор для новых файлов
unordered_map<string, time_t> temp_updated_files;  // Набор для обновленных файлов

// Конструктор формы, инициализирующий форму и очищающий временные наборы
__fastcall TForm1::TForm1(TComponent* Owner)
    : TForm(Owner)
{
    // Очистка временных наборов при запуске программы
    temp_new_files.clear();
    temp_updated_files.clear();
}

// Функция для преобразования time_t в строку
string time_to_str(time_t time) {
    char buf[100] = { 0 };  // Буфер для хранения строки
    struct tm timeinfo;  // Структура для хранения времени
    localtime_s(&timeinfo, &time); // Преобразуем time_t в структуру tm
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo); // Форматируем время в строку
    return string(buf);  // Возвращаем строку
}

// Функция для преобразования строки в time_t
time_t str_to_time(const string& time_str) {
    struct tm timeinfo = { 0 };  // Структура для хранения времени
    istringstream ss(time_str);  // Поток для строки времени
    ss >> get_time(&timeinfo, "%Y-%m-%d %H:%M:%S"); // Преобразуем строку в структуру tm
    return mktime(&timeinfo); // Преобразуем структуру tm в time_t
}

// Функция для загрузки реестра из файла
unordered_map<string, time_t> load_registry(const string& filename) {
    unordered_map<string, time_t> registry;  // Реестр файлов
    ifstream infile(filename); // Открываем файл для чтения
    string line;
    while (getline(infile, line)) { // Читаем файл построчно
        stringstream ss(line);
        string path;
        string time_str;
        if (getline(ss, path, ',') && getline(ss, time_str)) { // Разделяем строку на путь и время
            registry[path] = str_to_time(time_str); // Добавляем в реестр
        }
    }
    return registry;  // Возвращаем реестр
}

// Функция для сохранения реестра в файл
void save_registry(const string& filename, const unordered_map<string, time_t>& registry) {
    ofstream outfile(filename); // Открываем файл для записи
    for (const auto& entry : registry) {
        outfile << entry.first << "," << time_to_str(entry.second) << endl; // Записываем путь и время в файл
    }
}

// Функция для сканирования директории и создания реестра файлов
unordered_map<string, time_t> scan_directory(const string& directory) {
    unordered_map<string, time_t> file_info;  // Реестр файлов
    for (const auto& entry : fs::recursive_directory_iterator(directory)) { // Рекурсивно обходим директорию
        if (fs::is_regular_file(entry.path())) { // Проверяем, является ли путь файлом
            auto ftime = last_write_time(entry.path()); // Получаем время последнего изменения файла
            auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
                ftime - decltype(ftime)::clock::now() + chrono::system_clock::now()
            );
            time_t cftime = chrono::system_clock::to_time_t(sctp); // Преобразуем время в time_t
            file_info[entry.path().string()] = cftime; // Добавляем в реестр
        }
    }
    return file_info;  // Возвращаем реестр файлов
}

// Функция для проверки, является ли путь директорией
bool is_directory(const string& path) {
    return fs::is_directory(path);  // Возвращаем результат проверки
}

// Функция для обработки новых файлов
void handle_new_files(const string& directory_to_scan, const string& new_files_registry_filename) {
    if (!is_directory(directory_to_scan)) { // Проверяем, является ли путь директорией
        ShowMessage("Введенная строка не является папкой. Пожалуйста, введите корректный путь.");  // Выводим сообщение об ошибке
        return;
    }

    unordered_map<string, time_t> previous_registry;

    if (fs::exists(new_files_registry_filename)) {
        previous_registry = load_registry(new_files_registry_filename); // Загружаем предыдущий реестр, если файл существует
    }

    unordered_map<string, time_t> current_registry = scan_directory(directory_to_scan); // Сканируем текущую директорию
    vector<string> new_files;

    for (const auto& entry : current_registry) {
        auto it = previous_registry.find(entry.first);
        if (it == previous_registry.end() && temp_new_files.find(entry.first) == temp_new_files.end()) {
            new_files.push_back(entry.first); // Добавляем новые файлы
            temp_new_files[entry.first] = entry.second; // Добавляем в временный набор
        }
    }

    // Проверяем ранее обнаруженные новые файлы и добавляем в сообщение
    for (const auto& entry : temp_new_files) {
        if (current_registry.find(entry.first) != current_registry.end() && find(new_files.begin(), new_files.end(), entry.first) == new_files.end()) {
            new_files.push_back(entry.first);
        }
    }

    string message = "";
    if (!new_files.empty()) {
        message += "Новые файлы:\n";
        for (size_t i = 0; i < new_files.size(); ++i) {
            message += to_string(i + 1) + ") " + new_files[i] + " (обнаружен в " + time_to_str(temp_new_files[new_files[i]]) + ")\n";
        }
    } else {
        message = "Новых файлов не обнаружено.";
    }

    ShowMessage(message.c_str());

    // Сохранение текущего реестра новых файлов в файл
    save_registry(new_files_registry_filename, current_registry);
}

// Функция для обработки обновленных файлов
void handle_updated_files(const string& directory_to_scan, const string& updated_files_registry_filename) {
    if (!is_directory(directory_to_scan)) { // Проверяем, является ли путь директорией
        ShowMessage("Введенная строка не является папкой. Пожалуйста, введите корректный путь.");  // Выводим сообщение об ошибке
        return;
    }

    unordered_map<string, time_t> previous_registry;

    if (fs::exists(updated_files_registry_filename)) {
        previous_registry = load_registry(updated_files_registry_filename); // Загружаем предыдущий реестр, если файл существует
    }

    unordered_map<string, time_t> current_registry = scan_directory(directory_to_scan); // Сканируем текущую директорию
    vector<string> updated_files;

    for (const auto& entry : current_registry) {
        auto it = previous_registry.find(entry.first);
        if (it != previous_registry.end() && it->second != entry.second && temp_updated_files.find(entry.first) == temp_updated_files.end()) {
            updated_files.push_back(entry.first); // Добавляем обновленные файлы
            temp_updated_files[entry.first] = entry.second; // Добавляем в временный набор
        }
    }

    // Проверяем ранее обнаруженные обновленные файлы и добавляем в сообщение
    for (const auto& entry : temp_updated_files) {
        if (current_registry.find(entry.first) != current_registry.end() && find(updated_files.begin(), updated_files.end(), entry.first) == updated_files.end()) {
            updated_files.push_back(entry.first);
        }
    }

    string message = "";
    if (!updated_files.empty()) {
        message += "Обновленные файлы:\n";
        for (size_t i = 0; i < updated_files.size(); ++i) {
            message += to_string(i + 1) + ") " + updated_files[i] + " (обновлен в " + time_to_str(temp_updated_files[updated_files[i]]) + ")\n";
        }
    } else {
        message = "Обновленных файлов не обнаружено.";
    }

    ShowMessage(message.c_str());
    save_registry(updated_files_registry_filename, current_registry); // Сохраняем текущий реестр обновленных файлов в файл
}

// Функция для обработки оставшихся файлов
void handle_remaining_files(const string& directory_to_scan) {

    if (!is_directory(directory_to_scan)) { // Проверяем, является ли путь директорией
        ShowMessage("Введенная строка не является папкой. Пожалуйста, введите корректный путь.");  // Выводим сообщение об ошибке
        return;
	}

    const string new_files_registry_filename = "new_files_registry.txt";  // Имя файла для реестра новых файлов
    const string updated_files_registry_filename = "updated_files_registry.txt";  // Имя файла для реестра обновленных файлов

    unordered_map<string, time_t> previous_new_registry;
    unordered_map<string, time_t> previous_updated_registry;

    if (fs::exists(new_files_registry_filename)) {
        previous_new_registry = load_registry(new_files_registry_filename); // Загружаем реестр новых файлов, если файл существует
    }
    if (fs::exists(updated_files_registry_filename)) {
        previous_updated_registry = load_registry(updated_files_registry_filename); // Загружаем реестр обновленных файлов, если файл существует
    }

    unordered_map<string, time_t> current_registry = scan_directory(directory_to_scan); // Сканируем текущую директорию
    vector<string> new_files;
    vector<string> updated_files;

    for (const auto& entry : current_registry) {
        auto it_new = previous_new_registry.find(entry.first);
        auto it_updated = previous_updated_registry.find(entry.first);
        if (it_new == previous_new_registry.end() && it_updated == previous_updated_registry.end()) {
            new_files.push_back(entry.first); // Добавляем новые файлы
        } else if (it_updated != previous_updated_registry.end() && it_updated->second != entry.second) {
            updated_files.push_back(entry.first); // Добавляем обновленные файлы
        }
    }

    string message = "Остальные файлы:\n";
    size_t index = 1;
    for (const auto& entry : current_registry) {
        if (find(new_files.begin(), new_files.end(), entry.first) == new_files.end() &&
            find(updated_files.begin(), updated_files.end(), entry.first) == updated_files.end() &&
            temp_new_files.find(entry.first) == temp_new_files.end() &&
            temp_updated_files.find(entry.first) == temp_updated_files.end()) {
            message += to_string(index++) + ") " + entry.first + " (последнее изменение: " + time_to_str(entry.second) + ")\n"; // Добавляем оставшиеся файлы в сообщение
        }
    }

    ShowMessage(message.c_str());

    // Сохранение реестра новых и обновленных файлов
    unordered_map<string, time_t> combined_registry = previous_new_registry;
    combined_registry.insert(previous_updated_registry.begin(), previous_updated_registry.end());
    save_registry(new_files_registry_filename, combined_registry); // Сохраняем объединенный реестр новых файлов
    save_registry(updated_files_registry_filename, combined_registry); // Сохраняем объединенный реестр обновленных файлов
}

// Обработчики нажатий кнопок, обновленные для учета временного хранения новых и обновленных файлов
void __fastcall TForm1::Button1Click(TObject *Sender) {
    string directory_to_scan = AnsiString(Edit1->Text).c_str();  // Получаем путь из текстового поля
    handle_new_files(directory_to_scan, "new_files_registry.txt");  // Обрабатываем новые файлы
}

void __fastcall TForm1::Button2Click(TObject *Sender) {
    string directory_to_scan = AnsiString(Edit1->Text).c_str();  // Получаем путь из текстового поля
    handle_updated_files(directory_to_scan, "updated_files_registry.txt");  // Обрабатываем обновленные файлы
}

void __fastcall TForm1::Button3Click(TObject *Sender) {
    string directory_to_scan = AnsiString(Edit1->Text).c_str();  // Получаем путь из текстового поля
    handle_remaining_files(directory_to_scan);  // Обрабатываем оставшиеся файлы
}

// Обработчик нажатия кнопки для очистки временных наборов
void __fastcall TForm1::Button4Click(TObject *Sender)
{
    // Очистка временных наборов новых и обновленных файлов
    temp_new_files.clear();
    temp_updated_files.clear();

    // Удаление файлов реестров новых и обновленных файлов
    const string new_files_registry_filename = "new_files_registry.txt";
    const string updated_files_registry_filename = "updated_files_registry.txt";

    ShowMessage("Временные наборы и файлы реестров очищены.");
}

