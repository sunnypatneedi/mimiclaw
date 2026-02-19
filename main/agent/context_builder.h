#pragma once

#include "esp_err.h"
#include <stddef.h>

/**
 * Build the system prompt from bootstrap files (SOUL.md, USER.md)
 * and memory context (MEMORY.md + recent daily notes).
 *
 * @param buf   Output buffer (caller allocates, recommend MIMI_CONTEXT_BUF_SIZE)
 * @param size  Buffer size
 */
esp_err_t context_build_system_prompt(char *buf, size_t size);

/**
 * Build the system prompt with an active skill file injected.
 * When session_type is MIMI_SESSION_TUTORING, the full skill file
 * content is loaded into the prompt for richer pedagogical context.
 *
 * @param buf           Output buffer
 * @param size          Buffer size
 * @param session_type  MIMI_SESSION_CASUAL or MIMI_SESSION_TUTORING
 * @param skill_path    Path to active skill file (e.g. "/spiffs/skills/math-multiplication.md"), or NULL
 */
esp_err_t context_build_system_prompt_ex(char *buf, size_t size,
                                          uint8_t session_type,
                                          const char *skill_path);

/**
 * Build the complete messages JSON array for LLM call.
 * Combines session history + current user message.
 *
 * @param history_json    JSON array from session_get_history_json()
 * @param user_message    Current user message text
 * @param buf             Output buffer
 * @param size            Buffer size
 */
esp_err_t context_build_messages(const char *history_json, const char *user_message,
                                 char *buf, size_t size);
