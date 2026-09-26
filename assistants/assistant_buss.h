#pragma once

#include <iostream>
#include <string>
#include "Data/ObservableValue.h"

namespace assistant
{
    namespace buss
    {
        /** Счетчик выполнения асинхронных операций */
        ObservableValue<int>* asyncCounter = new ObservableValue<int>(false);
    }
}