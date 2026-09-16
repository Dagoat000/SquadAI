# UE5 SquadAI

A modular tactical AI framework for **Unreal Engine 5**, written primarily in C++.

SquadAI focuses on coordinated AI behaviour rather than isolated NPC decision-making. AI agents can perceive their environment, communicate through a squad coordinator, evaluate tactical options, find cover and coordinate actions such as flanking, suppression, advancing and investigation.

The project is designed as both a gameplay AI framework and a technical exploration of tactical decision-making in Unreal Engine.

---

## Features

### Squad Coordination

AI agents can operate as members of a coordinated squad instead of making completely independent decisions.

The squad system can coordinate:

* Squad membership
* Shared awareness
* Tactical objectives
* Enemy information
* Cover allocation
* Squad-level actions
* Individual AI decisions

---

### AI Decision Making

SquadAI supports multiple approaches to decision making.

#### GOAP

Goal-Oriented Action Planning allows agents to select sequences of actions based on their current world state and objectives.

Example:

```text
Goal: Eliminate Enemy

        ↓

Assess Situation
        ↓
    ┌───┴────┐
    ↓        ↓
Take Cover  Flank
    ↓        ↓
Suppress   Attack
        \   /
          ↓
      Engage Enemy
```

#### Utility AI

Utility-based decision making allows AI agents to evaluate possible actions and select the action with the highest calculated utility.

Example:

```text
Take Cover    0.82
Suppress      0.67
Flank         0.74
Advance       0.41
Investigate   0.25
Reload        0.18
```

The exact decision depends on the current world state and the agent's available actions.

---

## Tactical Behaviours

The framework currently includes behaviours such as:

* Taking cover
* Suppression
* Flanking
* Advancing
* Reloading
* Investigation
* Enemy engagement
* Tactical repositioning

The goal is to make AI behaviour depend on the tactical situation rather than relying entirely on fixed scripted sequences.

---

## Perception

AI agents can gather information through multiple perception sources.

### Sight

Sight perception can be used to detect visible enemies and update the agent's awareness of the battlefield.

### Hearing

Hearing can provide information about events that the AI cannot directly see.

This allows AI to investigate suspicious events rather than requiring direct visual contact.

---

## Cover System

SquadAI contains a cover-point system that allows AI agents to evaluate and select tactical positions.

A cover position can be evaluated based on factors such as:

* Distance
* Visibility
* Protection
* Tactical position
* Enemy exposure
* Squad positioning

The system is designed to prevent every AI agent from simply choosing the nearest available cover position.

### Example

```text
                 Enemy
                   X
                  / \
                 /   \
        Cover A /     \ Cover B
              ●         ●
               \       /
                \     /
                 \   /
                  AI
```

The AI can evaluate available positions and select an appropriate tactical location.

---

## Squad Architecture

A simplified overview of the architecture:

```text
                    Squad Coordinator
                           │
             ┌─────────────┼─────────────┐
             │             │             │
          AI Agent      AI Agent      AI Agent
             │             │             │
        ┌────┴────┐   ┌────┴────┐   ┌────┴────┐
        │          │   │          │   │          │
   Perception   Decision  Perception  Decision  Perception
        │          │        │          │        │
        └────┬─────┘        └────┬─────┘        │
             │                   │              │
             └───────────┬───────┴──────────────┘
                         │
                    Navigation
                         │
                     Movement
```

The architecture separates squad-level coordination from individual agent behaviour.

---

## Awareness

Agents maintain awareness of relevant events and actors.

Possible awareness states include:

```text
Unaware
   ↓
Suspicious
   ↓
Investigating
   ↓
Aware
   ↓
Engaged
```

Awareness can be influenced by:

* Visual detection
* Hearing events
* Shared squad information
* Previous enemy locations
* Investigation results

---

## Debug Visualization

The framework includes debugging functionality to make AI decision-making visible during development.

Debug information can include:

* AI state
* Current target
* Squad membership
* Selected cover
* Perception information
* Navigation
* Decision scores
* Tactical actions

Example:

```text
AI #04

Squad: Alpha
State: SUPPRESSING
Target: Enemy #12

Current Action:
    Suppress

Utility Scores:
    Cover       0.82
    Suppress    0.91
    Flank       0.73
    Advance     0.38
    Investigate 0.12
```

