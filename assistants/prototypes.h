#pragma once
#include <wx/wx.h>

namespace prototypes
{
    namespace text 
    {
        template<typename T>
        T *getInstance(JSContext *ctx, JSValue value) {
            auto address = assistant::js::getJsValue(ctx, value, "__wx_ptr")->to_int_64();
            if (!address.has_value()) {
                return nullptr;
            }
            T *control = reinterpret_cast<T *>(address.value());
            return control;
        }

        JSValue get_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto text = prototypes::text::getInstance<wxTextCtrl>(ctx, this_val);
            if (text) {
                return assistant::js::to_value(ctx, text->GetValue().ToStdString(wxConvUTF8));
            }

            return assistant::js::getUndefined();
        }

        JSValue set_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto text = prototypes::text::getInstance<wxTextCtrl>(ctx, this_val);
            if (text) {
                auto value = GET_VALUE(0);
                text->SetValue(wxString::FromUTF8(value->to_string().value_or("")));
            }
            return assistant::js::getUndefined();
        }
    }

    namespace checkBox {
        /** Поулчить значение */
        JSValue get_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto control = prototypes::text::getInstance<wxCheckBox>(ctx, this_val);
            if (control) {
                return assistant::js::to_value(ctx, control->GetValue());
            }
            return assistant::js::getUndefined();
        }

        /** Получить подсказку */
        JSValue get_label(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto control = prototypes::text::getInstance<wxCheckBox>(ctx, this_val);
            if (control) {
                return assistant::js::to_value(ctx, control->GetLabel().ToStdString(wxConvUTF8));
            }
            return assistant::js::getUndefined();
        }

        /** Установить значение  */
        JSValue set_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto control = prototypes::text::getInstance<wxCheckBox>(ctx, this_val);
            if (control) {
                auto value = GET_VALUE(0);
                control->SetValue(value->to_bool().value_or(false));
            }
            return assistant::js::getUndefined();
        }

        /** Установить подсказку */
        JSValue set_label(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto control = prototypes::text::getInstance<wxCheckBox>(ctx, this_val);
            if (control) {
                auto value = GET_VALUE(0);
                control->SetLabel(value->to_string().value_or(""));
            }

            return assistant::js::getUndefined();
        }
    }

    namespace comboBox
    {
        /** Установить элементы в выпадающий список */
        void set_items(wxComboBox *control, JsValue* items) {
            if (control) {
                control->Clear();
                if (items->isArray()) {
                    auto length = items->getLength();
                    for (int i = 0; i < length; i++) {
                        auto item = items->at(i);
                        auto text = item->getValue("text")->to_string().value_or("");
                        auto value = item->getValue("value")->to_string().value_or("");
                        control->Append(wxString::FromUTF8(text.c_str()), new wxStringClientData(wxString::FromUTF8(value.c_str())));
                    }
                }
            }
        }

        /** Установить элементы в выпадающий список */
        JSValue set_items(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto control = prototypes::text::getInstance<wxComboBox>(ctx, this_val);
            if (control) {
                prototypes::comboBox::set_items(control, GET_VALUE(0).get());
            }

            return assistant::js::getUndefined();
        }

        /** Вернуть выбранное значение в списке */
        JSValue get_selected_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto control = prototypes::text::getInstance<wxComboBox>(ctx, this_val);
            if (!control) {
                return assistant::js::getUndefined();
            }
            int selectedIndex = control->GetSelection();

            if (selectedIndex != wxNOT_FOUND)
            {
                wxClientData* rawData = control->GetClientObject(selectedIndex);

                if (rawData)
                {
                    wxStringClientData* stringData = dynamic_cast<wxStringClientData*>(rawData);
                    if (stringData)
                    {
                        std::string value = stringData->GetData().ToStdString();
                        return assistant::js::to_value(ctx, value);
                    }
                }
            }

            return assistant::js::getUndefined();
        }

        /** Установить значение в список */
        JSValue set_selected_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto control = prototypes::text::getInstance<wxComboBox>(ctx, this_val);
            if (!control) {
                return assistant::js::getUndefined();
            }
            if (argc < 1) {
                return assistant::js::getUndefined();
            }

            auto value = GET_VALUE(0)->to_string();
            if (!value.has_value()) {
                return assistant::js::getUndefined();
            }
            auto count = control->GetCount();
            for (int i = 0; i < count; i++) {
                wxClientData *rawData = control->GetClientObject(i);
                if (rawData) {
                    wxStringClientData *stringData = dynamic_cast<wxStringClientData *>(rawData);
                    if (stringData) {
                        std::string value_index = stringData->GetData().ToStdString();
                        if (value_index == value.value()) {
                            control->SetSelection(i);
                            return assistant::js::getUndefined();
                        }
                    }
                }
            }

            return assistant::js::getUndefined();
        }

        void registration (JSContext *ctx) {
            assistant::js::registerPrototype(ctx, "ComboBox", "setItems", prototypes::comboBox::set_items);
            assistant::js::registerPrototype(ctx, "ComboBox", "getSelectedValue", prototypes::comboBox::get_selected_value);
            assistant::js::registerPrototype(ctx, "ComboBox", "setSelectedValue", prototypes::comboBox::set_selected_value);
        }
    }
}