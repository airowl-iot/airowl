#include "schema.h"


Schema::Schema()
{
    schema["type"] = "object";
    schema.createNestedObject("properties");
    schema.createNestedArray("required");
}

Schema::~Schema()
{

}

void Schema::addStringProperty(String name, String description, bool required)
{
    schema["properties"][name.c_str()]["type"] = "string";
    schema["properties"][name.c_str()]["description"] = description.c_str();
    if (required) {
        schema["required"].add(name.c_str());
    }
}

void Schema::addStringEnumProperty(String name, String description, String options[], bool required)
{
    schema["properties"][name.c_str()]["type"] = "string";
    schema["properties"][name.c_str()]["description"] = description.c_str();
    JsonArray enumArray = schema["properties"][name.c_str()]["enum"].to<JsonArray>();
    for (int i = 0; i < sizeof(options)/sizeof(options[0]); i++) {
        enumArray.add(options[i].c_str());
    }
    if (required) {
        schema["required"].add(name.c_str());
    }
}

void Schema::addNumberProperty(String name, String description, bool required)
{
    schema["properties"][name.c_str()]["type"] = "number";
    schema["properties"][name.c_str()]["description"] = description.c_str();
    if (required) {
        schema["required"].add(name.c_str());
    }
}

void Schema::addNumberProperty(String name, String description, int min, int max, int def, bool required)
{
    schema["properties"][name.c_str()]["type"] = "number";
    schema["properties"][name.c_str()]["description"] = description.c_str();
    schema["properties"][name.c_str()]["minimum"] = min;
    schema["properties"][name.c_str()]["maximum"] = max;
    schema["properties"][name.c_str()]["default"] = def;
    if (required) {
        schema["required"].add(name.c_str());
    }
}

void Schema::addBooleanProperty(String name, String description, bool required)
{
    schema["properties"][name.c_str()]["type"] = "boolean";
    schema["properties"][name.c_str()]["description"] = description.c_str();
    if (required) {
        schema["required"].add(name.c_str());
    }
}


JsonDocument Schema::getSchema()
{
    schema["type"] = "object";
	return schema;
}