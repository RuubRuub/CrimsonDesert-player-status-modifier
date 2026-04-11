#include "damage_toggle.h"

#include "config.h"
#include "key_listener.h"
#include "logger.h"

#include <atomic>

namespace {

constexpr DWORD kDamageTogglePollMs = 10;

KeyListener g_damage_toggle_listener{};
std::atomic<bool> g_damage_toggle_started{false};

void ToggleOutgoingDamage() {
    const ModConfig current = GetConfig();
    ModConfig next = current;
    next.damage.outgoing.enabled = !current.damage.outgoing.enabled;
    SetConfigSnapshot(GetLoadedConfigPath(), next);
    Log("damage-toggle: outgoing damage %s", next.damage.outgoing.enabled ? "enabled" : "disabled");
}

bool StartDamageToggleListener(const DamageToggleConfig& config) {
    const bool started = g_damage_toggle_listener.Start(
        config.key,
        kDamageTogglePollMs,
        {},
        [](const bool is_down) {
            if (is_down) {
                ToggleOutgoingDamage();
            }
        });
    if (!started) {
        Log("damage-toggle: failed to start toggle key listener");
        return false;
    }

    g_damage_toggle_started.store(true, std::memory_order_release);
    Log("damage-toggle: enabled key=0x%X", static_cast<unsigned>(config.key));
    return true;
}

void StopDamageToggleListener() {
    if (g_damage_toggle_started.exchange(false, std::memory_order_acq_rel)) {
        g_damage_toggle_listener.Stop();
        Log("damage-toggle: toggle key listener stopped");
    }
}

bool ReconfigureDamageToggleListener(const DamageToggleConfig& previous, const DamageToggleConfig& current) {
    const bool changed = previous.enabled != current.enabled ||
                         (previous.enabled && current.enabled && previous.key != current.key);
    if (!changed) {
        return true;
    }

    const bool had_previous_listener = previous.enabled && g_damage_toggle_started.load(std::memory_order_acquire);
    if (had_previous_listener) {
        StopDamageToggleListener();
    }

    if (!current.enabled) {
        return true;
    }

    if (StartDamageToggleListener(current)) {
        return true;
    }

    if (had_previous_listener) {
        StartDamageToggleListener(previous);
    }
    return false;
}

}  // namespace

bool InitializeDamageToggle() {
    return ApplyDamageToggleConfig(DamageToggleConfig{}, GetConfig().damage_toggle);
}

bool ApplyDamageToggleConfig(const DamageToggleConfig& previous, const DamageToggleConfig& current) {
    return ReconfigureDamageToggleListener(previous, current);
}

void ShutdownDamageToggle() {
    StopDamageToggleListener();
}
