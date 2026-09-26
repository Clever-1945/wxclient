#include <wx/wx.h>
#include <wx/gbsizer.h>
#include <wx/stattext.h>
#include "controls/wxFrameSizer.h"
#include "Data/JsValue.h"

extern "C"
{
#include <quickjs.h>
}

namespace render
{
    /** Получить растяжения */
    std::optional<int> getGrowable(std::optional<std::string> text) {
        if (!text.has_value()) {
            return std::nullopt;
        }
        auto textValue = text.value();
        if (textValue == "*") {
            return 1;
        } else {
            auto growable = assistant::core::replaceAll(textValue, "*", "");
            if (growable.size() != textValue.size()) {
                auto growableInt = assistant::core::to_int(growable);
                if (growableInt.has_value()) {
                    return growableInt.value();
                }
            }
        }

        return std::nullopt;
    }

    namespace base
    {
        wxSizer *apply(wxSizer *sizer, JSContext *ctx, JSValue instance) {
            auto widthInt = assistant::js::to_int(ctx, instance, "width");
            auto heightInt = assistant::js::to_int(ctx, instance, "height");
            if (widthInt.has_value() || heightInt.has_value()) {
                sizer->SetMinSize(
                        widthInt.value_or(sizer->GetSize().GetWidth()),
                        heightInt.value_or(sizer->GetSize().GetHeight())
                );
            }

            auto clientData = new JSValueClientData(ctx, instance);
            sizer->SetClientObject(clientData);

            // Устанавливаем адрес ссылки в объект, в служебное поле
            int64_t address = reinterpret_cast<int64_t>(sizer);
            JSValue ptr_value = JS_NewInt64(ctx, address);
            JS_SetPropertyStr(ctx, instance, "__wx_ptr", ptr_value);

            return sizer;
        }

        /**
         * К каждому контролу созданному в окне привязан JSValue. этот JSValue объект, с конфогурацией и от этой уонфигурации был создан контрол
         * Функция возвращает объект конфигурации
         * */
        JSValue getValue(wxWindow *control) {
            if (!control) {
                return assistant::js::getUndefined();
            }

            wxClientData *clientDataVoid = control->GetClientObject();
            JSValueClientData *clientData = dynamic_cast<JSValueClientData *>(clientDataVoid);
            if (!clientData) {
                return assistant::js::getUndefined();
            }

            return clientData->getValue();
        }

        wxWindow *apply(wxWindow *control, JSContext *ctx, JSValue instance) {
            if (!control) {
                return control;
            }
            auto config = assistant::js::getValue(ctx, instance, "config");
            if (!JS_IsObject(config))
            {
                JS_FreeValue(ctx, config);
                return control;
            }

            auto widthInt = assistant::js::to_int(ctx, config, "width");
            auto heightInt = assistant::js::to_int(ctx, config, "height");
            if (widthInt.has_value() || heightInt.has_value()) {
                control->SetSize(
                        widthInt.value_or(control->GetSize().GetWidth()),
                        heightInt.value_or(control->GetSize().GetHeight())
                );
            }

            auto name = assistant::js::to_string(ctx, config, "name");
            if (name.has_value()) {
                control->SetName(wxString::FromUTF8(name.value()));
            }
            auto id = assistant::js::to_int(ctx, config, "id");
            if (id.has_value()) {
                control->SetId(id.value());
            }

            auto clientData = new JSValueClientData(ctx, instance);
            control->SetClientObject(clientData);

            // Устанавливаем адрес ссылки в объект, в служебное поле
            int64_t address = reinterpret_cast<int64_t>(control);
            JSValue ptr_value = JS_NewInt64(ctx, address);
            JS_SetPropertyStr(ctx, instance, "__wx_ptr", ptr_value);

            JS_FreeValue(ctx, config);
            return control;
        }

        /** Соединить событие из контрола, с функцией в JS */
        template <typename EventTag>
        void bindJsEvent(const EventTag& eventType, wxWindow *control, JSContext *ctx, JSValue instance, std::string eventName) {
            auto eventValue = assistant::js::to_function(ctx, instance, eventName);
            if (eventValue.has_value()) {
                JS_FreeValue(ctx, eventValue.value());
                control->Bind(eventType, [eventName](wxEvent& event) {
                    auto data = assistant::ui::getEventControlJsData(event);
                    if (data.has_value()) {
                        assistant::js::emitEvent(data.value(), eventName);
                    }
                    event.Skip();
                });
            }
        }
    }

    namespace checkBox {
        wxCheckBox *create(wxWindow* parent)
        {
            return new wxCheckBox(parent, wxID_ANY, "");
        }

        void apply(wxCheckBox *control, JSContext *ctx, JSValue instance) {
            auto config = assistant::js::getValue(ctx, instance, "config");
            if (!JS_IsObject(config))
            {
                JS_FreeValue(ctx, config);
                return;
            }

            auto label = assistant::js::to_string(ctx, config, "label").value_or("");
            control->SetLabel(wxString::FromUTF8(label));

            auto value = assistant::js::to_bool(ctx, config, "value").value_or(false);
            control->SetValue(value);
            render::base::bindJsEvent(wxEVT_CHECKBOX, control, ctx, instance, "changed");
            JS_FreeValue(ctx, config);
        }
    }

