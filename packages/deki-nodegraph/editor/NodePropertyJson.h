#pragma once

/**
 * @file NodePropertyJson.h
 * @brief JSON <-> node-instance property conversion, driven by DekiPropertyInfo.
 *
 * Node instances are plain reflected structs behind void* (NodeFactory-created,
 * no common base), so PropertyEditorFactory (DekiComponent*-typed) cannot be
 * reused. This mirrors the scene component JSON switch for the node-relevant
 * subset. Encodings match what the generated DeserializeMsgPack hash-switch
 * expects after the generic json::to_msgpack transcode: numbers for ints and
 * enums, JSON arrays for vectors, plain strings/bools.
 *
 * Unsupported property types on a node (Color, Vector2/3, Asset/AssetRef,
 * ObjectRef, Array of serializables) are a hard error: log naming the node
 * type + property, return false, caller aborts the operation. No silent skips.
 */

#include <nlohmann/json.hpp>

struct DekiNodeMeta;

namespace DekiEditor
{

// Write every reflected property of `instance` into `outValues` (an object).
bool NodePropertiesToJson(const void* instance, const DekiNodeMeta& meta,
                          nlohmann::json& outValues);

// Apply `values` onto `instance`. Properties absent from `values` keep their
// defaults; keys with no matching property are logged and skipped (forward
// compatibility, same policy as the msgpack reader).
bool NodePropertiesFromJson(void* instance, const DekiNodeMeta& meta,
                            const nlohmann::json& values);

} // namespace DekiEditor
