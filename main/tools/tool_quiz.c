#include "tools/tool_quiz.h"
#include "mimi_config.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "tool_quiz";

esp_err_t tool_quiz_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json);
    if (!root) {
        snprintf(output, output_size, "Error: invalid JSON input");
        return ESP_ERR_INVALID_ARG;
    }

    const char *topic = cJSON_GetStringValue(cJSON_GetObjectItem(root, "topic"));
    const char *difficulty = cJSON_GetStringValue(cJSON_GetObjectItem(root, "difficulty"));
    cJSON *num_q = cJSON_GetObjectItem(root, "num_questions");

    if (!topic) {
        snprintf(output, output_size, "Error: missing 'topic' field");
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    int questions = (num_q && cJSON_IsNumber(num_q)) ? (int)num_q->valuedouble : 3;
    if (questions < 1) questions = 1;
    if (questions > 10) questions = 10;

    if (!difficulty) difficulty = "medium";

    /* Get current time for the quiz record */
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);
    char date_str[16];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", &local);
    char time_str[16];
    strftime(time_str, sizeof(time_str), "%H:%M", &local);

    /* Build quiz prompt for the agent to generate questions.
     * The tool doesn't generate questions itself — it structures the request
     * and provides metadata. The LLM generates the actual content. */
    snprintf(output, output_size,
        "QUIZ SESSION STARTED\n"
        "Topic: %s\n"
        "Difficulty: %s\n"
        "Questions: %d\n"
        "Date: %s\n"
        "Time: %s\n"
        "\n"
        "Instructions for the agent:\n"
        "1. Read the skill file for '%s' from /spiffs/skills/ to get question templates.\n"
        "2. Generate %d questions at '%s' difficulty.\n"
        "3. Format each question with A/B/C/D options (parent reads aloud).\n"
        "4. Wait for the answer to each question before moving to the next.\n"
        "5. After all questions, calculate the score.\n"
        "6. Save results to MEMORY.md using this format:\n"
        "   ## Quiz: %s (%s)\n"
        "   Score: X/%d\n"
        "   Retention: [calculated from score]\n"
        "   Missed: [list concepts missed]\n"
        "7. Update the concept's spaced repetition schedule based on the score.\n",
        topic, difficulty, questions, date_str, time_str,
        topic, questions, difficulty,
        topic, date_str, questions);

    cJSON_Delete(root);
    ESP_LOGI(TAG, "Quiz started: topic=%s difficulty=%s questions=%d", topic, difficulty, questions);
    return ESP_OK;
}
