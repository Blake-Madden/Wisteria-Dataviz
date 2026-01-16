# Documentation Review

Review Quarto (.qmd) and Markdown (.md) files for documentation issues.

## Scope

Find and analyze all `.md` and `.qmd` files in the repository, excluding:
- Files in submodules (src/easyexif, src/wxSimpleJSON, src/CRCpp, src/utfcpp, src/wxStartPage)
- Third-party documentation
- Node modules or build output directories

## What to Look For

### Typos and Spelling
- Misspelled words (e.g., "teh", "recieve", "seperate")
- Doubled words (e.g., "the the", "is is")
- Wrong word usage (e.g., "it's" vs "its", "than" vs "then")
- Missing words that break grammar (e.g., "This performed after" missing "is")

### Grammar Issues
- Subject-verb disagreement (e.g., "the files is" instead of "the files are")
- Incorrect articles (e.g., "a multiple columns" instead of "multiple columns")
- Incomplete sentences in descriptions
- Missing plurals where needed (e.g., "grouped bar" instead of "grouped bars")

### Content Issues
- Contradictory statements within the same file or across related files
- Duplicate property names or parameters documented incorrectly
- Overly repetitive phrasing (same sentence structure repeated many times)
- Broken or obviously wrong internal references

### Formatting Problems
- Inconsistent formatting patterns within a file
- Mismatched code block delimiters
- Malformed tables or lists

## Output Format

For each issue found, report:
- File path and line number
- The problematic text
- What it should be (for typos/grammar) or description of the issue (for content problems)

## Important Guidelines

- **Low false positives**: Only flag clear, obvious errors. When in doubt, skip it.
- **Don't flag style preferences**: Focus on correctness, not style choices.
- **Ignore code blocks**: Don't check spelling/grammar inside code examples.
- **Ignore proper nouns**: Don't flag technical terms, class names, or identifiers.
- **Be conservative**: A missed issue is better than a false positive.

## After Review

Present findings as a table with File, Line, Issue, and Suggested Fix columns. Ask if the user wants any issues fixed.
