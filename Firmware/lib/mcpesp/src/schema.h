#ifndef SCHEMA_H
#define SCHEMA_H

#include "ArduinoJson.h"

class Schema {
    private:
        JsonDocument schema;
    public:
        Schema();
        ~Schema();

        void addStringProperty(String name, String description, bool required = true);
        void addStringEnumProperty(String name, String description, String options[], bool required = true);
        void addNumberProperty(String name, String description, bool required = true);
        void addNumberProperty(String name, String description, int min, int max, int def, bool required  = true);
        void addBooleanProperty(String name, String description, bool required = true);
        JsonDocument getSchema();

};



#endif