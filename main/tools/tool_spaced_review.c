#include "tools/tool_spaced_review.h"
#include "mimi_config.h"
#include "memory/memory_store.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "tool_sr";

/* Parse a YYYY-MM-DD date string to time_t (midnight local) */
static time_t parse_date(const char *date_str)
{
    struct tm t;
    memset(&t, 0, sizeof(t));
    if (sscanf(date_str, "%d-%d-%d", &t.tm_year, &t.tm_mon, &t.tm_mday) != 3) {
        return 0;
    }
    t.tm_year -= 1900;
    t.tm_mon -= 1;
    return mktime(&t);
}

/* Trim leading whitespace in-place, return pointer to first non-space char */
static const char *skip_ws(const char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

/* Trim trailing whitespace/newline from a mutable string */
static void trim_trailing(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' || s[len - 1] == ' ')) {
        s[--len] = '\0';
    }
}

/* A concept entry parsed from MEMORY.md */
typedef struct {
    char name[64];
    char next_review[16];   /* YYYY-MM-DD */
    float retention;
    int review_count;
    int days_overdue;       /* computed: positive = overdue */
} concept_entry_t;

#define MAX_CONCEPTS 32

esp_err_t tool_spaced_review_execute(const char *input_json, char *output, size_t output_size)
{
    (void)input_json;

    time_t now = time(NULL);
    if (now < 100000) {
        snprintf(output, output_size,
            "Error: system clock not set. Use get_current_time first.");
        return ESP_OK;
    }

    struct tm now_tm;
    localtime_r(&now, &now_tm);
    char today[16];
    strftime(today, sizeof(today), "%Y-%m-%d", &now_tm);
    time_t today_epoch = parse_date(today);

    /* Open MEMORY.md directly instead of reading into a buffer and using strtok.
     * This avoids mutation and handles arbitrary content gracefully. */
    FILE *f = fopen(MIMI_MEMORY_FILE, "r");
    if (!f) {
        snprintf(output, output_size,
            "No concepts tracked yet. Start a tutoring session to build the student's concept map.");
        return ESP_OK;
    }

    concept_entry_t concepts[MAX_CONCEPTS];
    int concept_count = 0;
    char line[256];

    while (fgets(line, sizeof(line), f) && concept_count < MAX_CONCEPTS) {
        trim_trailing(line);
        const char *p = skip_ws(line);

        /* Look for "## Concept: <name>" headers */
        if (strncmp(p, "## Concept:", 11) != 0) continue;

        concept_entry_t *c = &concepts[concept_count];
        memset(c, 0, sizeof(*c));
        c->retention = 0.5f;

        /* Extract concept name (skip "## Concept: " prefix) */
        const char *name_start = skip_ws(p + 11);
        strncpy(c->name, name_start, sizeof(c->name) - 1);

        /* Read subsequent metadata lines until next header or EOF */
        while (fgets(line, sizeof(line), f)) {
            trim_trailing(line);
            p = skip_ws(line);

            /* Stop at next section header or empty line after metadata */
            if (p[0] == '#') break;
            if (p[0] == '\0') {
                /* Allow one blank line between metadata fields, but stop at
                 * two consecutive blank lines (end of section) */
                long pos = ftell(f);
                if (fgets(line, sizeof(line), f)) {
                    trim_trailing(line);
                    const char *next = skip_ws(line);
                    if (next[0] == '\0' || next[0] == '#') {
                        /* Seek back so outer loop sees the header */
                        fseek(f, pos, SEEK_SET);
                        break;
                    }
                    p = next;
                } else {
                    break;
                }
            }

            /* Parse known metadata fields (case-insensitive prefix match) */
            if (strncasecmp(p, "Next review:", 12) == 0) {
                const char *val = skip_ws(p + 12);
                strncpy(c->next_review, val, sizeof(c->next_review) - 1);
            } else if (strncasecmp(p, "Retention score:", 16) == 0) {
                c->retention = strtof(skip_ws(p + 16), NULL);
            } else if (strncasecmp(p, "Retention:", 10) == 0) {
                c->retention = strtof(skip_ws(p + 10), NULL);
            } else if (strncasecmp(p, "Review count:", 13) == 0) {
                c->review_count = atoi(skip_ws(p + 13));
            }
        }

        /* Calculate days overdue */
        if (c->next_review[0] && today_epoch > 0) {
            time_t review_date = parse_date(c->next_review);
            if (review_date > 0) {
                c->days_overdue = (int)((today_epoch - review_date) / (24 * 3600));
            }
        }

        concept_count++;
    }

    fclose(f);

    if (concept_count == 0) {
        snprintf(output, output_size,
            "No concept entries found in MEMORY.md. "
            "Concepts are tracked with format: ## Concept: [name]");
        return ESP_OK;
    }

    /* Sort by priority: most overdue first, then lowest retention */
    for (int i = 0; i < concept_count - 1; i++) {
        for (int j = i + 1; j < concept_count; j++) {
            bool swap = false;
            if (concepts[j].days_overdue > concepts[i].days_overdue) {
                swap = true;
            } else if (concepts[j].days_overdue == concepts[i].days_overdue &&
                       concepts[j].retention < concepts[i].retention) {
                swap = true;
            }
            if (swap) {
                concept_entry_t tmp = concepts[i];
                concepts[i] = concepts[j];
                concepts[j] = tmp;
            }
        }
    }

    /* Build output */
    size_t off = 0;
    int overdue_count = 0;
    for (int i = 0; i < concept_count; i++) {
        if (concepts[i].days_overdue > 0) overdue_count++;
    }

    off += snprintf(output + off, output_size - off,
        "Spaced Review Report (today: %s)\n"
        "Total concepts tracked: %d\n"
        "Concepts due for review: %d\n\n",
        today, concept_count, overdue_count);

    if (overdue_count > 0) {
        off += snprintf(output + off, output_size - off, "OVERDUE (review now):\n");
        for (int i = 0; i < concept_count && off < output_size - 1; i++) {
            if (concepts[i].days_overdue <= 0) continue;
            off += snprintf(output + off, output_size - off,
                "  - %s: %d day(s) overdue, retention=%.2f, reviews=%d\n",
                concepts[i].name, concepts[i].days_overdue,
                concepts[i].retention, concepts[i].review_count);
        }
    }

    /* Also show upcoming reviews */
    off += snprintf(output + off, output_size - off, "\nUPCOMING:\n");
    for (int i = 0; i < concept_count && off < output_size - 1; i++) {
        if (concepts[i].days_overdue > 0) continue;
        off += snprintf(output + off, output_size - off,
            "  - %s: due in %d day(s), retention=%.2f\n",
            concepts[i].name, -concepts[i].days_overdue, concepts[i].retention);
    }

    ESP_LOGI(TAG, "Spaced review: %d concepts, %d overdue", concept_count, overdue_count);
    return ESP_OK;
}
