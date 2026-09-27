#pragma once
#include <string>
#include <optional>
#include <variant>
#include "Data/ObservableValue.h"
#include "Data/JSException.h"
#include "Data/JsVariant.h"
#include "Data/JsValue.h"

extern "C"
{
#include <quickjs.h>
}

namespace assistant
{
    namespace js
    {
        ObservableValue<JSException>* exceptions = new ObservableValue<JSException>();
        ObservableValue<std::string>* logs = new ObservableValue<std::string>();
        ObservableValue<std::string>* warning = new ObservableValue<std::string>();
        JsVariant to_variant(JsValue* value);
        /** Выполнить функцию в скрипте */
        std::unique_ptr<JsValue> call(JSContext *ctx, JSValueConst func_obj, JSValueConst this_obj, JSValueConst *argv, int argc);
        std::unique_ptr<JsValue> call(JSContext *ctx, JSValueConst func_obj, JSValueConst this_obj, JSValue parameter);
        JSValue getUndefined();
        /** Завершить все фоновые микротаски */
        void pendingJob (JSContext *ctx);
        /** Преобразовать TS контент в JS */
        std::optional<std::string> convert_ts_to_js(JSContext *ctx, const std::string& ts_source_code);

        namespace pprivate
        {
            bool processException(JSContext *ctx, JSValue result) {
                if (JS_IsException(result)) {
                    auto exceptionValue = JsValue::fromValue(ctx, JS_GetException(ctx));
                    auto message = exceptionValue->getValue("message")->to_string().value_or("");
                    auto stack = exceptionValue->getValue("stack")->to_string().value_or("");
                    if (!message.empty()) {
                        JSException ex{};
                        ex.error = message;
                        ex.stack = stack;
                        assistant::js::exceptions->set(ex);
                    }

                    return true;
                }

                return false;
            }

            /** Функция гарантирует получение или создание глобального объекта */
            JSValue getOrCreateGlobalObject(JSContext *ctx, std::string name, JSValue parentObject) {
                JSValue target_obj = JS_GetPropertyStr(ctx, parentObject, name.c_str());
                if (JS_IsObject(target_obj)) {
                    return target_obj;
                }

                JS_FreeValue(ctx, target_obj);

                target_obj = JS_NewObject(ctx);
                JS_SetPropertyStr(ctx, parentObject, name.c_str(), JS_DupValue(ctx, target_obj));
                return target_obj;
            }

            JSValue c_function_promise(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *data) {
                int64_t ptr_val;
                if (JS_ToInt64(ctx, &ptr_val, data[0]) < 0) {
                    JSException ex = {};
                    ex.error = "Failed to retrieve function pointer";
                    ex.stack = "";
                    assistant::js::exceptions->set(ex);
                    return JS_ThrowTypeError(ctx, ex.error.c_str());
                }

                auto *func = reinterpret_cast<std::function<JsVariant(JSContext*, JSValue, std::vector<JsVariant>)> *>(ptr_val);

                JSValue resolving_funcs[2];
                JSValue promise = JS_NewPromiseCapability(ctx, resolving_funcs);

                JSValue resolve_func = JS_DupValue(ctx, resolving_funcs[0]);
                JSValue reject_func = JS_DupValue(ctx, resolving_funcs[1]);

                JS_FreeValue(ctx, resolving_funcs[0]);
                JS_FreeValue(ctx, resolving_funcs[1]);

                std::vector<JsVariant> args;
                for (int i = 0; i < argc; i++) {
                    args.push_back(assistant::js::to_variant(GET_VALUE(i).get()));
                }

                std::thread worker([reject_func, resolve_func, args, func, ctx, this_val]() {
                    JsVariant result = JsVariant(std::monostate{});
                    bool isSucess = true;
                    try {
                        if (func) {
                            result = (*func)(ctx, this_val, args);
                        }
                    }
                    catch (const std::exception &e) {
                        isSucess = false;
                    }

                    wxTheApp->CallAfter([resolve_func, reject_func, result, ctx, isSucess]() {
                        auto js_result = result.to_js_value(ctx);
                        isSucess
                            ? assistant::js::call(ctx, resolve_func, assistant::js::getUndefined(), js_result)
                            : assistant::js::call(ctx, reject_func, assistant::js::getUndefined(), js_result);
                        JS_FreeValue(ctx, js_result);
                        JS_FreeValue(ctx, resolve_func);
                        JS_FreeValue(ctx, reject_func);

                        assistant::js::pendingJob(ctx);
                    });
                });
                worker.detach();
                return promise;
            }
        }

