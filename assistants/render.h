#include <wx/wx.h>
#include <wx/gbsizer.h>
#include <wx/stattext.h>
#include "controls/wxFrameSizer.h"
#include "Data/JsValue.h"
#include "prototypes.h"

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
        wxSizer *apply(wxSizer *sizer, JSContext *ctx, JsValue* instance) {
            auto config = instance->getConfigValue();
            auto widthInt = config->getValue("width")->to_int();
            auto heightInt = config->getValue("height")->to_int();
            if (widthInt.has_value() || heightInt.has_value()) {
                sizer->SetMinSize(
                        widthInt.value_or(sizer->GetSize().GetWidth()),
                        heightInt.value_or(sizer->GetSize().GetHeight())
                );
            }

            auto dupInstance = instance->getRawDupValue();
            auto clientData = new JSValueClientData(ctx, dupInstance);
            sizer->SetClientObject(clientData);

            // Устанавливаем адрес ссылки в объект, в служебное поле
            int64_t address = reinterpret_cast<int64_t>(sizer);
            JSValue ptr_value = JS_NewInt64(ctx, address);
            JS_SetPropertyStr(ctx, dupInstance, "__wx_ptr", ptr_value);

            return sizer;
        }

        /**
         * К каждому контролу созданному в окне привязан JSValue. этот JSValue объект, с конфогурацией и от этой уонфигурации был создан контрол
         * Функция возвращает объект конфигурации
         * */
        std::unique_ptr<JsValue> getValue(wxWindow *control) {
            if (!control) {
                return std::make_unique<JsValue>(nullptr, JS_UNDEFINED);
            }

            wxClientData *clientDataVoid = control->GetClientObject();
            JSValueClientData *clientData = dynamic_cast<JSValueClientData *>(clientDataVoid);
            if (!clientData) {
                return std::make_unique<JsValue>(nullptr, JS_UNDEFINED);
            }

            return std::make_unique<JsValue>(clientData->getContext(), clientData->getValue(), true);
        }

        wxWindow *apply(wxWindow *control, JSContext *ctx, JsValue* instance) {
            if (!control) {
                return control;
            }

            auto config = instance->getConfigValue();

            auto widthInt = config->getValue("width")->to_int();
            auto heightInt = config->getValue("height")->to_int();
            if (widthInt.has_value() || heightInt.has_value()) {
                control->SetSize(
                        widthInt.value_or(control->GetSize().GetWidth()),
                        heightInt.value_or(control->GetSize().GetHeight())
                );
            }

            auto name = config->getValue("name")->to_string();
            if (name.has_value()) {
                control->SetName(wxString::FromUTF8(name.value()));
            }
            auto id = config->getValue("id")->to_int();
            if (id.has_value()) {
                control->SetId(id.value());
            }

            auto dupInstance = instance->getRawDupValue();
            auto clientData = new JSValueClientData(ctx, dupInstance);
            control->SetClientObject(clientData);

            // Устанавливаем адрес ссылки в объект, в служебное поле
            int64_t address = reinterpret_cast<int64_t>(control);
            JSValue ptr_value = JS_NewInt64(ctx, address);
            JS_SetPropertyStr(ctx, dupInstance, "__wx_ptr", ptr_value);

            return control;
        }

        /** Соединить событие из контрола, с функцией в JS */
        template <typename EventTag>
        void bindJsEvent(const EventTag& eventType, wxWindow *control, JsValue* instance, const std::string& eventName) {
            if (instance->getConfigValue()->getValue(eventName)->is_function()) {
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
        wxCheckBox *create(wxWindow *parent) {
            return new wxCheckBox(parent, wxID_ANY, "");
        }

        void apply(wxCheckBox *control, JSContext *ctx, JsValue* instance) {
            auto config = instance->getConfigValue();

            auto label = config->getValue("label")->to_string().value_or("");
            control->SetLabel(wxString::FromUTF8(label));

            auto value = config->getValue("value")->to_bool().value_or(false);
            control->SetValue(value);
            render::base::bindJsEvent(wxEVT_CHECKBOX, control, instance, "changed");
        }
    }

    namespace comboBox {
        wxComboBox *create(wxWindow *parent) {
            return new wxComboBox(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
        }

        void setItems(wxComboBox *control, JsValue* items) {
            prototypes::comboBox::set_items(control, items);
        }

        void setSelectedIndex(wxComboBox *control, JsValue* selectedIndexValue) {
            auto selectedIndex = selectedIndexValue->to_int().value_or(0);
            if (control->GetCount() > 0 && selectedIndex < control->GetCount()) {
                control->SetSelection(selectedIndex);
            }
        }

        void apply(wxComboBox *control, JSContext *ctx, JsValue* instance) {
            if (!control) {
                return;
            }
            auto config = instance->getConfigValue();

            auto items = config->getValue("items");
            render::comboBox::setItems(control, items.get());

            auto selectedIndex = config->getValue("selectedIndex");
            render::comboBox::setSelectedIndex(control, selectedIndex.get());

            render::base::bindJsEvent(wxEVT_COMBOBOX, control, instance, "changed");
        }
    }

    namespace label {
        wxStaticText *create(wxWindow *parent) {
            return new wxStaticText(parent, wxID_ANY, "");
        }

        void apply(wxStaticText *label, JSContext *ctx, JsValue* instance) {
            auto config = instance->getConfigValue();

            auto labelText = config->getValue("label")->to_string().value_or("");
            label->SetLabel(wxString::FromUTF8(labelText));
        }
    }

    namespace text {
        wxTextCtrl *create(wxWindow *parent, bool isMultiline = false) {
            return !isMultiline
                   ? new wxTextCtrl(parent, wxID_ANY, "")
                   : new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxTE_WORDWRAP);
        }

        void apply(wxTextCtrl *textControl, JSContext *ctx, JsValue* instance) {
            auto config = instance->getConfigValue();

            auto value = config->getValue("value")->to_string().value_or("");
            textControl->SetValue(wxString::FromUTF8(value));

            auto placeholder = config->getValue("placeholder")->to_string().value_or("");
            if (!placeholder.empty()) {
                textControl->SetValue(wxString::FromUTF8(placeholder));
            }

            render::base::bindJsEvent(wxEVT_TEXT, textControl, instance, "changed");
        }
    }

    namespace button {
        wxButton *create(wxWindow *parent) {
            return new wxButton(parent, wxID_ANY);
        }

        void apply(wxButton *button, JSContext *ctx, JsValue* instance) {
            auto config = instance->getConfigValue();

            auto label = config->getValue("label")->to_string().value_or("");
            button->SetLabel(wxString::FromUTF8(label));

            render::base::bindJsEvent(wxEVT_BUTTON, button, instance, "click");
        }
    }

    namespace gridBag
    {
        int defaultGup = 5;
        void applyItems(wxGridBagSizer *grid, JSContext *ctx, JsValue* items, wxWindow *parent);

        wxGridBagSizer *create(int gap = gridBag::defaultGup) {
            return new wxGridBagSizer(gap, gap);
        }

        void apply(wxGridBagSizer *grid, JSContext *ctx, JsValue* instance, wxWindow *parent)
        {
            auto config = instance->getConfigValue();
            auto items = config->getValue("items");

            render::gridBag::applyItems(grid, ctx, items.get(), parent);
        }

        void applyItems(wxGridBagSizer *grid, JSContext *ctx, JsValue *items, wxWindow *parent) {
            if (!items->isArray() || !grid) {
                return;
            }

            auto length = items->getLength();
            for (int i = 0; i < length; i++) {
                auto item = items->at(i);
                auto className = item->getClassName();
                wxWindow *control = nullptr;
                wxSizer *sizer = nullptr;

                auto config = item->getConfigValue();
                auto row = config->getValue("row")->to_int().value_or(0);
                auto rowSpan = config->getValue("rowSpan")->to_int().value_or(1);
                auto column = config->getValue("column")->to_int().value_or(0);
                auto columnSpan = config->getValue("columnSpan")->to_int().value_or(1);

                auto widthString = config->getValue("width")->to_string();
                auto heightString = config->getValue("height")->to_string();

                if (className == "Button") {
                    auto button = render::button::create(parent);
                    control = render::base::apply(button, ctx, item.get());
                    render::button::apply(button, ctx, item.get());
                }
                if (className == "Label") {
                    auto label = render::label::create(parent);
                    control = render::base::apply(label, ctx, item.get());
                    render::label::apply(label, ctx, item.get());
                }
                if (className == "Text") {
                    auto multiline = config->getValue("multiline")->to_bool().value_or(false);
                    auto text = render::text::create(parent, multiline);
                    control = render::base::apply(text, ctx, item.get());
                    render::text::apply(text, ctx, item.get());
                }
                if (className == "CheckBox") {
                    auto checkBox = render::checkBox::create(parent);
                    control = render::base::apply(checkBox, ctx, item.get());
                    render::checkBox::apply(checkBox, ctx, item.get());
                }
                if (className == "ComboBox") {
                    auto comboBox = render::comboBox::create(parent);
                    control = render::base::apply(comboBox, ctx, item.get());
                    render::comboBox::apply(comboBox, ctx, item.get());
                }
                if (className == "GridBagLayout") {
                    auto gridBag = render::gridBag::create();
                    sizer = render::base::apply(gridBag, ctx, item.get());
                    render::gridBag::apply(gridBag, ctx, item.get(), parent);
                }

                if (control) {
                    grid->Add(control, wxGBPosition(row, column), wxGBSpan(rowSpan, columnSpan), wxEXPAND | wxALL, 0);
                } else if (sizer) {
                    grid->Add(sizer, wxGBPosition(row, column), wxGBSpan(rowSpan, columnSpan), wxEXPAND | wxALL, 0);
                }

                if (columnSpan == 1) {
                    auto growable = render::getGrowable(widthString);
                    if (growable.has_value()) {
                        if (!grid->IsColGrowable(column)) {
                            grid->AddGrowableCol(column, growable.value());
                        }
                    }
                }
                if (rowSpan == 1) {
                    auto growable = render::getGrowable(heightString);
                    if (growable.has_value()) {
                        if (!grid->IsRowGrowable(column)) {
                            grid->AddGrowableRow(row, growable.value());
                        }
                    }
                }
            }
        }
    }

    namespace frame
    {
        void apply(wxFrame *frame, JSContext *ctx, JsValue* instance) {
            if (!frame) {
                return;
            }

            auto config = instance->getConfigValue();
            auto title = config->getValue("title")->to_string();
            if (title.has_value()) {
                frame->SetTitle(wxString::FromUTF8(title.value()));
            }

            auto name = config->getValue("name")->to_string();
            if (name.has_value()) {
                frame->SetName(wxString::FromUTF8(name.value()));
            }

            auto width = config->getValue("width")->to_int();
            auto height = config->getValue("height")->to_int();
            if (width.has_value() || height.has_value()) {
                auto widthValue = width.value_or(frame->GetSize().GetWidth());
                auto heightValue = height.value_or(frame->GetSize().GetHeight());
                frame->SetSize(widthValue, heightValue);
            }

            auto sizer = new wxFrameSizer(frame, render::gridBag::defaultGup);

            auto items = config->getValue("items");
            render::gridBag::applyItems(sizer->getGridItems(), ctx, items.get(), sizer->getPanel());

            frame->Layout();
        }
    }
}