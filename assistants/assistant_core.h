#pragma once
#include <iostream>
#include <filesystem>
#include <wx/wx.h>
#include <wx/stdpaths.h>
#include <fstream>
#include <string>
#include <string_view>
#include <sstream>
#include <charconv> 
#include <optional>
#include <ranges>
#include <vector>
#include "Data/JSValueClientData.h"
#include "Data/wxEventControlJsData.h"

namespace fs = std::filesystem;

namespace assistant
{
    namespace ui
    {
        /** Уведомление в отдельном окне */
        void alert(std::string message, std::string title = "") {
            wxMessageBox(wxString::FromUTF8(message), title.empty() ? wxT("Уведомление") : wxString::FromUTF8(title), wxOK | wxICON_INFORMATION);
        }

        /** Ошибка в отдельном окне */
        void error(std::string message, std::string title = "") {
            wxMessageBox(wxString::FromUTF8(message), title.empty() ? wxT("Уведомление") : wxString::FromUTF8(title), wxOK | wxICON_ERROR);
        }

        /** Из события получить информацию о контроле */
        std::optional<wxEventControlJsData> getEventControlJsData(wxEvent &event) {
            wxWindow *control = dynamic_cast<wxWindow *>(event.GetEventObject());
            if (control) {
                wxClientData* clientDataVoid = control->GetClientObject();
                JSValueClientData* clientData = dynamic_cast<JSValueClientData*>(clientDataVoid);
                if (clientData) {
                    wxEventControlJsData data = {};
                    data.control = control;
                    data.clientDataVoid = clientDataVoid;
                    data.clientData = clientData;
                    return data;
                }
            }
            return std::nullopt;
        }
    }

    namespace core
    {
        const std::string _WHITESPACE = " \n\r\t\f\v";

        /** Получить текст из файла ресурса */
        std::string getContentResource(const std::string &resource_name) {
            HRSRC hRes = FindResourceA(NULL, resource_name.c_str(), "TEXT");
            if (!hRes) {
                return "";
            }

            HGLOBAL hData = LoadResource(NULL, hRes);
            if (!hData) {
                return "";
            }

            const char *pData = reinterpret_cast<const char *>(LockResource(hData));
            DWORD dataSize = SizeofResource(NULL, hRes);

            if (!pData || dataSize == 0) {
                return "";
            }

            return std::string(pData, dataSize);
        }

        /** Преобразовать строку в число */
        std::optional<int> to_int(std::string text) {
            int num = 0;
            auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), num);

            if (ec == std::errc()) {
                return num;
            }

            return std::nullopt;
        }

        std::string replaceAll(std::string str, const std::string from, const std::string to) {
            if (from.empty())
                return str;
            size_t start_pos = 0;

            while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
                str.replace(start_pos, from.length(), to);
                start_pos += to.length();
            }

            return str;
        }

        std::vector<std::string> split(std::string text, std::string delim) {
            std::vector<std::string> tokens;
            size_t start = 0;
            size_t end = text.find(delim);

            while (end != std::string::npos) {
                tokens.push_back(text.substr(start, end - start));
                start = end + delim.length();
                end = text.find(delim, start);
                continue;
            }
            auto lastText = text.substr(start);
            if (!lastText.empty()) {
                tokens.push_back(lastText);
            }
            return tokens;
        }

        /** Удаление пробелов СЛЕВА */
        std::string ltrim(const std::string &s) {
            size_t start = s.find_first_not_of(_WHITESPACE);
            return (start == std::string::npos) ? "" : s.substr(start);
        }

        /** Удаление пробелов СПРАВА */
        std::string rtrim(const std::string &s) {
            size_t end = s.find_last_not_of(_WHITESPACE);
            return (end == std::string::npos) ? "" : s.substr(0, end + 1);
        }

        /** Удаление пробелов С ОБЕИХ СТОРОН */
        std::string trim(const std::string &s) {
            return rtrim(ltrim(s));
        }
    }

    namespace file
    {
        bool exists(std::string fileName) {
            return fs::exists(fileName);
        }

        /** Создать новый файл и записать туда контент */
        bool write(const std::string &filePath, const std::string &content) {
            fs::path p(filePath);
            if (p.has_parent_path()) {
                fs::create_directories(p.parent_path());
            }

            std::ofstream out(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
            if (!out.is_open()) {
                return false;
            }
            out << content;
            out.close();
            return true;
        }

        /** Прочитать контент из файла */
        std::string read(const std::string &filePath) {
            std::ifstream in(filePath, std::ios::in | std::ios::binary);
            if (!in.is_open()) {
                return "";
            }

            std::stringstream buffer;
            buffer << in.rdbuf();
            return buffer.str();
        }
    }

    namespace directory
    {
        bool exists(std::string directoryName) {
            return fs::is_directory(directoryName);
        }

        bool create(std::string directoryName)
        {
            return fs::create_directories(directoryName);
        }

        /** получить папку запуска нашего приложения */
        std::string getDirectoryExecutable()
        {
            auto executablePath = wxStandardPaths::Get().GetExecutablePath();
            auto fileName = std::string(executablePath.ToUTF8());
            fs::path p(fileName);
            return p.parent_path().string();
        }
    }

    namespace path
    {
        /**сложить два пути */
        std::string combine(std::string left, std::string right) {
            fs::path full_path = fs::path(left) / right;
            std::string result = full_path.string();
            return result;
        }

        /**сложить два пути */
        std::string combine(std::string left, std::string right, std::string right1) {
            fs::path full_path = fs::path(left) / right / right1;
            std::string result = full_path.string();
            return result;
        }

        /**сложить два пути */
        std::string combine(std::string left, std::string right, std::string right1, std::string right2) {
            fs::path full_path = fs::path(left) / right / right1 / right2;
            std::string result = full_path.string();
            return result;
        }

        /**сложить два пути */
        std::string combine(std::string left, std::string right, std::string right1, std::string right2, std::string right3) {
            fs::path full_path = fs::path(left) / right / right1 / right2 / right3;
            std::string result = full_path.string();
            return result;
        }

        /** Получить папку от полного пути файла или родительскую папку от указанной папки */
        std::string parent(std::string path) {
            std::filesystem::path p(path);
            std::filesystem::path dir = p.parent_path();
            return dir.string();
        }
    }
}

