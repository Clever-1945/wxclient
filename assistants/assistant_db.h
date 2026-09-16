#pragma once

#include <iostream>
#include <sqlite3.h>
#include <wx/wx.h>
#include <set>
#include "assistant_core.h"
#include "assistant_js.h"
#include "Data/JSException.h"
#include "Data/ObservableValue.h"

namespace assistant {
    namespace db {
        namespace pprivate {
            /** Таблицы, которые проверены и они точно есть в БД */
            std::set<std::string> createdCollections;

            sqlite3 *_db = nullptr;

            /** Получить подключение к базе данных */
            sqlite3* getDb() {
                if (_db) {
                    return _db;
                }
                auto directoryExecutable = assistant::directory::getDirectoryExecutable();
                auto fileName = assistant::path::combine(directoryExecutable, "data.db");
                auto rc = sqlite3_open(fileName.c_str(), &_db);

                if (rc) {
                    assistant::ui::error(std::string("Не удалось открыть файл БД: ") + sqlite3_errmsg(_db));
                    wxTheApp->Exit();
                    return nullptr;
                }

                return _db;
            }

            /** Подготовить коллекцию. Если ее нету, то создать */
            void prepareCollection(std::string name) {
                auto it = createdCollections.find(name);
                if (it != createdCollections.end()) {
                    return;
                }

                std::string sql_create =
                        "CREATE TABLE IF NOT EXISTS " + name + " ("
                        "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "JSON_CONTENT TEXT NOT NULL);";

                auto db = getDb();
                char* errMsg = nullptr;
                auto rc = sqlite3_exec(db, sql_create.c_str(), nullptr, nullptr, &errMsg);
                if (rc != SQLITE_OK) {
                    assistant::ui::error(std::string("Ошибка SQL (создание таблицы): ") + errMsg);
                    sqlite3_free(errMsg);
                    wxTheApp->Exit();
                }

                createdCollections.insert(name);
            }

            /** Оповестить приложение о ошибке */
            void emitError(std::string message) {
                JSException ex{};
                ex.error = message + ": " + sqlite3_errmsg(getDb());
                ex.stack = "";
                assistant::js::exceptions->set(ex);
            }
        }

