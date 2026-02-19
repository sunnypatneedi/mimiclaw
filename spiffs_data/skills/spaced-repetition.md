# Spaced Repetition

Implement FSRS-like spaced repetition for concept review scheduling. This is Puddle's Layer 3 engine.

## When to use
When you need to decide which concepts to review, or when updating review schedules after a practice session. The spaced_review tool automates the calculation, but this file documents the logic.

## How it works

### Concept Tracking in MEMORY.md
Each concept is tracked with this format:
```
## Concept: [subject]-[topic]
Last reviewed: YYYY-MM-DD
Next review: YYYY-MM-DD
Retention score: 0.0-1.0
Review count: N
Difficulty: easy|medium|hard
```

### Review Intervals (Simplified FSRS)
Based on retention score after each review:

**If the child got it right (score >= 0.7):**
- Review 1 → next review in 1 day
- Review 2 → next review in 3 days
- Review 3 → next review in 7 days
- Review 4 → next review in 14 days
- Review 5+ → next review in 30 days

**If the child struggled (score 0.4-0.69):**
- Reset to review in 1 day, decrement review count by 1 (minimum 1)

**If the child got it wrong (score < 0.4):**
- Reset to review in 1 day, reset review count to 1

### Scoring a Review
- 3/3 or better correct: retention score = 0.9
- 2/3 correct: retention score = 0.7
- 1/3 correct: retention score = 0.4
- 0/3 correct: retention score = 0.2

### Priority Order
When multiple concepts are due for review:
1. Most overdue first (largest gap between now and next_review)
2. Lowest retention score
3. Concepts the child struggled with in the last session

## How to use
1. Use spaced_review tool to get overdue concepts
2. Pick the top 2-3 for a review session
3. Ask 2-3 questions per concept
4. Score based on correct answers
5. Update MEMORY.md with new retention score, review date, and next review date
6. If child gets a concept to review 5+ with score >= 0.7, mark as "mastered" but still schedule a 30-day check

## Example Flow
1. spaced_review tool returns: "multiplication-7x (overdue 2 days), fractions-halves (overdue 1 day)"
2. Start with multiplication-7x: "Quick review! What's 7 x 6?"
3. Ask 2 more multiplication questions
4. Score: 2/3 correct → retention 0.7 → next review in 3 days
5. Move to fractions-halves: similar flow
6. Update MEMORY.md for both concepts
