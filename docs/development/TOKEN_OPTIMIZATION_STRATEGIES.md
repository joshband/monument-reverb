# Token Optimization Strategies

## Why Sessions Start at 30-40% Tokens Used

### Current Token Drain Sources

**1. CLAUDE.md Files (BIGGEST CULPRIT)**
Your project's `CLAUDE.md` is **~15,000+ tokens** loaded at EVERY session start:
- Multiple session summaries (Dec 31, Dec 9, Dec 5, Dec 4, Nov 19, Nov 18)
- Extensive documentation lists
- Detailed commit histories
- Previous session notes
- Strategic vision summaries

**2. Global CLAUDE.md (~2,000 tokens)**
- Token optimization reminders
- Session budget warnings
- Handoff procedures

**3. Git Status & Context (~3,000 tokens)**
- Recent commits
- Branch status
- Untracked files

**Total Session Start:** ~20,000 tokens = **10% of 200K budget BEFORE YOU TYPE**

When you add:
- System reminders (5K tokens)
- IDE context (3K tokens)
- Previous conversation history if not cleared (10K-50K tokens)

**You're at 30-40% before starting work.**

---

## Optimization Strategy

### 1. Trim CLAUDE.md Aggressively ✂️

**Current:** 15,000+ tokens (extensive history)
**Target:** 2,000-3,000 tokens (essentials only)

**What to Keep:**
```markdown
# Monument Reverb - Quick Start

## Current Session (REPLACE EACH SESSION)
**Date:** 2026-01-03
**Task:** LED ring implementation for enhanced knobs
**Files:** scripts/generate_knob_blender_enhanced.py
**Handoff:** NEXT_SESSION_HANDOFF.md

## Commands (From Project Root)
**Run Blender:** ./scripts/run_blender_enhanced.sh
**Preview:** python3 scripts/preview_knob_composite_enhanced.py
**Output:** assets/ui/knobs_enhanced/

## Project Essentials
- Current phase: Phase 4 UI enhancements
- Design references: docs/ui/design-references/
- Main doc: docs/ui/ENHANCED_UI_SUMMARY.md

## Previous Work (Last 3 Sessions Only)
- 2026-01-03: Enhanced geometry documentation
- 2025-12-31: W3C token audit
- 2025-12-09: Adapter pattern implementation
```

**What to MOVE to Separate Docs:**
- ❌ Full session summaries → `docs/session-history/`
- ❌ Architecture details → Already in `docs/architecture/`
- ❌ Test suite analysis → Already in `TEST_SUITE_WRAP_UP_2025_12_05.md`
- ❌ Multimodal vision → Already in `docs/architecture/MODULAR_TOKEN_PLATFORM_VISION.md`

**Savings:** 15,000 → 3,000 tokens = **12,000 tokens saved (6% of budget)**

---

### 2. Use Session Handoff Pattern

**Instead of:** Bloating CLAUDE.md with every session's details
**Do this:** Create dated handoff files

```bash
# At end of each session
NEXT_SESSION_HANDOFF_2026_01_03.md  # 500-1000 tokens
NEXT_SESSION_HANDOFF_2026_01_04.md  # New one next session
```

**Archive old handoffs:** Move to `docs/session-history/` after 1 week

---

### 3. Aggressive /clear Usage

**Current Pattern:** Keep context growing across multiple tasks
**Better Pattern:** Clear after EACH distinct task

```
✅ Implement LED ring → /clear
✅ Add concave cap → /clear
✅ Update documentation → /clear
```

**Why:** Conversation history accumulates fast (10K-50K tokens)

---

### 4. Use Haiku for Simple Tasks

**90% cheaper than Sonnet** for:
- File reads/searches
- Documentation updates
- Simple code changes
- Running tests

**Command:** "using haiku, update ENHANCED_UI_SUMMARY.md with LED ring details"

**Only use Sonnet when:**
- Complex architecture decisions
- Multi-file refactoring
- Debugging gnarly issues
- Strategic planning

---

### 5. Smart Context Loading

**Instead of:** Loading entire doc files into context
**Do this:** Reference by path, load only when needed

