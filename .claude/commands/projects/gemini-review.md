---
allowed-tools: Bash(gh api:*), Bash(gh pr list:*), Bash(gh pr view:*), Bash(gh pr comment:*), Bash(gh pr create:*), Bash(gh repo view:*), Bash(git rev-parse:*), Bash(git status:*), Bash(git diff:*), Bash(git commit:*), Bash(git push:*), Bash(git add:*), Read, Write, Edit, Update
argument-hint: [optional-pr-number]
description: Analyze Gemini Code Assistant's PR review comments and implement fixes
---

# Gemini Review Analysis and Implementation

**MANDATORY EXECUTION SEQUENCE**: You MUST complete ALL steps below in order. Do NOT skip implementation or commit steps.

<thinking>
This is a critical workflow that requires careful execution of each step. I need to ensure Gemini reviews are properly retrieved, analyzed, implemented, and committed.
</thinking>

## Context Information

**First, gather current repository context:**

1. Current branch: Execute `git rev-parse --abbrev-ref HEAD`
2. Current git status: Execute `git status --porcelain`
3. Repository info: Execute `gh repo view --json owner,name`

## MANDATORY EXECUTION STEPS

### STEP 1: Find Current PR (REQUIRED)
Execute these commands to locate the PR:
```bash
# Find PR for current branch
gh pr list --head $(git rev-parse --abbrev-ref HEAD) --json number,title,url
```

If no PR exists, create one:
```bash
gh pr create --title "Auto-generated PR for $(git rev-parse --abbrev-ref HEAD)" --body "Automated PR for review"
```

### STEP 2: Retrieve Gemini Reviews (REQUIRED)
Use GitHub API to get ALL review comments from gemini-code-assist[bot]:
```bash
# Get PR number from step 1, then fetch review comments
PR_NUMBER=$(gh pr list --head $(git rev-parse --abbrev-ref HEAD) --json number --jq '.[0].number')
gh api "repos/:owner/:repo/pulls/$PR_NUMBER/reviews" --jq '.[] | select(.user.login == "gemini-code-assist[bot]")'
gh api "repos/:owner/:repo/pulls/$PR_NUMBER/comments" --jq '.[] | select(.user.login == "gemini-code-assist[bot]" and (.body | contains("[RESOLVED]") | not))'
```

### STEP 3: Analysis and Categorization (REQUIRED)
For EACH unresolved Gemini comment, categorize by priority:
- **CRITICAL** 🔴: Security vulnerabilities, runtime errors, data corruption
- **HIGH** 🟠: Performance issues, significant bugs, architectural problems
- **MEDIUM** 🟡: Code quality, testing gaps, maintainability issues
- **LOW** 🟢: Style preferences, minor optimizations, documentation

### STEP 4: MANDATORY IMPLEMENTATION (DO NOT SKIP)
You MUST implement ALL suggestions starting with CRITICAL priority:

**IMPLEMENTATION CHECKLIST:**
- [ ] Read the file mentioned in each comment
- [ ] Understand the specific issue
- [ ] Make the exact code changes suggested
- [ ] Verify the fix addresses the comment
- [ ] Move to next priority level

**CRITICAL RULE**: Do NOT provide explanations only. You MUST make actual file edits.

### STEP 5: MANDATORY TESTING (IF APPLICABLE)
Run available tests after each priority level:
```bash
# Check for common test commands
npm test || yarn test || python -m pytest || make test || ./gradlew test
```

### STEP 6: MANDATORY COMMIT & PUSH (DO NOT SKIP)
You MUST commit and push changes after implementing each priority level:

```bash
# Stage all changes
git add -A

# Create descriptive commit
git commit -m "fix: implement Gemini review suggestions - [PRIORITY_LEVEL]

- Address [specific issues fixed]
- Resolve security/performance/quality concerns
- Based on gemini-code-assist[bot] feedback"

# Push to remote
git push origin $(git rev-parse --abbrev-ref HEAD)
```

### STEP 7: Mark Comments as Resolved (REQUIRED)
Update each implemented comment with **[RESOLVED]** marker:
```bash
# For each comment ID that was addressed
gh api "repos/:owner/:repo/pulls/comments/COMMENT_ID" -X PATCH -f body="**[RESOLVED]** [original comment text]"
```

### STEP 8: Trigger Re-review (REQUIRED)
Post comment to request new Gemini review:
```bash
gh pr comment $PR_NUMBER --body "/gemini review"
```

## EXECUTION ENFORCEMENT RULES

**MANDATORY REQUIREMENTS:**
1. ✅ **NO EXPLANATION-ONLY RESPONSES** - You MUST make actual code changes
2. ✅ **NO SKIPPING IMPLEMENTATION** - Every identified issue must be fixed
3. ✅ **NO SKIPPING COMMITS** - Changes must be committed after each priority level
4. ✅ **NO SKIPPING PUSH** - All commits must be pushed to remote
5. ✅ **SEQUENTIAL EXECUTION** - Follow steps 1-8 in exact order

**VALIDATION CHECKLIST:**
After each step, confirm:
- [ ] PR was found/created successfully
- [ ] Gemini comments were retrieved (show count)
- [ ] Comments were categorized by priority
- [ ] Files were actually edited (not just planned)
- [ ] Tests were run (if available)
- [ ] Changes were committed with descriptive message
- [ ] Changes were pushed to remote branch
- [ ] Comments were marked as resolved
- [ ] Re-review was triggered

**ERROR HANDLING:**
If any step fails:
1. Show the exact error message
2. Attempt alternative approach
3. Do NOT proceed to next step until current step succeeds
4. If unable to proceed, create detailed error report

**SUCCESS CRITERIA:**
- All Gemini suggestions implemented
- All changes committed and pushed
- All comments marked as resolved
- New review triggered
- Zero pending review items

Use appropriate file editing tools (Read, Write, Edit) to make precise code changes. Always verify changes were applied correctly by reading the file back.
