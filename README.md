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

## 🐳 Deployment

The backend runs in a **self-hosted Docker container**.

### Build locally
```bash
docker build -t asciigame-backend .
```

### Run locally
```bash
docker run -p 8080:8080 asciigame-backend
```

---
## 👤 Author
Developed by [Roosta255](https://github.com/Roosta255).