        /** Завершить все фоновые микротаски */
        void pendingJob (JSContext *ctx) {
            JSContext *pctx;
            while (JS_ExecutePendingJob(JS_GetRuntime(ctx), &pctx) > 0) {

            }
        }

        JSValue getUndefined() {
            return JS_UNDEFINED;
        }

        JSValue to_value(JSContext *ctx, std::string value) {
            return JS_NewString(ctx, value.c_str());
        }

        JSValue to_value(JSContext *ctx, int value) {
            return JS_NewInt32(ctx, value);
        }

        JSValue to_value(JSContext *ctx, long value) {
            return JS_NewInt64(ctx, value);
        }

        JSValue to_value(JSContext *ctx, int64_t value) {
            return JS_NewInt64(ctx, value);
        }

        JSValue to_value(JSContext *ctx, double value) {
            return JS_NewFloat64(ctx, value);
        }

        JSValue to_value(JSContext *ctx, bool value) {
            return JS_NewBool(ctx, value);
        }

        JSValue to_value(JSContext *ctx, std::vector<std::string> value) {
            JSValue js_arr = JS_NewArray(ctx);
            if (JS_IsException(js_arr)) {
                return JS_UNDEFINED;
            }

            auto size = value.size();
            for (int i = 0; i < size; i++) {
                JSValue elem = assistant::js::to_value(ctx, value[i]);
                JS_DefinePropertyValueUint32(ctx, js_arr, i, elem, JS_PROP_C_W_E);
            }
            return js_arr;
        }

        /** Преобразовать JSON строку в JS значение */
        JSValue to_value_from_json(JSContext *ctx, const std::string& json) {
            JSValue global_obj = JS_GetGlobalObject(ctx);
            JSValue json_obj = JS_GetPropertyStr(ctx, global_obj, "JSON");
            JSValue parse_fn = JS_GetPropertyStr(ctx, json_obj, "parse");
            JSValue json_js_string = JS_NewString(ctx, json.c_str());
            JSValue result = JS_Call(ctx, parse_fn, json_obj, 1, &json_js_string);

            JS_FreeValue(ctx, global_obj);
            JS_FreeValue(ctx, json_obj);
            JS_FreeValue(ctx, parse_fn);
            JS_FreeValue(ctx, json_js_string);

            return result;
        }

        JsVariant to_variant(JsValue* value) {
            if (value->isUndefined() || value->isNull()) {
                return JsVariant(std::monostate{});
            }
            if (JS_IsBool(value->getRawValue())) {
                return JsVariant(value->to_bool().value_or(false));
            }
            if (JS_IsNumber(value->getRawValue())) {
                auto int_64 = value->to_int_64();
                if (int_64.has_value()) {
                    return JsVariant(int_64.value_or(0));
                }

                return JsVariant(value->to_double().value_or(0));
            }

            if (JS_IsString(value->getRawValue())) {
                return JsVariant(value->to_string().value_or(""));
            }
            if (value->isArray()) {
                JsArray arr;
                int len = value->getLength().value_or(0);
                arr.reserve(len);

                for (int i = 0; i < len; i++) {
                    arr.push_back(to_variant(value->at(i).get()));
                }
                return JsVariant(arr);
            }
            if (value->isObject()) {
                JsObject obj;
                JSPropertyEnum *ptab = nullptr;
                uint32_t plen = 0;

                if (JS_GetOwnPropertyNames(value->getContext(), &ptab, &plen, value->getRawValue(), JS_GPN_STRING_MASK | JS_GPN_SYMBOL_MASK) >= 0) {
                    for (uint32_t i = 0; i < plen; i++) {
                        const char *key = JS_AtomToCString(value->getContext(), ptab[i].atom);
                        JSValue prop_val = JS_GetProperty(value->getContext(), value->getRawValue(), ptab[i].atom);
                        auto prop_value = std::make_unique<JsValue>(value->getContext(), prop_val, true);

                        if (key) {
                            obj[std::string(key)] = to_variant(prop_value.get());
                            JS_FreeCString(value->getContext(), key);
                        }

                        JS_FreeValue(value->getContext(), prop_val);
                        JS_FreeAtom(value->getContext(), ptab[i].atom);
                    }
                    js_free(value->getContext(), ptab);
                }
                return JsVariant(obj);
            }

            return JsVariant(std::monostate{});
        }

