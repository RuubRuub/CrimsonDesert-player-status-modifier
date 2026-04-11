#pragma once

#include "config.h"

bool InitializeDamageToggle();
bool ApplyDamageToggleConfig(const DamageToggleConfig& previous, const DamageToggleConfig& current);
void ShutdownDamageToggle();
