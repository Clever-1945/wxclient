#pragma once

#include "assistant_js.h"
#include "assistant_core.h"
#include <iostream>
#include <string>
#include <windows.h>
#include "Data/ObservableValue.h"
#include "Data/JSException.h"
#include "Data/JsVariant.h"

extern "C"
{
#include <quickjs.h>
}

namespace assistant
{
    namespace process
    {
        /** Сообщить об ошибке в процессе */
        void emitError(std::string error) {
            JSException e = {};
            e.error = error;
            e.stack = "";
            assistant::js::exceptions->set(e);
        }

        /** Запустить с аргументами и получать данные из выходного потока */
        void run(std::string command, std::string currentDirectory, const std::function<void(std::string)>& receiveOutput) {
            SECURITY_ATTRIBUTES saAttr;
            saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
            saAttr.bInheritHandle = TRUE;
            saAttr.lpSecurityDescriptor = NULL;

            HANDLE hReadPipe, hWritePipe;
            if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) {
                emitError("Запуск процесса -> Ошибка создания Pipe");
                return;
            }

            if (!SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0)) {
                emitError("Запуск процесса -> Ошибка SetHandleInformation");
                return;
            }

            STARTUPINFOA si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);

            si.hStdOutput = hWritePipe;
            si.hStdError = hWritePipe;
            si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            ZeroMemory(&pi, sizeof(pi));
            LPCSTR lpCurrentDirectory = currentDirectory.empty() ? NULL : currentDirectory.data();

            if (!CreateProcessA(NULL, command.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, lpCurrentDirectory, &si, &pi)) {
                std::string error = "Запуск процесса -> Ошибка CreateProcess: " + std::to_string(GetLastError());
                emitError(error);
                CloseHandle(hReadPipe);
                CloseHandle(hWritePipe);
                return;
            }

            CloseHandle(hWritePipe);

            char buffer[4096 * 2];
            DWORD bytesRead;

            while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                receiveOutput(std::string(buffer));
            }
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hReadPipe);
        }

        /** Запустить с аргументами и получать данные из выходного потока */
        void run(std::string command, const std::function<void(std::string)>& receiveOutput) {
            assistant::process::run(command, std::string(), receiveOutput);
        }

        /** Запустить с аргументами и получать данные из выходного потока */
        JSValue run(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, const std::function<void(std::string, const std::function<void(std::string)>&)>& runner) {
            ctx = JS_DupContext(ctx);
            JSValue resolving_funcs[2];
            JSValue promise = JS_NewPromiseCapability(ctx, resolving_funcs);
            JSValue resolve_func = JS_DupValue(ctx, resolving_funcs[0]);
            JS_FreeValue(ctx, resolving_funcs[0]);
            JS_FreeValue(ctx, resolving_funcs[1]);

            std::string command = "";
            if (argc >= 1) {
                command = assistant::js::to_string(ctx, argv[0]).value_or("");
            }

            if (command.empty()) {
                auto return_value = assistant::js::call(ctx, resolve_func, assistant::js::getUndefined(), nullptr, 0);
                JS_FreeValue(ctx, return_value);
                JS_FreeValue(ctx, resolve_func);
                JS_FreeContext(ctx);
                return promise;
            }


            JSValue receiveOutput = assistant::js::getUndefined();
            if (argc >= 2) {
                if (JS_IsFunction(ctx, argv[1])) {
                    receiveOutput = JS_DupValue(ctx, argv[1]);
                }
            }

            std::thread worker([&runner, resolve_func, ctx, receiveOutput, command]() {
                runner(command, [ctx, receiveOutput](std::string line){
                    wxTheApp->CallAfter([ctx, receiveOutput, line_copy = line]() {
                        if (!JS_IsUndefined(receiveOutput)) {
                            JSValue js_line = assistant::js::to_value(ctx, line_copy);
                            assistant::js::call(ctx, receiveOutput, assistant::js::getUndefined(), js_line);
                            JS_FreeValue(ctx, js_line);
                        }
                    });
                });

                wxTheApp->CallAfter([resolve_func, ctx, receiveOutput]() {
                    if (!JS_IsUndefined(receiveOutput)) {
                        JS_FreeValue(ctx, receiveOutput);
                    }
                    assistant::js::call(ctx, resolve_func, assistant::js::getUndefined(), nullptr, 0);
                    JS_FreeValue(ctx, resolve_func);
                    assistant::js::pendingJob(ctx);
                    JS_FreeContext(ctx);
                });
            });
            worker.detach();

            return promise;
        }
    }

    namespace cmd
    {
        /** Выполнить команду при помощи командной строки */
        void runCmd(std::string command, const std::function<void(std::string)>& receiveOutput) {
            assistant::process::run("cmd.exe /c chcp 65001 && " + command, receiveOutput);
        }

        JSValue run(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            return assistant::process::run(ctx, this_val, argc, argv, assistant::cmd::runCmd);
        }
    }

    namespace git
    {
        std::string runGitCommand(std::string command, std::string currentDirectory) {
            std::stringstream ss;
            assistant::process::run(std::string("git ") + command, currentDirectory, [&ss](std::string line) {
                if (!line.empty()) {
                    ss << line << "\r\n";
                }
            });

            return ss.str();
        }

        void registrationPrototypes (JSContext *ctx) {
            assistant::js::registerPromiseInClass(ctx, "GitRepository", "runGitCommand", [](JSContext* ctx, JSValue this_instance, std::vector<JsVariant> parameters) {
                if (parameters.size() < 1) {
                    return JsVariant();
                }
                auto _repositoryFileName = assistant::js::to_string(ctx, this_instance, "_repositoryFileName");
                if (!_repositoryFileName.has_value()) {
                    return JsVariant();
                }

                auto command = parameters.at(0).to_string();
                if (!command.has_value()) {
                    return JsVariant();
                }
                auto currentDirectory = assistant::path::parent(_repositoryFileName.value());
                auto result = assistant::git::runGitCommand(command.value(), currentDirectory);
                return JsVariant(result);
            });
        }
    }
}