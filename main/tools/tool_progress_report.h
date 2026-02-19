#pragma once

#include "esp_err.h"
#include <stddef.h>

/**
 * Generate a parent-friendly progress report.
 * Reads MEMORY.md + last 7 days of daily notes and composes a summary
 * of mastered concepts, struggling areas, and upcoming review schedule.
 *
 * Input JSON: {
 *   "days": 7  (optional, default 7)
 * }
 *
 * Returns a formatted progress report text suitable for Telegram.
 */
esp_err_t tool_progress_report_execute(const char *input_json, char *output, size_t output_size);
