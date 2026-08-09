# Vision — [Game Codename / Title]

## Hero Statement
A single, vast ASCII world birthed whole before you enter.  
Choose your race, take up your life, and carve your story with no fate but what you make.  
The world ticks forward with or without you — monsters hunt, factions war, seasons change.  
This is grim fantasy sandbox freedom, played out one deliberate action at a time.

---

## Core Pillars
These are the non‑negotiable truths. If a design decision violates one of these, it’s the wrong direction.

1. **One World, One Generation**  
   The entire world is generated once at the start of a new game. No chunk‑loading, no runtime generation. It’s a living diorama, fully realized from moment zero.

2. **Radical Player Agency**  
   No main quest. No required story. The player can ignore or engage any system. The game asks “What do you do?” and never “What should you do?”

3. **Grim Freedom Fantasy**  
   Tone is low fantasy, morally ambiguous, and unforgiving. Inspirations: The Witcher’s world, not its plot. Monsters are a daily threat; races have meaningful differences; survival is earned, not given.

4. **Pressured Deliberation (Hybrid Time)**  
   A turn passes when the player acts *or* after 1–2 seconds of inactivity. The world is always one heartbeat away from moving, forcing constant tension without twitch reflexes.

5. **Terminal as Lens**  
   All visuals are ASCII/Unicode in the terminal. No external window, no graphical tileset. The constraint fuels creativity and keeps the scope grounded.

---

## Direction Test — Stop & Ask Checklist
Before adding a feature, expanding a system, or pursuing a tangent, ask:

- [ ] Does this make the world feel more like a real, breathing place?
- [ ] Does it deepen player agency (more choices that matter, not just more buttons)?
- [ ] Will it work seamlessly with the pre‑generated, one‑shot map?
- [ ] Does it respect the hybrid time system, or would it demand true real‑time/stop‑time loops?
- [ ] Can it be fully expressed through terminal ASCII and keyboard input?
- [ ] Does it risk pulling the game toward a directed story or a “main path”?
- [ ] Is it something every player *might* experience, or only a tiny fraction? (Beware dead‑end, over‑specific content.)
- [ ] Can it be implemented without breaking the other pillars?

If the answer to any is “no,” rethink the feature until it fits — or discard it.

---

## Scope Boundaries (Anti‑Pillars)
These are explicit promises of what the game will **not** become, to guard against feature creep.

- Not a roguelike — no permadeath unless the world itself ends you.
- Not a story engine — no hand‑crafted narrative arcs, no dialogue trees that pretend to be a novel.
- Not a graphics engine — ASCII only. Beauty comes from symbolism, not sprites.
- Not a simulation of every possible action — depth over breadth. A rich hunting system, a few social levers, not 1000 shallow verbs.
- Not multiplayer — this is your solitary footprint on the world.