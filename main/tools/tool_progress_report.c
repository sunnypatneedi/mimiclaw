#include "tools/tool_progress_report.h"
#include "mimi_config.h"
#include "memory/memory_store.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "tool_report";

esp_err_t tool_progress_report_execute(const char *input_json, char *output, size_t output_size)
{
    int days = 7;

    cJSON *root = cJSON_Parse(input_json);
    if (root) {
        cJSON *days_j = cJSON_GetObjectItem(root, "days");
        if (days_j && cJSON_IsNumber(days_j)) {
            days = (int)days_j->valuedouble;
            if (days < 1) days = 1;
            if (days > 30) days = 30;
        }
        cJSON_Delete(root);
    }

    /* Read long-term memory */
    char mem_buf[4096];
    esp_err_t err = memory_read_long_term(mem_buf, sizeof(mem_buf));
    bool has_memory = (err == ESP_OK && mem_buf[0] != '\0');

    /* Read recent daily notes */
    char recent_buf[4096];
    err = memory_read_recent(recent_buf, sizeof(recent_buf), days);
    bool has_recent = (err == ESP_OK && recent_buf[0] != '\0');

    if (!has_memory && !has_recent) {
        snprintf(output, output_size,
            "No learning data available yet. Start a tutoring session first!\n"
            "The progress report will be available after the student completes some practice.");
        return ESP_OK;
    }

    /* Get current date */
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);
    char date_str[16];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", &local);

    /* Build a structured context for the agent to compose the report.
     * The agent uses this data to generate a parent-friendly summary. */
    size_t off = 0;

    off += snprintf(output + off, output_size - off,
        "PROGRESS REPORT DATA (for agent to compose parent-friendly summary)\n"
        "Report date: %s\n"
        "Period: last %d days\n"
        "\n",
        date_str, days);

    if (has_memory) {
        off += snprintf(output + off, output_size - off,
            "=== LONG-TERM MEMORY ===\n%s\n\n", mem_buf);
    }

    if (has_recent) {
        off += snprintf(output + off, output_size - off,
            "=== RECENT DAILY NOTES (%d days) ===\n%s\n\n", days, recent_buf);
    }

    off += snprintf(output + off, output_size - off,
        "=== INSTRUCTIONS ===\n"
        "Compose a warm, encouraging message for the PARENT. Include:\n"
        "1. What the child has been working on\n"
        "2. Concepts mastered (retention >= 0.7)\n"
        "3. Areas that need more practice (retention < 0.5)\n"
        "4. Number of sessions and total engagement\n"
        "5. What's coming up next (scheduled reviews)\n"
        "6. One specific encouraging observation\n"
        "\n"
        "Keep it concise (under 20 lines). Use simple language.\n"
        "Start with the child's name. End with encouragement.\n");

    ESP_LOGI(TAG, "Progress report generated: %d bytes of context", (int)off);
    return ESP_OK;
}
