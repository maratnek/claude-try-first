# Progress log

One dated entry per scheduled agent run (newest first), so the project
owner can skim what happened without digging through git log. Format:

```
## YYYY-MM-DD HH:MM UTC — <night|day> run
- Did: <what, one line per item>
- Why: <reasoning, one line>
- Verified: <build/run result, or "not verifiable without a human">
- Open: <anything waiting on the human, e.g. a pending asset request>
- PR: <link or branch, if one was opened>
```
