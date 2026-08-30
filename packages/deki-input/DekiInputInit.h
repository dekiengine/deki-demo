#pragma once

/**
 * @brief Initialize the input system (creates DekiInputSystem, registers with engine).
 *
 * Called from deki_init_package_systems() on firmware builds,
 * and from DekiInput_EnsureRegistered() on editor builds.
 *
 * Idempotent — safe to call multiple times.
 */
void DekiInput_InitSystem();
void DekiInput_ShutdownSystem();