Debug visualization can be enabled when developing or testing the AI.

---

# Technical Architecture

The project is implemented using Unreal Engine's C++ gameplay and AI systems.

Major components include:

```text
SquadAI
│
├── Squad Coordination
│
├── AI Controllers
│
├── Decision Making
│   ├── GOAP
│   └── Utility AI
│
├── Perception
│   ├── Sight
│   └── Hearing
│
├── Cover System
│
├── Navigation
│
├── Tactical Actions
│   ├── Take Cover
│   ├── Suppress
│   ├── Flank
│   ├── Advance
│   ├── Reload
│   └── Investigate
│
└── Debug Visualization
```

---

# Unreal Engine

SquadAI is developed for **Unreal Engine 5**.

The project makes use of Unreal Engine systems including:

* C++
* AIController
* Navigation System
* AI Perception
* Gameplay Framework
* State Trees
* Enhanced Input
* Components
* Subsystems
* Debug drawing

---

# Project Goals

The main goals of SquadAI are:

1. Create believable tactical AI.
2. Keep AI systems modular and extensible.
3. Separate tactical decision-making from low-level movement.
4. Allow multiple decision-making approaches to be tested.
5. Provide useful debugging tools for AI development.
6. Explore how coordinated AI can be implemented in a real-time game.

---

# Performance

The framework is designed with real-time performance in mind.

Areas being considered include:

* Avoiding unnecessary AI updates
* Efficient perception handling
* Reusing tactical information
* Limiting expensive searches
* Efficient cover evaluation
* Navigation query costs
* Squad-level information sharing

Performance profiling is performed during development to identify expensive systems and unnecessary work.

---

# Example Use Case

A squad encounters an enemy outside a building.

```text
                ENEMY
                  X
                  │
          ┌───────┴───────┐
          │               │
       AI #1            AI #2
     Suppress           Flank
          │               │
          │          ┌────┘
          │          │
          │        AI #3
          │       Take Cover
          │
          └──────┬───────┘
                 │
              Advance
```

Instead of every agent independently attacking the enemy, the squad can distribute tactical roles.

One agent may suppress the enemy while another attempts to flank and another provides cover.

---

# Development Status

SquadAI is an actively developed experimental AI framework.

### Implemented

* [x] Squad coordination
* [x] AI controllers
* [x] AI perception
* [x] Sight perception
* [x] Hearing
* [x] Cover-point system
* [x] Cover selection
* [x] Tactical actions
* [x] GOAP experimentation
* [x] Utility AI experimentation
* [x] Navigation integration
* [x] Debug visualization

### Planned

* [ ] More advanced squad tactics
* [ ] Dynamic squad roles
* [ ] Improved tactical scoring
* [ ] Better group movement
* [ ] More advanced suppression behaviour
* [ ] Improved investigation behaviour
* [ ] Performance profiling tools
* [ ] Automated AI testing
* [ ] Additional decision-making strategies

---

# Requirements

* Unreal Engine 5
* Visual Studio
* C++ development tools
* Windows
* DirectX 12-compatible hardware recommended

---

# Building

Clone the repository:

```bash
git clone https://github.com/Dagoat000/UE5-SquadAI.git
```

Open the Unreal project and allow Unreal Engine to generate the required project files.

Build the project using Visual Studio or Unreal Engine.

---

# Repository Structure

```text
UE5-SquadAI/
│
├── Config/
├── Content/
├── Source/
│   └── SquadAI/
│       ├── AI/
│       ├── Squad/
│       ├── Perception/
│       ├── Cover/
│       ├── GOAP/
│       ├── Utility/
│       └── Debug/
│
├── .gitignore
├── README.md
└── SquadAI.uproject
```

---

# Why I Built This

I built SquadAI to explore how tactical AI systems can be designed and implemented in a real-time game.

Rather than relying entirely on scripted behaviour, the project experiments with systems that allow AI agents to reason about their environment, evaluate different actions and coordinate with other agents.

The project also serves as an exploration of **C++ gameplay programming, AI architecture and Unreal Engine development**.

---


# License

This project is currently available for educational and portfolio purposes.

See `LICENSE` for details.