    namespace comboBox {
        wxComboBox *create(wxWindow *parent) {
            return new wxComboBox(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
        }

        void setItems(wxComboBox *control, JSContext *ctx, JSValue items) {
            prototypes::comboBox::set_items(control, ctx, items);
        }

        void setSelectedIndex(wxComboBox *control, JSContext *ctx, JSValue selectedIndexValue) {
            auto selectedIndex = assistant::js::to_int(ctx, selectedIndexValue).value_or(0);
            if (control->GetCount() > 0 && selectedIndex < control->GetCount()) {
                control->SetSelection(selectedIndex);
            }
        }

        void apply(wxComboBox *control, JSContext *ctx, JSValue instance) {
            if (!control) {
                return;
            }
            auto config = assistant::js::getValue(ctx, instance, "config");
            if (!JS_IsObject(config))
            {
                JS_FreeValue(ctx, config);
                return;
            }

            auto items = assistant::js::getValue(ctx, config, "items");
            render::comboBox::setItems(control, ctx, items);
            JS_FreeValue(ctx, items);

            auto selectedIndex = assistant::js::getValue(ctx, config, "selectedIndex");
            render::comboBox::setSelectedIndex(control, ctx, selectedIndex);
            JS_FreeValue(ctx, selectedIndex);

            render::base::bindJsEvent(wxEVT_COMBOBOX, control, ctx, instance, "changed");
            JS_FreeValue(ctx, config);
        }
    }

    namespace label {
        wxStaticText *create(wxWindow *parent) {
            return new wxStaticText(parent, wxID_ANY, "");
        }

        void apply(wxStaticText *label, JSContext *ctx, JSValue instance) {
            auto config = assistant::js::getValue(ctx, instance, "config");
            if (!JS_IsObject(config))
            {
                JS_FreeValue(ctx, config);
                return;
            }

            auto labelText = assistant::js::to_string(ctx, config, "label").value_or("");
            label->SetLabel(wxString::FromUTF8(labelText));
            JS_FreeValue(ctx, config);
        }
    }

    namespace text {
        wxTextCtrl *create(wxWindow *parent, bool isMultiline = false) {
            return !isMultiline
                   ? new wxTextCtrl(parent, wxID_ANY, "")
                   : new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxTE_WORDWRAP);
        }

        void apply(wxTextCtrl *textControl, JSContext *ctx, JSValue instance) {
            auto config = assistant::js::getValue(ctx, instance, "config");
            if (!JS_IsObject(config))
            {
                JS_FreeValue(ctx, config);
                return;
            }

            auto value = assistant::js::to_string(ctx, config, "value").value_or("");
            textControl->SetValue(wxString::FromUTF8(value));

            auto placeholder = assistant::js::to_string(ctx, config, "placeholder").value_or("");
            if (!placeholder.empty()) {
                textControl->SetValue(wxString::FromUTF8(placeholder));
            }

            render::base::bindJsEvent(wxEVT_TEXT, textControl, ctx, instance, "changed");
            JS_FreeValue(ctx, config);
        }
    }

    namespace button {
        wxButton *create(wxWindow *parent) {
            return new wxButton(parent, wxID_ANY);
        }

        void apply(wxButton *button, JSContext *ctx, JSValue instance) {
            auto config = assistant::js::getValue(ctx, instance, "config");
            if (!JS_IsObject(config))
            {
                JS_FreeValue(ctx, config);
                return;
            }

            auto label = assistant::js::to_string(ctx, config, "label").value_or("");
            button->SetLabel(wxString::FromUTF8(label));
            render::base::bindJsEvent(wxEVT_BUTTON, button, ctx, instance, "click");

            JS_FreeValue(ctx, config);
        }
    }

    namespace gridBag
    {
        int defaultGup = 5;
        void applyItems(wxGridBagSizer *grid, JSContext *ctx, JSValue items, wxWindow *parent);

        wxGridBagSizer *create(int gap = gridBag::defaultGup) {
            return new wxGridBagSizer(gap, gap);
        }

        void apply(wxGridBagSizer *grid, JSContext *ctx, JSValue instance, wxWindow *parent)
        {
            auto config = assistant::js::getValue(ctx, instance, "config");
            if (!JS_IsObject(config))
            {
                JS_FreeValue(ctx, config);
                return;
            }

            auto items = assistant::js::getValue(ctx, config, "items");
            render::gridBag::applyItems(grid, ctx, items, parent);
            JS_FreeValue(ctx, items);
            JS_FreeValue(ctx, config);
        }

