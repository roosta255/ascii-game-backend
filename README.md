# Asciigame Backend

This repository contains the backend engine for **Asciigame**, a multiplayer, turn-based puzzle dungeon game.  
The backend is written in modern C++ and designed for **deterministic gameplay, no dynamic memory**, and a clean separation of model and controller logic.

---

## 🚀 Features

### Match Engine
- `MatchModel` encapsulates game state and turn order.
- `MatchController` resolves player actions and transitions.
- Supports **optimistic locking** for persistence (stateless server model).

### Teams
- **Titan** (1 player) vs **Builders** (3 players).
- Asymmetric mechanics and goals.

### Custom Safe Containers
- `Array<T, N>` – bounds-safe wrapper over `std::array`.
- `Pointer<T>` – safe non-nullable reference with presence-checking.

---

## 🏗️ Project Structure
```text
main.cpp         # Entrypoint
Dockerfile       # Deployment container

/tests
  └── ...              # Unit tests

```

---

## Deployment

### How it works

There are two environments:

- **Dev**: The backend runs in a VS Code devcontainer on your local machine. A Cloudflare tunnel exposes it to the internet at a stable URL so the frontend can reach it from anywhere.
- **Production**: On every push to `main`, GitHub Actions builds a Docker image, pushes it to GitHub Container Registry (GHCR), SSHes into an Oracle Cloud VM, and restarts the containers there. A separate Cloudflare tunnel exposes the production backend.

Both environments use `docker-compose` to run two containers: the Drogon backend and the Cloudflare tunnel.

---

### Dev server setup

1. Open the repo in VS Code and reopen in the devcontainer when prompted.
2. In [Cloudflare Zero Trust](https://one.dash.cloudflare.com) → Networks → Tunnels → Create a tunnel:
   - Connector: Docker
   - Copy the tunnel token
   - Public Hostname: your chosen subdomain (e.g. `game-backend.yourdomain.com`) → Service: `HTTP` → `drogon_backend:8080`
3. Create a `.env` file in the repo root:
   ```
   CLOUDFLARE_TUNNEL_TOKEN=your-token-here
   ```
4. Start the containers:
   ```bash
   docker compose up
   ```
5. The backend is now reachable at your Cloudflare tunnel URL.

---

### Production server setup

#### One-time Oracle VM setup

1. Provision an Oracle Cloud VM (Always Free tier, `VM.Standard.E2.1.Micro`, Oracle Linux) with a public IP.
2. SSH into the VM and install Docker:
   ```bash
   sudo dnf remove -y docker podman-docker
   sudo dnf config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
   sudo dnf install -y docker-ce docker-ce-cli containerd.io docker-compose-plugin
   sudo systemctl enable --now docker
   sudo usermod -aG docker opc
   ```
3. Log out and back in, then create the app directory and `.env`:
   ```bash
   mkdir -p ~/app
   echo "CLOUDFLARE_TUNNEL_TOKEN=your-prod-token-here" > ~/app/.env
   ```
4. In [Cloudflare Zero Trust](https://one.dash.cloudflare.com) → Networks → Tunnels → Create a tunnel:
   - Connector: Docker
   - Copy the tunnel token (use this in step 3 above)
   - Public Hostname: your prod subdomain (e.g. `game-backend-prod.yourdomain.com`) → Service: `HTTP` → `drogon_backend:8080`

#### SSH key for GitHub Actions

Generate a dedicated key pair:
```bash
ssh-keygen -t ed25519 -C "github-actions" -f ~/.ssh/github_actions_oracle -N ""
```

Add the public key to the Oracle VM:
```bash
# On the Oracle VM:
echo "ssh-ed25519 AAAA..." >> ~/.ssh/authorized_keys
```

#### GitHub Actions secrets

Add these in your repo under Settings → Secrets and variables → Actions → Repository secrets:

| Secret | Value |
|---|---|
| `ORACLE_HOST` | Public IP of the Oracle VM |
| `ORACLE_USER` | `opc` |
| `ORACLE_SSH_KEY` | Contents of `~/.ssh/github_actions_oracle` (the private key) |
| `GHCR_TOKEN` | GitHub PAT with `read:packages` scope |

#### Deploying

Push to `main`. GitHub Actions will:
1. Build and push the Docker image to `ghcr.io/<owner>/<repo>:latest`
2. SCP `docker-compose.prod.yml` to `~/app` on the Oracle VM
3. Pull the new image and restart the containers

The production backend will be live at your prod Cloudflare tunnel URL.

---

## 👤 Author
Developed by [Roosta255](https://github.com/Roosta255).
