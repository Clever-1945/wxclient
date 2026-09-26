#pragma once

#include "assistant_js.h"
#include "assistant_core.h"
#include <iostream>
#include <string>

extern "C"
{
#include <quickjs.h>
}

namespace assistant
{
    namespace app {
        /** Получить текст для вывода сообщения */
        std::optional<std::string> getMessageText(JSContext *ctx, JSValue value) {
            if (JS_IsObject(value)) {
                auto json = assistant::js::to_json(ctx, value);
                return json;
            }

            if (JS_IsArray(ctx, value)) {
                auto json = assistant::js::to_json(ctx, value);
                return json;
            }

            auto text = assistant::js::to_string(ctx, value);
            if (text.has_value()) {
                return text.value();
            }

            auto number_int = assistant::js::to_int_64(ctx, value);
            if (number_int.has_value()) {
                return std::to_string(number_int.value());
            }

            auto boolean = assistant::js::to_bool(ctx, value);
            if (boolean.has_value()) {
                return boolean.value() ? "True" : "False";
            }

            auto number_double = assistant::js::to_double(ctx, value);
            if (number_double.has_value()) {
                return std::to_string(number_double.value());
            }

            return std::nullopt;
        }

        JSValue alert(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc < 1) {
                return assistant::js::getUndefined();
            }
            auto value = argv[0];
            auto text = getMessageText(ctx, value);
            assistant::ui::alert(text.value_or(""));
            return assistant::js::getUndefined();
        }

