#include "FieldEnum.hpp"

const char* field_to_text(const FieldEnum& index) {
    static const char* names[] = {
        #define FIELD_DECL( name_text, resolver_, provider_, mask_, min_, max_ ) #name_text,
        #include "Field.enum"
        #undef FIELD_DECL
    };
    if (index < 0) {
        return "FIELD_INDEX_NEGATIVE";
    }
    if (index >= FIELD_COUNT) {
        return "FIELD_INDEX_OUT_OF_BOUNDS";
    }
    return names[index];
}
