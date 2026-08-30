#include "deki-nodegraph/editor/NodePropertyJson.h"

#include "deki-nodegraph/DekiNode.h"
#include "reflection/NodeRef.h"
#include "reflection/PropertyRef.h"
#include "assets/AssetRef.h"
#include "DekiLogSystem.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace DekiEditor
{

namespace
{
    using nlohmann::json;

    inline void* FieldPtr(void* base, const DekiPropertyInfo& p)
    {
        return static_cast<char*>(base) + p.offset;
    }
    inline const void* FieldPtr(const void* base, const DekiPropertyInfo& p)
    {
        return static_cast<const char*>(base) + p.offset;
    }

    // Enum values live in 1/2/4-byte storage (enumSize); move through int64.
    int64_t ReadEnumValue(const void* field, uint8_t enumSize)
    {
        switch (enumSize)
        {
            case 1: { uint8_t v;  std::memcpy(&v, field, 1); return v; }
            case 2: { uint16_t v; std::memcpy(&v, field, 2); return v; }
            default: { uint32_t v; std::memcpy(&v, field, 4); return v; }
        }
    }
    void WriteEnumValue(void* field, uint8_t enumSize, int64_t value)
    {
        switch (enumSize)
        {
            case 1: { uint8_t v = static_cast<uint8_t>(value);  std::memcpy(field, &v, 1); break; }
            case 2: { uint16_t v = static_cast<uint16_t>(value); std::memcpy(field, &v, 2); break; }
            default: { uint32_t v = static_cast<uint32_t>(value); std::memcpy(field, &v, 4); break; }
        }
    }

    bool IsSupportedType(const DekiPropertyInfo& p)
    {
        switch (p.type)
        {
            case DekiPropertyType::Int8:
            case DekiPropertyType::Int16:
            case DekiPropertyType::Int32:
            case DekiPropertyType::Int64:
            case DekiPropertyType::UInt8:
            case DekiPropertyType::UInt16:
            case DekiPropertyType::UInt32:
            case DekiPropertyType::UInt64:
            case DekiPropertyType::Float:
            case DekiPropertyType::Double:
            case DekiPropertyType::Bool:
            case DekiPropertyType::String:
            case DekiPropertyType::Enum:
            case DekiPropertyType::NodeRef:
            case DekiPropertyType::PropertyRef:
            case DekiPropertyType::AssetRef:
                return true;
            case DekiPropertyType::Array:
                return p.elementType == DekiPropertyType::Float ||
                       p.elementType == DekiPropertyType::Int32 ||
                       p.elementType == DekiPropertyType::String;
            default:
                return false;
        }
    }

    void LogUnsupported(const DekiNodeMeta& meta, const DekiPropertyInfo& p)
    {
        DEKI_LOG_ERROR("NodePropertyJson: node type '%s' property '%s' has unsupported "
                       "property type %d for node graphs",
                       meta.name ? meta.name : "?", p.name ? p.name : "?",
                       static_cast<int>(p.type));
    }
}

bool NodePropertiesToJson(const void* instance, const DekiNodeMeta& meta,
                          nlohmann::json& outValues)
{
    outValues = json::object();
    if (!instance)
        return false;

    for (int i = 0; i < meta.propertyCount; ++i)
    {
        const DekiPropertyInfo& p = meta.properties[i];
        if (!IsSupportedType(p))
        {
            LogUnsupported(meta, p);
            return false;
        }

        const void* field = FieldPtr(instance, p);
        switch (p.type)
        {
            case DekiPropertyType::Int8:   { int8_t v;   std::memcpy(&v, field, sizeof v); outValues[p.name] = v; break; }
            case DekiPropertyType::Int16:  { int16_t v;  std::memcpy(&v, field, sizeof v); outValues[p.name] = v; break; }
            case DekiPropertyType::Int32:  { int32_t v;  std::memcpy(&v, field, sizeof v); outValues[p.name] = v; break; }
            case DekiPropertyType::Int64:  { int64_t v;  std::memcpy(&v, field, sizeof v); outValues[p.name] = v; break; }
            case DekiPropertyType::UInt8:  { uint8_t v;  std::memcpy(&v, field, sizeof v); outValues[p.name] = v; break; }
            case DekiPropertyType::UInt16: { uint16_t v; std::memcpy(&v, field, sizeof v); outValues[p.name] = v; break; }
            case DekiPropertyType::UInt32: { uint32_t v; std::memcpy(&v, field, sizeof v); outValues[p.name] = v; break; }
            case DekiPropertyType::UInt64: { uint64_t v; std::memcpy(&v, field, sizeof v); outValues[p.name] = v; break; }
            case DekiPropertyType::Float:  { float v;    std::memcpy(&v, field, sizeof v); outValues[p.name] = v; break; }
            case DekiPropertyType::Double: { double v;   std::memcpy(&v, field, sizeof v); outValues[p.name] = v; break; }
            case DekiPropertyType::Bool:   { bool v;     std::memcpy(&v, field, sizeof v); outValues[p.name] = v; break; }
            case DekiPropertyType::String:
                outValues[p.name] = *static_cast<const std::string*>(field);
                break;
            case DekiPropertyType::Enum:
                outValues[p.name] = ReadEnumValue(field, p.enumSize);
                break;
            case DekiPropertyType::NodeRef:
                outValues[p.name] = static_cast<const NodeRef*>(field)->targetId;
                break;
            case DekiPropertyType::PropertyRef:
            {
                // Authored form only (object/component/field); the ids are
                // derived by Rehash on the way back in, never serialized.
                const auto* ref = static_cast<const PropertyRef*>(field);
                json obj = json::object();
                obj["object"] = ref->object;
                obj["component"] = ref->component;
                obj["field"] = ref->field;
                outValues[p.name] = std::move(obj);
                break;
            }
            case DekiPropertyType::AssetRef:
            {
                // Same {guid, source} shape components use.
                const auto* ref = static_cast<const Deki::AssetRefBase*>(field);
                json obj = json::object();
                obj["guid"] = ref->guid;
                obj["source"] = ref->source;
                outValues[p.name] = std::move(obj);
                break;
            }
            case DekiPropertyType::Array:
                switch (p.elementType)
                {
                    case DekiPropertyType::Float:
                        outValues[p.name] = *static_cast<const std::vector<float>*>(field);
                        break;
                    case DekiPropertyType::Int32:
                        outValues[p.name] = *static_cast<const std::vector<int32_t>*>(field);
                        break;
                    default:
                        outValues[p.name] = *static_cast<const std::vector<std::string>*>(field);
                        break;
                }
                break;
            default:
                break;   // unreachable (IsSupportedType)
        }
    }
    return true;
}

bool NodePropertiesFromJson(void* instance, const DekiNodeMeta& meta,
                            const nlohmann::json& values)
{
    if (!instance || !values.is_object())
        return false;

    for (int i = 0; i < meta.propertyCount; ++i)
    {
        const DekiPropertyInfo& p = meta.properties[i];
        auto it = values.find(p.name);
        if (it == values.end())
            continue;   // absent -> keep default

        if (!IsSupportedType(p))
        {
            LogUnsupported(meta, p);
            return false;
        }

        void* field = FieldPtr(instance, p);
        try
        {
            switch (p.type)
            {
                case DekiPropertyType::Int8:   { int8_t v = it->get<int8_t>();     std::memcpy(field, &v, sizeof v); break; }
                case DekiPropertyType::Int16:  { int16_t v = it->get<int16_t>();   std::memcpy(field, &v, sizeof v); break; }
                case DekiPropertyType::Int32:  { int32_t v = it->get<int32_t>();   std::memcpy(field, &v, sizeof v); break; }
                case DekiPropertyType::Int64:  { int64_t v = it->get<int64_t>();   std::memcpy(field, &v, sizeof v); break; }
                case DekiPropertyType::UInt8:  { uint8_t v = it->get<uint8_t>();   std::memcpy(field, &v, sizeof v); break; }
                case DekiPropertyType::UInt16: { uint16_t v = it->get<uint16_t>(); std::memcpy(field, &v, sizeof v); break; }
                case DekiPropertyType::UInt32: { uint32_t v = it->get<uint32_t>(); std::memcpy(field, &v, sizeof v); break; }
                case DekiPropertyType::UInt64: { uint64_t v = it->get<uint64_t>(); std::memcpy(field, &v, sizeof v); break; }
                case DekiPropertyType::Float:  { float v = it->get<float>();       std::memcpy(field, &v, sizeof v); break; }
                case DekiPropertyType::Double: { double v = it->get<double>();     std::memcpy(field, &v, sizeof v); break; }
                case DekiPropertyType::Bool:   { bool v = it->get<bool>();         std::memcpy(field, &v, sizeof v); break; }
                case DekiPropertyType::String:
                    *static_cast<std::string*>(field) = it->get<std::string>();
                    break;
                case DekiPropertyType::Enum:
                    WriteEnumValue(field, p.enumSize, it->get<int64_t>());
                    break;
                case DekiPropertyType::NodeRef:
                    static_cast<NodeRef*>(field)->targetId = it->get<uint32_t>();
                    break;
                case DekiPropertyType::PropertyRef:
                {
                    auto* ref = static_cast<PropertyRef*>(field);
                    ref->object    = it->value("object", std::string());
                    ref->component = it->value("component", std::string());
                    ref->field     = it->value("field", std::string());
                    ref->Rehash();
                    break;
                }
                case DekiPropertyType::AssetRef:
                {
                    auto* ref = static_cast<Deki::AssetRefBase*>(field);
                    ref->guid   = it->value("guid", std::string());
                    ref->source = it->value("source", std::string());
                    ref->ptr = nullptr;            // re-resolved on next Get()
                    ref->loadAttempted = false;
                    break;
                }
                case DekiPropertyType::Array:
                    switch (p.elementType)
                    {
                        case DekiPropertyType::Float:
                            *static_cast<std::vector<float>*>(field) = it->get<std::vector<float>>();
                            break;
                        case DekiPropertyType::Int32:
                            *static_cast<std::vector<int32_t>*>(field) = it->get<std::vector<int32_t>>();
                            break;
                        default:
                            *static_cast<std::vector<std::string>*>(field) = it->get<std::vector<std::string>>();
                            break;
                    }
                    break;
                default:
                    break;   // unreachable (IsSupportedType)
            }
        }
        catch (const json::exception& e)
        {
            DEKI_LOG_ERROR("NodePropertyJson: node type '%s' property '%s': bad JSON value (%s)",
                           meta.name ? meta.name : "?", p.name ? p.name : "?", e.what());
            return false;
        }
    }

    // Keys with no matching property: forward-compat skip, but say so.
    for (auto it = values.begin(); it != values.end(); ++it)
    {
        bool known = false;
        for (int i = 0; i < meta.propertyCount && !known; ++i)
            known = it.key() == meta.properties[i].name;
        if (!known)
            DEKI_LOG_WARNING("NodePropertyJson: node type '%s' has no property '%s' (skipped)",
                             meta.name ? meta.name : "?", it.key().c_str());
    }
    return true;
}

} // namespace DekiEditor
