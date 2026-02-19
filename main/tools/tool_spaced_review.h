#pragma once

#include "esp_err.h"
#include <stddef.h>

/**
 * Check which concepts are due for review based on FSRS-like scheduling.
 * Reads concept entries from MEMORY.md, calculates next review dates,
 * and returns overdue items sorted by priority.
 *
 * Input JSON: {} (no required fields)
 *
 * Returns a list of overdue concepts with days overdue and retention scores.
 */
esp_err_t tool_spaced_review_execute(const char *input_json, char *output, size_t output_size);