        /** Выполнить функцию в скрипте */
        std::unique_ptr<JsValue> call(JSContext *ctx, JSValueConst func_obj, JSValueConst this_obj, JSValueConst *argv, int argc) {
            JSValue result = JS_Call(ctx, func_obj, this_obj, argc, argv);
            assistant::js::pprivate::processException(ctx, result);
            assistant::js::pendingJob(ctx);
            return std::make_unique<JsValue>(ctx, result);
        }

        /** Выполнить функцию в скрипте */
        std::unique_ptr<JsValue> call(JSContext *ctx, JSValueConst func_obj, JSValueConst this_obj, JSValue parameter) {
            JSValue argv[1] = {parameter};
            return call(ctx, func_obj, this_obj, argv, 1);
        }

        /** Выполнить скрипт из файла */
        void evalScript(JSContext *ctx, const std::string& script, const std::string& fileName) {
            if (script.empty()) {
                return;
            }
            JSValue result = JS_Eval(ctx, script.c_str(), script.length(), fileName.c_str(), JS_EVAL_TYPE_GLOBAL);
            assistant::js::pprivate::processException(ctx, result);
            JS_FreeValue(ctx, result);
            assistant::js::pendingJob(ctx);
        }

        /** Выполнить скрипт из файла */
        void evalFile(JSContext *ctx, const std::string& fileName) {
            auto executableDirectory = assistant::directory::getDirectoryExecutable();
            auto fullFileName = assistant::path::combine(executableDirectory, fileName);
            std::string script = assistant::file::read(fullFileName);
            auto ts = assistant::js::convert_ts_to_js(ctx, script).value_or("");
            evalScript(ctx, ts, fileName);
        }

        /** Преобразовать TS контент в JS */
        std::optional<std::string> convert_ts_to_js(JSContext *ctx, const std::string& ts_source_code) {
            JSValue global_obj = JS_GetGlobalObject(ctx);
            JSValue transpile_fn = JS_GetPropertyStr(ctx, global_obj, "transpileTS");

            if (!JS_IsFunction(ctx, transpile_fn)) {
                JS_FreeValue(ctx, transpile_fn);
                JS_FreeValue(ctx, global_obj);
                JSException ex = {};
                ex.error = "Ошибка: Функция transpileTS не найдена в контексте QuickJS";
                ex.stack = std::string ();
                exceptions->set(ex);
                return std::nullopt;
            }

            JSValue arg_ts_code = JS_NewStringLen(ctx, ts_source_code.c_str(), ts_source_code.size());
            JSValue js_result_value = JS_Call(ctx, transpile_fn, global_obj, 1, &arg_ts_code);
            std::string pure_js_code = "";
            if (!JS_IsException(js_result_value)) {
                const char* c_str = JS_ToCString(ctx, js_result_value);
                if (c_str) {
                    pure_js_code = c_str;
                    JS_FreeCString(ctx, c_str);
                }
            } else {
                JSException ex = {};
                ex.error = "Ошибка во время транспиляции внутри Sucrase";
                ex.stack = std::string ();
                exceptions->set(ex);
            }

            JS_FreeValue(ctx, arg_ts_code);
            JS_FreeValue(ctx, js_result_value);
            JS_FreeValue(ctx, transpile_fn);
            JS_FreeValue(ctx, global_obj);

            return pure_js_code;
        }

        /** Получить значение по ключу в объекте */
        std::unique_ptr<JsValue> getJsValue(JSContext *ctx, JSValue value, const std::string& propertyName, bool disableFree = false) {
            return std::make_unique<JsValue>(ctx, value, propertyName, disableFree);
        }

        /** Получить значение по ключу в объекте */
        JSValue getValue(JSContext *ctx, JSValue value, std::string propertyName) {
            return JS_GetPropertyStr(ctx, value, propertyName.c_str());
        }

        /** Функция гарантирует получение или создание глобального объекта */
        JSValue getOrCreateGlobalObject(JSContext *ctx, const std::string& name) {
            auto segments = assistant::core::split(name, ".");
            JSValue currentObject = JS_GetGlobalObject(ctx);
            for (const auto &s: segments) {
                auto segment = assistant::core::trim(s);
                if (segment.empty()) {
                    continue;
                }

                JSValue nextObject = assistant::js::pprivate::getOrCreateGlobalObject(ctx, segment, currentObject);
                JS_FreeValue(ctx, currentObject);
                currentObject = nextObject;
            }

            return currentObject;
        }