        JSValue error(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc < 1) {
                return assistant::js::getUndefined();
            }
            auto value = argv[0];
            auto text = getMessageText(ctx, value);
            assistant::ui::error(text.value_or(""));
            return assistant::js::getUndefined();
        }

        /** Диалоговое окно с вопросом Да НЕТ */
        JSValue yesNo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc > 0) {
                auto text = getMessageText(ctx, argv[0]);
                std::optional<std::string> title = argc >= 2 ? assistant::js::to_string(ctx, argv[1]) : std::nullopt;
                auto isYes = assistant::ui::yesNo(text.value_or(""), title.value_or(""));
                return assistant::js::to_value(ctx, isYes);
            }
            return assistant::js::getUndefined();
        }

        /** Список аргументов с которыми запущено приложение */
        JSValue getArguments(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            int argumentCount = wxTheApp->argc;

            std::vector<std::string> list;
            for (int i = 1; i < argumentCount; ++i) {
                std::string arg = wxTheApp->argv[i].ToStdString();
                list.push_back(arg);
            }

            return assistant::js::to_value(ctx, list);
        }

        /** Закрыть приложение */
        JSValue exit(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            wxTheApp->Exit();
            return assistant::js::getUndefined();
        }

        void registration (JSContext *ctx) {
            assistant::js::registerFn(ctx, "app", "alert", assistant::app::alert);
            assistant::js::registerFn(ctx, "app", "error", assistant::app::error);
            assistant::js::registerFn(ctx, "app", "getArguments", assistant::app::getArguments);
            assistant::js::registerFn(ctx, "app", "yesNo", assistant::app::yesNo);
            assistant::js::registerFn(ctx, "app", "exit", assistant::app::exit);
        }
    }

    namespace console {
        JSValue log(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc > 0) {
                auto text = assistant::app::getMessageText(ctx, argv[0]);
                assistant::js::logs->set(text.value_or(""));
            }

            return assistant::js::getUndefined();
        }

        JSValue war(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc > 0) {
                auto text = assistant::app::getMessageText(ctx, argv[0]);
                assistant::js::warning->set(text.value_or(""));
            }

            return assistant::js::getUndefined();
        }
    }

    namespace directory {
        JSValue exists(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
        JSValue create(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
        JSValue getDirectoryExecutable(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
        JSValue getListFile(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

        void registration (JSContext *ctx) {
            assistant::js::registerFn(ctx, std::string("app.directory"), std::string("exists"), assistant::directory::exists);
            assistant::js::registerFn(ctx, std::string("app.directory"), std::string("create"), assistant::directory::create);
            assistant::js::registerFn(ctx, std::string("app.directory"), std::string("getDirectoryExecutable"), assistant::directory::getDirectoryExecutable);
            assistant::js::registerFn(ctx, std::string("app.directory"), std::string("getListFile"), assistant::directory::getListFile);
        }

        JSValue exists(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc < 1) {
                return assistant::js::getUndefined();
            }
            auto directoryName = assistant::js::to_string(ctx, argv[0]);
            return directoryName.has_value()
                   ? assistant::js::to_value(ctx, assistant::directory::exists(directoryName.value()))
                   : assistant::js::getUndefined();
        }

        JSValue create(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc < 1) {
                return assistant::js::getUndefined();
            }
            auto directoryName = assistant::js::to_string(ctx, argv[0]);
            return directoryName.has_value()
                   ? assistant::js::to_value(ctx, assistant::directory::create(directoryName.value()))
                   : assistant::js::getUndefined();
        }

        /** получить папку запуска нашего приложения */
        JSValue getDirectoryExecutable(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            return assistant::js::to_value(ctx, assistant::directory::getDirectoryExecutable());
        }

        /** Получить список файлов в папке */
        JSValue getListFile(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc < 1) {
                return assistant::js::getUndefined();
            }
            auto directoryName = assistant::js::to_string(ctx, argv[0]);
            auto mask = argc >= 2 ? assistant::js::to_string(ctx, argv[1]) : std::nullopt;
            auto list = getListFile(directoryName.value(), mask.value_or(""));
            return assistant::js::to_value(ctx, list);
        }
    }

    namespace file {
        JSValue exists(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
        JSValue write(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
        JSValue read(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
        JSValue extension(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
        JSValue fileName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
        JSValue size(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
        JSValue directory(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

        void registration (JSContext *ctx) {
            assistant::js::registerFn(ctx, std::string("app.file"), std::string("exists"), assistant::file::exists);
            assistant::js::registerFn(ctx, std::string("app.file"), std::string("write"), assistant::file::write);
            assistant::js::registerFn(ctx, std::string("app.file"), std::string("read"), assistant::file::read);
            assistant::js::registerFn(ctx, std::string("app.file"), std::string("extension"), assistant::file::extension);
            assistant::js::registerFn(ctx, std::string("app.file"), std::string("fileName"), assistant::file::fileName);
            assistant::js::registerFn(ctx, std::string("app.file"), std::string("size"), assistant::file::size);
            assistant::js::registerFn(ctx, std::string("app.file"), std::string("directory"), assistant::file::directory);
        }

        JSValue directory(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc < 1) {
                return assistant::js::getUndefined();
            }
            auto fileName = assistant::js::to_string(ctx, argv[0]);
            if (!fileName.has_value()) {
                return assistant::js::getUndefined();
            }
            return assistant::js::to_value(ctx, assistant::file::directory(fileName.value()));
        }

        JSValue extension(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc < 1) {
                return assistant::js::getUndefined();
            }
            auto fileName = assistant::js::to_string(ctx, argv[0]);
            return fileName.has_value()
                   ? assistant::js::to_value(ctx, assistant::file::extension(fileName.value()))
                   : assistant::js::getUndefined();
        }

        JSValue fileName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc < 1) {
                return assistant::js::getUndefined();
            }
            auto fileName = assistant::js::to_string(ctx, argv[0]);
            return fileName.has_value()
                   ? assistant::js::to_value(ctx, assistant::file::fileName(fileName.value()))
                   : assistant::js::getUndefined();
        }

        JSValue size(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc < 1) {
                return assistant::js::getUndefined();
            }
            auto fileName = assistant::js::to_string(ctx, argv[0]);
            return fileName.has_value()
                   ? assistant::js::to_value(ctx, assistant::file::size(fileName.value()))
                   : assistant::js::getUndefined();
        }

        JSValue exists(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc < 1) {
                return assistant::js::getUndefined();
            }
            auto fileName = assistant::js::to_string(ctx, argv[0]);
            return fileName.has_value()
                   ? assistant::js::to_value(ctx, assistant::file::exists(fileName.value()))
                   : assistant::js::getUndefined();
        }

        /** Записать контент в файл */
        JSValue write(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc >= 2) {
                auto fileName = assistant::js::to_string(ctx, argv[0]);
                auto content = assistant::js::to_string(ctx, argv[1]);
                if (fileName.has_value() && content.has_value()) {
                    assistant::file::write(fileName.value(), content.value());
                }
            }
            return assistant::js::getUndefined();
        }

        /** Прочитать контент из файла */
        JSValue read(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            if (argc >= 1) {
                auto fileName = assistant::js::to_string(ctx, argv[0]);
                if (fileName.has_value() && assistant::file::exists(fileName.value())) {
                    auto content = assistant::file::read(fileName.value());
                    return assistant::js::to_value(ctx, content);
                }
            }
            return assistant::js::getUndefined();
        }
    }
}

