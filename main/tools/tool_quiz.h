#pragma once

#include "esp_err.h"
#include <stddef.h>

/**
 * Generate a quiz for the current topic. Reads the active skill file
 * and produces formatted questions with A/B/C/D options.
 *
 * Input JSON: {
 *   "topic": "multiplication-7x",
 *   "difficulty": "easy"|"medium"|"hard",
 *   "num_questions": 3
 * }
 *
 * Returns formatted quiz text. Results should be scored by the agent
 * and saved to MEMORY.md via write_file/edit_file.
 */
esp_err_t tool_quiz_execute(const char *input_json, char *output, size_t output_size);