        /** Получить указатель на портотип. Нужно удалить объект после использваония */
        JSValue getPrototype(JSContext *ctx, const std::string& className) {
            JSValue global_obj = JS_GetGlobalObject(ctx);
            JSValue class_constructor = JS_GetPropertyStr(ctx, global_obj, className.c_str());

            if (JS_IsException(class_constructor)) {
                JS_FreeValue(ctx, global_obj);
                return assistant::js::getUndefined();
            }

            if (JS_IsUndefined(class_constructor)) {
                JS_FreeValue(ctx, global_obj);
                return assistant::js::getUndefined();
            }

            JSValue proto = JS_GetPropertyStr(ctx, class_constructor, "prototype");
            JS_FreeValue(ctx, class_constructor);
            JS_FreeValue(ctx, global_obj);

            if (JS_IsException(proto)) {
                return assistant::js::getUndefined();
            }

            if (JS_IsObject(proto)) {
                return proto;
            }

            JS_FreeValue(ctx, proto);
            return assistant::js::getUndefined();
        }

        /** Зарегистрировать функцию для оъекта */
        void registerFn(JSContext *ctx, JSValue instanceObject, std::string functionName, JSCFunction *func) {
            if (JS_IsObject(instanceObject)) {
                JSValue js_func = JS_NewCFunction(ctx, func, functionName.c_str(), 0);
                if (JS_IsFunction(ctx, js_func)) {
                    JS_SetPropertyStr(ctx, instanceObject, functionName.c_str(), js_func);
                    JS_FreeValue(ctx, instanceObject);
                }
            }
        }

        /** Зарегистрировать функцию для оъекта */
        void registerFn(JSContext *ctx, std::string objectName, std::string functionName, JSCFunction *func) {
            JSValue instanceObject = assistant::js::getOrCreateGlobalObject(ctx, objectName);
            registerFn(ctx, instanceObject, functionName, func);
        }

        /** Зарегистрировать портатип для класса */
        void registerPrototype(JSContext *ctx, std::string className, std::string functionName, JSCFunction *func) {
            JSValue instanceObject = assistant::js::getPrototype(ctx, className);
            registerFn(ctx, instanceObject, functionName, func);
        }

        /** Регистрация функции, которая возвращает промис */
        void registerPromiseFn(JSContext *ctx, JSValue instanceObject, std::string functionName, const std::function<JsVariant(JSContext*, JSValue, std::vector<JsVariant>)>& fn) {
            auto smartFn = new std::function<JsVariant(JSContext*, JSValue, std::vector<JsVariant>)>(std::move(fn));
            uintptr_t rawAddress = reinterpret_cast<uintptr_t>(smartFn);
            int64_t addressAsInt64 = static_cast<int64_t>(rawAddress);

            JSValue data = JS_NewInt64(ctx, addressAsInt64);

            JSValue js_func = JS_NewCFunctionData(ctx, assistant::js::pprivate::c_function_promise, 0, 0, 1, &data);

            JS_SetPropertyStr(ctx, instanceObject, functionName.c_str(), js_func);
            JS_FreeValue(ctx, instanceObject);
            JS_FreeValue(ctx, data);
        }

        /** Регистрация функции, которая возвращает промис, для класса ( портотип ) */
        void registerPromiseInClass(JSContext *ctx, std::string className, std::string functionName, const std::function<JsVariant(JSContext*, JSValue, std::vector<JsVariant>)>& fn) {
            JSValue instanceObject = assistant::js::getPrototype(ctx, className);
            registerPromiseFn(ctx, instanceObject, functionName, fn);
        }

        /** Регистрация функции, которая возвращает промис, для объекта ( например app.otherObject.functionName ) */
        void registerPromiseInObject(JSContext *ctx, std::string objectName, std::string functionName, const std::function<JsVariant(JSContext*, JSValue, std::vector<JsVariant>)>& fn) {
            JSValue instanceObject = assistant::js::getOrCreateGlobalObject(ctx, objectName);
            registerPromiseFn(ctx, instanceObject, functionName, fn);
        }

        /** Послать событие обратно в JS и передать в событии ссылку на компонент */
        void emitEvent(wxEventControlJsData data, const std::string& eventName) {
            auto context = data.clientData->getContext();
            auto eventFunction = data.clientData->getJsValue(true)->getConfigValue()->getValue(eventName);
            if (eventFunction->is_function()) {
                assistant::js::call(context, eventFunction->getRawValue(), assistant::js::getUndefined(), data.clientData->getValue());
            }
        }
    }
}

