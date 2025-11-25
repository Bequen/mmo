#pragma once

#define GET_REF(attribute, name) \
    inline const typeof(attribute)& name() const { return attribute; }

#define GET_MUT_REF(attribute, name) \
    GET_REF(attribute, name) \
    inline typeof(attribute)& name() { return attribute; }

#define GET(attribute, name) \
    inline typeof(attribute) name() const { return attribute; }
