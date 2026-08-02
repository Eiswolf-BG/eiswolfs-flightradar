#pragma once
#include <Arduino.h>
#include "aircraft.h"

namespace AirlineLookup {

    void init();
    void resolve(Aircraft& a);

    void clearCache();

}