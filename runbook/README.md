# Runbooks

Operational procedures for credentials and infrastructure that this
project depends on. Each runbook covers: what the credential is, where it
lives, how to tell it's expiring/expired, how to rotate it, and how to
verify the rotation worked.

| Runbook | Credential | Expires? |
|---|---|---|
| [`claude-api-key-rotation.md`](./claude-api-key-rotation.md) | Anthropic API key used by the agent orchestrator on the DGX Spark | Yes — expiration set in the Anthropic Console |
| [`ghcr-token-rotation.md`](./ghcr-token-rotation.md) | `GHCR_TOKEN` — GitHub PAT (`read:packages`) used by the prod deploy pipeline | Yes — expiration set when the PAT is created |

## Out of scope (for now)

- `ORACLE_SSH_KEY` (SSH keypair for GitHub Actions → Oracle VM deploy) —
  doesn't expire on its own; only needs rotation if compromised. See
  `README.md`'s "SSH key for GitHub Actions" section for how it was
  created.
- `CLOUDFLARE_TUNNEL_TOKEN` / `CLOUDFLARE_TUNNEL_TOKEN_PROD` — Cloudflare
  tunnel tokens, not GitHub/Anthropic credentials. Add a runbook here if
  these turn out to expire too.

## Why this lives in `ascii-game-backend`

The GHCR rotation is a backend-deploy concern and belongs next to the
existing secrets documentation in the root `README.md`. The Claude API key
is currently a Spark/agent concern; once `ascii-game-agent` is rewritten
(see `puzzle-dungeon-agent-implementation-plan.md` §2) and actually owns
its own secrets loading, consider moving `claude-api-key-rotation.md` there
instead of duplicating it.
