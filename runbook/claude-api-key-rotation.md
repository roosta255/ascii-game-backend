# Runbook: Rotating the Claude API key on the DGX Spark

## What this is

An Anthropic API key, issued with an expiration date in the Anthropic
Console, used by the agent orchestrator running on the DGX Spark to call
Claude for the roles routed to `provider: claude` in `workspace.yaml`
(Analyst, Architect, Reviewer — see the implementation plan's Phase 16
model routing). This is a **server-to-server API key**, distinct from any
interactive `claude login` / subscription auth you might also have set up
on the Spark for running Claude Code itself — don't confuse the two when
rotating.

**This runbook doesn't yet know the exact on-disk location** — the agent
orchestrator (Milestone 1) hasn't been built yet as of this writing, so
nothing has *read* this key programmatically. The suggested location below
was proposed during Phase 0a provisioning, not confirmed against what you
actually did on the Spark. Step 1 has you confirm/fix this before
proceeding.

## When to act

- The key has a visible expiration date in the Anthropic Console
  (console.anthropic.com → Settings → API Keys) — check it periodically, or
  set a personal reminder ~1 week before expiry.
- The orchestrator starts failing Claude-routed stages with `401
  Unauthorized` / `invalid x-api-key` — that's the key having already
  expired. Treat this as the trigger to do this runbook immediately, not a
  scheduled rotation.

## Steps

### 1. Locate the current key on the Spark

```bash
# Check the suggested convention first
cat ~/.config/ascii-game-agent/secrets.env 2>/dev/null | grep ANTHROPIC_API_KEY

# If that's empty, search more broadly for wherever it actually landed
grep -rl "ANTHROPIC_API_KEY" ~/.bashrc ~/.profile ~/.config 2>/dev/null
env | grep ANTHROPIC_API_KEY
```

Note the exact file (or shell profile line, or systemd unit's
`Environment=`) — you'll edit that same place in step 4. If you find it
somewhere other than `~/.config/ascii-game-agent/secrets.env`, update this
runbook with the real location so the next rotation doesn't need this
search again.

### 2. Generate the replacement key

1. Go to [console.anthropic.com](https://console.anthropic.com) → **Settings → API Keys**.
2. **Create Key** — give it a name that identifies its purpose and host,
   e.g. `ascii-game-agent-orchestrator-spark`, so it's identifiable in the
   console's key list later.
3. Set an expiration if you want forced periodic rotation (recommended —
   that's presumably how the current key got its expiration too).
4. Copy the key immediately — the full value is only shown once.

### 3. Do **not** revoke the old key yet

Keep the expiring key active until step 5 confirms the new one works —
avoids an outage if the new key has a typo or the wrong permissions.

### 4. Install the new key on the Spark

At whatever location step 1 found:

```bash
# Example, if using the suggested convention:
$EDITOR ~/.config/ascii-game-agent/secrets.env
# Replace the ANTHROPIC_API_KEY= line with the new value
```

If the orchestrator is running as a long-lived process (systemd
service, `screen`/`tmux` session, etc.) rather than invoked fresh per job,
restart it so it picks up the new environment — a process that loaded the
old key at startup won't see the file change.

```bash
# adjust to however the orchestrator is actually run once it exists
sudo systemctl restart ascii-game-agent   # if run as a systemd service
```

### 5. Verify

```bash
# Minimal check that doesn't depend on the orchestrator being built yet:
curl -s https://api.anthropic.com/v1/messages \
  -H "x-api-key: $ANTHROPIC_API_KEY" \
  -H "anthropic-version: 2023-06-01" \
  -H "content-type: application/json" \
  -d '{"model":"claude-sonnet-5","max_tokens":8,"messages":[{"role":"user","content":"ping"}]}'
```

A `200` with a JSON response body means the new key works. A `401` means
the file/env wasn't actually updated, or the key was mistyped — recheck
step 4. Once the orchestrator exists, prefer running an actual
Claude-routed stage (e.g. `agent job run` against a trivial job) over this
raw `curl` check.

### 6. Revoke the old key

Back in the Anthropic Console, delete/revoke the expiring key now that
step 5 passed. Don't skip this — an unrevoked key with a known expiration
is still a live credential until it actually expires.