        void applyItems(wxGridBagSizer *grid, JSContext *ctx, JSValue items, wxWindow *parent)
        {
            if (!JS_IsArray(ctx, items) || !grid)
            {
                return;
            }

            auto length = assistant::js::to_int(ctx, items, "length").value_or(0);
            for (int i = 0; i < length; i++)
            {
                JSValue item = JS_GetPropertyUint32(ctx, items, i);
                auto className = assistant::js::getClassName(ctx, item);
                auto config = assistant::js::getValue(ctx, item, "config");
                if (!JS_IsObject(config))
                {
                    JS_FreeValue(ctx, config);
                    continue;
                }

                wxWindow *control = nullptr;
                wxSizer *sizer = nullptr;

                auto row = assistant::js::to_int(ctx, config, "row").value_or(0);
                auto rowSpan = assistant::js::to_int(ctx, config, "rowSpan").value_or(1);
                auto column = assistant::js::to_int(ctx, config, "column").value_or(0);
                auto columnSpan = assistant::js::to_int(ctx, config, "columnSpan").value_or(1);

                auto widthString = assistant::js::to_string(ctx, config, "width");
                auto heightString = assistant::js::to_string(ctx, config, "height");

                if (className == "Button")
                {
                    auto button = render::button::create(parent);
                    control = render::base::apply(button, ctx, item);
                    render::button::apply(button, ctx, item);
                }
                if (className == "Label")
                {
                    auto label = render::label::create(parent);
                    control = render::base::apply(label, ctx, item);
                    render::label::apply(label, ctx, item);
                }
                if (className == "Text")
                {
                    auto multiline = assistant::js::to_bool(ctx, config, "multiline").value_or(false);
                    auto text = render::text::create(parent, multiline);
                    control = render::base::apply(text, ctx, item);
                    render::text::apply(text, ctx, item);
                }
                if (className == "CheckBox")
                {
                    auto checkBox = render::checkBox::create(parent);
                    control = render::base::apply(checkBox, ctx, item);
                    render::checkBox::apply(checkBox, ctx, item);
                }
                if (className == "ComboBox")
                {
                    auto comboBox = render::comboBox::create(parent);
                    control = render::base::apply(comboBox, ctx, item);
                    render::comboBox::apply(comboBox, ctx, item);
                }
                if (className == "GridBagLayout")
                {
                    auto gridBag = render::gridBag::create();
                    sizer = render::base::apply(gridBag, ctx, item);
                    render::gridBag::apply(gridBag, ctx, item, parent);
                }

                if (control)
                {
                    grid->Add(control, wxGBPosition(row, column), wxGBSpan(rowSpan, columnSpan), wxEXPAND | wxALL, 0);
                }
                else if (sizer)
                {
                    grid->Add(sizer, wxGBPosition(row, column), wxGBSpan(rowSpan, columnSpan), wxEXPAND | wxALL, 0);
                }
                else 
                {
                    JS_FreeValue(ctx, item);
                }
                JS_FreeValue(ctx, config);

                if (columnSpan == 1)
                {
                    auto growable = render::getGrowable(widthString);
                    if (growable.has_value())
                    {
                        if (!grid->IsColGrowable(column)) 
                        {
                            grid->AddGrowableCol(column, growable.value());
                        }
                    }
                }
                if (rowSpan == 1)
                {
                    auto growable = render::getGrowable(heightString);
                    if (growable.has_value())
                    {
                        if (!grid->IsRowGrowable(column)) 
                        {
                            grid->AddGrowableRow(row, growable.value());
                        }
                    }
                }
            }
        }
    }

    namespace frame
    {
        JsValue getTest(JSContext *ctx, JSValue value) {
            return JsValue(ctx, value);
        }

        int getTest(const JsValue& v) {
            return 0;
        }

        void apply(wxFrame *frame, JSContext *ctx, JSValue instance)
        {
            if (!JS_IsObject(instance) || !frame)
            {
                return;
            }

            {
                auto a = getTest(ctx, assistant::js::getValue(ctx, instance, "config"));
                auto a1 = getTest(ctx, assistant::js::getValue(ctx, instance, "config"));
                getTest(a);
            }
            auto config = assistant::js::getValue(ctx, instance, "config");
            if (!JS_IsObject(config))
            {
                JS_FreeValue(ctx, config);
                return;
            }

            auto title = assistant::js::to_string(ctx, config, "title");
            if (title.has_value())
            {
                frame->SetTitle(wxString::FromUTF8(title.value()));
            }

            auto name = assistant::js::to_string(ctx, config, "name");
            if (name.has_value())
            {
                frame->SetName(wxString::FromUTF8(name.value()));
            }

            auto width = assistant::js::to_int(ctx, config, "width");
            auto height = assistant::js::to_int(ctx, config, "height");
            if (width.has_value() || height.has_value())
            {
                auto widthValue = width.value_or(frame->GetSize().GetWidth());
                auto heightValue = height.value_or(frame->GetSize().GetHeight());
                frame->SetSize(widthValue, heightValue);
            }

            auto sizer = new wxFrameSizer(frame, render::gridBag::defaultGup);

            auto items = assistant::js::getValue(ctx, config, "items");
            render::gridBag::applyItems(sizer->getGridItems(), ctx, items, sizer->getPanel());
            JS_FreeValue(ctx, items);

            frame->Layout();
            JS_FreeValue(ctx, config);
        }
    }
}