        /** Добавить объект в хранилище и возвращает Id обхекта */
        std::optional<int64_t> add(std::string collection, std::string json) {
            pprivate::prepareCollection(collection);
            std::string sql_insert = "INSERT INTO " + collection + " (JSON_CONTENT) VALUES (?);";

            sqlite3_stmt* stmt;
            auto db = pprivate::getDb();
            if (!db) {
                return std::nullopt;
            }

            auto rc = sqlite3_prepare_v2(db, sql_insert.c_str(), -1, &stmt, nullptr);

            if (rc != SQLITE_OK) {
                pprivate::emitError("Ошибка подготовки запроса");
                return std::nullopt;
            }

            rc = sqlite3_bind_text(stmt, 1, json.c_str(), -1, SQLITE_TRANSIENT);

            if (rc != SQLITE_OK) {
                pprivate::emitError("Ошибка привязки параметра");
                sqlite3_finalize(stmt);
                return std::nullopt;
            }

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                pprivate::emitError("Ошибка выполнения запроса");
                sqlite3_finalize(stmt);
                return std::nullopt;
            }
            sqlite3_finalize(stmt);
            sqlite3_int64 last_id = sqlite3_last_insert_rowid(db);
            return last_id;
        }

        /** Удалить запись из коллекции по идентификтаору */
        void remove(std::string collection, int64_t id) {
            pprivate::prepareCollection(collection);
            std::string sql_delete = "DELETE FROM " + collection + " WHERE ID = ?;";

            sqlite3_stmt* stmt;
            auto db = pprivate::getDb();
            if (!db) {
                return;
            }

            int rc = sqlite3_prepare_v2(db, sql_delete.c_str(), -1, &stmt, nullptr);
            if (rc != SQLITE_OK) {
                pprivate::emitError("Ошибка подготовки запроса удаления");
                return;
            }

            rc = sqlite3_bind_int64(stmt, 1, id);
            if (rc != SQLITE_OK) {
                pprivate::emitError("Ошибка привязки параметра ID");
                sqlite3_finalize(stmt);
                return;
            }

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                pprivate::emitError("Ошибка выполнения запроса удаления");
                sqlite3_finalize(stmt);
                return;
            }

            sqlite3_finalize(stmt);
        }

        /** Обновить объект по идентификтаору */
        void update(std::string collection, int64_t id, std::string json) {
            pprivate::prepareCollection(collection);
            std::string sql_update = "UPDATE " + collection + " SET JSON_CONTENT = ? WHERE ID = ?;";

            sqlite3_stmt* stmt;
            auto db = pprivate::getDb();
            if (!db) {
                return;
            }

            int rc = sqlite3_prepare_v2(db, sql_update.c_str(), -1, &stmt, nullptr);
            if (rc != SQLITE_OK) {
                pprivate::emitError("Ошибка подготовки запроса обновления");
                return;
            }

            rc = sqlite3_bind_text(stmt, 1, json.c_str(), -1, SQLITE_TRANSIENT);
            if (rc != SQLITE_OK) {
                pprivate::emitError("Ошибка привязки параметра JSON");
                sqlite3_finalize(stmt);
                return;
            }

            rc = sqlite3_bind_int64(stmt, 2, id);
            if (rc != SQLITE_OK) {
                pprivate::emitError("Ошибка привязки параметра ID");
                sqlite3_finalize(stmt);
                return;
            }

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                pprivate::emitError("Ошибка выполнения запроса обновления");
                sqlite3_finalize(stmt);
                return;
            }

            sqlite3_finalize(stmt);
        }

        /**
         * Фильтрация объектов
         * '$.age' = 25"
         * */
        std::vector<std::pair<int64_t, std::string>> filter(std::string collection, std::string query) {
            pprivate::prepareCollection(collection);

            std::string sql_select = query.empty()
                                     ? "SELECT ID, JSON_CONTENT FROM " + collection + ";"
                                     : "SELECT ID, JSON_CONTENT FROM " + collection + " WHERE JSON_CONTENT ->> " + query + ";";

            sqlite3_stmt* stmt;
            auto db = pprivate::getDb();
            std::vector<std::pair<int64_t, std::string>> results;
            if (!db) {
                return results;
            }

            int rc = sqlite3_prepare_v2(db, sql_select.c_str(), -1, &stmt, nullptr);
            if (rc != SQLITE_OK) {
                pprivate::emitError("Ошибка подготовки запроса фильтрации");
                return results;
            }

            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                int64_t obj_id = sqlite3_column_int64(stmt, 0);
                const unsigned char* text_ptr = sqlite3_column_text(stmt, 1);

                if (text_ptr != nullptr) {
                    std::string json_str = reinterpret_cast<const char*>(text_ptr);
                    results.push_back({obj_id, json_str});
                }
            }

            if (rc != SQLITE_DONE) {
                pprivate::emitError("Ошибка во время извлечения строк фильтра");
            }

            sqlite3_finalize(stmt);
            return results;
        }

        /** Получить объект по идентификатору */
        std::optional<std::string> getById(std::string collection, int64_t id) {
            pprivate::prepareCollection(collection);
            std::string sql_select = "SELECT JSON_CONTENT FROM " + collection + " WHERE ID = ?;";

            sqlite3_stmt* stmt;
            auto db = pprivate::getDb();
            if (!db) {
                return std::nullopt;
            }

            int rc = sqlite3_prepare_v2(db, sql_select.c_str(), -1, &stmt, nullptr);
            if (rc != SQLITE_OK) {
                pprivate::emitError("Ошибка подготовки запроса getById");
                return std::nullopt;
            }

            rc = sqlite3_bind_int64(stmt, 1, id);
            if (rc != SQLITE_OK) {
                pprivate::emitError("Ошибка привязки параметра ID в getById");
                sqlite3_finalize(stmt);
                return std::nullopt;
            }

            rc = sqlite3_step(stmt);

            std::optional<std::string> result = std::nullopt;

            if (rc == SQLITE_ROW) {
                const unsigned char* text_ptr = sqlite3_column_text(stmt, 0);
                if (text_ptr != nullptr) {
                    result = reinterpret_cast<const char*>(text_ptr);
                }
            } else if (rc != SQLITE_DONE) {
                pprivate::emitError("Ошибка выполнения запроса getById");
            }

            sqlite3_finalize(stmt);
            return result;
        }

        /** Добавить объект в хранилище и возвращает Id обхекта */
        JSValue add(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc < 2) {
                pprivate::emitError("Ожидается 2 параметра");
                return assistant::js::getUndefined();
            }

            auto collection = assistant::js::to_string(ctx, argv[0]);
            if (!collection.has_value()) {
                pprivate::emitError("Не указана коллекция первым параметром");
                return assistant::js::getUndefined();
            }

            auto json = assistant::js::to_json(ctx, argv[1]);
            if (!json.has_value()) {
                pprivate::emitError("Не указан объект выторым параметром");
                return assistant::js::getUndefined();
            }

            auto id = add(collection.value(), json.value());
            if (!id.has_value()) {
                return assistant::js::getUndefined();
            }

            return assistant::js::to_value(ctx, id.value());
        }

        /** Удалить запись из коллекции по идентификтаору */
        JSValue remove(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc < 2) {
                pprivate::emitError("Ожидается 2 параметра");
                return assistant::js::getUndefined();
            }

            auto collection = assistant::js::to_string(ctx, argv[0]);
            if (!collection.has_value()) {
                pprivate::emitError("Не указана коллекция первым параметром");
                return assistant::js::getUndefined();
            }

            auto id = assistant::js::to_int_64(ctx, argv[1]);
            if (!id.has_value()) {
                pprivate::emitError("Не указан id вторым параметром");
                return assistant::js::getUndefined();
            }

            remove(collection.value(), id.value());
            return assistant::js::getUndefined();
        }

        /** Обновить объект в хранилище */
        JSValue update(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc < 2) {
                pprivate::emitError("Ожидается 2 параметра");
                return assistant::js::getUndefined();
            }
            auto collection = assistant::js::to_string(ctx, argv[0]);
            if (!collection.has_value()) {
                pprivate::emitError("Не указана коллекция первым параметром");
                return assistant::js::getUndefined();
            }

            auto value = argv[1];
            auto id = assistant::js::to_int_64(ctx, value, "id");
            if (!id.has_value()) {
                pprivate::emitError("В объекте отсутствует поле id");
                return assistant::js::getUndefined();
            }

            JSAtom id_atom = JS_NewAtom(ctx, "id");
            if (JS_HasProperty(ctx, value, id_atom)) {
                JS_DeleteProperty(ctx, value, id_atom, 0);
            }
            JS_FreeAtom(ctx, id_atom);

            auto json = assistant::js::to_json(ctx, value);
            if (!json.has_value()) {
                pprivate::emitError("Не указан объект выторым параметром");
                return assistant::js::getUndefined();
            }

            update(collection.value(), id.value(), json.value());
            return assistant::js::getUndefined();
        }

        /**
         * Фильтрация объектов
         * '$.age' = 25
         * */
        JSValue filter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc < 1) {
                return assistant::js::getUndefined();
            }
            auto collection = assistant::js::to_string(ctx, argv[0]);
            if (!collection.has_value()) {
                pprivate::emitError("Не указана коллекция первым параметром");
                return assistant::js::getUndefined();
            }

            std::optional<std::string> query = argc >= 2
                    ? assistant::js::to_string(ctx, argv[1])
                    : "";

            auto results = filter(collection.value(), query.value_or(""));

            JSValue js_arr = JS_NewArray(ctx);
            for (size_t i = 0; i < results.size(); i++)
            {
                auto result = results.at(i);
                auto elem = assistant::js::to_value_from_json(ctx, result.second);

                JSValue id = assistant::js::to_value(ctx, result.first);
                JS_DefinePropertyValueStr(ctx, elem, "id", id, JS_PROP_C_W_E);
                JS_DefinePropertyValueUint32(ctx, js_arr, i, elem, JS_PROP_C_W_E);
            }

            return js_arr;
        }

        /** Получить объект по идентификатору */
        JSValue getById(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc < 2) {
                pprivate::emitError("Ожидается 2 параметра");
                return assistant::js::getUndefined();
            }
            auto collection = assistant::js::to_string(ctx, argv[0]);
            if (!collection.has_value()) {
                pprivate::emitError("Не указана коллекция первым параметром");
                return assistant::js::getUndefined();
            }

            auto id = assistant::js::to_int_64(ctx, argv[1]);
            if (!id.has_value()) {
                pprivate::emitError("Не указан id вторым параметром");
                return assistant::js::getUndefined();
            }

            auto json = getById(collection.value(), id.value());
            if (!json.has_value()) {
                return assistant::js::getUndefined();
            }

            auto value = assistant::js::to_value_from_json(ctx, json.value());
            if (JS_IsException(value)) {
                return assistant::js::getUndefined();
            }
            if (!JS_IsObject(value)) {
                return assistant::js::getUndefined();
            }

            JSValue value_id = assistant::js::to_value(ctx, id.value());
            JS_DefinePropertyValueStr(ctx, value, "id", value_id, JS_PROP_C_W_E);

            return value;
        }
    }
}
