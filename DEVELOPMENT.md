# Development Guide: sts_lightspeed Gymnasium Environment

This document provides detailed information for developers and researchers who want to work with the `sts_lightspeed` Gymnasium environment.

## Project Structure

- `src/`: Core C++ simulation logic.
- `include/`: C++ header files.
- `bindings/`: pybind11 code that bridges C++ and Python.
- `gym_sts_lightspeed/`: Gymnasium environment implementation.
- `json/`: submodule for JSON parsing.
- `pybind11/`: submodule for Python bindings.

## Submodules

This project uses git submodules for its dependencies. To ensure they are correctly fetched, run:

```bash
git submodule update --init --recursive
```

## Installation for Development

To install the package in editable mode, which allows you to modify the Python code and see changes immediately (though C++ changes still require a rebuild):

```bash
pip install -e .
```

## C++ Extension

The C++ extension is named `slaythespire`. It is built automatically during the `pip install` process using the custom `CMakeBuild` class in `setup.py`.

If you modify the C++ code, you should re-run `pip install -e .` or build manually using CMake:

```bash
mkdir build
cd build
cmake ..
make -j
```

## Gymnasium Environment (`STS-v0`)

The environment is implemented in `gym_sts_lightspeed/envs/sts_env.py`.

### Observation Space

The observation space is a `Box` of size 1200 with integer values. It provides a comprehensive representation of the game state:

| Indices | Description |
|---------|-------------|
| 0-8 | Global state: HP, Max HP, Gold, Floor, Act, Screen State, Keys |
| 9-18 | Boss encoding (One-hot for the current act's boss) |
| 19-238 | Deck composition (Counts of each card, up to 220 card types) |
| 239-416 | Relics (One-hot encoding for possessed relics) |
| 417-421 | Potions |
| 422-444 | Player combat state: Block, Energy, Cards drawn, Powers/Statuses |
| 445-514 | Enemy state: HP, Max HP, Block, Intent, Powers (up to 5 enemies) |
| 515-734 | Hand cards |
| 735-954 | Discard pile cards |
| 955-1174 | Draw pile cards |

### Action Space

The action space is `Discrete(128)`.

Because the set of valid actions changes depending on the game state (e.g., which cards are in hand, which rewards are available), the environment provides an `action_mask` in the `info` dictionary.

Researchers should use this mask when sampling actions:

```python
import gymnasium as gym
import gym_sts_lightspeed

env = gym.make("STS-v0")
obs, info = env.reset()

# Sample only from valid actions
action = env.action_space.sample(mask=info['action_mask'])
obs, reward, terminated, truncated, info = env.step(action)
```

### Reward Function

The default reward function is designed to encourage progress:
- **Floor Progress**: +10.0 per floor.
- **HP Changes**: ±1.0 per HP point lost/gained.
- **Victory**: +1000.0 for winning the game.
- **Loss**: -100.0 for losing the game.

## Adding New Features

### Modifying the Observation Space
To change what information is passed to the AI:
1. Modify `bindings/slaythespire.h` to update `observation_space_size`.
2. Update `bindings/bindings-util.cpp`'s `getObservation` and `getObservationMaximums` methods.
3. Update `gym_sts_lightspeed/envs/sts_env.py` if the size changes.

### Exposing New C++ Methods
To expose more of the underlying simulator to Python, add new definitions in `bindings/slaythespire.cpp`.

## Testing

Run the test suite to ensure everything is working correctly:

```bash
python3 -m pytest tests/
```
