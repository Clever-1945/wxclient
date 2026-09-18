#include <wx/wx.h>
#include <string>
#include <atomic>
#include <thread>
#include "Data/ObservableValue.h"
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
        wxFrameData *data = nullptr;
        if (app && app->getJs()->getClassName(value) == "Frame")
        {
            render::frame::apply(app->mainFrame, ctx, value);
            data = render::frame::getFrameData(app->mainFrame);
        }

        return assistant::js::getUndefined();
    }

    static JSValue js_find_by_name(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
    {
        auto value = assistant::js::getValue(argc, argv, 0);
        auto name = assistant::js::to_string(ctx, value);
        wxWindow* control = wxWindow::FindWindowByName(wxString::FromUTF8(name.value().c_str()), nullptr);
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
            JSValue argv_error[1] =
            {
                JS_NewBool(ctx, false)
            };
            auto return_value = assistant::js::call(ctx, resolving_funcs[0], assistant::js::getUndefined(), 1, argv_error);
            JS_FreeValue(ctx, argv_error[0]);
            JS_FreeValue(ctx, return_value);
            return promise;
        }

        JSValue function = argv[0];
        if (!JS_IsFunction(ctx, function))
        {
            JSValue argv_error[1] =
            {
                JS_NewBool(ctx, false)
            };
            auto return_value = assistant::js::call(ctx, resolving_funcs[0], assistant::js::getUndefined(), 1, argv_error);
            JS_FreeValue(ctx, argv_error[0]);
            JS_FreeValue(ctx, return_value);
            return promise;
        }

        JSValue resolve_func = resolving_funcs[0];
        JSValue reject_func = resolving_funcs[1];

        auto value_promise = assistant::js::call(ctx, function, assistant::js::getUndefined(), 0, nullptr);
        if (assistant::js::is_promise(ctx, value_promise))
        {
            JSValue finally_fn = JS_GetPropertyStr(ctx, value_promise, "finally");

            if (JS_IsFunction(ctx, finally_fn))
            {
                app->showAsyncMask(1);
                JSValue finally_cb = JS_NewCFunctionData(ctx, on_finally_callback, 0, 0, 0, nullptr);
                JSValue finally_ret = assistant::js::call(ctx, finally_fn, value_promise, 1, &finally_cb);

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

        auto eto_js = assistant::path::combine(js_directory, "eto.js");
        auto main_js = assistant::path::combine(js_directory, "main.js");

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

        this->js->registerPrototype("ComboBox", "setItems", prototypes::comboBox::set_items);
        this->js->registerPrototype("ComboBox", "getSelectedValue", prototypes::comboBox::get_selected_value);
        this->js->registerPrototype("ComboBox", "setSelectedValue", prototypes::comboBox::set_selected_value);

        this->js->registerPrototype("CheckBox", "getLabel", prototypes::checkBox::get_label);
        this->js->registerPrototype("CheckBox", "getValue", prototypes::checkBox::get_value);
        this->js->registerPrototype("CheckBox", "setLabel", prototypes::checkBox::set_label);
        this->js->registerPrototype("CheckBox", "setValue", prototypes::checkBox::set_value);
    }

    void showAsyncMask(int countMask)
    {
        this->counterAsync += countMask;
        wxTheApp->CallAfter([this]() 
        {
            int counterAsync = this->counterAsync;
            for (wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst(); node; node = node->GetNext())
            {
                wxFrame *frame = dynamic_cast<wxFrame*>(node->GetData());                
                auto data = render::frame::getFrameData(frame);
                if (data)
                {
                    data->showLoading(counterAsync);
                }
            }
        });
    }

    /** Запустить скрипт приложения */
    void runMainJs()
    {
        this->js->evalFile("js\\main.js");
        auto data = render::frame::getFrameData(this->mainFrame);
        if (data)
        {
            data->setClickRefresh([this] 
            {
                for (wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst(); node; node = node->GetNext())
                {
                    wxFrame *frame = dynamic_cast<wxFrame*>(node->GetData());                
                    if (frame != this->mainFrame)
                    {
                        frame->Close();
                    }
                }

                this->mainFrame->DestroyChildren();

                wxTheApp->CallAfter([]() 
                {
                    jsApp* app = dynamic_cast<jsApp*>(wxTheApp);
                    if (app) 
                    {
                        app->runMainJs();
                    }
                });
            });
        }
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

        js->registerFn("app", "alert", assistant::app::alert);
        js->registerFn("app", "error", assistant::app::error);
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

        js->registerPromiseFn("app", "testAsync", [](std::vector<JsVariant> args) -> JsVariant 
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            return JsVariant(std::string("test"));
        });

        mainFrame = new wxFrame(NULL, wxID_ANY, wxT("Простое окно wxWidgets"));
        mainFrame->Show(true);
        // Вызываем лямбду сразу после того, как frame полностью отобразится
        mainFrame->CallAfter([this]() 
        {
            this->js->evalFile("js\\eto.js");
            this->registerPrototypes();
            this->runMainJs();
        });

        return true;
    }

    virtual int OnExit()
    {
        js->free();
        delete js;
        return 0;
    }

    /** Вернуть указатель на движок JS */
    QuickJsEngine *getJs()
    {
        return this->js;
    }
};
