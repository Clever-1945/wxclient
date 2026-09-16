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
    namespace app
    {
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
    }

    namespace console
    {
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
}