```
❌ "Here's all 10K lines of STRATEGIC_VISION_AND_ARCHITECTURE.md"
✅ "See docs/architecture/STRATEGIC_VISION_AND_ARCHITECTURE.md for details"
```

**Use Grep/Read strategically:**
```bash
# Find specific sections
grep "LED" docs/ui/ENHANCED_UI_SUMMARY.md

# Read only relevant parts
read docs/ui/ENHANCED_UI_SUMMARY.md --offset=100 --limit=50
```

---

### 6. Minimize Git Status Bloat

**Current:** Lists all untracked files every session
**Better:** Keep working directory clean

```bash
# Add to .gitignore
assets/ui/knobs_enhanced/*.png  # Generated assets
__pycache__/
.DS_Store
```

---

## Action Items for Next Session

### Immediate (Do Before Next Session)
1. **Trim CLAUDE.md:** Reduce from 15K to 3K tokens
   - Keep only: Current task, commands, last 3 sessions
   - Move rest to `docs/session-history/2025_12_31_w3c_audit.md`

2. **Save Reference Images Manually:**
   - Images uploaded in chat are NOT accessible via file system
   - Save them to: `docs/ui/design-references/vintage-panel-0[1-4].jpg`

3. **Clean Git Status:**
   - `git add docs/ui/ENHANCED_UI_SUMMARY.md`
   - `git add scripts/run_blender_enhanced.sh`
   - Update `.gitignore` for generated assets

### Workflow for Future Sessions

**Start of Session:**
1. Check `NEXT_SESSION_HANDOFF.md` for context (500 tokens)
2. Start working immediately (no need to load history)

**During Session:**
3. Use `/clear` after each distinct task
4. Create NEW handoff doc at end

**End of Session:**
5. Update `CLAUDE.md` with current task ONLY
6. Create `NEXT_SESSION_HANDOFF_YYYY_MM_DD.md`
7. Archive old handoff to `docs/session-history/`

---

## Estimated Token Savings

**Before Optimization:**
- Session start: 20K tokens (10%)
- After 3 tasks without /clear: 80K tokens (40%)
- **Usable budget: 120K tokens**

**After Optimization:**
- Session start: 5K tokens (2.5%)
- With aggressive /clear: 5K tokens per task
- **Usable budget: 180K tokens**

**Gain: 60K more usable tokens (30% more work per session)**

---

## Token Budget Alerts

**At 10K tokens remaining (~95% used):**
1. STOP current task
2. Create handoff document
3. Ask user: "Safe to /clear? Created handoff doc."
4. NEVER auto-compact (costs money)

**At 5% context left:**
1. Emergency stop
2. Create minimal handoff
3. Force /clear before continuing

---

## Cost Tracking

**Your Daily Budget:** $12/day (~200K tokens)
**Target:** $6/day (50% savings)

**Session Math:**
- Current: 4 sessions/day × 50K tokens = 200K tokens ($12)
- Optimized: 8 sessions/day × 25K tokens = 200K tokens ($12)
  - **BUT:** More tasks completed per session
  - **OR:** Stay at 4 sessions, use 100K total = $6/day ✅

**Handoff to Codex When:**
- Approaching $10/day (~160K tokens used)
- Simple/routine tasks remaining
- End of day (save budget for tomorrow)

---

## Quick Reference

**Lean CLAUDE.md Template:**
```markdown
# Project Name - Quick Start

## Current Task (REPLACE EACH SESSION)
- Date: YYYY-MM-DD
- Task: One-line description
- Handoff: NEXT_SESSION_HANDOFF.md

## Commands
[Essential commands only]

## Last 3 Sessions
[One line each]
```

**Handoff Template:**
```markdown
# Next Session Handoff - [Task Name]
**Date:** YYYY-MM-DD
**Estimated Time:** X minutes

## Quick Context
[2-3 sentences]

## Task
[Specific steps]

## Files
[List 3-5 files]

## Success Criteria
[Checklist]
```

---

**Remember:** Context is expensive. Be ruthless about what you load.
