#include <wx/wx.h>
#include <string>
#include <atomic>
#include <thread>
#include "Data/ObservableValue.h"
#include "assistants/assistant_buss.h"
#include "assistants/assistant_core.h"
#include "assistants/assistant_js.h"
#include "assistants/assistant_db.h"
#include "assistants/assistant_app.h"
#include "assistants/assistant_cmd.h"
#include "Data/JSValueClientData.h"
#include "controls/wxDebugPanel.h"
#include "assistants/prototypes.h"
#include "assistants/render.h"
#include "assistants/quickJsEngine.h"


class jsApp : public wxApp
{
private:
    wxFrame *mainFrame;
    QuickJsEngine *js = new QuickJsEngine();
    bool isRegisterPrototypes = false;
    inline static std::atomic<int> counterAsync{0};

    static JSValue js_init_main_frame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
    {        
        auto app = QuickJsEngine::getContextOpaque<jsApp>(ctx);
        auto value = assistant::js::getValue(argc, argv, 0);
        if (app && app->getJs()->getClassName(value) == "Frame")
        {
            render::frame::apply(app->mainFrame, ctx, value);
        }

        return assistant::js::getUndefined();
    }

    static JSValue js_find_by_name(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
    {
        auto value = assistant::js::getValue(argc, argv, 0);
        auto name = assistant::js::to_string(ctx, value);
        wxWindow* control = wxWindow::FindWindowByName(wxString::FromUTF8(name.value().c_str()), nullptr);
        auto className = assistant::js::getClassName(ctx, render::base::getValue(control));
        return JS_DupValue(ctx, render::base::getValue(control));
    }

    static JSValue on_finally_callback(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *data)
    {
        auto app = QuickJsEngine::getContextOpaque<jsApp>(ctx);
        if (app)
        {
            app->showAsyncMask(-1);
        }
        return assistant::js::getUndefined();
    }

    static JSValue js_start_async(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
    {
        JSValue resolving_funcs[2];
        JSValue promise = JS_NewPromiseCapability(ctx, resolving_funcs);
        auto app = QuickJsEngine::getContextOpaque<jsApp>(ctx);

        if (argc < 1 || !app)
        {
            auto parameter = JS_NewBool(ctx, false);
            auto return_value = assistant::js::call(ctx, resolving_funcs[0], assistant::js::getUndefined(), parameter);
            JS_FreeValue(ctx, parameter);
            JS_FreeValue(ctx, return_value);
            return promise;
        }

        JSValue function = argv[0];
        if (!JS_IsFunction(ctx, function))
        {
            auto parameter = JS_NewBool(ctx, false);
            auto return_value = assistant::js::call(ctx, resolving_funcs[0], assistant::js::getUndefined(), parameter);
            JS_FreeValue(ctx, parameter);
            JS_FreeValue(ctx, return_value);
            return promise;
        }

        JSValue resolve_func = resolving_funcs[0];
        JSValue reject_func = resolving_funcs[1];

        auto value_promise = assistant::js::call(ctx, function, assistant::js::getUndefined(), nullptr, 0);
        if (assistant::js::is_promise(ctx, value_promise))
        {
            JSValue finally_fn = JS_GetPropertyStr(ctx, value_promise, "finally");

            if (JS_IsFunction(ctx, finally_fn))
            {
                app->showAsyncMask(1);
                JSValue finally_cb = JS_NewCFunctionData(ctx, on_finally_callback, 0, 0, 0, nullptr);
                JSValue finally_ret = assistant::js::call(ctx, finally_fn, value_promise, &finally_cb, 1);

                JS_FreeValue(ctx, finally_ret);
                JS_FreeValue(ctx, finally_cb);
            }

            JS_FreeValue(ctx, finally_fn);
        }

        JS_FreeValue(ctx, value_promise);
        JS_FreeValue(ctx, resolve_func);
        JS_FreeValue(ctx, reject_func);

        return promise;
    }

    /** Факт того, что джаваскрипты готовы к работе */
    void prepareScripts() {
        auto executablePath = assistant::directory::getDirectoryExecutable();
        auto js_directory = assistant::path::combine(executablePath, "js");
        assistant::directory::create(js_directory);

        auto eto_js = assistant::path::combine(js_directory, "eto.ts");
        auto main_js = assistant::path::combine(js_directory, "main.ts");

        if (!assistant::file::exists(eto_js)) {
            auto content = assistant::core::getContentResource("ETO_SCRIPT");
            assistant::file::write(eto_js, content);
        }

        if (!assistant::file::exists(main_js)) {
            auto content = assistant::core::getContentResource("MAIN_SCRIPT");
            assistant::file::write(main_js, content);
        }
    }

    void registerPrototypes() 
    {
        if (this->isRegisterPrototypes)
        {
            return;
        }
        this->isRegisterPrototypes = true;
        this->js->registerPrototype("Text", "getValue", prototypes::text::get_value);
        this->js->registerPrototype("Text", "setValue", prototypes::text::set_value);

        prototypes::comboBox::registration(this->js->getCtx());

        this->js->registerPrototype("CheckBox", "getLabel", prototypes::checkBox::get_label);
        this->js->registerPrototype("CheckBox", "getValue", prototypes::checkBox::get_value);
        this->js->registerPrototype("CheckBox", "setLabel", prototypes::checkBox::set_label);
        this->js->registerPrototype("CheckBox", "setValue", prototypes::checkBox::set_value);
        assistant::git::registrationPrototypes(this->js->getCtx());
    }

    void showAsyncMask(int countMask) {
        this->counterAsync += countMask;
        assistant::buss::asyncCounter->set(this->counterAsync);
    }

    void refreshScript() {
        for (wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst(); node; node = node->GetNext()) {
            wxFrame *frame = dynamic_cast<wxFrame *>(node->GetData());
            if (frame != this->mainFrame) {
                frame->Close();
            }
        }

        this->mainFrame->DestroyChildren();

        wxTheApp->CallAfter([]() {
            jsApp *app = dynamic_cast<jsApp *>(wxTheApp);
            if (app) {
                app->runMainJs();
            }
        });
    }

    /** Запустить скрипт приложения */
    void runMainJs() {
        this->js->evalFile("js\\main.ts");
    }

    int FilterEvent(wxEvent& event) {
        if (event.GetEventType() == wxEVT_KEY_DOWN) {
            wxKeyEvent& keyEvent = static_cast<wxKeyEvent&>(event);
            if (keyEvent.GetKeyCode() == WXK_F5) {
                this->refreshScript();
                return Event_Processed;
            }
            if (keyEvent.GetKeyCode() == WXK_F11) {
                auto sizer = dynamic_cast<wxFrameSizer *>(this->mainFrame->GetSizer());
                if (sizer) {
                    sizer->changeShowDebugPanel();
                    return Event_Processed;
                }
            }
        }
        return Event_Skip;
    }

public:
    virtual bool OnInit()
    {
        prepareScripts();

        if (!js->init())
        {
            assistant::ui::error(js->getErrorInit());
            this->Exit();
            return false;
        }
        js->setContextOpaque(this);

        assistant::app::registration(js->getCtx());

        js->registerFn("app", "initMainFrame", js_init_main_frame);
        js->registerFn("app", "findByName", js_find_by_name);
        js->registerFn("app", "startAsync", js_start_async);

        js->registerFn("console", "log", assistant::console::log);
        js->registerFn("console", "war", assistant::console::war);

        js->registerFn("app.storage", "add", assistant::db::add);
        js->registerFn("app.storage", "filter", assistant::db::filter);
        js->registerFn("app.storage", "remove", assistant::db::remove);
        js->registerFn("app.storage", "update", assistant::db::update);
        js->registerFn("app.storage", "getById", assistant::db::getById);
        js->registerFn("app.cmd", "run", assistant::cmd::run);

        assistant::file::registration(js->getCtx());
        assistant::directory::registration(js->getCtx());

        js->registerPromiseInObject("app", "testAsync", [](JSContext* ctx, JSValue this_instance, std::vector<JsVariant> args) -> JsVariant
        {
            auto seconds = args.size() > 1 ? args.at(1).to_int().value_or(0) : 0;
            std::this_thread::sleep_for(std::chrono::seconds(seconds));
            return JsVariant(std::string("test"));
        });

        mainFrame = new wxFrame(NULL, wxID_ANY, wxT("Простое окно wxWidgets"));
        mainFrame->Show(true);
        // Вызываем лямбду сразу после того, как frame полностью отобразится
        mainFrame->CallAfter([this]() 
        {
            auto converterTypeScript = assistant::core::getContentResource("SUCRASE_SCRIPT");
            assistant::js::evalScript(this->js->getCtx(), converterTypeScript, "sucrase.js");

            auto initScript = assistant::core::getContentResource("ETO_SCRIPT");
            initScript = assistant::js::convert_ts_to_js(this->js->getCtx(), initScript).value_or("");
//
            assistant::js::evalScript(this->js->getCtx(), "globalThis.exports = {};", "<init>");
            assistant::js::evalScript(this->js->getCtx(), initScript, "<init>");
            this->registerPrototypes();
            this->runMainJs();
        });

        return true;
    }

    virtual int OnExit() {
        js->free();
        delete js;
        return 0;
    }

    /** Вернуть указатель на движок JS */
    QuickJsEngine *getJs() {
        return this->js;
    }
};
