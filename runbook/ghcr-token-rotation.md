# Runbook: Rotating `GHCR_TOKEN` (prod deploy)

## What this is

`GHCR_TOKEN` is a **GitHub Personal Access Token with `read:packages`
scope**, stored as a GitHub Actions repository secret on
`ascii-game-backend`. It's the GitHub credential in this project with an
expiration (classic PATs can be given a fixed lifetime; fine-grained PATs
always have one, max 1 year).

Confirmed from [`.github/workflows/deploy-prod.yml`](../.github/workflows/deploy-prod.yml):

- **Building and pushing** the image to GHCR uses the workflow's built-in
  `secrets.GITHUB_TOKEN` (auto-generated per run, never expires, nothing to
  rotate) — `GHCR_TOKEN` is **not** involved in that step.
- `GHCR_TOKEN` is used exactly once, inside the SSH step that runs on the
  **Oracle prod VM**:
  ```bash
  echo "${{ secrets.GHCR_TOKEN }}" | docker login ghcr.io -u "${{ github.repository_owner }}" --password-stdin
  trap 'docker logout ghcr.io' EXIT
  ```
  so the VM can `docker pull` the private image. The token is injected
  fresh from the GitHub secret on every deploy and logged out immediately
  after (`trap ... EXIT`) — **it is never written to disk on the Oracle
  VM**. This means rotation only ever touches the GitHub secret; there is
  no VM-side file to update, unlike `CLOUDFLARE_TUNNEL_TOKEN_PROD` (which
  *is* persisted in `~/app/.env` on the VM).

## When to act

- GitHub emails the token owner (`roosta255`) as its expiration
  approaches — don't rely solely on this; also check
  github.com → Settings → Developer settings → Personal access tokens →
  the token's expiry date directly.
- `deploy-prod.yml`'s "Pull and restart on Oracle" step starts failing on
  `docker login` with an authentication error — that means it already
  expired. Rotate immediately; production stops receiving new deploys
  until this is fixed (the currently-running container keeps serving
  traffic, but `docker compose pull` in the next deploy will fail).

## Steps

### 1. Generate the replacement PAT

1. github.com → **Settings → Developer settings → Personal access tokens**
   (classic, to match what's documented in the root `README.md` — a
   fine-grained PAT scoped to just this repo's packages works too, but
   switching token types means also updating how it's used if GitHub's
   fine-grained package permissions differ; classic is the tested path).
2. **Generate new token** — name it identifiably, e.g.
   `ascii-game-backend-ghcr-pull-prod`.
3. Scope: **`read:packages`** only — this token only ever needs to pull,
   never push (pushing uses `GITHUB_TOKEN`, not this).
4. Set an expiration (recommended, matching current practice).
5. Copy the token — shown once.

### 2. Do not revoke the old token yet

Same reasoning as any credential rotation: keep it valid until step 4
confirms the new one works, so a bad copy/paste doesn't take prod down.

### 3. Update the GitHub Actions secret

1. github.com/roosta255/ascii-game-backend → **Settings → Secrets and
   variables → Actions**.
2. Find `GHCR_TOKEN` → **Update** → paste the new value → **Update secret**.

There is nothing to change on the Oracle VM itself (see "What this is"
above — the token is never persisted there).

### 4. Verify

Trigger a deploy: push to `main`, or re-run the most recent
"Deploy to Production" run from the **Actions** tab (`Re-run all jobs`).
Watch the "Pull and restart on Oracle" step — a successful `docker login`
(no `unauthorized` error) followed by a successful `docker compose pull`
confirms the new token works.

### 5. Revoke the old PAT

github.com → Settings → Developer settings → Personal access tokens →
delete the old one now that step 4 passed.
